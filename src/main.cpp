#include <SDL.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace {
constexpr int W = 1000, H = 650;
enum class Screen { Title, Select, Play, Result, Profile, Online, Dojo };
enum class NoteKind { Don, Ka };
struct Note { int timeMs; NoteKind kind; bool judged = false; bool hit = false; };
struct Options { float speed = 1.0f; bool swap = false; int random = 0; bool automatic = false; };

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
Screen screen = Screen::Title;
Options options;
std::string playerName = "Player";
std::string profileTitle = "はじめての太鼓";
std::vector<std::string> songs = {"Sakura practice", "Wonder beat", "Festival steps", "Dojo course"};
int selected = 0, menuIndex = 0, score = 0, combo = 0, maxCombo = 0, good = 0, ok = 0, miss = 0;
std::vector<Note> notes;
Uint32 playStart = 0;
bool editingName = false;

void color(SDL_Color c) { SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a); }
void box(int x, int y, int w, int h, SDL_Color c) { color(c); SDL_Rect r{x,y,w,h}; SDL_RenderFillRect(renderer, &r); }
void text(const std::string& value, int x, int y, int size = 25, SDL_Color c = {245,242,235,255}) {
  if (!font) return;
  TTF_SetFontSize(font, size);
  SDL_Surface* s = TTF_RenderUTF8_Blended(font, value.c_str(), c);
  if (!s) return;
  SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
  SDL_Rect dst{x,y,s->w,s->h}; SDL_RenderCopy(renderer,t,nullptr,&dst);
  SDL_DestroyTexture(t); SDL_FreeSurface(s);
}
void centered(const std::string& value, int y, int size, SDL_Color c = {245,242,235,255}) {
  if (!font) return;
  TTF_SetFontSize(font,size); int tw=0, th=0; TTF_SizeUTF8(font,value.c_str(),&tw,&th);
  text(value,(W-tw)/2,y,size,c);
}
void background() {
  box(0,0,W,H,{22,17,42,255});
  box(0,0,W,12,{255,89,116,255});
  box(35,35,930,580,{34,28,58,255});
}
void heading(const std::string& s) { centered(s,52,40,{255,205,88,255}); }
void loadProfile() {
  std::ifstream in("taiko-wonder.profile");
  std::getline(in,playerName); std::getline(in,profileTitle);
  if (playerName.empty()) playerName="Player";
  if (profileTitle.empty()) profileTitle="はじめての太鼓";
}
void saveProfile() { std::ofstream out("taiko-wonder.profile"); out<<playerName<<'\n'<<profileTitle<<'\n'; }
void makeChart(int seed, int count=42) {
  notes.clear(); std::mt19937 rng(seed); std::uniform_int_distribution<int> gap(330,650), type(0,1);
  int t=1800;
  for(int i=0;i<count;++i) {
    t += gap(rng);
    NoteKind k=type(rng)?NoteKind::Don:NoteKind::Ka;
    if(options.random==1 && i%4==0) k=type(rng)?NoteKind::Don:NoteKind::Ka;
    if(options.random==2) k=type(rng)?NoteKind::Don:NoteKind::Ka;
    notes.push_back({t,k,false,false});
  }
  score=combo=maxCombo=good=ok=miss=0;
  playStart=SDL_GetTicks(); screen=Screen::Play;
}
void startSong() { makeChart(100+selected*71, selected==3?30:42); }
void titleScreen() {
  background(); centered("taiko wonder",82,58,{255,112,126,255});
  const std::array<std::string,5> labels={"演奏モード", "段位道場", "オンライン", "プレイヤー設定", "終了"};
  for(int i=0;i<(int)labels.size();++i) {
    int y=205+i*66; box(300,y,400,52,i==menuIndex?SDL_Color{232,75,105,255}:SDL_Color{61,52,88,255});
    centered(labels[i],y+9,27);
  }
  centered("↑ / ↓ で選択、Enter で決定",552,19,{190,182,207,255});
}
void selectionScreen(bool dojo=false) {
  background(); heading(dojo?"段位道場 - practice course":"曲を選ぶ / 演奏オプション");
  if(dojo) {
    centered("3曲連続で演奏する練習コースです",133,22,{190,182,207,255});
    for(int i=0;i<3;++i) { box(245,200+i*76,510,58,i==selected?SDL_Color{232,75,105,255}:SDL_Color{61,52,88,255}); centered(songs[i],211+i*76,25); }
  } else {
    for(int i=0;i<3;++i) { int n=(selected+i)%3; box(85,150+i*66,400,52,i==0?SDL_Color{232,75,105,255}:SDL_Color{61,52,88,255}); text(songs[n],110,161+i*66,23); }
    box(520,150,390,254,{48,41,71,255}); text("演奏オプション",545,168,26,{255,205,88,255});
    text("速度: "+std::to_string(options.speed).substr(0,3)+"x",545,213,22);
    text(std::string("あべこべ: ")+(options.swap?"ON":"OFF"),545,249,22);
    text(std::string("ランダム: ")+(options.random==0?"OFF":options.random==1?"きまぐれ":"でたらめ"),545,285,22);
    text(std::string("オート: ")+(options.automatic?"ON":"OFF"),545,321,22);
    text("左右で変更 / Tabで項目切替",545,361,17,{190,182,207,255});
    text("選曲: "+songs[selected%3],95,390,23,{255,205,88,255});
  }
  centered("Enter: 開始   Esc: 戻る   ↑↓: 選択",548,20,{190,182,207,255});
}
void profileScreen() {
  background(); heading("プレイヤー設定");
  text("プレイヤー名",180,185,25,{255,205,88,255}); text(playerName+(editingName?"▏":""),470,185,25);
  text("称号",180,250,25,{255,205,88,255}); text(profileTitle,470,250,25);
  text("Enter: 名前を編集 / 称号を切替",180,342,21,{190,182,207,255});
  centered("Esc: 保存して戻る",548,20,{190,182,207,255});
}
void onlineScreen() {
  background(); heading("オンライン");
  centered("ルームサーバーは別プロセスで起動できます",167,24);
  centered("公開 / 非公開ルームのディレクトリ試作を同梱しています",214,20,{190,182,207,255});
  centered("ゲーム本体との通信・対戦同期はまだ未実装です",272,22,{255,130,130,255});
  centered("詳細: server/README.md",334,22,{255,205,88,255});
  centered("Esc: 戻る",548,20,{190,182,207,255});
}
void drawPlay() {
  background();
  const Uint32 elapsed=SDL_GetTicks()-playStart;
  const std::string mode=selected==3?"段位道場":"演奏中";
  centered(mode+"   "+playerName,45,24,{255,205,88,255});
  text("SCORE  "+std::to_string(score),75,105,28);
  text("COMBO  "+std::to_string(combo),760,105,28);
  box(80,295,840,8,{230,220,210,255});
  box(475,265,50,68,{255,205,88,255});
  for(const auto& n:notes) {
    if(n.judged) continue;
    const float appear=n.timeMs-static_cast<float>(elapsed);
    const float x=500.0f-appear*0.32f*options.speed;
    if(x < 60 || x > 940) continue;
    bool don=n.kind==NoteKind::Don; if(options.swap) don=!don;
    SDL_Color c=don?SDL_Color{255,86,102,255}:SDL_Color{75,177,255,255};
    box((int)x-18,277,36,44,c);
  }
  centered("DON: F / J       KA: D / K",380,23);
  centered(std::string("良 ")+std::to_string(good)+"   可 "+std::to_string(ok)+"   不可 "+std::to_string(miss),452,22,{190,182,207,255});
  if(options.automatic) centered("AUTO PRACTICE",520,18,{255,205,88,255});
}
void finishNote(Note& n, bool perfect) {
  n.judged=true; n.hit=perfect;
  if(perfect) { ++good; ++combo; score+=1000; maxCombo=std::max(maxCombo,combo); }
  else { ++ok; ++combo; score+=500; maxCombo=std::max(maxCombo,combo); }
}
void judge(bool inputDon) {
  const int now=(int)(SDL_GetTicks()-playStart);
  Note* best=nullptr; int delta=100000;
  for(auto& n:notes) if(!n.judged) {
    int d=std::abs(n.timeMs-now); if(d<delta) {delta=d;best=&n;}
  }
  if(!best || delta>115) { combo=0; return; }
  bool expectedDon=best->kind==NoteKind::Don; if(options.swap) expectedDon=!expectedDon;
  if(inputDon!=expectedDon) { combo=0; ++miss; best->judged=true; return; }
  finishNote(*best,delta<=48);
}
void drawResult() {
  background(); heading("演奏結果");
  centered("SCORE  "+std::to_string(score),165,42,{255,205,88,255});
  centered("良 "+std::to_string(good)+"     可 "+std::to_string(ok)+"     不可 "+std::to_string(miss),245,27);
  centered("最大コンボ  "+std::to_string(maxCombo),305,27);
  centered("Enter / Esc: タイトルへ",510,21,{190,182,207,255});
}
void draw() {
  switch(screen) {
    case Screen::Title:titleScreen();break;
    case Screen::Select:selectionScreen();break;
    case Screen::Dojo:selectionScreen(true);break;
    case Screen::Profile:profileScreen();break;
    case Screen::Online:onlineScreen();break;
    case Screen::Play:drawPlay();break;
    case Screen::Result:drawResult();break;
  }
  SDL_RenderPresent(renderer);
}
void handleKey(SDL_Keycode key) {
  if(screen==Screen::Title) {
    if(key==SDLK_UP) menuIndex=(menuIndex+4)%5;
    if(key==SDLK_DOWN) menuIndex=(menuIndex+1)%5;
    if(key==SDLK_RETURN) {
      if(menuIndex==0) {selected=0;screen=Screen::Select;}
      if(menuIndex==1) {selected=0;screen=Screen::Dojo;}
      if(menuIndex==2) screen=Screen::Online;
      if(menuIndex==3) screen=Screen::Profile;
      if(menuIndex==4) SDL_Event e{.type=SDL_QUIT};
    }
  } else if(screen==Screen::Select || screen==Screen::Dojo) {
    if(key==SDLK_ESCAPE) screen=Screen::Title;
    else if(key==SDLK_UP || key==SDLK_DOWN) selected=(selected+(key==SDLK_UP?2:1))%3;
    else if(key==SDLK_TAB) menuIndex=(menuIndex+1)%4;
    else if(key==SDLK_LEFT || key==SDLK_RIGHT) {
      int d=key==SDLK_RIGHT?1:-1;
      if(menuIndex==0) options.speed=std::clamp(options.speed+d*0.5f,1.0f,4.0f);
      if(menuIndex==1) options.swap=!options.swap;
      if(menuIndex==2) options.random=(options.random+(d+3))%3;
      if(menuIndex==3) options.automatic=!options.automatic;
    } else if(key==SDLK_RETURN) startSong();
  } else if(screen==Screen::Profile) {
    if(editingName) {
      if(key==SDLK_RETURN) {editingName=false; SDL_StopTextInput();saveProfile();}
      else if(key==SDLK_BACKSPACE && !playerName.empty()) { playerName.pop_back(); while(!playerName.empty() && (static_cast<unsigned char>(playerName.back())&0xC0)==0x80) playerName.pop_back(); }
    } else if(key==SDLK_ESCAPE) {saveProfile();screen=Screen::Title;}
    else if(key==SDLK_RETURN) {editingName=true;SDL_StartTextInput();}
    else if(key==SDLK_DOWN || key==SDLK_UP) profileTitle=profileTitle=="はじめての太鼓"?"Wonder player":"はじめての太鼓";
  } else if(screen==Screen::Online) { if(key==SDLK_ESCAPE) screen=Screen::Title; }
  else if(screen==Screen::Play) {
    if(key==SDLK_f||key==SDLK_j) judge(true);
    if(key==SDLK_d||key==SDLK_k) judge(false);
    if(key==SDLK_ESCAPE) screen=Screen::Result;
  } else if(screen==Screen::Result) { if(key==SDLK_RETURN||key==SDLK_ESCAPE) screen=Screen::Title; }
}
}

int main(int, char**) {
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0) return 1;
  if(TTF_Init()!=0) { SDL_Quit(); return 1; }
  window=SDL_CreateWindow("taiko wonder",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
  renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
  const std::array<const char*,6> fontCandidates={"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc","/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc","/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc","C:/Windows/Fonts/meiryo.ttc","C:/Windows/Fonts/msgothic.ttc","/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"};
  for(auto p:fontCandidates) { font=TTF_OpenFont(p,25); if(font) break; }
  if(!window||!renderer) { if(font)TTF_CloseFont(font); TTF_Quit(); SDL_Quit(); return 1; }
  loadProfile(); bool running=true;
  while(running) {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
      if(e.type==SDL_QUIT) running=false;
      else if(e.type==SDL_KEYDOWN && !e.key.repeat) {
        if(screen==Screen::Title && menuIndex==4 && e.key.keysym.sym==SDLK_RETURN) running=false;
        else handleKey(e.key.keysym.sym);
      } else if(e.type==SDL_TEXTINPUT && editingName) { if(playerName.size()<24) playerName+=e.text.text; }
    }
    if(screen==Screen::Play) {
      int elapsed=(int)(SDL_GetTicks()-playStart);
      if(options.automatic) for(auto& n:notes) if(!n.judged && elapsed>=n.timeMs) finishNote(n,true);
      for(auto& n:notes) if(!n.judged && elapsed>n.timeMs+140) {n.judged=true;++miss;combo=0;}
      if(!notes.empty() && elapsed>notes.back().timeMs+2600) screen=Screen::Result;
    }
    draw(); SDL_Delay(8);
  }
  saveProfile(); if(font)TTF_CloseFont(font); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 0;
}
