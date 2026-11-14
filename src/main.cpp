#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#if __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#else
#include <SDL_ttf.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr int W=1000,H=650;
enum class Screen { Title, Modes, Songs, Play, Pause, Result, Profile, Dojo };
enum class Kind { Don, Ka };
struct Note { int ms; Kind kind; bool judged=false; };
struct Song { std::string title; int genre=0; std::vector<Kind> chart; int seed=0; };
struct Skin { SDL_Color top{255,221,68,255}, bottom{255,132,76,255}, accent{255,94,119,255}, panel{255,255,255,255}, ink{51,35,54,255}; std::string font; };
SDL_Window* window=nullptr; SDL_Renderer* renderer=nullptr; TTF_Font* font=nullptr; SDL_Texture* skinBackground=nullptr;
fs::path appDir, songsDir, skinsDir, activeSkinDir;
Screen screen=Screen::Title; Skin skin;
const std::array<std::string,7> genres={"ポップス","アニメ","ボーカロイド","ゲームミュージック","バラエティ","クラシック","ナムコオリジナル"};
const std::array<SDL_Color,7> genreColors={SDL_Color{255,86,117,255},SDL_Color{255,151,64,255},SDL_Color{160,93,220,255},SDL_Color{48,166,224,255},SDL_Color{53,187,115,255},SDL_Color{213,178,42,255},SDL_Color{235,95,172,255}};
const std::array<std::string,4> modes={"演奏モード","段位道場","オンライン","プレイヤー設定"};
const std::array<std::string,3> pauseModes={"演奏を続ける","はじめからやりなおす","曲選択に戻る"};
std::vector<Song> songs; std::vector<Note> notes;
int modeIndex=0,genreIndex=0,songIndex=0,pauseIndex=0,dojoStage=0,score=0,combo=0,maxCombo=0,good=0,ok=0,miss=0,chartSeed=0;
bool inDojo=false,swapNotes=false,autoPlay=false; int randomMode=0; float speed=1.0f; Uint32 playStart=0,pauseStart=0;

std::string trim(std::string s){while(!s.empty()&&std::isspace((unsigned char)s.front()))s.erase(s.begin());while(!s.empty()&&std::isspace((unsigned char)s.back()))s.pop_back();return s;}
SDL_Color colorValue(const std::string& v,SDL_Color fallback){if(v.size()!=7||v[0]!='#')return fallback;auto b=[&](size_t i){return (Uint8)std::strtoul(v.substr(i,2).c_str(),nullptr,16);};return {b(1),b(3),b(5),255};}
void rect(int x,int y,int w,int h,SDL_Color c){SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);SDL_Rect r{x,y,w,h};SDL_RenderFillRect(renderer,&r);}
void text(const std::string& s,int x,int y,int size=24,SDL_Color c={255,255,255,255}){if(!font)return;TTF_SetFontSize(font,size);SDL_Surface* a=TTF_RenderUTF8_Blended(font,s.c_str(),c);if(!a)return;SDL_Texture* t=SDL_CreateTextureFromSurface(renderer,a);if(t){SDL_Rect d{x,y,a->w,a->h};SDL_RenderCopy(renderer,t,nullptr,&d);SDL_DestroyTexture(t);}SDL_FreeSurface(a);}
void center(const std::string& s,int y,int size,SDL_Color c={255,255,255,255}){if(!font)return;TTF_SetFontSize(font,size);int w=0,h=0;TTF_SizeUTF8(font,s.c_str(),&w,&h);text(s,(W-w)/2,y,size,c);}
void loadSkin(){skinsDir=appDir/"skins";std::error_code ec;fs::create_directories(skinsDir/"default",ec);fs::path folder=skinsDir/"default";if(const char* env=std::getenv("TAIKO_WONDER_SKIN")){if(*env){fs::path p(env);if(p.is_relative())p=skinsDir/p;if(fs::exists(p/"skin.ini"))folder=p;}}activeSkinDir=folder;std::ifstream in(folder/"skin.ini");std::string l;while(std::getline(in,l)){auto p=l.find('=');if(p==std::string::npos)continue;auto k=trim(l.substr(0,p)),v=trim(l.substr(p+1));if(k=="background_top")skin.top=colorValue(v,skin.top);else if(k=="background_bottom")skin.bottom=colorValue(v,skin.bottom);else if(k=="accent")skin.accent=colorValue(v,skin.accent);else if(k=="panel")skin.panel=colorValue(v,skin.panel);else if(k=="text")skin.ink=colorValue(v,skin.ink);else if(k=="font")skin.font=v;}SDL_Surface* bmp=SDL_LoadBMP((folder/"background.bmp").string().c_str());if(bmp){skinBackground=SDL_CreateTextureFromSurface(renderer,bmp);SDL_FreeSurface(bmp);}}
void loadSongs(){songsDir=appDir/"songs";std::error_code ec;fs::create_directories(songsDir,ec);songs.clear();for(const auto& e:fs::directory_iterator(songsDir,ec)){if(!e.is_regular_file()||e.path().extension()!=".twn")continue;std::ifstream in(e.path());Song s;s.title=e.path().stem().string();s.seed=(int)std::hash<std::string>{}(s.title);std::string l;while(std::getline(in,l)){l=trim(l);if(l.empty()||l[0]=='#')continue;auto p=l.find('=');if(p==std::string::npos)continue;auto k=trim(l.substr(0,p)),v=trim(l.substr(p+1));if(k=="title")s.title=v;else if(k=="genre")s.genre=std::clamp(std::atoi(v.c_str()),0,6);else if(k=="notes"){std::istringstream ns(v);std::string n;while(ns>>n){if(n=="D"||n=="DON"||n=="1")s.chart.push_back(Kind::Don);else if(n=="K"||n=="KA"||n=="2")s.chart.push_back(Kind::Ka);}}}if(!s.chart.empty())songs.push_back(std::move(s));}
 if(songs.empty()){const std::array<const char*,14> names={"Sakura practice","Wonder pop","Anime-style beat","Hero practice","Synth color","Digital voice","Pixel adventure","Wonder game","Festival steps","Summer parade","Classic march","Moonlight practice","Taiko wonder","Wonder rhythm"};for(int i=0;i<14;++i){Song s;s.title=names[i];s.genre=i/2;s.seed=1201+i;songs.push_back(s);}}}
int genreCount(){int n=0;for(const auto&s:songs)if(s.genre==genreIndex)++n;return n;}
int selectedGlobal(){int n=0;for(int i=0;i<(int)songs.size();++i)if(songs[i].genre==genreIndex){if(n==songIndex)return i;++n;}return 0;}
void drawBg(){if(skinBackground)SDL_RenderCopy(renderer,skinBackground,nullptr,nullptr);else{rect(0,0,W,H,skin.top);rect(0,H/2,W,H/2,skin.bottom);for(int y=40;y<H;y+=48)for(int x=20+(y/48%2)*20;x<W;x+=48)rect(x,y,5,5,{255,247,180,120});}}
void panel(int x,int y,int w,int h,SDL_Color c,bool active=false){rect(x+5,y+6,w,h,{55,37,43,255});rect(x,y,w,h,active?SDL_Color{255,246,92,255}:SDL_Color{36,28,34,255});rect(x+6,y+6,w-12,h-12,c);}
void header(const std::string& s){rect(0,0,W,64,{255,255,255,245});text("taiko wonder",32,15,28,skin.accent);text(s,300,17,24,skin.ink);}
void generate(int seed,int count,bool reset=true){notes.clear();const int si=selectedGlobal();if(!inDojo&&!songs[si].chart.empty()){int t=1500;for(auto k:songs[si].chart){t+=450;notes.push_back({t,k,false});}}else{std::mt19937 rng(seed);std::uniform_int_distribution<int> gap(340,620),kind(0,1);int t=1600;for(int i=0;i<count;++i){t+=gap(rng);notes.push_back({t,kind(rng)?Kind::Don:Kind::Ka,false});}}chartSeed=seed;if(reset)score=combo=maxCombo=good=ok=miss=0;playStart=SDL_GetTicks();screen=Screen::Play;}
void startSong(bool dojo=false){inDojo=dojo;generate((songs[selectedGlobal()].seed? songs[selectedGlobal()].seed:1201)+dojoStage*101,dojo?30:42);}
void drawTitle(){drawBg();center("taiko wonder",70,54,skin.accent);panel(55,250,230,88,{255,139,92,255});panel(715,250,230,88,{108,201,224,255});center("太鼓をたたいてスタート！",400,25,skin.ink);panel(405,442,190,58,skin.panel);center("FREE PLAY",454,23,skin.ink);center("Enter または F/J/D/K でスタート",540,18,skin.ink);}
void drawModes(){drawBg();header("モードを選んでください");for(int i=0;i<4;++i){panel(220,108+i*110,560,82,genreColors[i],i==modeIndex);center(modes[i],130+i*110,28,skin.ink);}center("↑↓ 選択  Enter 決定  Esc 戻る",564,18,skin.ink);}
void drawSongs(){drawBg();header("ジャンルから曲を選ぶ");for(int i=0;i<7;++i){panel(15+i*140,82,132,55,genreColors[i],i==genreIndex);center(genres[i],98,15,skin.ink);}int row=0;for(const auto&s:songs)if(s.genre==genreIndex){panel(135,158+row*77,730,62,genreColors[genreIndex],row==songIndex);text(s.title,166,177+row*77,22,skin.ink);++row;}center("←→ ジャンル  ↑↓ 曲  Enter 演奏  Esc 戻る",548,17,skin.ink);}
void drawDojo(){drawBg();header("段位道場");center("練習コース / オリジナル生成譜面",105,20,skin.ink);const std::array<std::string,3> ranks={"十級 - 練習","五級 - 挑戦","初段 - 挑戦"};for(int i=0;i<3;++i){panel(235,168+i*105,530,76,genreColors[i+1],i==modeIndex);center(ranks[i],188+i*105,25,skin.ink);}center("3曲コース  ↑↓ 段位  Enter 開始  Esc 戻る",550,18,skin.ink);}
void drawPlay(){drawBg();header(inDojo?"段位道場 演奏中":"演奏中");int elapsed=SDL_GetTicks()-playStart;text("SCORE "+std::to_string(score),55,83,24,skin.ink);text("COMBO "+std::to_string(combo),805,83,24,skin.ink);rect(60,300,880,14,skin.panel);for(const auto& n:notes)if(!n.judged){float x=500-(n.ms-elapsed)*.32f*speed;if(x>40&&x<960){bool don=n.kind==Kind::Don;if(swapNotes)don=!don;rect((int)x-18,282,36,44,don?SDL_Color{244,71,77,255}:SDL_Color{53,163,219,255});}}center("F/J ドン   D/K カッ   Esc 一時停止",380,20,skin.ink);center("良 "+std::to_string(good)+"  可 "+std::to_string(ok)+"  不可 "+std::to_string(miss),438,20,skin.ink);}
void drawPause(){drawPlay();rect(0,0,W,H,{20,18,28,155});panel(260,120,480,390,skin.panel);center("一時停止",145,34,skin.accent);for(int i=0;i<3;++i){panel(310,210+i*82,380,60,skin.panel,i==pauseIndex);center(pauseModes[i],228+i*82,20,skin.ink);}}
void drawResult(){drawBg();header("演奏結果");center("SCORE "+std::to_string(score),170,38,skin.accent);center("良 "+std::to_string(good)+"  可 "+std::to_string(ok)+"  不可 "+std::to_string(miss),250,24,skin.ink);center("最大コンボ "+std::to_string(maxCombo),310,23,skin.ink);center("Enter: モード選択へ",520,18,skin.ink);}
void finish(Note& n,bool perfect){n.judged=true;if(perfect){++good;++combo;score+=1000;}else{++ok;++combo;score+=500;}maxCombo=std::max(maxCombo,combo);}
void judge(bool don){int now=SDL_GetTicks()-playStart,bestDelta=100000;Note* best=nullptr;for(auto& n:notes)if(!n.judged){int d=std::abs(n.ms-now);if(d<bestDelta){bestDelta=d;best=&n;}}if(!best||bestDelta>115){combo=0;return;}bool expected=best->kind==Kind::Don;if(swapNotes)expected=!expected;if(expected!=don){++miss;combo=0;best->judged=true;}else finish(*best,bestDelta<=48);}
void restart(){generate(chartSeed,42);}
void handle(SDL_Keycode k){
 if(screen==Screen::Title){if(k==SDLK_RETURN||k==SDLK_f||k==SDLK_j||k==SDLK_d||k==SDLK_k)screen=Screen::Modes;return;}
 if(screen==Screen::Modes){if(k==SDLK_ESCAPE)screen=Screen::Title;else if(k==SDLK_UP)modeIndex=(modeIndex+3)%4;else if(k==SDLK_DOWN)modeIndex=(modeIndex+1)%4;else if(k==SDLK_RETURN){if(modeIndex==0)screen=Screen::Songs;else if(modeIndex==1){modeIndex=0;screen=Screen::Dojo;}else if(modeIndex==2)screen=Screen::Online;else screen=Screen::Profile;}return;}
 if(screen==Screen::Songs){if(k==SDLK_ESCAPE)screen=Screen::Modes;else if(k==SDLK_LEFT||k==SDLK_RIGHT){genreIndex=(genreIndex+(k==SDLK_RIGHT?1:6))%7;songIndex=0;}else if(k==SDLK_UP||k==SDLK_DOWN)songIndex=(songIndex+(k==SDLK_UP?std::max(0,genreCount()-1):1))%std::max(1,genreCount());else if(k==SDLK_RETURN)startSong();return;}
 if(screen==Screen::Dojo){if(k==SDLK_ESCAPE)screen=Screen::Modes;else if(k==SDLK_UP||k==SDLK_DOWN)modeIndex=(modeIndex+(k==SDLK_DOWN?1:2))%3;else if(k==SDLK_RETURN){inDojo=true;dojoStage=0;startSong(true);}return;}
 if(screen==Screen::Play){if(k==SDLK_ESCAPE){pauseStart=SDL_GetTicks();pauseIndex=0;screen=Screen::Pause;}else if(k==SDLK_f||k==SDLK_j)judge(true);else if(k==SDLK_d||k==SDLK_k)judge(false);return;}
 if(screen==Screen::Pause){if(k==SDLK_ESCAPE){playStart+=SDL_GetTicks()-pauseStart;screen=Screen::Play;}else if(k==SDLK_UP||k==SDLK_DOWN)pauseIndex=(pauseIndex+(k==SDLK_DOWN?1:2))%3;else if(k==SDLK_RETURN){if(pauseIndex==0){playStart+=SDL_GetTicks()-pauseStart;screen=Screen::Play;}else if(pauseIndex==1)restart();else{screen=Screen::Songs;inDojo=false;}}return;}
 if(screen==Screen::Result){if(k==SDLK_RETURN||k==SDLK_ESCAPE)screen=Screen::Modes;return;}
 if(screen==Screen::Profile){if(k==SDLK_ESCAPE)screen=Screen::Modes;return;}if(screen==Screen::Online&&k==SDLK_ESCAPE)screen=Screen::Modes;
}
void update(){if(screen!=Screen::Play)return;int elapsed=SDL_GetTicks()-playStart;for(auto& n:notes)if(!n.judged&&elapsed>n.ms+140){n.judged=true;++miss;combo=0;}if(!notes.empty()&&elapsed>notes.back().ms+2400){if(inDojo&&dojoStage<2){++dojoStage;generate(chartSeed+dojoStage*101,30);}else screen=Screen::Result;}}
}
int main(int argc,char** argv){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0)return 1;if(TTF_Init()!=0){SDL_Quit();return 1;}window=SDL_CreateWindow("taiko wonder",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);if(window)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);if(!window||!renderer){SDL_Quit();return 1;}
 const char* base=SDL_GetBasePath();appDir=base?fs::path(base):fs::current_path();if(base)SDL_free((void*)base);loadSkin();loadSongs();std::vector<std::string> fonts;const char* env=std::getenv("TAIKO_WONDER_FONT");if(env&&*env)fonts.emplace_back(env);if(!skin.font.empty())fonts.push_back((activeSkinDir/skin.font).string());if(argc>2&&std::string(argv[1])=="--font")fonts.emplace_back(argv[2]);fonts.insert(fonts.end(),{"C:/Windows/Fonts/meiryo.ttc","C:/Windows/Fonts/Yu Gothic UI.ttf","/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc","/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"});for(auto&p:fonts){font=TTF_OpenFont(p.c_str(),24);if(font)break;}
 bool running=true;while(running){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=false;else if(e.type==SDL_KEYDOWN&&!e.key.repeat)handle(e.key.keysym.sym);}update();switch(screen){case Screen::Title:drawTitle();break;case Screen::Modes:drawModes();break;case Screen::Songs:drawSongs();break;case Screen::Dojo:drawDojo();break;case Screen::Play:drawPlay();break;case Screen::Pause:drawPause();break;case Screen::Result:drawResult();break;case Screen::Profile:drawProfile();break;case Screen::Online:drawOnline();break;}SDL_RenderPresent(renderer);SDL_Delay(8);}
 if(skinBackground)SDL_DestroyTexture(skinBackground);if(font)TTF_CloseFont(font);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();return 0;
}
