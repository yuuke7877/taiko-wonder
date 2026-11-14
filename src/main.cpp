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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr int W = 1000;
constexpr int H = 650;
enum class Screen { Title, Modes, Songs, Play, Pause, Result, Profile, Online, Dojo };
enum class Kind { Don, Ka };
struct Note { int ms = 0; Kind kind = Kind::Don; bool judged = false; };
struct Song { std::string title; int genre = 0; std::vector<Kind> chart; int seed = 0; };
struct Profile { std::string name = "Player"; std::string title = "はじめての太鼓"; std::string epithet; int avatar = 0; };
struct Theme {
    SDL_Color top{255,221,68,255};
    SDL_Color bottom{255,132,76,255};
    SDL_Color accent{255,94,119,255};
    SDL_Color panel{255,255,255,255};
    SDL_Color ink{51,35,54,255};
    std::string font;
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
SDL_Texture* skinBackground = nullptr;
fs::path appDir, songsDir, skinsDir, activeSkinDir;
Screen screen = Screen::Title;
Theme theme;
Profile profile;
std::vector<Song> songs;
std::vector<Note> notes;

const std::array<std::string, 7> genres = {"ポップス", "アニメ", "ボーカロイド", "ゲームミュージック", "バラエティ", "クラシック", "ナムコオリジナル"};
const std::array<SDL_Color, 7> genreColors = {
    SDL_Color{255,86,117,255}, SDL_Color{255,151,64,255}, SDL_Color{160,93,220,255},
    SDL_Color{48,166,224,255}, SDL_Color{53,187,115,255}, SDL_Color{213,178,42,255}, SDL_Color{235,95,172,255}
};
const std::array<std::string, 4> modeLabels = {"演奏モード", "段位道場", "オンライン", "プレイヤー設定"};
const std::array<std::string, 3> pauseLabels = {"演奏を続ける", "はじめからやりなおす", "曲選択に戻る"};
const std::array<std::string, 4> avatarLabels = {"デフォルト", "サンライト", "ネコ", "フェスティバル"};
const std::array<std::string, 3> titleLabels = {"はじめての太鼓", "Wonder player", "祭りの達人"};
const std::array<std::string, 4> onlineModes = {"スコアを競う", "仲良し演奏", "バトンタッチ演奏", "チーム戦 2v2 / 4v4"};

int modeIndex = 0, genreIndex = 0, songIndex = 0, pauseIndex = 0, profileIndex = 0;
int onlineIndex = 0, onlineModeIndex = 0, batonMeasures = 4, dojoStage = 0;
bool inDojo = false, swapNotes = false, autoPlay = false, roomPublic = true;
bool editingName = false, editingEpithet = false;
int randomMode = 0, score = 0, combo = 0, maxCombo = 0, good = 0, ok = 0, miss = 0;
int chartSeed = 1201, chartCount = 42;
float speed = 1.0f;
Uint32 playStart = 0, pauseStart = 0, titleStart = 0;
std::string roomName = "wonder-room";

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}
SDL_Color parseColor(const std::string& value, SDL_Color fallback) {
    if (value.size() != 7 || value[0] != '#') return fallback;
    auto byte = [&](size_t i) { return static_cast<Uint8>(std::strtoul(value.substr(i, 2).c_str(), nullptr, 16)); };
    return SDL_Color{byte(1), byte(3), byte(5), 255};
}
void fillRect(int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(renderer, &r);
}
void drawText(const std::string& value, int x, int y, int size, SDL_Color c) {
    if (!font) return;
    TTF_SetFontSize(font, size);
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, value.c_str(), c);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect dst{x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}
void centered(const std::string& value, int y, int size, SDL_Color c) {
    if (!font) return;
    TTF_SetFontSize(font, size);
    int width = 0, height = 0;
    TTF_SizeUTF8(font, value.c_str(), &width, &height);
    drawText(value, (W - width) / 2, y, size, c);
}
void drawBackground() {
    if (skinBackground) {
        SDL_RenderCopy(renderer, skinBackground, nullptr, nullptr);
        return;
    }
    fillRect(0, 0, W, H, theme.top);
    fillRect(0, H / 2, W, H / 2, theme.bottom);
    for (int y = 40; y < H; y += 48)
        for (int x = 20 + (y / 48 % 2) * 20; x < W; x += 48)
            fillRect(x, y, 5, 5, SDL_Color{255,247,180,120});
}
void drawPanel(int x, int y, int w, int h, SDL_Color c, bool selected = false) {
    fillRect(x + 5, y + 6, w, h, SDL_Color{55,37,43,255});
    fillRect(x, y, w, h, selected ? SDL_Color{255,246,92,255} : SDL_Color{36,28,34,255});
    fillRect(x + 6, y + 6, w - 12, h - 12, c);
}
void drawHeader(const std::string& value) {
    fillRect(0, 0, W, 64, SDL_Color{255,255,255,245});
    drawText("taiko wonder", 32, 15, 28, theme.accent);
    drawText(value, 300, 17, 24, theme.ink);
}
void loadSkin() {
    skinsDir = appDir / "skins";
    std::error_code ec;
    fs::create_directories(skinsDir / "default", ec);
    fs::path folder = skinsDir / "default";
    const char* configured = std::getenv("TAIKO_WONDER_SKIN");
    if (configured && *configured) {
        fs::path candidate(configured);
        if (candidate.is_relative()) candidate = skinsDir / candidate;
        if (fs::exists(candidate / "skin.ini")) folder = candidate;
    }
    activeSkinDir = folder;
    std::ifstream input(folder / "skin.ini");
    std::string line;
    while (std::getline(input, line)) {
        const size_t equals = line.find('=');
        if (equals == std::string::npos) continue;
        const std::string key = trim(line.substr(0, equals));
        const std::string value = trim(line.substr(equals + 1));
        if (key == "background_top") theme.top = parseColor(value, theme.top);
        else if (key == "background_bottom") theme.bottom = parseColor(value, theme.bottom);
        else if (key == "accent") theme.accent = parseColor(value, theme.accent);
        else if (key == "panel") theme.panel = parseColor(value, theme.panel);
        else if (key == "text") theme.ink = parseColor(value, theme.ink);
        else if (key == "font") theme.font = value;
    }
    SDL_Surface* bitmap = SDL_LoadBMP((folder / "background.bmp").string().c_str());
    if (bitmap) {
        skinBackground = SDL_CreateTextureFromSurface(renderer, bitmap);
        SDL_FreeSurface(bitmap);
    }
}
void loadSongs() {
    songsDir = appDir / "songs";
    std::error_code ec;
    fs::create_directories(songsDir, ec);
    songs.clear();
    for (const auto& entry : fs::directory_iterator(songsDir, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".twn") continue;
        std::ifstream input(entry.path());
        Song song;
        song.title = entry.path().stem().string();
        song.seed = static_cast<int>(std::hash<std::string>{}(song.title));
        std::string line;
        while (std::getline(input, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            const size_t equals = line.find('=');
            if (equals == std::string::npos) continue;
            const std::string key = trim(line.substr(0, equals));
            const std::string value = trim(line.substr(equals + 1));
            if (key == "title") song.title = value;
            else if (key == "genre") song.genre = std::clamp(std::atoi(value.c_str()), 0, 6);
            else if (key == "notes") {
                std::istringstream notesIn(value);
                std::string token;
                while (notesIn >> token) {
                    if (token == "D" || token == "DON" || token == "1") song.chart.push_back(Kind::Don);
                    else if (token == "K" || token == "KA" || token == "2") song.chart.push_back(Kind::Ka);
                }
            }
        }
        if (!song.chart.empty()) songs.push_back(std::move(song));
    }
    if (songs.empty()) {
        const std::array<const char*, 14> names = {"Sakura practice","Wonder pop","Anime-style beat","Hero practice","Synth color","Digital voice","Pixel adventure","Wonder game","Festival steps","Summer parade","Classic march","Moonlight practice","Taiko wonder","Wonder rhythm"};
        for (int i = 0; i < 14; ++i) {
            Song song;
            song.title = names[i]; song.genre = i / 2; song.seed = 1201 + i;
            songs.push_back(std::move(song));
        }
    }
}
int songsInGenre() {
    int count = 0;
    for (const auto& song : songs) if (song.genre == genreIndex) ++count;
    return count;
}
int selectedSongGlobal() {
    int localIndex = 0;
    for (int i = 0; i < static_cast<int>(songs.size()); ++i) {
        if (songs[i].genre == genreIndex && localIndex++ == songIndex) return i;
    }
    return 0;
}
void generateChart(int seed, int count, bool resetScore = true) {
    notes.clear();
    const int selected = selectedSongGlobal();
    if (!inDojo && selected >= 0 && selected < static_cast<int>(songs.size()) && !songs[selected].chart.empty()) {
        int time = 1500;
        for (Kind kind : songs[selected].chart) { time += 450; notes.push_back(Note{time, kind, false}); }
    } else {
        std::mt19937 rng(static_cast<unsigned int>(seed));
        std::uniform_int_distribution<int> gap(340, 620), kindDistribution(0, 1);
        int time = 1600;
        for (int i = 0; i < count; ++i) {
            time += gap(rng);
            Kind kind = kindDistribution(rng) ? Kind::Don : Kind::Ka;
            if (randomMode == 1 && i % 4 == 0) kind = kindDistribution(rng) ? Kind::Don : Kind::Ka;
            if (randomMode == 2) kind = kindDistribution(rng) ? Kind::Don : Kind::Ka;
            notes.push_back(Note{time, kind, false});
        }
    }
    chartSeed = seed;
    if (resetScore) score = combo = maxCombo = good = ok = miss = 0;
    playStart = SDL_GetTicks();
    screen = Screen::Play;
}
void startSong(bool dojo = false) {
    inDojo = dojo;
    const int selected = selectedSongGlobal();
    const int seed = (selected >= 0 && selected < static_cast<int>(songs.size()) && songs[selected].seed != 0) ? songs[selected].seed : 1201;
    generateChart(seed + dojoStage * 101, dojo ? 30 : 42);
}
void drawTitle() {
    drawBackground(); centered("taiko wonder", 70, 54, theme.accent);
    drawPanel(55, 250, 230, 88, SDL_Color{255,139,92,255});
    drawPanel(715, 250, 230, 88, SDL_Color{108,201,224,255});
    centered("太鼓をたたいてスタート！", 400, 25, theme.ink);
    drawPanel(405, 442, 190, 58, theme.panel); centered("FREE PLAY", 454, 23, theme.ink);
    centered("Enter または F/J/D/K でスタート", 540, 18, theme.ink);
}
void drawModes() {
    drawBackground(); drawHeader("モードを選んでください");
    for (int i = 0; i < 4; ++i) { drawPanel(220,108+i*110,560,82,genreColors[i],i==modeIndex); centered(modeLabels[i],130+i*110,28,theme.ink); }
    centered("↑↓ 選択  Enter 決定  Esc 戻る",564,18,theme.ink);
}
void drawSongs() {
    drawBackground(); drawHeader("ジャンルから曲を選ぶ");
    for (int i = 0; i < 7; ++i) { drawPanel(15+i*140,82,132,55,genreColors[i],i==genreIndex); centered(genres[i],98,15,theme.ink); }
    int row = 0;
    for (const auto& song : songs) if (song.genre == genreIndex) { drawPanel(135,158+row*77,730,62,genreColors[genreIndex],row==songIndex); drawText(song.title,166,177+row*77,22,theme.ink); ++row; }
    centered("←→ ジャンル  ↑↓ 曲  Enter 演奏  Esc 戻る",548,17,theme.ink);
}
void drawDojo() {
    drawBackground(); drawHeader("段位道場"); centered("練習コース / オリジナル生成譜面",105,20,theme.ink);
    const std::array<std::string,3> ranks={"十級 - 練習","五級 - 挑戦","初段 - 挑戦"};
    for(int i=0;i<3;++i){drawPanel(235,168+i*105,530,76,genreColors[i+1],i==modeIndex);centered(ranks[i],188+i*105,25,theme.ink);}
    centered("3曲コース  ↑↓ 段位  Enter 開始  Esc 戻る",550,18,theme.ink);
}
void drawProfile() {
    drawBackground(); drawHeader("プレイヤー設定");
    const std::array<std::string,4> labels={"名前","見た目","称号","二つ名"};
    const std::array<std::string,4> values={profile.name,avatarLabels[profile.avatar],profile.title,profile.epithet};
    for(int i=0;i<4;++i){drawPanel(145,150+i*76,710,58,theme.panel,i==profileIndex);drawText(labels[i],180,168+i*76,20,theme.ink);drawText(values[i],485,168+i*76,20,theme.ink);}
    centered("↑↓ 項目  ←→ 変更  Enter 編集  Esc 戻る",520,17,theme.ink);
}
void drawOnline() {
    drawBackground(); drawHeader("オンライン設定");
    const std::array<std::string,4> labels={"公開/非公開","ルーム名","モード","バトンタッチ"};
    const std::array<std::string,4> values={roomPublic?"公開":"非公開・4桁コード",roomName,onlineModes[onlineModeIndex],std::to_string(batonMeasures)+" 小節ごと"};
    for(int i=0;i<4;++i){drawPanel(130,125+i*75,740,58,theme.panel,i==onlineIndex);drawText(labels[i],160,143+i*75,20,theme.ink);drawText(values[i],490,143+i*75,20,theme.ink);}
    centered("ルーム通信はサーバー設定後に利用できます",455,17,theme.ink);
    centered("↑↓ 項目  ←→ 設定  Esc 戻る",535,17,theme.ink);
}
void drawPlay() {
    drawBackground(); drawHeader(inDojo?"段位道場 演奏中":"演奏中");
    const int elapsed=static_cast<int>(SDL_GetTicks()-playStart);
    drawText("SCORE "+std::to_string(score),55,83,24,theme.ink); drawText("COMBO "+std::to_string(combo),805,83,24,theme.ink);
    fillRect(60,300,880,14,theme.panel);
    for(const auto& note:notes)if(!note.judged){const float x=500.0f-(note.ms-elapsed)*0.32f*speed;if(x>40&&x<960){bool don=note.kind==Kind::Don;if(swapNotes)don=!don;fillRect(static_cast<int>(x)-18,282,36,44,don?SDL_Color{244,71,77,255}:SDL_Color{53,163,219,255});}}
    centered("F/J ドン   D/K カッ   Esc 一時停止",380,20,theme.ink);
    centered("良 "+std::to_string(good)+"  可 "+std::to_string(ok)+"  不可 "+std::to_string(miss),438,20,theme.ink);
}
void drawPause() {
    drawPlay(); fillRect(0,0,W,H,SDL_Color{20,18,28,155}); drawPanel(260,120,480,390,theme.panel); centered("一時停止",145,34,theme.accent);
    for(int i=0;i<3;++i){drawPanel(310,210+i*82,380,60,theme.panel,i==pauseIndex);centered(pauseLabels[i],228+i*82,20,theme.ink);}
}
void drawResult() {
    drawBackground(); drawHeader("演奏結果"); centered("SCORE "+std::to_string(score),170,38,theme.accent);
    centered("良 "+std::to_string(good)+"  可 "+std::to_string(ok)+"  不可 "+std::to_string(miss),250,24,theme.ink);
    centered("最大コンボ "+std::to_string(maxCombo),310,23,theme.ink); centered("Enter: モード選択へ",520,18,theme.ink);
}
void finishNote(Note& note,bool perfect){note.judged=true;if(perfect){++good;++combo;score+=1000;}else{++ok;++combo;score+=500;}maxCombo=std::max(maxCombo,combo);}
void judge(bool inputDon){const int now=static_cast<int>(SDL_GetTicks()-playStart);int delta=100000;Note* best=nullptr;for(auto& note:notes)if(!note.judged){const int d=std::abs(note.ms-now);if(d<delta){delta=d;best=&note;}}if(!best||delta>115){combo=0;return;}bool expected=best->kind==Kind::Don;if(swapNotes)expected=!expected;if(inputDon!=expected){++miss;combo=0;best->judged=true;}else finishNote(*best,delta<=48);}
void restartChart(){generateChart(chartSeed,chartCount);}
void handleKey(SDL_Keycode key){
    if(screen==Screen::Title){if(key==SDLK_RETURN||key==SDLK_f||key==SDLK_j||key==SDLK_d||key==SDLK_k)screen=Screen::Modes;return;}
    if(screen==Screen::Modes){if(key==SDLK_ESCAPE)screen=Screen::Title;else if(key==SDLK_UP)modeIndex=(modeIndex+3)%4;else if(key==SDLK_DOWN)modeIndex=(modeIndex+1)%4;else if(key==SDLK_RETURN){if(modeIndex==0)screen=Screen::Songs;else if(modeIndex==1){modeIndex=0;screen=Screen::Dojo;}else if(modeIndex==2)screen=Screen::Online;else screen=Screen::Profile;}return;}
    if(screen==Screen::Songs){if(key==SDLK_ESCAPE)screen=Screen::Modes;else if(key==SDLK_LEFT||key==SDLK_RIGHT){genreIndex=(genreIndex+(key==SDLK_RIGHT?1:6))%7;songIndex=0;}else if(key==SDLK_UP||key==SDLK_DOWN){const int count=songsInGenre();songIndex=(songIndex+(key==SDLK_UP?std::max(0,count-1):1))%std::max(1,count);}else if(key==SDLK_TAB)optionIndex=(optionIndex+1)%4;else if(key==SDLK_RETURN)startSong();return;}
    if(screen==Screen::Dojo){if(key==SDLK_ESCAPE)screen=Screen::Modes;else if(key==SDLK_UP||key==SDLK_DOWN)modeIndex=(modeIndex+(key==SDLK_DOWN?1:2))%3;else if(key==SDLK_RETURN){inDojo=true;dojoStage=0;startSong(true);}return;}
    if(screen==Screen::Play){if(key==SDLK_ESCAPE){pauseStart=SDL_GetTicks();pauseIndex=0;screen=Screen::Pause;}else if(key==SDLK_f||key==SDLK_j)judge(true);else if(key==SDLK_d||key==SDLK_k)judge(false);return;}
    if(screen==Screen::Pause){if(key==SDLK_ESCAPE){playStart+=SDL_GetTicks()-pauseStart;screen=Screen::Play;}else if(key==SDLK_UP||key==SDLK_DOWN)pauseIndex=(pauseIndex+(key==SDLK_DOWN?1:2))%3;else if(key==SDLK_RETURN){if(pauseIndex==0){playStart+=SDL_GetTicks()-pauseStart;screen=Screen::Play;}else if(pauseIndex==1)restartChart();else{screen=Screen::Songs;inDojo=false;}}return;}
    if(screen==Screen::Result){if(key==SDLK_RETURN||key==SDLK_ESCAPE)screen=Screen::Modes;return;}
    if(screen==Screen::Profile){if(key==SDLK_ESCAPE)screen=Screen::Modes;return;}
    if(screen==Screen::Online){if(key==SDLK_ESCAPE)screen=Screen::Modes;else if(key==SDLK_UP||key==SDLK_DOWN)onlineIndex=(onlineIndex+(key==SDLK_DOWN?1:3))%4;else if(key==SDLK_LEFT||key==SDLK_RIGHT){const int delta=key==SDLK_RIGHT?1:-1;if(onlineIndex==0)roomPublic=!roomPublic;else if(onlineIndex==2)onlineModeIndex=(onlineModeIndex+delta+4)%4;else if(onlineIndex==3)batonMeasures=std::clamp(batonMeasures+delta,1,16);}return;}
}
void update(){if(screen!=Screen::Play)return;const int elapsed=static_cast<int>(SDL_GetTicks()-playStart);if(autoPlay&&!inDojo){for(auto& note:notes)if(!note.judged&&elapsed>=note.ms)finishNote(note,true);}for(auto& note:notes)if(!note.judged&&elapsed>note.ms+140){note.judged=true;++miss;combo=0;}if(!notes.empty()&&elapsed>notes.back().ms+2400){if(inDojo&&dojoStage<2){++dojoStage;generateChart(chartSeed+dojoStage*101,30);}else screen=Screen::Result;}}
}

int main(int argc,char** argv){
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0)return 1;
    if(TTF_Init()!=0){SDL_Quit();return 1;}
    window=SDL_CreateWindow("taiko wonder",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
    if(window)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!window||!renderer){if(window)SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();return 1;}
    const char* base=SDL_GetBasePath();appDir=base?fs::path(base):fs::current_path();if(base)SDL_free(const_cast<char*>(base));
    loadSkin();loadSongs();std::vector<std::string> fontPaths;
    const char* envFont=std::getenv("TAIKO_WONDER_FONT");if(envFont&&*envFont)fontPaths.emplace_back(envFont);
    if(!theme.font.empty())fontPaths.push_back((activeSkinDir/theme.font).string());
    if(argc>2&&std::string(argv[1])=="--font")fontPaths.emplace_back(argv[2]);
    fontPaths.insert(fontPaths.end(),{"C:/Windows/Fonts/meiryo.ttc","C:/Windows/Fonts/Yu Gothic UI.ttf","/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc","/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"});
    for(const auto& path:fontPaths){font=TTF_OpenFont(path.c_str(),24);if(font)break;}
    bool running=true;
    while(running){SDL_Event event;while(SDL_PollEvent(&event)){if(event.type==SDL_QUIT)running=false;else if(event.type==SDL_KEYDOWN&&!event.key.repeat)handleKey(event.key.keysym.sym);}update();switch(screen){case Screen::Title:drawTitle();break;case Screen::Modes:drawModes();break;case Screen::Songs:drawSongs();break;case Screen::Dojo:drawDojo();break;case Screen::Play:drawPlay();break;case Screen::Pause:drawPause();break;case Screen::Result:drawResult();break;case Screen::Profile:drawProfile();break;case Screen::Online:drawOnline();break;}SDL_RenderPresent(renderer);SDL_Delay(8);}
    if(skinBackground)SDL_DestroyTexture(skinBackground);if(font)TTF_CloseFont(font);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);TTF_Quit();SDL_Quit();return 0;
}
