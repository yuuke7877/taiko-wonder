#include <SDL.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr int W=1000,H=650;
enum class Screen { Title, Modes, Songs, Play, Pause, Result, Profile, Online, Dojo };
enum class Kind { Don, Ka };
struct Note { int ms; Kind kind; bool judged=false; };
struct Song { std::string title; int genre=0; std::vector<Kind> chart; int seed=0; };
struct Skin { SDL_Color top{255,221,68,255}, bottom{255,132,76,255}, accent{255,94,119,255}, panel{255,255,255,255}, ink{51,35,54,255}; std::string font; };
struct Profile { std::string name="Player", title="はじめての太鼓", epithet; int avatar=0; };
SDL_Window* window=nullptr; SDL_Renderer* renderer=nullptr; TTF_Font* font=nullptr; SDL_Texture* skinBackground=nullptr;
fs::path appDir, songsDir, skinsDir, activeSkinDir;
Screen screen=Screen::Title; Skin skin; Profile profile;
const std::array<std::string,7> genres={"ポップス","アニメ","ボーカロイド","ゲームミュージック","バラエティ","クラシック","ナムコオリジナル"};
const std::array<std::string,4> modes={"演奏モード","段位道場","オンライン","プレイヤー設定"};
const std::array<std::string,3> pauseModes={"演奏を続ける","はじめからやりなおす","曲選択に戻る"};
const std::array<std::string,3> titles={"はじめての太鼓","Wonder player","祭りの達人"};
const std::array<std::string,4> avatars={"デフォルト","サンライト","ネコ","フェスティバル"};
std::vector<Song> songList;
std::vector<Note> notes;
int modeIndex=0,genreIndex=0,songIndex=0,pauseIndex=0,profileIndex=0,optionIndex=0,dojoStage=0;
bool inDojo=false,swapNotes=false,autoPlay=false,editingName=false,editingEpithet=false;
int randomMode=0,score=0,combo=0,maxCombo=0,good=0,ok=0,miss=0,chartSeed=0,chartCount=42;
float speed=1.0f;
Uint32 playStart=0,pauseStart=0,titleStart=0;

SDL_Color parseColor(const std::string& value,SDL_Color fallback){
  if(value.size()!=7||value[0]!='#')return fallback;
  auto byte=[&](size_t i){return (Uint8)std::strtoul(value.substr(i,2).c_str(),nullptr,16);};
  return {byte(1),byte(3),byte(5),255};
}
std::string trim(std::string s){while(!s.empty()&&std::isspace((unsigned char)s.front()))s.erase(s.begin());while(!s.empty()&&std::isspace((unsigned char)s.back()))s.pop_back();return s;}
void readSkin(const fs::path& folder){
  skin=Skin{};activeSkinDir=folder;std::ifstream in(folder/"skin.ini");std::string line;
  while(std::getline(in,line)){auto p=line.find('=');if(p==std::string::npos)continue;auto k=trim(line.substr(0,p)),v=trim(line.substr(p+1));
    if(k=="background_top")skin.top=parseColor(v,skin.top);else if(k=="background_bottom")skin.bottom=parseColor(v,skin.bottom);
    else if(k=="accent")skin.accent=parseColor(v,skin.accent);else if(k=="panel")skin.panel=parseColor(v,skin.panel);
    else if(k=="text")skin.ink=parseColor(v,skin.ink);else if(k=="font")skin.font=v;
  }
  if(skinBackground){SDL_DestroyTexture(skinBackground);skinBackground=nullptr;}
  SDL_Surface* bmp=SDL_LoadBMP((folder/"background.bmp").string().c_str());
  if(bmp){skinBackground=SDL_CreateTextureFromSurface(renderer,bmp);SDL_FreeSurface(bmp);}
}
void discoverFiles(){
  appDir=fs::path(SDL_GetBasePath()?SDL_GetBasePath():".");songsDir=appDir/"songs";skinsDir=appDir/"skins";
  std::error_code ec;fs::create_directories(songsDir,ec);fs::create_directories(skinsDir,ec);
  fs::path fallback=skinsDir/"default";fs::create_directories(fallback,ec);
  if(!fs::exists(fallback/"skin.ini")){std::ofstream o(fallback/"skin.ini");o<<"# Original Taiko Wonder color theme\nbackground_top=#ffdd44\nbackground_bottom=#ff844c\naccent=#ff5e77\npanel=#ffffff\ntext=#332336\n";}
  fs::path chosen=fallback;
  const char* requested=std::getenv("TAIKO_WONDER_SKIN");if(requested&&*requested){fs::path p=requested;if(p.is_relative())p=skinsDir/p;if(fs::exists(p/"skin.ini"))chosen=p;}
  readSkin(chosen);
  songList.clear();
  for(const auto& e:fs::directory_iterator(songsDir,ec))if(e.is_regular_file()&&e.path().extension()==".twn"){
    std::ifstream in(e.path());Song s;s.title=e.path().stem().string();s.seed=(int)std::hash<std::string>{}(s.title);std::string line;
    while(std::getline(in,line)){line=trim(line);if(line.empty()||line[0]=='#')continue;auto p=line.find('=');if(p==std::string::npos)continue;auto key=trim(line.substr(0,p)),val=trim(line.substr(p+1));
      if(key=="title")s.title=val;else if(key=="genre")s.genre=std::clamp(std::atoi(val.c_str()),0,6);else if(key=="notes"){
        std::istringstream ns(val);std::string token;while(ns>>token){if(token=="D"||token=="DON"||token=="1")s.chart.push_back(Kind::Don);else if(token=="K"||token=="KA"||token=="2")s.chart.push_back(Kind::Ka);}
      }
    }
    if(s.chart.empty())continue;songList.push_back(std::move(s));
  }
  if(songList.empty()){
    const std::array<const char*,14> names={"Sakura practice","Wonder pop","Anime-style beat","Hero practice","Synth color","Digital voice","Pixel adventure","Wonder game","Festival steps","Summer parade","Classic march","Moonlight practice","Taiko wonder","Wonder rhythm"};
    for(int i=0;i<14;++i)songList.push_back({names[i],i/2,1201+i,{}});
  }
}
void loadProfile(){std::ifstream i(appDir/"taiko-wonder.profile");if(i){std::getline(i,profile.name);std::getline(i,profile.title);std::getline(i,profile.epithet);i>>profile.avatar;}if(profile.name.empty())profile.name="Player";if(profile.avatar<0||profile.avatar>=4)profile.avatar=0;}
void saveProfile(){std::ofstream o(appDir/"taiko-wonder.profile");o<<profile.name<<'\n'<<profile.title<<'\n'<<profile.epithet<<'\n'<<profile.avatar<<'\n';}
void rect(int x,int y,int w,int h,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_Rect r{x,y,w,h};SDL_RenderFillRect(renderer,&r);}
void outline(int x,int y,int w,int h,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_Rect r{x,y,w,h};SDL_RenderDrawRect(renderer,&r);}
void text(const std::string& s,int x,int y,int size=24,SDL_Color c={255,255,255,255}){if(!font)return;TTF_SetFontSize(font,size);SDL_Surface* a=TTF_RenderUTF8_Blended(font,s.c_str(),c);if(!a)return;SDL_Texture* t=SDL_CreateTextureFromSurface(renderer,a);if(t){SDL_Rect d{x,y,a->w,a->h};SDL_RenderCopy(renderer,t,nullptr,&d);SDL_DestroyTexture(t);}SDL_FreeSurface(a);}
void center(const std::string& s,int y,int size,SDL_Color c={255,255,255,255}){if(!font)return;TTF_SetFontSize(font,size);int w=0,h=0;TTF_SizeUTF8(font,s.c_str(),&w,&h);text(s,(W-w)/2,y,size,c);}
void bg(){if(skinBackground)SDL_RenderCopy(renderer,skinBackground,nullptr,nullptr);else{rect(0,0,W,H,skin.top);rect(0,H/2,W,H/2,skin.bottom);for(int y=40;y<H;y+=48)for(int x=20+(y/48%2)*20;x<W;x+=48)rect(x,y,5,5,{255,247,180,120});}rect(0,0,W,10,{255,255,255,255});}
void panel(int x,int y,int w,int h,SDL_Color c,bool active=false){rect(x+5,y+6,w,h,{55,37,43,255});rect(x,y,w,h,active?SDL_Color{255,246,92,255}:SDL_Color{36,28,34,255});rect(x+6,y+6,w-12,h-12,c);if(active)outline(x-3,y-3,w+6,h+6,{255,255,255,255});}
void header(const std::string& s){rect(0,0,W,64,{255,255,255,245});text("taiko wonder",32,15,28,skin.accent);text(s,300,17,24,skin.ink);}
int songForSelection(){std::vector<int> matches;for(int i=0;i<(int)songList.size();++i)if(songList[i].genre==genreIndex)matches.push_back(i);if(matches.empty())return 0;return matches[std::clamp(songIndex,0,(int)matches.size()-1)];}
void generate(int seed,int count,bool reset=true){notes.clear();if(!songList[songForSelection()].chart.empty()&&!inDojo){int t=1500;for(auto k:songList[songForSelection()].chart){t+=450;notes.push_back({t,k,false});}}else{std::mt19937 rng(seed);std::uniform_int_distribution<int> gap(340,620),kind(0,1);int t=1600;for(int i=0;i<count;++i){t+=gap(rng);Kind k=kind(rng)?Kind::Don:Kind::Ka;if(randomMode==1&&i%4==0)k=kind(rng)?Kind::Don:Kind::Ka;if(randomMode==2)k=kind(rng)?Kind::Don:Kind::Ka;notes.push_back({t,k,false});}}
  chartSeed=seed;chartCount=count;if(reset)score=combo=maxCombo=good=ok=miss=0;playStart=SDL_GetTicks();screen=Screen::Play;}
void startSong(bool dojo=false){inDojo=dojo;auto si=songForSelection();generate((songList[si].seed? songList[si].seed:1201)+dojoStage*101,dojo?30:42);}
void drum(int cx,int cy,int r){for(int a=r;a>0;--a)rect(cx-a,cy-a,a*2,a*2,{39,177,190,255});for(int a=r-14;a>0;--a)rect(cx-a,cy-a,a*2,a*2,{246,235,198,255});for(int a=r-28;a>0;--a)rect(cx-a,cy-a,a*2,a*2,{244,77,52,255});}
void drawTitle(){bg();center("taiko wonder",68,56,skin.accent);drum(500,340,104);panel(55,250,230,88,{255,139,92,255});panel(715,250,230,88,{108,201,224,255});center("太鼓をたたいてスタート！",490,23,skin.ink);panel(405,442,190,58,skin.panel);center("FREE PLAY",454,23,skin.ink);center(((SDL_GetTicks()-titleStart)/10000)%2?"ドン！カッ！リズムを楽しもう":"左右の太鼓またはEnterでスタート",533,18,skin.ink);}
void drawModes(){bg();header("モードを選んでください");for(int i=0;i<4;++i){panel(220,108+i*110,560,82,genreColors[i],i==modeIndex);center(modes[i],130+i*110,28,skin.ink);}center("↑↓ 選択  Enter 決定  Esc 戻る",564,18,skin.ink);}
void drawSongs(){bg();header("ジャンルから曲を選ぶ");for(int i=0;i<7;++i){panel(15+i*140,82,132,55,genreColors[i],i==genreIndex);center(genres[i],98,15,skin.ink);}int row=0;for(int i=0;i<(int)songList.size();++i)if(songList[i].genre==genreIndex){panel(135,158+row*77,730,62,genreColors[genreIndex],row==songIndex);text(songList[i].title,166,177+row*77,22,skin.ink);++row;}text("速度 "+std::to_string(speed)+"x  あべこべ "+(swapNotes?"ON":"OFF"),140,450,18,skin.ink);text("ランダム "+(randomMode==0?"OFF":randomMode==1?"きまぐれ":"でたらめ")+"  オート "+(autoPlay?"ON":"OFF"),140,478,18,skin.ink);center("←→ ジャンル  ↑↓ 曲  Tab オプション  Enter 演奏  Esc 戻る",548,17,skin.ink);}
void drawDojo(){bg();header("段位道場");center("練習コース / オリジナル生成譜面",105,20,skin.ink);const std::array<std::string,3> ranks={"十級 - 練習","五級 - 挑戦","初段 - 挑戦"};for(int i=0;i<3;++i){panel(235,168+i*105,530,76,genreColors[i+1],i==modeIndex);center(ranks[i],188+i*105,25,skin.ink);}center("3曲コース  ↑↓ 段位  Enter 開始  Esc 戻る",550,18,skin.ink);}
void drawOnline(){bg();header("オンライン設定");center("ルームディレクトリ接続 / 同期プレイは未実装",83,17,skin.ink);text("公開/非公開",125,135,20,skin.ink);text("ルーム名",125,190,20,skin.ink);text("モード",125,245,20,skin.ink);text("接続先",125,300,20,skin.ink);center("↑↓ 項目  ←→ 設定  Enter 接続 / 操作  Esc 戻る",550,17,skin.ink);}
void drawProfile(){bg();header("プレイヤー設定");const std::array<std::string,4> labels={"名前","見た目","称号","二つ名"};std::array<std::string,4> vals={profile.name,avatars[profile.avatar],profile.title,profile.epithet};for(int i=0;i<4;++i){panel(145,150+i*76,710,58,skin.panel,i==profileIndex);text(labels[i],180,168+i*76,20,skin.ink);text(vals[i],485,168+i*76,20,skin.ink);}center("↑↓ 項目  ←→ 変更  Enter 編集  Esc 保存",520,17,skin.ink);}
void drawPlay(){bg();header(inDojo?"段位道場 演奏中":"演奏中");int elapsed=SDL_GetTicks()-playStart;text("SCORE "+std::to_string(score),55,83,24,skin.ink);text("COMBO "+std::to_string(combo),805,83,24,skin.ink);rect(60,300,880,14,skin.panel);drum(500,307,42);for(const auto& n:notes)if(!n.judged){float x=500-(n.ms-elapsed)*.32f*speed;if(x>40&&x<960){bool don=n.kind==Kind::Don;if(swapNotes)don=!don;rect((int)x-18,282,36,44,don?SDL_Color{244,71,77,255}:SDL_Color{53,163,219,255});}}center("F/J ドン   D/K カッ   Esc 一時停止",380,20,skin.ink);center("良 "+std::to_string(good)+"  可 "+std::to_string(ok)+"  不可 "+std::to_string(miss),438,20,skin.ink);}
void drawPause(){drawPlay();rect(0,0,W,H,{20,18,28,155});panel(260,120,480,390,skin.panel);center("一時停止",145,34,skin.accent);for(int i=0;i<3;++i){panel(310,210+i*82,380,60,skin.panel,i==pauseIndex);center(pauseModes[i],228+i*82,20,skin.ink);}}
void drawResult(){bg();header("演奏結果");center("SCORE "+std::to_string(score),170,38,skin.accent);center("良 "+std::to_string(good)+"  可 "+std::to_string(ok)+"  不可 "+std::to_string(miss),250,24,skin.ink);center("最大コンボ "+std::to_string(maxCombo),310,23,skin.ink);center("Enter: モード選択へ",520,18,skin.ink);}
void finish(Note& n,bool perfect){n.judged=true;if(perfect){++good;++combo;score+=1000;}else{++ok;++combo;score+=500;}maxCombo=std::max(maxCombo,combo);}
void judge(bool don){int now=SDL_GetTicks()-playStart,bestDelta=100000;Note* best=nullptr;for(auto& n:notes)if(!n.judged){int d=std::abs(n.ms-now);if(d<bestDelta){bestDelta=d;best=&n;}}if(!best||bestDelta>115){combo=0;return;}bool expected=best->kind==Kind::Don;if(swapNotes)expected=!expected;if(expected!=don){++miss;combo=0;best->judged=true;}else finish(*best,bestDelta<=48);}
void restart(){generate(chartSeed,chartCount);}
void handle(SDL_Keycode k){
 if(screen==Screen::Title){if(k==SDLK_RETURN||k==SDLK_f||k==SDLK_j||k==SDLK_d||k==SDLK_k)screen=Screen::Modes;return;}
 if(screen==Screen::Modes){if(k==SDLK_ESCAPE)screen=Screen::Title;else if(k==SDLK_UP)modeIndex=(modeIndex+3)%4;else if(k==SDLK_DOWN)modeIndex=(modeIndex+1)%4;else if(k==SDLK_RETURN){if(modeIndex==0)screen=Screen::Songs;else if(modeIndex==1){modeIndex=0;screen=Screen::Dojo;}else if(modeIndex==2)screen=Screen::Online;else screen=Screen::Profile;}return;}
 if(screen==Screen::Songs){if(k==SDLK_ESCAPE)screen=Screen::Modes;else if(k==SDLK_TAB)optionIndex=(optionIndex+1)%4;else if(k==SDLK_LEFT||k==SDLK_RIGHT){int d=k==SDLK_RIGHT?1:-1;if(optionIndex==0){speed=std::clamp(speed+d*.5f,1.f,4.f);}else if(optionIndex==1)swapNotes=!swapNotes;else if(optionIndex==2)randomMode=(randomMode+d+3)%3;else autoPlay=!autoPlay;}else if(k==SDLK_UP||k==SDLK_DOWN){if(songIndex==0&&k==SDLK_UP)genreIndex=(genreIndex+6)%7;else if(songIndex==0&&k==SDLK_DOWN)genreIndex=(genreIndex+1)%7;else{int count=0;for(auto&s:songList)if(s.genre==genreIndex)++count;songIndex=(songIndex+(k==SDLK_UP?count-1:1))%std::max(1,count);}}else if(k==SDLK_RETURN)startSong();return;}
 if(screen==Screen::Dojo){if(k==SDLK_ESCAPE)screen=Screen::Modes;else if(k==SDLK_UP||k==SDLK_DOWN)modeIndex=(modeIndex+(k==SDLK_DOWN?1:2))%3;else if(k==SDLK_RETURN){inDojo=true;dojoStage=0;startSong(true);}return;}
 if(screen==Screen::Play){if(k==SDLK_ESCAPE){pauseStart=SDL_GetTicks();pauseIndex=0;screen=Screen::Pause;}else if(k==SDLK_f||k==SDLK_j)judge(true);else if(k==SDLK_d||k==SDLK_k)judge(false);return;}
 if(screen==Screen::Pause){if(k==SDLK_ESCAPE){playStart+=SDL_GetTicks()-pauseStart;screen=Screen::Play;}else if(k==SDLK_UP||k==SDLK_DOWN)pauseIndex=(pauseIndex+(k==SDLK_DOWN?1:2))%3;else if(k==SDLK_RETURN){if(pauseIndex==0){playStart+=SDL_GetTicks()-pauseStart;screen=Screen::Play;}else if(pauseIndex==1)restart();else{screen=Screen::Songs;inDojo=false;}}return;}
 if(screen==Screen::Result){if(k==SDLK_RETURN||k==SDLK_ESCAPE)screen=Screen::Modes;return;}
 if(screen==Screen::Profile){if(editingName||editingEpithet){if(k==SDLK_RETURN){editingName=editingEpithet=false;SDL_StopTextInput();saveProfile();}else if(k==SDLK_BACKSPACE){auto& s=editingName?profile.name:profile.epithet;if(!s.empty())s.pop_back();}return;}if(k==SDLK_ESCAPE){saveProfile();screen=Screen::Modes;}else if(k==SDLK_UP||k==SDLK_DOWN)profileIndex=(profileIndex+(k==SDLK_DOWN?1:3))%4;else if(k==SDLK_RETURN){if(profileIndex==0){editingName=true;SDL_StartTextInput();}else if(profileIndex==3){editingEpithet=true;SDL_StartTextInput();}}else if(k==SDLK_LEFT||k==SDLK_RIGHT){int d=k==SDLK_RIGHT?1:-1;if(profileIndex==1)profile.avatar=(profile.avatar+d+4)%4;else if(profileIndex==2){auto it=std::find(titles.begin(),titles.end(),profile.title);int i=it==titles.end()?0:(int)(it-titles.begin());profile.title=titles[(i+d+3)%3];}}return;}
 if(screen==Screen::Online){if(k==SDLK_ESCAPE)screen=Screen::Modes;return;}
}
void update(){if(screen!=Screen::Play)return;int elapsed=SDL_GetTicks()-playStart;if(autoPlay&&!inDojo)for(auto& n:notes)if(!n.judged&&elapsed>=n.ms)finish(n,true);for(auto& n:notes)if(!n.judged&&elapsed>n.ms+140){n.judged=true;++miss;combo=0;}if(!notes.empty()&&elapsed>notes.back().ms+2400){if(inDojo&&dojoStage<2){++dojoStage;generate(chartSeed+dojoStage*101,30);}else screen=Screen::Result;}}
}
int main(int argc,char** argv){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0)return 1;if(TTF_Init()!=0){SDL_Quit();return 1;}window=SDL_CreateWindow("taiko wonder",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);if(window)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);if(!window||!renderer){SDL_Quit();return 1;}
 discoverFiles();std::vector<std::string> fonts;const char* env=std::getenv("TAIKO_WONDER_FONT");if(env&&*env)fonts.emplace_back(env);if(!skin.font.empty())fonts.push_back((activeSkinDir/skin.font).string());if(argc>2&&std::string(argv[1])=="--font")fonts.emplace_back(argv[2]);fonts.insert(fonts.end(),{"C:/Windows/Fonts/meiryo.ttc","C:/Windows/Fonts/Yu Gothic UI.ttf","/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc","/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"});for(auto&p:fonts){font=TTF_OpenFont(p.c_str(),24);if(font)break;}loadProfile();titleStart=SDL_GetTicks();bool running=true;
 while(running){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=false;else if(e.type==SDL_KEYDOWN&&!e.key.repeat)handle(e.key.keysym.sym);else if(e.type==SDL_TEXTINPUT&&(editingName||editingEpithet)){auto& s=editingName?profile.name:profile.epithet;if(s.size()<32)s+=e.text.text;}}update();switch(screen){case Screen::Title:drawTitle();break;case Screen::Modes:drawModes();break;case Screen::Songs:drawSongs();break;case Screen::Dojo:drawDojo();break;case Screen::Play:drawPlay();break;case Screen::Pause:drawPause();break;case Screen::Result:drawResult();break;case Screen::Profile:drawProfile();break;case Screen::Online:drawOnline();break;}SDL_RenderPresent(renderer);SDL_Delay(8);}
 saveProfile();if(skinBackground)SDL_DestroyTexture(skinBackground);if(font)TTF_CloseFont(font);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();return 0;
}
