#include <SDL.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <random>

namespace {
constexpr int W = 1000;
constexpr int H = 650;

enum class Screen {
    Title,
    SongSelect,
    DojoSelect,
    Play,
    Result,
    Profile,
    Online,
};

enum class NoteKind { Don, Ka };
struct Note { int timeMs = 0; NoteKind kind = NoteKind::Don; bool judged = false; bool hit = false; };
struct Options {
    float speed = 1.0f;
    bool swap = false;
    int random = 0;        // 0=off, 1=きまぐれ, 2=でたらめ
    bool autoPlay = false;
};
struct Profile {
    std::string name = "Player";
    std::string title = "はじめての太鼓";
    std::string epithet = "";
    int avatar = 0;
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;

Screen currentScreen = Screen::Title;
Options options;
Profile profile;

std::array<std::string, 4> songNames = {"Sakura practice", "Wonder beat", "Festival steps", "Summer parade"};
std::array<std::string, 5> titleItems = {"演奏モード", "段位道場", "オンライン", "プレイヤー設定", "終了"};
std::array<std::string, 4> avatars = {"デフォルト", "サンライト", "ネコ", "フェスティバル"};
std::array<std::string, 3> titleChoices = {"はじめての太鼓", "Wonder player", "祭りの達人"};

int titleIndex = 0;
int selectedSong = 0;
int optionIndex = 0;
int dojoIndex = 0;
int onlineModeIndex = 0;
int batonMeasure = 4;
bool roomPublic = true;
std::string roomName = "wonder-room";
bool editingName = false;
bool editingEpithet = false;
int profileFieldIndex = 0;

std::vector<Note> notes;
Uint32 playStartMs = 0;
int score = 0;
int combo = 0;
int maxCombo = 0;
int good = 0;
int ok = 0;
int miss = 0;

bool inDojo = false;
int dojoStage = 0;
int dojoSongCount = 3;

void setRenderColor(SDL_Color c) {
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

void fillRect(int x, int y, int w, int h, SDL_Color c) {
    setRenderColor(c);
    SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(renderer, &r);
}

void drawText(const std::string& text, int x, int y, int size = 24, SDL_Color color = {245, 242, 235, 255}) {
    if (!font) return;
    TTF_SetFontSize(font, size);
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect dst{x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

void drawCenteredText(const std::string& text, int y, int size, SDL_Color color = {245, 242, 235, 255}) {
    if (!font) return;
    TTF_SetFontSize(font, size);
    int tw = 0, th = 0;
    TTF_SizeUTF8(font, text.c_str(), &tw, &th);
    drawText(text, (W - tw) / 2, y, size, color);
}

void drawBackground() {
    fillRect(0, 0, W, H, {18, 15, 33, 255});
    fillRect(0, 0, W, 16, {255, 104, 128, 255});
    fillRect(28, 28, W - 56, H - 56, {31, 24, 53, 255});
    fillRect(28, 28, 10, H - 56, {255, 205, 88, 255});
}

void drawHeader(const std::string& text) {
    drawCenteredText(text, 48, 38, {255, 205, 88, 255});
}

void loadProfile() {
    std::ifstream in("taiko-wonder.profile");
    if (!in) return;
    std::getline(in, profile.name);
    std::getline(in, profile.title);
    std::getline(in, profile.epithet);
    in >> profile.avatar;
    if (profile.name.empty()) profile.name = "Player";
    if (profile.title.empty()) profile.title = "はじめての太鼓";
    if (profile.avatar < 0 || profile.avatar >= (int)avatars.size()) profile.avatar = 0;
}

void saveProfile() {
    std::ofstream out("taiko-wonder.profile");
    out << profile.name << '\n';
    out << profile.title << '\n';
    out << profile.epithet << '\n';
    out << profile.avatar << '\n';
}

std::string optionTextAt(int index) {
    switch (index) {
        case 0: {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << options.speed << "x";
            return ss.str();
        }
        case 1: return options.swap ? "ON" : "OFF";
        case 2: return options.random == 0 ? "OFF" : options.random == 1 ? "きまぐれ" : "でたらめ";
        case 3: return options.autoPlay ? "ON" : "OFF";
        default: return "戻る";
    }
}

bool noteMatchesExpected(NoteKind kind) {
    return (options.swap ? (kind == NoteKind::Ka ? true : false) : (kind == NoteKind::Don ? true : false));
}

void generateChart(int seed, int count = 42) {
    notes.clear();
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> gap(340, 640);
    std::uniform_int_distribution<int> typeRoll(0, 1);

    int t = 1700;
    for (int i = 0; i < count; ++i) {
        t += gap(rng);
        NoteKind kind = typeRoll(rng) == 0 ? NoteKind::Don : NoteKind::Ka;
        if (options.random == 1 && (i % 4 == 0)) {
            kind = typeRoll(rng) == 0 ? NoteKind::Don : NoteKind::Ka;
        }
        if (options.random == 2) {
            kind = typeRoll(rng) == 0 ? NoteKind::Don : NoteKind::Ka;
        }
        notes.push_back({t, kind, false, false});
    }

    score = 0;
    combo = 0;
    maxCombo = 0;
    good = 0;
    ok = 0;
    miss = 0;
    playStartMs = SDL_GetTicks();
    currentScreen = Screen::Play;
}

void startSong(bool dojoMode = false) {
    const int baseSeed = 100 + selectedSong * 77 + (dojoMode ? dojoStage * 101 : 0);
    generateChart(baseSeed, dojoMode ? 28 : 42);
    inDojo = dojoMode;
}

void drawTitleScreen() {
    drawBackground();
    drawCenteredText("taiko wonder", 78, 58, {255, 118, 142, 255});
    drawCenteredText("SimpleStyle-inspired rhythm simulator", 132, 18, {191, 181, 205, 255});

    for (int i = 0; i < (int)titleItems.size(); ++i) {
        int y = 188 + i * 64;
        SDL_Color c = (i == titleIndex) ? SDL_Color{232, 75, 105, 255} : SDL_Color{61, 52, 88, 255};
        fillRect(310, y, 380, 52, c);
        drawCenteredText(titleItems[i], y + 9, 24);
    }

    drawCenteredText("↑ / ↓ で選択、Enter で決定", 554, 19, {190, 182, 207, 255});
}

void drawSongSelect(bool dojo = false) {
    drawBackground();
    drawHeader(dojo ? "段位道場 - practice course" : "演奏モード - song select");

    if (dojo) {
        drawCenteredText("3曲連続の段位コース  •  公式譜面ではなく練習用生成譜面", 120, 19, {190, 182, 207, 255});
        for (int i = 0; i < 3; ++i) {
            int y = 170 + i * 72;
            fillRect(245, y, 510, 56, i == dojoIndex ? SDL_Color{232, 75, 105, 255} : SDL_Color{61, 52, 88, 255});
            drawCenteredText(songNames[i], y + 8, 24);
        }
        drawCenteredText("↑↓: コース選択  Enter: 開始  Esc: 戻る", 548, 19, {190, 182, 207, 255});
        return;
    }

    drawText("曲", 95, 140, 22, {255, 205, 88, 255});
    for (int i = 0; i < (int)songNames.size(); ++i) {
        int y = 170 + i * 58;
        fillRect(80, y, 400, 46, i == selectedSong ? SDL_Color{232, 75, 105, 255} : SDL_Color{61, 52, 88, 255});
        drawText(songNames[i], 110, y + 10, 21);
    }

    fillRect(520, 150, 390, 310, {48, 41, 71, 255});
    drawText("演奏オプション", 540, 170, 26, {255, 205, 88, 255});
    for (int i = 0; i < 4; ++i) {
        int y = 220 + i * 44;
        if (i == optionIndex) {
            fillRect(536, y - 4, 350, 32, {90, 74, 112, 255});
        }
        drawText("速度", 540, y, 19); // placeholder, replaced below in a more concise way
        drawText(optionTextAt(i), 760, y, 19, {255, 205, 88, 255});
    }
    drawText("速度", 540, 220, 19);
    drawText("あべこべ", 540, 264, 19);
    drawText("ランダム", 540, 308, 19);
    drawText("オート", 540, 352, 19);
    drawText(optionTextAt(0), 760, 220, 19, {255, 205, 88, 255});
    drawText(optionTextAt(1), 760, 264, 19, {255, 205, 88, 255});
    drawText(optionTextAt(2), 760, 308, 19, {255, 205, 88, 255});
    drawText(optionTextAt(3), 760, 352, 19, {255, 205, 88, 255});
    drawText("↑↓: 曲 / Tab: オプション / ←→: 値変更", 90, 445, 18, {190, 182, 207, 255});
    drawCenteredText("Enter: 開始  Esc: 戻る", 548, 19, {190, 182, 207, 255});
}

void drawProfileScreen() {
    drawBackground();
    drawHeader("プレイヤー設定");
    drawText("プレイヤー名", 160, 165, 23, {255, 205, 88, 255});
    drawText(profile.name + (editingName ? "_" : ""), 470, 165, 23);

    drawText("見た目", 160, 225, 23, {255, 205, 88, 255});
    drawText(avatars[profile.avatar], 470, 225, 23);

    drawText("称号", 160, 285, 23, {255, 205, 88, 255});
    drawText(profile.title, 470, 285, 23);

    drawText("二つ名", 160, 345, 23, {255, 205, 88, 255});
    drawText(profile.epithet + (editingEpithet ? "_" : ""), 470, 345, 23);

    drawCenteredText("↑↓: 項目選択  ←→: 見た目 / 称号変更  Enter: 編集", 430, 18, {190, 182, 207, 255});
    drawCenteredText("Esc: 保存して戻る", 548, 19, {190, 182, 207, 255});
}

void drawOnlineScreen() {
    drawBackground();
    drawHeader("オンライン - room setup");

    const std::array<std::string, 4> modes = {"スコアを競う", "仲良し演奏", "バトンタッチ演奏", "チーム戦"};

    drawText("公開 / 非公開", 150, 145, 22, {255, 205, 88, 255});
    drawText(roomPublic ? "公開ルーム" : "非公開ルーム (4桁コード)", 500, 145, 22);

    drawText("ルーム名", 150, 196, 22, {255, 205, 88, 255});
    drawText(roomName, 500, 196, 22);

    drawText("モード", 150, 247, 22, {255, 205, 88, 255});
    drawText(modes[onlineModeIndex], 500, 247, 22);

    drawText("段位道場", 150, 298, 22, {255, 205, 88, 255});
    drawText((onlineModeIndex == 1 || onlineModeIndex == 2) ? "仲良し / バトンタッチのみ" : "オンラインでは非対応", 500, 298, 18);

    drawText("バトンタッチ", 150, 349, 22, {255, 205, 88, 255});
    drawText(std::to_string(batonMeasure) + " 小節ごとに交代", 500, 349, 22);

    drawText("ホスト / 選曲権", 150, 400, 22, {255, 205, 88, 255});
    drawText("ホスト指定 → その人が選曲 / ホスト交代可能", 500, 400, 18);
    drawCenteredText("↑↓: 項目  ←→: 設定変更  Enter: ルーム作成  Esc: 戻る", 470, 18, {190, 182, 207, 255});
    drawCenteredText("オンラインの実際の通信は別サーバー実装が必要", 540, 17, {255, 134, 134, 255});
}

void drawPlayScreen() {
    drawBackground();
    const Uint32 elapsed = SDL_GetTicks() - playStartMs;

    drawCenteredText(inDojo ? "段位道場" : "演奏中", 34, 24, {255, 205, 88, 255});
    drawText("SCORE  " + std::to_string(score), 80, 85, 26);
    drawText("COMBO  " + std::to_string(combo), 770, 85, 26);

    fillRect(80, 305, 840, 8, {230, 220, 210, 255});
    fillRect(474, 267, 52, 72, {255, 205, 88, 255});

    for (const auto& n : notes) {
        if (n.judged) continue;
        const float appear = (n.timeMs - static_cast<int>(elapsed));
        const float x = 500.0f - appear * 0.32f * options.speed;
        if (x < 60 || x > 940) continue;
        bool targetDon = n.kind == NoteKind::Don;
        if (options.swap) targetDon = !targetDon;
        SDL_Color c = targetDon ? SDL_Color{255, 86, 102, 255} : SDL_Color{75, 177, 255, 255};
        fillRect((int)x - 18, 280, 36, 46, c);
    }

    drawCenteredText("DON: F / J       KA: D / K", 382, 22);
    drawCenteredText("良 " + std::to_string(good) + "   可 " + std::to_string(ok) + "   不可 " + std::to_string(miss), 445, 21, {190, 182, 207, 255});
    if (options.autoPlay) {
        drawCenteredText("AUTO PRACTICE", 514, 18, {255, 205, 88, 255});
    }
}

void drawResultScreen() {
    drawBackground();
    drawHeader(inDojo ? "段位道場結果" : "演奏結果");
    drawCenteredText("SCORE  " + std::to_string(score), 165, 42, {255, 205, 88, 255});
    drawCenteredText("良 " + std::to_string(good) + "    可 " + std::to_string(ok) + "    不可 " + std::to_string(miss), 240, 27, {245, 242, 235, 255});
    drawCenteredText("最大コンボ  " + std::to_string(maxCombo), 300, 27, {245, 242, 235, 255});
    drawCenteredText("Enter / Esc: タイトルへ", 510, 20, {190, 182, 207, 255});
}

void draw() {
    switch (currentScreen) {
        case Screen::Title: drawTitleScreen(); break;
        case Screen::SongSelect: drawSongSelect(false); break;
        case Screen::DojoSelect: drawSongSelect(true); break;
        case Screen::Play: drawPlayScreen(); break;
        case Screen::Result: drawResultScreen(); break;
        case Screen::Profile: drawProfileScreen(); break;
        case Screen::Online: drawOnlineScreen(); break;
    }
    SDL_RenderPresent(renderer);
}

void finishNote(Note& n, bool perfect) {
    n.judged = true;
    n.hit = perfect;
    if (perfect) {
        ++good;
        ++combo;
        score += 1000;
    } else {
        ++ok;
        ++combo;
        score += 500;
    }
    maxCombo = std::max(maxCombo, combo);
}

void judge(bool inputDon) {
    const int now = static_cast<int>(SDL_GetTicks() - playStartMs);
    Note* best = nullptr;
    int bestDelta = 100000;

    for (auto& n : notes) {
        if (!n.judged) {
            const int d = std::abs(n.timeMs - now);
            if (d < bestDelta) {
                bestDelta = d;
                best = &n;
            }
        }
    }

    if (!best || bestDelta > 115) {
        combo = 0;
        return;
    }

    const bool expectedDon = (best->kind == NoteKind::Don);
    if (options.swap) {
        const bool swapped = !expectedDon;
        if (inputDon != swapped) {
            combo = 0;
            ++miss;
            best->judged = true;
            return;
        }
    } else {
        if (inputDon != expectedDon) {
            combo = 0;
            ++miss;
            best->judged = true;
            return;
        }
    }

    finishNote(*best, bestDelta <= 48);
}

void advanceOption(int delta) {
    if (optionIndex == 0) {
        options.speed = std::clamp(options.speed + delta * 0.5f, 1.0f, 4.0f);
    } else if (optionIndex == 1) {
        options.swap = !options.swap;
    } else if (optionIndex == 2) {
        options.random = (options.random + delta + 3) % 3;
    } else if (optionIndex == 3) {
        options.autoPlay = !options.autoPlay;
    }
}

void handleKey(SDL_Keycode key) {
    if (currentScreen == Screen::Title) {
        if (key == SDLK_UP) titleIndex = (titleIndex + 4) % 5;
        if (key == SDLK_DOWN) titleIndex = (titleIndex + 1) % 5;
        if (key == SDLK_RETURN) {
            if (titleIndex == 0) currentScreen = Screen::SongSelect;
            if (titleIndex == 1) currentScreen = Screen::DojoSelect;
            if (titleIndex == 2) currentScreen = Screen::Online;
            if (titleIndex == 3) currentScreen = Screen::Profile;
            if (titleIndex == 4) SDL_Event quitEvent{}; quitEvent.type = SDL_QUIT; SDL_PushEvent(&quitEvent);
        }
    } else if (currentScreen == Screen::SongSelect) {
        if (key == SDLK_ESCAPE) currentScreen = Screen::Title;
        else if (key == SDLK_TAB) optionIndex = (optionIndex + 1) % 4;
        else if (key == SDLK_UP) selectedSong = (selectedSong + 3) % (int)songNames.size();
        else if (key == SDLK_DOWN) selectedSong = (selectedSong + 1) % (int)songNames.size();
        else if (key == SDLK_LEFT || key == SDLK_RIGHT) advanceOption(key == SDLK_RIGHT ? 1 : -1);
        else if (key == SDLK_RETURN) {
            inDojo = false;
            dojoStage = 0;
            startSong(false);
        }
    } else if (currentScreen == Screen::DojoSelect) {
        if (key == SDLK_ESCAPE) currentScreen = Screen::Title;
        else if (key == SDLK_UP) dojoIndex = (dojoIndex + 2) % 3;
        else if (key == SDLK_DOWN) dojoIndex = (dojoIndex + 1) % 3;
        else if (key == SDLK_RETURN) {
            inDojo = true;
            selectedSong = dojoIndex;
            dojoStage = 0;
            startSong(true);
        }
    } else if (currentScreen == Screen::Profile) {
        if (editingName || editingEpithet) {
            if (key == SDLK_RETURN) {
                editingName = false;
                editingEpithet = false;
                SDL_StopTextInput();
                saveProfile();
            } else if (key == SDLK_BACKSPACE) {
                std::string& target = editingName ? profile.name : profile.epithet;
                if (!target.empty()) {
                    target.pop_back();
                    while (!target.empty() && (static_cast<unsigned char>(target.back()) & 0xC0) == 0x80) {
                        target.pop_back();
                    }
                }
            }
        } else {
            if (key == SDLK_ESCAPE) {
                saveProfile();
                currentScreen = Screen::Title;
            } else if (key == SDLK_UP || key == SDLK_DOWN) {
                profileFieldIndex = (profileFieldIndex + (key == SDLK_DOWN ? 1 : 3)) % 4;
            } else if (key == SDLK_LEFT || key == SDLK_RIGHT) {
                int delta = key == SDLK_RIGHT ? 1 : -1;
                if (profileFieldIndex == 0) {
                    profile.avatar = (profile.avatar + delta + (int)avatars.size()) % (int)avatars.size();
                } else if (profileFieldIndex == 1) {
                    int index = std::find(titleChoices.begin(), titleChoices.end(), profile.title) - titleChoices.begin();
                    if (index >= (int)titleChoices.size()) index = 0;
                    profile.title = titleChoices[(index + delta + (int)titleChoices.size()) % (int)titleChoices.size()];
                }
            } else if (key == SDLK_RETURN) {
                if (profileFieldIndex == 0) {
                    editingName = true;
                    SDL_StartTextInput();
                } else if (profileFieldIndex == 1) {
                    // title is handled by left/right, no editing
                } else if (profileFieldIndex == 2) {
                    editingEpithet = true;
                    SDL_StartTextInput();
                }
            }
        }
    } else if (currentScreen == Screen::Online) {
        if (key == SDLK_ESCAPE) currentScreen = Screen::Title;
        else if (key == SDLK_UP) {
            // no-op screen navigation placeholder
        } else if (key == SDLK_DOWN) {
            // no-op screen navigation placeholder
        } else if (key == SDLK_LEFT || key == SDLK_RIGHT) {
            int delta = key == SDLK_RIGHT ? 1 : -1;
            if (key == SDLK_LEFT || key == SDLK_RIGHT) {
                if (onlineModeIndex == 0) roomPublic = !roomPublic;
                if (onlineModeIndex == 2) batonMeasure = std::clamp(batonMeasure + delta, 1, 16);
            }
        } else if (key == SDLK_RETURN) {
            // online room creation placeholder: logic intentionally not connected to a real server yet.
        }
    } else if (currentScreen == Screen::Play) {
        if (key == SDLK_f || key == SDLK_j) judge(true);
        if (key == SDLK_d || key == SDLK_k) judge(false);
        if (key == SDLK_ESCAPE) currentScreen = Screen::Result;
    } else if (currentScreen == Screen::Result) {
        if (key == SDLK_RETURN || key == SDLK_ESCAPE) currentScreen = Screen::Title;
    }
}

void updatePlay() {
    if (currentScreen != Screen::Play) return;
    const int elapsed = static_cast<int>(SDL_GetTicks() - playStartMs);

    if (options.autoPlay && !inDojo) {
        for (auto& n : notes) {
            if (!n.judged && elapsed >= n.timeMs) {
                finishNote(n, true);
            }
        }
    }

    for (auto& n : notes) {
        if (!n.judged && elapsed > n.timeMs + 140) {
            n.judged = true;
            ++miss;
            combo = 0;
        }
    }

    if (!notes.empty() && elapsed > notes.back().timeMs + 2500) {
        if (inDojo && dojoStage < 2) {
            ++dojoStage;
            selectedSong = std::min(selectedSong + 1, (int)songNames.size() - 1);
            startSong(true);
        } else {
            currentScreen = Screen::Result;
        }
    }
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return 1;
    if (TTF_Init() != 0) { SDL_Quit(); return 1; }

    window = SDL_CreateWindow("taiko wonder", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    const std::array<const char*, 7> fontCandidates = {
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/fonts-japanese-gothic.ttf",
        "/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc",
        "C:/Windows/Fonts/meiryo.ttc",
        "C:/Windows/Fonts/Yu Gothic UI.ttf",
        ""
    };

    for (auto path : fontCandidates) {
        if (*path) {
            font = TTF_OpenFont(path, 24);
            if (font) break;
        }
    }

    if (!window || !renderer) {
        if (font) TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    loadProfile();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                handleKey(event.key.keysym.sym);
            } else if (event.type == SDL_TEXTINPUT && (editingName || editingEpithet)) {
                std::string& target = editingName ? profile.name : profile.epithet;
                if (target.size() < 32) {
                    target += event.text.text;
                }
            }
        }

        updatePlay();
        draw();
        SDL_Delay(8);
    }

    saveProfile();
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

}
