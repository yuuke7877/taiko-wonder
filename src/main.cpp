#include <SDL.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr int W = 1000;
constexpr int H = 650;

enum class Screen { Title, ModeSelect, SongSelect, DojoSelect, Play, Pause, Result, Profile, Online };
enum class NoteKind { Don, Ka };
struct Note { int timeMs; NoteKind kind; bool judged = false; };
struct Song { const char* title; int genre; int seed; };
struct Options { float speed = 1.0f; bool swap = false; int random = 0; bool autoPlay = false; };
struct Profile { std::string name = "Player", title = "はじめての太鼓", epithet; int avatar = 0; };

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
Screen screen = Screen::Title;
Options options;
Profile profile;

const std::array<std::string, 7> genres = {"ポップス", "アニメ", "ボーカロイド", "ゲームミュージック", "バラエティ", "クラシック", "ナムコオリジナル"};
const std::array<SDL_Color, 7> genreColors = {{{255,86,117,255},{255,151,64,255},{160,93,220,255},{48,166,224,255},{53,187,115,255},{213,178,42,255},{235,95,172,255}}};
// Original generated practice-chart labels only; no commercial songs or official charts are included.
const std::array<Song, 14> songs = {{{"Sakura practice",0,1201},{"Wonder pop",0,1202},
    {"Anime-style beat",1,2201},{"Hero practice",1,2202},
    {"Synth color",2,3201},{"Digital voice",2,3202},
    {"Pixel adventure",3,4201},{"Wonder game",3,4202},
    {"Festival steps",4,5201},{"Summer parade",4,5202},
    {"Classic march",5,6201},{"Moonlight practice",5,6202},
    {"Taiko wonder",6,7201},{"Wonder rhythm",6,7202}}};
const std::array<std::string, 4> modeLabels = {"演奏モード", "段位道場", "オンライン", "プレイヤー設定"};
const std::array<std::string, 3> pauseLabels = {"演奏を続ける", "はじめからやりなおす", "曲選択に戻る"};
const std::array<std::string, 4> avatars = {"デフォルト", "サンライト", "ネコ", "フェスティバル"};
const std::array<std::string, 3> titleChoices = {"はじめての太鼓", "Wonder player", "祭りの達人"};
const std::array<std::string, 4> onlineModes = {"スコアを競う", "仲良し演奏", "バトンタッチ演奏", "チーム戦 2v2 / 4v4"};

int modeIndex = 0, selectedGenre = 0, selectedSongIndex = 0, optionIndex = 0;
int pauseIndex = 0, dojoStage = 0, onlineModeIndex = 0, batonMeasures = 4;
bool inDojo = false, roomPublic = true, editingName = false, editingEpithet = false;
int profileFieldIndex = 0;
std::string roomName = "wonder-room";
std::vector<int> genreSongIndices;
std::vector<Note> notes;
int score = 0, combo = 0, maxCombo = 0, good = 0, ok = 0, miss = 0;
int chartSeed = 1201, chartCount = 42;
Uint32 playStartMs = 0, pauseStartMs = 0, titleStartMs = 0;

void setColor(SDL_Color c) { SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a); }
void rect(int x, int y, int w, int h, SDL_Color c) {
    setColor(c); SDL_Rect r{x,y,w,h}; SDL_RenderFillRect(renderer, &r);
}
void outline(int x, int y, int w, int h, int thickness, SDL_Color c) {
    rect(x,y,w,thickness,c); rect(x,y+h-thickness,w,thickness,c);
    rect(x,y,thickness,h,c); rect(x+w-thickness,y,thickness,h,c);
}
void text(const std::string& value, int x, int y, int size=24, SDL_Color c={255,255,255,255}) {
    if (!font) return;
    TTF_SetFontSize(font,size);
    SDL_Surface* s=TTF_RenderUTF8_Blended(font,value.c_str(),c);
    if (!s) return;
    SDL_Texture* t=SDL_CreateTextureFromSurface(renderer,s);
    if (t) { SDL_Rect d{x,y,s->w,s->h}; SDL_RenderCopy(renderer,t,nullptr,&d); SDL_DestroyTexture(t); }
    SDL_FreeSurface(s);
}
void centered(const std::string& value,int y,int size,SDL_Color c={255,255,255,255}) {
    if (!font) return;
    TTF_SetFontSize(font,size); int w=0,h=0; TTF_SizeUTF8(font,value.c_str(),&w,&h);
    text(value,(W-w)/2,y,size,c);
}
void background() {
    rect(0,0,W,H,{255,221,68,255});
    rect(0,330,W,320,{255,132,76,255});
    // Original abstract dots and confetti, drawn with SDL primitives (no skin images).
    for (int y=48;y<H;y+=48) for (int x=(y/48%2)*24;x<W;x+=48) rect(x,y,5,5,{255,246,173,110});
    rect(0,0,W,12,{255,255,255,255});
}
void panel(int x,int y,int w,int h,SDL_Color c,bool active=false) {
    rect(x+5,y+7,w,h,{55,37,43,255});
    rect(x,y,w,h,active?SDL_Color{255,246,92,255}:SDL_Color{32,24,28,255});
    rect(x+6,y+6,w-12,h-12,c);
    rect(x+15,y+13,w-30,8,{255,255,255,100});
    if(active) outline(x-4,y-4,w+8,h+8,4,{255,255,255,255});
}
void header(const std::string& s) {
    rect(0,0,W,66,{255,255,255,245});
    text("taiko wonder",36,17,29,{218,59,86,255});
    text(s,300,19,25,{51,35,54,255});
}
void loadProfile() {
    std::ifstream in("taiko-wonder.profile");
    if (!in) return;
    std::getline(in,profile.name); std::getline(in,profile.title); std::getline(in,profile.epithet); in>>profile.avatar;
    if(profile.name.empty())profile.name="Player";
    if(profile.title.empty())profile.title=titleChoices[0];
    if(profile.avatar<0||profile.avatar>=(int)avatars.size())profile.avatar=0;
}
void saveProfile() {
    std::ofstream out("taiko-wonder.profile");
    out<<profile.name<<'\n'<<profile.title<<'\n'<<profile.epithet<<'\n'<<profile.avatar<<'\n';
}
std::string speedText() { std::ostringstream s; s<<std::fixed<<std::setprecision(1)<<options.speed<<"x"; return s.str(); }
std::vector<int> getGenreSongs() {
    std::vector<int> out;
    for(int i=0;i<(int)songs.size();++i) if(songs[i].genre==selectedGenre) out.push_back(i);
    return out;
}
void resetScore() { score=combo=maxCombo=good=ok=miss=0; }
void generateChart(int seed,int count) {
    notes.clear(); std::mt19937 rng(seed); std::uniform_int_distribution<int> gap(340,620), type(0,1);
    int t=1650;
    for(int i=0;i<count;++i) {
        t+=gap(rng); NoteKind k=type(rng)?NoteKind::Don:NoteKind::Ka;
        if(options.random==1 && i%4==0) k=type(rng)?NoteKind::Don:NoteKind::Ka;
        if(options.random==2) k=type(rng)?NoteKind::Don:NoteKind::Ka;
        notes.push_back({t,k,false});
    }
    resetScore(); playStartMs=SDL_GetTicks(); screen=Screen::Play;
}
void startSelectedSong(bool dojo=false) {
    inDojo=dojo; chartSeed=songs[genreSongIndices.empty()?0:genreSongIndices[selectedSongIndex]].seed + dojoStage*100;
    chartCount=dojo?28:42; generateChart(chartSeed,chartCount);
}
void drawDrum(int cx,int cy,int radius) {
    setColor({41,28,34,255}); for(int r=radius;r>0;--r){ SDL_Rect a{cx-r,cy-r,r*2,r*2}; SDL_RenderFillRect(renderer,&a); }
    setColor({39,177,190,255}); SDL_Rect body{cx-radius+12,cy-radius+12,(radius-12)*2,(radius-12)*2}; SDL_RenderFillRect(renderer,&body);
    setColor({246,235,198,255}); SDL_Rect face{cx-radius+23,cy-radius+23,(radius-23)*2,(radius-23)*2}; SDL_RenderFillRect(renderer,&face);
    setColor({244,77,52,255}); SDL_Rect core{cx-radius+37,cy-radius+37,(radius-37)*2,(radius-37)*2}; SDL_RenderFillRect(renderer,&core);
    rect(cx-radius/3,cy-radius/5,8,8,{40,35,35,255}); rect(cx+radius/5,cy-radius/5,8,8,{40,35,35,255});
}
void drawTitle() {
    background();
    const bool attract=((SDL_GetTicks()-titleStartMs)/10000)%2==1;
    centered("taiko wonder",66,58,{222,53,88,255});
    drawDrum(500,350,112);
    panel(60,260,235,90,{255,139,92,255});
    panel(705,260,235,90,{108,201,224,255});
    centered("太鼓をたたいてスタート！",510,24,{55,35,49,255});
    panel(405,468,190,60,{255,255,255,255});
    centered("FREE PLAY",482,24,{44,35,55,255});
    if(attract) {
        panel(300,157,400,58,{255,142,87,255});
        centered("ドン！カッ！リズムを楽しもう",168,23,{60,35,48,255});
        centered("F/J = ドン     D/K = カッ",545,18,{70,48,56,255});
    } else {
        centered("左右の太鼓をたたくか Enter でスタート",545,19,{70,48,56,255});
    }
    centered("10秒ごとにデモ画面が切り替わります",590,15,{105,74,73,255});
}
void drawModeSelect() {
    background(); header("モードを選んでください");
    const std::array<SDL_Color,4> colors={{{255,94,119,255},{255,174,64,255},{68,184,215,255},{165,112,215,255}}};
    for(int i=0;i<4;++i){int y=112+i*112; panel(220,y,560,86,colors[i],i==modeIndex); centered(modeLabels[i],y+23,29,{42,34,49,255});}
    centered("↑↓で選択  Enterで決定  Escでタイトル",570,18,{64,49,58,255});
}
void drawSongSelect() {
    background(); header("ジャンルから曲を選ぶ");
    for(int i=0;i<(int)genres.size();++i){int x=26+i*136;panel(x,86,126,56,genreColors[i],i==selectedGenre);centered(genres[i],101,16,{35,32,42,255});}
    genreSongIndices=getGenreSongs();
    for(int i=0;i<(int)genreSongIndices.size();++i){int y=170+i*84; panel(135,y,730,66,genreColors[selectedGenre],i==selectedSongIndex); text(songs[genreSongIndices[i]].title,170,y+19,24,{42,32,42,255}); text("★  " + std::to_string(3+i*2),750,y+19,19,{67,47,45,255});}
    panel(42,172,70,270,{255,255,255,255}); centered("1P",400,22,{55,45,50,255});
    panel(888,172,70,270,{255,255,255,255}); centered("OPTIONS",518,14,{55,45,50,255});
    text("速度 " + speedText()+"    あべこべ "+(options.swap?"ON":"OFF"),145,470,18,{53,38,47,255});
    text("ランダム "+(options.random==0?"OFF":options.random==1?"きまぐれ":"でたらめ")+"    オート "+(options.autoPlay?"ON":"OFF"),145,500,18,{53,38,47,255});
    centered("←→: ジャンル  ↑↓: 曲  Tab: オプション切替  Enter: 演奏  Esc: 戻る",566,17,{64,49,58,255});
}
void drawDojoSelect() {
    background(); header("段位道場");
    centered("練習用の3曲コース（オリジナル生成譜面）",110,20,{64,49,58,255});
    const std::array<std::string,3> ranks={"十級 - 練習", "五級 - 挑戦", "初段 - 挑戦"};
    for(int i=0;i<3;++i){int y=180+i*105;panel(235,y,530,76,genreColors[i+1],i==modeIndex);centered(ranks[i],y+20,26,{45,36,49,255});}
    centered("↑↓: 段位  Enter: 開始  Esc: 戻る",555,18,{64,49,58,255});
}
void drawProfile() {
    background(); header("プレイヤー設定");
    const std::array<std::string,4> labels={"プレイヤー名","見た目","称号","二つ名"};
    const std::array<std::string,4> values={profile.name+(editingName?"_":""),avatars[profile.avatar],profile.title,profile.epithet+(editingEpithet?"_":"")};
    for(int i=0;i<4;++i){int y=160+i*75;panel(150,y,700,58,{255,255,255,255},i==profileFieldIndex);text(labels[i],180,y+15,21,{55,42,52,255});text(values[i],480,y+15,21,{55,42,52,255});}
    centered("↑↓: 項目  ←→: 見た目/称号  Enter: 編集  Esc: 保存",520,18,{64,49,58,255});
}
void drawOnline() {
    background(); header("オンラインルーム設定（サーバー接続は未実装）");
    text("公開/非公開",135,140,21,{55,42,52,255});text(roomPublic?"公開":"非公開・4桁コード",510,140,21,{55,42,52,255});
    text("ルーム名",135,205,21,{55,42,52,255});text(roomName,510,205,21,{55,42,52,255});
    text("モード",135,270,21,{55,42,52,255});text(onlineModes[onlineModeIndex],510,270,21,{55,42,52,255});
    text("バトンタッチ",135,335,21,{55,42,52,255});text(std::to_string(batonMeasures)+" 小節ごと",510,335,21,{55,42,52,255});
    centered("↑↓: 項目  ←→: 変更  Esc: 戻る",490,18,{64,49,58,255});
}
void drawPlay() {
    background(); header(inDojo?"段位道場 演奏中":"演奏中");
    int elapsed=(int)(SDL_GetTicks()-playStartMs);
    text("SCORE  "+std::to_string(score),55,88,25,{50,38,48,255});text("COMBO  "+std::to_string(combo),795,88,25,{50,38,48,255});
    rect(65,305,870,13,{255,255,255,255});
    // original drum judgement mark
    drawDrum(500,312,43);
    for(const auto& n:notes){if(n.judged)continue;float x=500.0f-(n.timeMs-elapsed)*0.32f*options.speed;if(x<45||x>955)continue;bool don=n.kind==NoteKind::Don;if(options.swap)don=!don;rect((int)x-18,290,38,42,don?SDL_Color{244,71,77,255}:SDL_Color{53,163,219,255});outline((int)x-18,290,38,42,3,{255,255,255,255});}
    centered("DON: F / J     KA: D / K     Esc: 一時停止",390,21,{53,42,50,255});
    centered("良 "+std::to_string(good)+"     可 "+std::to_string(ok)+"     不可 "+std::to_string(miss),452,21,{53,42,50,255});
    if(options.autoPlay&&!inDojo)centered("AUTO PRACTICE",510,17,{205,71,74,255});
}
void drawPause() {
    drawPlay(); rect(0,0,W,H,{20,18,28,150});
    panel(265,120,470,400,{255,247,217,255}); centered("一時停止",143,38,{208,58,77,255});
    for(int i=0;i<3;++i){int y=220+i*82;panel(315,y,370,60,i==pauseIndex?SDL_Color{255,145,112,255}:SDL_Color{255,255,255,255},i==pauseIndex);centered(pauseLabels[i],y+17,21,{49,37,45,255});}
    centered("↑↓: 選択  Enter: 決定",477,17,{67,51,58,255});
}
void drawResult() {
    background();header(inDojo?"段位道場 結果":"演奏結果");
    centered("SCORE  "+std::to_string(score),160,42,{205,57,79,255});
    centered("良 "+std::to_string(good)+"    可 "+std::to_string(ok)+"    不可 "+std::to_string(miss),240,25,{50,38,48,255});
    centered("最大コンボ  "+std::to_string(maxCombo),300,25,{50,38,48,255});
    centered("Enter / Esc: モード選択へ",520,19,{64,49,58,255});
}
void draw() {
    switch(screen){
        case Screen::Title:drawTitle();break;case Screen::ModeSelect:drawModeSelect();break;
        case Screen::SongSelect:drawSongSelect();break;case Screen::DojoSelect:drawDojoSelect();break;
        case Screen::Play:drawPlay();break;case Screen::Pause:drawPause();break;case Screen::Result:drawResult();break;
        case Screen::Profile:drawProfile();break;case Screen::Online:drawOnline();break;
    }
    SDL_RenderPresent(renderer);
}
void finishNote(Note& n,bool perfect){n.judged=true;if(perfect){++good;++combo;score+=1000;}else{++ok;++combo;score+=500;}maxCombo=std::max(maxCombo,combo);}
void judge(bool inputDon){
    int now=(int)(SDL_GetTicks()-playStartMs),delta=100000;Note* best=nullptr;
    for(auto& n:notes)if(!n.judged){int d=std::abs(n.timeMs-now);if(d<delta){delta=d;best=&n;}}
    if(!best||delta>115){combo=0;return;}
    bool expected=best->kind==NoteKind::Don;if(options.swap)expected=!expected;
    if(inputDon!=expected){combo=0;++miss;best->judged=true;return;}finishNote(*best,delta<=48);
}
void changeOption(int d){if(optionIndex==0)options.speed=std::clamp(options.speed+d*.5f,1.0f,4.0f);else if(optionIndex==1)options.swap=!options.swap;else if(optionIndex==2)options.random=(options.random+d+3)%3;else options.autoPlay=!options.autoPlay;}
void restartChart(){generateChart(chartSeed,chartCount);}
void handleKey(SDL_Keycode k){
    if(screen==Screen::Title){if(k==SDLK_RETURN||k==SDLK_f||k==SDLK_j||k==SDLK_d||k==SDLK_k)screen=Screen::ModeSelect;return;}
    if(screen==Screen::ModeSelect){if(k==SDLK_ESCAPE){screen=Screen::Title;titleStartMs=SDL_GetTicks();}else if(k==SDLK_UP)modeIndex=(modeIndex+3)%4;else if(k==SDLK_DOWN)modeIndex=(modeIndex+1)%4;else if(k==SDLK_RETURN){if(modeIndex==0){selectedGenre=0;selectedSongIndex=0;genreSongIndices=getGenreSongs();screen=Screen::SongSelect;}else if(modeIndex==1){modeIndex=0;screen=Screen::DojoSelect;}else if(modeIndex==2)screen=Screen::Online;else screen=Screen::Profile;}return;}
    if(screen==Screen::SongSelect){if(k==SDLK_ESCAPE){screen=Screen::ModeSelect;return;}if(k==SDLK_TAB){optionIndex=(optionIndex+1)%4;return;}if(k==SDLK_LEFT||k==SDLK_RIGHT){if(optionIndex==0&&k==SDLK_LEFT&&selectedSongIndex==0){selectedGenre=(selectedGenre+6)%7;selectedSongIndex=0;}else if(optionIndex==0&&k==SDLK_RIGHT&&selectedSongIndex==0){selectedGenre=(selectedGenre+1)%7;selectedSongIndex=0;}else changeOption(k==SDLK_RIGHT?1:-1);return;}genreSongIndices=getGenreSongs();if(k==SDLK_UP)selectedSongIndex=(selectedSongIndex+(int)genreSongIndices.size()-1)%genreSongIndices.size();else if(k==SDLK_DOWN)selectedSongIndex=(selectedSongIndex+1)%genreSongIndices.size();else if(k==SDLK_RETURN){dojoStage=0;startSelectedSong(false);}return;}
    if(screen==Screen::DojoSelect){if(k==SDLK_ESCAPE){screen=Screen::ModeSelect;return;}if(k==SDLK_UP||k==SDLK_DOWN)modeIndex=(modeIndex+(k==SDLK_DOWN?1:2))%3;else if(k==SDLK_RETURN){dojoStage=0;selectedGenre=0;genreSongIndices=getGenreSongs();selectedSongIndex=0;startSelectedSong(true);}return;}
    if(screen==Screen::Pause){if(k==SDLK_ESCAPE){playStartMs+=SDL_GetTicks()-pauseStartMs;screen=Screen::Play;}else if(k==SDLK_UP)pauseIndex=(pauseIndex+2)%3;else if(k==SDLK_DOWN)pauseIndex=(pauseIndex+1)%3;else if(k==SDLK_RETURN){if(pauseIndex==0){playStartMs+=SDL_GetTicks()-pauseStartMs;screen=Screen::Play;}else if(pauseIndex==1)restartChart();else{screen=Screen::SongSelect;inDojo=false;}}return;}
    if(screen==Screen::Play){if(k==SDLK_ESCAPE){pauseStartMs=SDL_GetTicks();pauseIndex=0;screen=Screen::Pause;}else if(k==SDLK_f||k==SDLK_j)judge(true);else if(k==SDLK_d||k==SDLK_k)judge(false);return;}
    if(screen==Screen::Result){if(k==SDLK_RETURN||k==SDLK_ESCAPE)screen=Screen::ModeSelect;return;}
    if(screen==Screen::Profile){if(editingName||editingEpithet){if(k==SDLK_RETURN){editingName=editingEpithet=false;SDL_StopTextInput();saveProfile();}else if(k==SDLK_BACKSPACE){std::string& s=editingName?profile.name:profile.epithet;if(!s.empty()){s.pop_back();while(!s.empty()&&(static_cast<unsigned char>(s.back())&0xC0)==0x80)s.pop_back();}}return;}if(k==SDLK_ESCAPE){saveProfile();screen=Screen::ModeSelect;}else if(k==SDLK_UP||k==SDLK_DOWN)profileFieldIndex=(profileFieldIndex+(k==SDLK_DOWN?1:3))%4;else if(k==SDLK_LEFT||k==SDLK_RIGHT){int d=k==SDLK_RIGHT?1:-1;if(profileFieldIndex==1)profile.avatar=(profile.avatar+d+4)%4;if(profileFieldIndex==2){auto it=std::find(titleChoices.begin(),titleChoices.end(),profile.title);int i=it==titleChoices.end()?0:(int)(it-titleChoices.begin());profile.title=titleChoices[(i+d+3)%3];}}else if(k==SDLK_RETURN){if(profileFieldIndex==0){editingName=true;SDL_StartTextInput();}if(profileFieldIndex==3){editingEpithet=true;SDL_StartTextInput();}}return;}
    if(screen==Screen::Online){if(k==SDLK_ESCAPE)screen=Screen::ModeSelect;else if(k==SDLK_UP||k==SDLK_DOWN)onlineModeIndex=(onlineModeIndex+(k==SDLK_DOWN?1:3))%4;else if(k==SDLK_LEFT||k==SDLK_RIGHT){int d=k==SDLK_RIGHT?1:-1;if(onlineModeIndex==0)roomPublic=!roomPublic;else if(onlineModeIndex==2)batonMeasures=std::clamp(batonMeasures+d,1,16);}return;}
}
void update(){if(screen!=Screen::Play)return;int elapsed=(int)(SDL_GetTicks()-playStartMs);if(options.autoPlay&&!inDojo)for(auto& n:notes)if(!n.judged&&elapsed>=n.timeMs)finishNote(n,true);for(auto& n:notes)if(!n.judged&&elapsed>n.timeMs+140){n.judged=true;++miss;combo=0;}if(!notes.empty()&&elapsed>notes.back().timeMs+2500){if(inDojo&&dojoStage<2){++dojoStage;startSelectedSong(true);}else screen=Screen::Result;}}
}

int main(int argc,char** argv){
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0)return 1;
    if(TTF_Init()!=0){SDL_Quit();return 1;}
    window=SDL_CreateWindow("taiko wonder",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
    if(window)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    std::vector<std::string> candidates;
    const char* env=std::getenv("TAIKO_WONDER_FONT");if(env&&*env)candidates.emplace_back(env);
    if(argc>=3&&std::string(argv[1])=="--font")candidates.emplace_back(argv[2]);
    candidates.insert(candidates.end(),{"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc","/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc","/usr/share/fonts/truetype/fonts-japanese-gothic.ttf","/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc","C:/Windows/Fonts/meiryo.ttc","C:/Windows/Fonts/Yu Gothic UI.ttf"});
    for(const auto& p:candidates){font=TTF_OpenFont(p.c_str(),24);if(font)break;}
    if(!window||!renderer){if(font)TTF_CloseFont(font);TTF_Quit();SDL_Quit();return 1;}
    loadProfile();titleStartMs=SDL_GetTicks();bool running=true;
    while(running){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=false;else if(e.type==SDL_KEYDOWN&&!e.key.repeat)handleKey(e.key.keysym.sym);else if(e.type==SDL_TEXTINPUT&&(editingName||editingEpithet)){std::string& s=editingName?profile.name:profile.epithet;if(s.size()<32)s+=e.text.text;}}
        update();draw();SDL_Delay(8);}
    saveProfile();if(font)TTF_CloseFont(font);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();return 0;
}
