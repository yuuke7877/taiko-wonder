#include <SDL.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace {
constexpr int W = 1000, H = 650;
enum class Screen { Title, Select, Play, Result, Profile, Online, Dojo };
enum class NoteKind { Don, Ka };
enum class PlayMode { Solo, Dojo, OnlineFriendly, OnlineBaton, OnlineScore, Team };
struct Note { int timeMs; NoteKind kind; bool judged = false; };
struct Options { float speed = 1.0f; bool swap = false; int random = 0; bool automatic = false; };

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
Screen screen = Screen::Title;
PlayMode playMode = PlayMode::Solo;
Options options;
std::string playerName = "Player", playerTitle = "はじめての太鼓", epithet = "";
int avatar = 0;
const std::array<std::string, 4> avatars = {"赤い太鼓", "青い太鼓", "ねこ", "おまつり"};
const std::array<std::string, 3> titles = {"はじめての太鼓", "Wonder player", "祭りの達人"};
const std::array<std::string, 4> songs = {"Sakura practice", "Wonder beat", "Festival steps", "Summer parade"};
const std::array<std::string, 5> titleItems = {"演奏モード", "段位道場", "オンライン", "プレイヤー設定", "終了"};
const std::array<std::string, 5> optionNames = {"速度", "あべこべ", "でたらめ / きまぐれ", "オート", "戻る"};
int selected = 0, menuIndex = 0, optionIndex = 0;
int score = 0, combo = 0, maxCombo = 0, good = 0, ok = 0, miss = 0;
std::vector<Note> notes;
Uint32 playStart = 0;
bool editingName = false, editingEpithet = false;
int dojoStage = 0;
int batonMeasures = 4;
bool roomPublic = true;
int onlineModeIndex = 0;
std::string roomName = "wonder-room";

void color(SDL_Color c) { SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a); }
void box(int x, int y, int w, int h, SDL_Color c) {
  color(c); SDL_Rect r{x, y, w, h}; SDL_RenderFillRect(renderer, &r);
}
void text(const std::string& value, int x, int y, int size = 25, SDL_Color c = {245,242,235,255}) {
  if (!font) return;
  TTF_SetFontSize(font, size);
  SDL_Surface* surface = TTF_RenderUTF8_Blended(font, value.c_str(), c);
  if (!surface) return;
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_Rect dst{x, y, surface->w, surface->h};
  if (texture) { SDL_RenderCopy(renderer, texture, nullptr, &dst); SDL_DestroyTexture(texture); }
  SDL_FreeSurface(surface);
}
void centered(const std::string& value, int y, int size, SDL_Color c = {245,242,235,255}) {
  if (!font) return;
  TTF_SetFontSize(font, size);
  int tw = 0, th = 0;
  TTF_SizeUTF8(font, value.c_str(), &tw, &th);
  text(value, (W - tw) / 2, y, size, c);
}
void background() {
  box(0, 0, W, H, {22,17,42,255});
  box(0, 0, W, 12, {255,89,116,255});
  box(26, 28, 948, 594, {34,28,58,255});
  box(26, 28, 10, 594, {255,205,88,255});
}
void heading(const std::string& value) { centered(value, 48, 38, {255,205,88,255}); }
void loadProfile() {
  std::ifstream in("taiko-wonder.profile");
  std::getline(in, playerName); std::getline(in, playerTitle); std::getline(in, epithet);
  in >> avatar;
  if (playerName.empty()) playerName = "Player";
  if (playerTitle.empty()) playerTitle = titles[0];
  if (avatar < 0 || avatar >= (int)avatars.size()) avatar = 0;
}
void saveProfile() {
  std::ofstream out("taiko-wonder.profile");
  out << playerName << '\n' << playerTitle << '\n' << epithet << '\n' << avatar << '\n';
}
void resetResults() { score = combo = maxCombo = good = ok = miss = 0; }
void makeChart(int seed, int count = 42) {
  notes.clear(); std::mt19937 rng(seed);
  std::uniform_int_distribution<int> gap(330, 620), type(0, 1);
  int t = 1700;
  for (int i = 0; i < count; ++i) {
    t += gap(rng);
    NoteKind kind = type(rng) ? NoteKind::Don : NoteKind::Ka;
    if (options.random == 1 && i % 4 == 0) kind = type(rng) ? NoteKind::Don : NoteKind::Ka;
    if (options.random == 2) kind = type(rng) ? NoteKind::Don : NoteKind::Ka;
    notes.push_back({t, kind, false});
  }
  resetResults(); playStart = SDL_GetTicks(); screen = Screen::Play;
}
void startSong() {
  if (screen == Screen::Dojo) { playMode = PlayMode::Dojo; dojoStage = 0; }
  makeChart(170 + selected * 83 + dojoStage * 117, screen == Screen::Dojo ? 30 : 42);
}
void titleScreen() {
  background(); centered("taiko wonder", 75, 58, {255,112,126,255});
  centered("DRUM  •  RHYTHM  •  WONDER", 139, 16, {190,182,207,255});
  for (int i = 0; i < (int)titleItems.size(); ++i) {
    int y = 190 + i * 64;
    box(300, y, 400, 50, i == menuIndex ? SDL_Color{232,75,105,255} : SDL_Color{61,52,88,255});
    centered(titleItems[i], y + 8, 26);
  }
  centered("↑ / ↓ で選択、Enter で決定", 548, 19, {190,182,207,255});
}
std::string optionValue(int i) {
  if (i == 0) { char buf[32]; std::snprintf(buf, sizeof(buf), "%.1fx", options.speed); return buf; }
  if (i == 1) return options.swap ? "ON" : "OFF";
  if (i == 2) return options.random == 0 ? "OFF" : options.random == 1 ? "きまぐれ" : "でたらめ";
  if (i == 3) return options.automatic ? "ON" : "OFF";
  return "決定";
}
void selectionScreen(bool dojo = false) {
  background(); heading(dojo ? "段位道場  /  練習コース" : "演奏モード  /  曲を選ぶ");
  if (dojo) {
    centered("3曲連続の練習コース  •  公式譜面・段位認定は含みません", 115, 19, {190,182,207,255});
    for (int i = 0; i < 3; ++i) {
      int y = 165 + i * 72;
      box(245, y, 510, 54, i == selected ? SDL_Color{232,75,105,255} : SDL_Color{61,52,88,255});
      centered(songs[i], y + 10, 24);
    }
    centered("↑↓: コース選択  Enter: 開始  Esc: 戻る", 548, 19, {190,182,207,255});
    return;
  }
  text("曲", 94, 132, 22, {255,205,88,255});
  for (int i = 0; i < 4; ++i) {
    int y = 165 + i * 62;
    box(75, y, 400, 49, i == selected ? SDL_Color{232,75,105,255} : SDL_Color{61,52,88,255});
    text(songs[i], 100, y + 10, 22);
  }
  box(510, 135, 410, 355, {48,41,71,255});
  text("演奏オプション", 535, 151, 25, {255,205,88,255});
  for (int i = 0; i < 5; ++i) {
    int y = 200 + i * 49;
    if (optionIndex == i) box(527, y - 4, 374, 39, {75,62,97,255});
    text(optionNames[i], 540, y, 19);
    text(optionValue(i), 754, y, 19, {255,205,88,255});
  }
  text("↑↓: 曲 / Tab: オプション項目", 90, 445, 18, {190,182,207,255});
  centered("← →: 設定変更  Enter: 開始  Esc: 戻る", 548, 19, {190,182,207,255});
}
void profileScreen() {
  background(); heading("プレイヤー設定");
  text("プレイヤー名", 160, 155, 23, {255,205,88,255});
  text(playerName + (editingName ? "_" : ""), 470, 155, 23);
  text("見た目", 160, 213, 23, {255,205,88,255});
  text(avatars[avatar], 470, 213, 23);
  text("称号", 160, 271, 23, {255,205,88,255});
  text(playerTitle, 470, 271, 23);
  text("二つ名", 160, 329, 23, {255,205,88,255});
  text(epithet + (editingEpithet ? "_" : ""), 470, 329, 23);
  centered("↑↓: 項目選択  ←→: 見た目 / 称号変更  Enter: 編集", 434, 18, {190,182,207,255});
  centered("Esc: 保存して戻る", 548, 19, {190,182,207,255});
}
void onlineScreen() {
  background(); heading("オンラインルーム設定");
  const std::array<std::string, 4> modes = {"スコアを競う", "仲良し演奏", "バトンタッチ演奏", "チーム戦 (2v2 / 4v4)"};
  text("ルーム公開", 150, 145, 22, {255,205,88,255});
  text(roomPublic ? "公開ルーム" : "非公開ルーム (4桁コード)", 500, 145, 22);
  text("ルーム名", 150, 196, 22, {255,205,88,255}); text(roomName, 500, 196, 22);
  text("モード", 150, 247, 22, {255,205,88,255});
  text(modes[onlineModeIndex], 500, 247, 22);
  text("段位道場", 150, 298, 22, {255,205,88,255});
  text(onlineModeIndex == 1 || onlineModeIndex == 2 ? "選択可能 (仲良し / バトンタッチ)" : "このモードでは選択不可", 500, 298, 18);
  text("バトンタッチ", 150, 349, 22, {255,205,88,255});
  text(std::to_string(batonMeasures) + " 小節ごとに交代", 500, 349, 22);
  text("選曲担当", 150, 400, 22, {255,205,88,255});
  text("ホスト / ホスト指定プレイヤー (ホストが切替)", 500, 400, 18);
  centered("↑↓: 項目  ←→: 変更  Enter: ルーム作成設定  Esc: 戻る", 482, 18, {190,182,207,255});
  centered("オンライン演奏には、別途サーバー接続・同期実装が必要です", 542, 17, {255,130,130,255});
}
void drawPlay() {
  background();
  const int elapsed = (int)(SDL_GetTicks() - playStart);
  const std::string mode = playMode == PlayMode::Dojo ? "段位道場 練習" : "演奏中";
  centered(mode + "   " + playerName, 42, 23, {255,205,88,255});
  text("SCORE  " + std::to_string(score), 75, 100, 26);
  text("COMBO  " + std::to_string(combo), 760, 100, 26);
  box(80, 296, 840, 7, {230,220,210,255});
  box(474, 265, 52, 70, {255,205,88,255});
  for (const auto& n : notes) {
    if (n.judged) continue;
    const float x = 500.0f - (n.timeMs - elapsed) * 0.32f * options.speed;
    if (x < 55 || x > 950) continue;
    bool don = n.kind == NoteKind::Don; if (options.swap) don = !don;
    const SDL_Color c = don ? SDL_Color{255,86,102,255} : SDL_Color{75,177,255,255};
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_Rect note{(int)x - 18, 278, 36, 42}; SDL_RenderFillRect(renderer, &note);
    color({255,255,255,255}); SDL_Rect shine{(int)x - 9, 283, 8, 8}; SDL_RenderFillRect(renderer, &shine);
  }
  centered("DON: F / J       KA: D / K", 374, 22);
  centered("良 " + std::to_string(good) + "    可 " + std::to_string(ok) + "    不可 " + std::to_string(miss), 440, 21, {190,182,207,255});
  if (options.automatic && playMode != PlayMode::OnlineFriendly && playMode != PlayMode::OnlineBaton && playMode != PlayMode::OnlineScore && playMode != PlayMode::Team)
    centered("AUTO PRACTICE", 500, 18, {255,205,88,255});
}
void finishNote(Note& n, bool perfect) {
  n.judged = true;
  if (perfect) { ++good; ++combo; score += 1000; }
  else { ++ok; ++combo; score += 500; }
  maxCombo = std::max(maxCombo, combo);
}
void judge(bool inputDon) {
  const int now = (int)(SDL_GetTicks() - playStart);
  Note* best = nullptr; int delta = 100000;
  for (auto& n : notes) if (!n.judged) {
    int d = std::abs(n.timeMs - now); if (d < delta) { delta = d; best = &n; }
  }
  if (!best || delta > 115) { combo = 0; return; }
  bool expectedDon = best->kind == NoteKind::Don; if (options.swap) expectedDon = !expectedDon;
  if (inputDon != expectedDon) { combo = 0; ++miss; best->judged = true; return; }
  finishNote(*best, delta <= 48);
}
void drawResult() {
  background(); heading(screen == Screen::Result && playMode == PlayMode::Dojo ? "段位道場 練習結果" : "演奏結果");
  centered("SCORE  " + std::to_string(score), 160, 42, {255,205,88,255});
  centered("良 " + std::to_string(good) + "     可 " + std::to_string(ok) + "     不可 " + std::to_string(miss), 240, 26);
  centered("最大コンボ  " + std::to_string(maxCombo), 300, 26);
  centered("Enter / Esc: タイトルへ", 510, 20, {190,182,207,255});
}
void draw() {
  switch (screen) {
    case Screen::Title: titleScreen(); break;
    case Screen::Select: selectionScreen(); break;
    case Screen::Dojo: selectionScreen(true); break;
    case Screen::Profile: profileScreen(); break;
    case Screen::Online: onlineScreen(); break;
    case Screen::Play: drawPlay(); break;
    case Screen::Result: drawResult(); break;
  }
  SDL_RenderPresent(renderer);
}
void adjustOption(int direction) {
  if (optionIndex == 0) options.speed = std::clamp(options.speed + direction * 0.5f, 1.0f, 4.0f);
  else if (optionIndex == 1) options.swap = !options.swap;
  else if (optionIndex == 2) options.random = (options.random + direction + 3) % 3;
  else if (optionIndex == 3) options.automatic = !options.automatic;
}
void handleKey(SDL_Keycode key) {
  if (screen == Screen::Title) {
    if (key == SDLK_UP) menuIndex = (menuIndex + 4) % 5;
    if (key == SDLK_DOWN) menuIndex = (menuIndex + 1) % 5;
    if (key == SDLK_RETURN) {
      if (menuIndex == 0) { selected = 0; screen = Screen::Select; }
      if (menuIndex == 1) { selected = 0; screen = Screen::Dojo; }
      if (menuIndex == 2) { menuIndex = 0; screen = Screen::Online; }
      if (menuIndex == 3) { menuIndex = 0; screen = Screen::Profile; }
    }
  } else if (screen == Screen::Select) {
    if (key == SDLK_ESCAPE) screen = Screen::Title;
    else if (key == SDLK_TAB) optionIndex = (optionIndex + 1) % 5;
    else if (key == SDLK_UP) selected = (selected + (int)songs.size() - 1) % (int)songs.size();
    else if (key == SDLK_DOWN) selected = (selected + 1) % (int)songs.size();
    else if (key == SDLK_LEFT || key == SDLK_RIGHT) adjustOption(key == SDLK_RIGHT ? 1 : -1);
    else if (key == SDLK_RETURN) { playMode = PlayMode::Solo; startSong(); }
  } else if (screen == Screen::Dojo) {
    if (key == SDLK_ESCAPE) screen = Screen::Title;
    else if (key == SDLK_UP) selected = (selected + 2) % 3;
    else if (key == SDLK_DOWN) selected = (selected + 1) % 3;
    else if (key == SDLK_RETURN) startSong();
  } else if (screen == Screen::Profile) {
    if (editingName || editingEpithet) {
      if (key == SDLK_RETURN) { editingName = editingEpithet = false; SDL_StopTextInput(); saveProfile(); }
      else if (key == SDLK_BACKSPACE) {
        std::string& target = editingName ? playerName : epithet;
        if (!target.empty()) { target.pop_back(); while (!target.empty() && (static_cast<unsigned char>(target.back()) & 0xC0) == 0x80) target.pop_back(); }
      }
    } else if (key == SDLK_ESCAPE) { saveProfile(); screen = Screen::Title; }
    else if (key == SDLK_UP || key == SDLK_DOWN) menuIndex = (menuIndex + (key == SDLK_DOWN ? 1 : 3)) % 4;
    else if (key == SDLK_LEFT || key == SDLK_RIGHT) {
      int d = key == SDLK_RIGHT ? 1 : -1;
      if (menuIndex == 1) avatar = (avatar + d + (int)avatars.size()) % (int)avatars.size();
      if (menuIndex == 2) {
        auto it = std::find(titles.begin(), titles.end(), playerTitle);
        int idx = it == titles.end() ? 0 : (int)(it - titles.begin());
        playerTitle = titles[(idx + d + (int)titles.size()) % (int)titles.size()];
      }
    } else if (key == SDLK_RETURN) {
      if (menuIndex == 0) { editingName = true; SDL_StartTextInput(); }
      else if (menuIndex == 3) { editingEpithet = true; SDL_StartTextInput(); }
    }
  } else if (screen == Screen::Online) {
    if (key == SDLK_ESCAPE) screen = Screen::Title;
    else if (key == SDLK_UP) menuIndex = (menuIndex + 4) % 5;
    else if (key == SDLK_DOWN) menuIndex = (menuIndex + 1) % 5;
    else if (key == SDLK_LEFT || key == SDLK_RIGHT) {
      int d = key == SDLK_RIGHT ? 1 : -1;
      if (menuIndex == 0) roomPublic = !roomPublic;
      if (menuIndex == 2) onlineModeIndex = (onlineModeIndex + d + 4) % 4;
      if (menuIndex == 4) batonMeasures = std::clamp(batonMeasures + d, 1, 16);
    } else if (key == SDLK_RETURN && (menuIndex == 1 || menuIndex == 3)) {
      if (menuIndex == 1) { roomName = roomName == "wonder-room" ? "taiko-room" : "wonder-room"; }
      else { playMode = onlineModeIndex == 0 ? PlayMode::OnlineScore : onlineModeIndex == 1 ? PlayMode::OnlineFriendly : onlineModeIndex == 2 ? PlayMode::OnlineBaton : PlayMode::Team; }
    }
  } else if (screen == Screen::Play) {
    if (key == SDLK_f || key == SDLK_j) judge(true);
    if (key == SDLK_d || key == SDLK_k) judge(false);
    if (key == SDLK_ESCAPE) screen = Screen::Result;
  } else if (screen == Screen::Result && (key == SDLK_RETURN || key == SDLK_ESCAPE)) screen = Screen::Title;
}
}

int main(int, char**) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return 1;
  if (TTF_Init() != 0) { SDL_Quit(); return 1; }
  window = SDL_CreateWindow("taiko wonder", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_SHOWN);
  if (window) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  const std::array<const char*, 7> fontCandidates = {
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/truetype/fonts-japanese-gothic.ttf",
    "/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc",
    "C:/Windows/Fonts/meiryo.ttc", "C:/Windows/Fonts Yu Gothic UI.ttf", ""
  };
  for (auto path : fontCandidates) { if (*path) { font = TTF_OpenFont(path, 25); if (font) break; } }
  if (!window || !renderer) { if (window) SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1; }
  loadProfile(); bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
      else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
        if (screen == Screen::Title && menuIndex == 4 && event.key.keysym.sym == SDLK_RETURN) running = false;
        else handleKey(event.key.keysym.sym);
      } else if (event.type == SDL_TEXTINPUT && (editingName || editingEpithet)) {
        std::string& target = editingName ? playerName : epithet;
        if (target.size() < 48) target += event.text.text;
      }
    }
    if (screen == Screen::Play) {
      int elapsed = (int)(SDL_GetTicks() - playStart);
      const bool online = playMode == PlayMode::OnlineFriendly || playMode == PlayMode::OnlineBaton || playMode == PlayMode::OnlineScore || playMode == PlayMode::Team;
      if (options.automatic && !online) for (auto& n : notes) if (!n.judged && elapsed >= n.timeMs) finishNote(n, true);
      for (auto& n : notes) if (!n.judged && elapsed > n.timeMs + 140) { n.judged = true; ++miss; combo = 0; }
      if (!notes.empty() && elapsed > notes.back().timeMs + 2600) {
        if (playMode == PlayMode::Dojo && dojoStage < 2) { ++dojoStage; makeChart(170 + selected * 83 + dojoStage * 117, 30); }
        else screen = Screen::Result;
      }
    }
    draw(); SDL_Delay(8);
  }
  saveProfile(); if (font) TTF_CloseFont(font); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit();
  return 0;
}
