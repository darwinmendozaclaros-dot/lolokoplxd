#include "gfx/Sprites.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace {
    constexpr int N = Sprites::CELL;
    using Grid = std::array<std::array<char, N>, N>;

    struct Seg { int y, x; const char* s; };
    using Segs = std::vector<Seg>;

    void Clear(Grid& g) { for (auto& r : g) r.fill('.'); }

    void Put(Grid& g, const Segs& segs, bool mirror) {
        for (const Seg& sg : segs) {
            for (int i = 0; sg.s[i]; ++i) {
                if (sg.s[i] == '.') continue;
                const int cx = mirror ? (N - 1) - (sg.x + i) : sg.x + i;
                if (sg.y >= 0 && sg.y < N && cx >= 0 && cx < N) g[sg.y][cx] = sg.s[i];
            }
        }
    }

    struct Mat  { char c; Color t[4]; };
    struct Neon { char c; Color col; };

    constexpr Color KCOL { 10, 6, 20, 255 };
    constexpr Color OUTL { 5, 3, 12, 255 };

    const Neon NEONS[] = {
        { 'm', { 255, 43, 214, 255 } }, { 'r', { 255, 48, 64, 255 } }, { 'c', { 24, 240, 255, 255 } },
        { 'w', { 240, 255, 255, 255 } }, { 'y', { 255, 230, 60, 255 } },
    };
    const Mat BOSS_MATS[] = {
        { 'a', { { 40, 18, 70, 255 },  { 84, 38, 140, 255 },  { 130, 66, 200, 255 }, { 190, 130, 250, 255 } } },
        { 'e', { { 50, 56, 80, 255 },  { 110, 120, 150, 255 }, { 165, 175, 200, 255 }, { 225, 232, 245, 255 } } },
        { 'h', { { 18, 20, 34, 255 },  { 40, 44, 66, 255 },   { 70, 76, 104, 255 },  { 110, 118, 150, 255 } } },
    };
    const Mat HERO_MATS[] = {
        { 'b', { { 20, 36, 110, 255 }, { 42, 92, 255, 255 },  { 90, 140, 255, 255 }, { 160, 200, 255, 255 } } },
        { 'q', { { 14, 24, 70, 255 },  { 26, 56, 170, 255 },  { 50, 90, 200, 255 },  { 90, 130, 230, 255 } } },
        { 'o', { { 150, 70, 10, 255 }, { 255, 150, 30, 255 }, { 255, 200, 80, 255 }, { 255, 235, 160, 255 } } },
        { 'e', { { 50, 56, 80, 255 },  { 110, 120, 150, 255 }, { 165, 175, 200, 255 }, { 225, 232, 245, 255 } } },
        { 'h', { { 18, 20, 34, 255 },  { 40, 44, 66, 255 },   { 70, 76, 104, 255 },  { 110, 118, 150, 255 } } },
    };

    template <size_t NM>
    const Mat* FindMat(const Mat (&mats)[NM], char c) {
        for (const Mat& m : mats) if (m.c == c) return &m;
        return nullptr;
    }
    const Neon* FindNeon(char c) {
        for (const Neon& n : NEONS) if (n.c == c) return &n;
        return nullptr;
    }

    template <size_t NM>
    void PaintFrame(const Grid& g, Image& img, Image& glow, int ox, const Mat (&mats)[NM]) {
        auto diff = [&](int yy, int xx, char c) {
            return yy < 0 || yy >= N || xx < 0 || xx >= N || g[yy][xx] != c;
        };
        for (int y = 0; y < N; ++y) {
            for (int x = 0; x < N; ++x) {
                const char c = g[y][x];
                if (c == '.') continue;
                Color col = KCOL;
                bool em = false;
                if (c == 'k') {
                    col = KCOL;
                } else if (const Neon* n = FindNeon(c)) {
                    col = n->col; em = true;
                } else if (const Mat* m = FindMat(mats, c)) {
                    int tone = 1;
                    if (diff(y - 1, x, c)) ++tone;
                    if (diff(y, x - 1, c)) ++tone;
                    if (diff(y + 1, x, c)) --tone;
                    if (diff(y, x + 1, c)) --tone;
                    if (tone == 1 && ((x + y) & 3) == 0 && y < 22) tone = 2;
                    tone = std::max(0, std::min(3, tone));
                    col = m->t[tone];
                } else {
                    col = Color{ 255, 0, 255, 255 };
                }
                ImageDrawPixel(&img, ox + x, y, col);
                if (em) ImageDrawPixel(&glow, ox + x, y, col);
            }
        }
        for (int y = 0; y < N; ++y) {
            for (int x = 0; x < N; ++x) {
                if (g[y][x] != '.') continue;
                const bool near = (y > 0 && g[y - 1][x] != '.') || (y < N - 1 && g[y + 1][x] != '.') ||
                                  (x > 0 && g[y][x - 1] != '.') || (x < N - 1 && g[y][x + 1] != '.');
                if (near) ImageDrawPixel(&img, ox + x, y, OUTL);
            }
        }
    }

    const Segs BOSS_BASE = {
        {0,4,"aa"},{0,13,"aa"},
        {1,4,"aa"},{1,12,"aaaa"},
        {2,5,"aa"},{2,12,"aaaa"},
        {3,5,"aaa"},{3,11,"aaaaa"},
        {4,6,"aaa"},{4,10,"aaaaaa"},
        {5,7,"aaaaaaaaa"},
        {6,6,"aaaaaaaaaa"},
        {7,6,"akkkkkkkkk"},
        {8,6,"akrrrrrrrr"},
        {9,6,"akkrrrrrrw"},
        {10,6,"akkkkkkkkk"},
        {11,6,"aaaaaaaaaa"},
        {12,5,"eaaeeeeemmm"},
        {13,5,"eeekmkmkmkm"},
        {14,2,"aaaaaa"},{14,9,"eeeeeee"},
        {15,1,"aaaaaaaakaaaaaa"},
        {16,1,"aaaaaaaakaaeeee"},
        {17,1,"aaeaaaaakaaemmm"},
        {18,1,"aaaaaaaakaaemww"},
        {19,1,"aaaaaaaakaaemmm"},
        {20,2,"aaaaaa"},{20,9,"kaaeeee"},
        {21,8,"kaaaaaaa"},
        {22,8,"hhhekhhh"},
        {23,8,"hheeeehh"},
        {24,8,"hhhhyyyy"},
        {25,9,"hhhhhhh"},
    };
    const Segs ARM_REST = {
        {21,2,"eeee"},{22,2,"eaae"},{23,2,"eaae"},{24,2,"hhhh"},{25,2,"eaae"},{26,2,"eaae"},
        {27,1,"eeeeee"},{28,1,"emmmme"},{29,1,"eeeeee"},
    };
    const Segs ARM_RAISED = {
        {2,1,"eeeeee"},{3,1,"emmmme"},{4,1,"eeeeee"},{5,2,"eaae"},{6,2,"eaae"},{7,2,"hhhh"},
        {8,2,"eaae"},{9,2,"eaae"},{10,2,"hhhh"},{11,2,"eaae"},{12,2,"eaae"},{13,2,"eaae"},
    };
    const Segs ARM_SLAM = {
        {21,2,"eeee"},{22,2,"eaae"},{23,2,"eaae"},{24,2,"hhhh"},{25,2,"eaae"},{26,1,"eeaaee"},
        {27,1,"eaaaae"},{28,1,"emmmme"},{29,1,"emmmme"},{30,1,"eeeeee"},
    };
    const Segs ARM_CANNON = { {16,1,"eeeeeeeeee"},{17,1,"ceeeeeeeee"},{18,1,"eeeeeeeeee"} };

    const Segs LEG_STAND  = { {26,8,"eaaaae"},{27,8,"eaaaae"},{28,8,"hhhhhh"},{29,8,"eaaaae"},{30,7,"eeeeeeee"} };
    const Segs LEG_LIFT   = { {26,8,"eaaaae"},{27,8,"hhhhhh"},{28,8,"eaaae"},{29,7,"eeeeeee"} };
    const Segs LEG_TUCK   = { {26,8,"eaaaae"},{27,7,"eaaaaae"},{28,6,"eeaaaee"},{29,6,"eeeeee"} };
    const Segs LEG_SPREAD = { {26,6,"eaaaae"},{27,6,"eaaaae"},{28,6,"hhhhhh"},{29,6,"eaaaae"},{30,5,"eeeeeeee"} };

    struct BossPose {
        const Segs* armL; const Segs* armL2;
        const Segs* armR; const Segs* armR2;
        const Segs* legL; const Segs* legR;
    };

    void BuildBoss(Grid& g, const BossPose& p) {
        Clear(g);
        Put(g, BOSS_BASE, false); Put(g, BOSS_BASE, true);
        Put(g, *p.legL, false);   Put(g, *p.legR, true);
        Put(g, *p.armL, false);   if (p.armL2) Put(g, *p.armL2, false);
        Put(g, *p.armR, true);    if (p.armR2) Put(g, *p.armR2, true);
    }

    const Segs HERO_BASE = {
        {0,9,"y"},
        {1,10,"ee"},{1,12,"bbbbbb"},
        {2,11,"bbbbbbbb"},
        {3,10,"bbbbbbbbbb"},
        {4,10,"bbbkcccccb"},
        {5,10,"bbbkcwwccb"},
        {6,10,"bbbkkkkkkb"},
        {7,10,"bbbbbbbbbb"},
        {8,11,"bbeeeebb"},
        {9,13,"hhhh"},
        {10,6,"eeee"},{11,5,"eeeee"},{12,5,"ehhhe"},{13,5,"ehyhe"},{14,5,"ehhhe"},{15,5,"eeeee"},{16,6,"eee"},{17,6,"hkh"},
        {10,11,"bbbbbbbbbb"},
        {11,10,"bbbbbbbbbbbb"},
        {12,10,"bbboooobbbbb"},
        {13,10,"bbbooccobbbb"},
        {14,10,"bbbboooobbbb"},
        {15,11,"bbbbbbbbbb"},
        {16,11,"hhhhoyyohh"},
        {17,12,"bbbbbbbb"},
        {18,12,"bbebbebb"},
        {19,12,"eeeeeeee"},
    };
    const Segs HERO_ARM_READY = { {11,19,"bbb"},{12,19,"bbbeeeee"},{13,20,"bbehhhhc"},{14,21,"eee"} };
    const Segs HERO_ARM_AIM   = { {11,19,"bbb"},{12,19,"bbbeeeeeee"},{13,19,"bbbehhhhhhc"},{14,20,"bbeeeeeee"},{15,22,"hh"} };

    void HeroLeg(Grid& g, float topX, float footX, int lift, char mat) {
        const int y0 = 20, y1 = 30 - lift;
        for (int y = y0; y <= y1; ++y) {
            const float t = (y1 == y0) ? 1.f : (float)(y - y0) / (float)(y1 - y0);
            const int cx = (int)std::lround(topX + (footX - topX) * t);
            const bool foot = (y >= y1 - 1);
            for (int x = cx - 2; x <= (foot ? cx + 4 : cx + 1); ++x) {
                if (x < 0 || x >= N) continue;
                g[y][x] = foot ? 'e' : (y == 24 ? 'h' : mat);
            }
        }
    }

    struct LegSpec { float farTop, farFoot; int farLift; float nearTop, nearFoot; int nearLift; };
    const LegSpec HERO_LEGS[Sprites::L_COUNT] = {
        { 14, 13, 0, 17, 18, 0 },
        { 14,  9, 2, 17, 22, 0 },
        { 14, 14, 0, 17, 17, 4 },
        { 14, 19, 0, 17, 12, 2 },
        { 14, 14, 4, 17, 17, 0 },
        { 14, 10, 3, 17, 21, 5 },
        { 14, 12, 0, 17, 20, 0 },
    };

    void BuildHero(Grid& g, int arms, int legs) {
        Clear(g);
        const LegSpec& L = HERO_LEGS[legs];
        HeroLeg(g, L.farTop, L.farFoot, L.farLift, 'q');
        Put(g, HERO_BASE, false);
        HeroLeg(g, L.nearTop, L.nearFoot, L.nearLift, 'b');
        Put(g, arms == Sprites::A_AIM ? HERO_ARM_AIM : HERO_ARM_READY, false);
    }

    Texture2D ToTexture(Image& img) {
        Texture2D t = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(t, TEXTURE_FILTER_POINT);
        return t;
    }
}

void Sprites::Init() {
    {
        Image img = GenImageColor(N * B_COUNT, N, BLANK);
        Image glow = GenImageColor(N * B_COUNT, N, BLANK);
        const BossPose poses[B_COUNT] = {
              { &ARM_REST,   nullptr,     &ARM_REST,   nullptr,      &LEG_STAND,  &LEG_STAND  },
              { &ARM_REST,   nullptr,     &ARM_REST,   nullptr,      &LEG_LIFT,   &LEG_STAND  },
              { &ARM_REST,   nullptr,     &ARM_REST,   nullptr,      &LEG_STAND,  &LEG_LIFT   },
              { &ARM_RAISED, nullptr,     &ARM_RAISED, nullptr,      &LEG_TUCK,   &LEG_TUCK   },
              { &ARM_RAISED, nullptr,     &ARM_RAISED, nullptr,      &LEG_STAND,  &LEG_STAND  },
              { &ARM_RAISED, nullptr,     &ARM_RAISED, nullptr,      &LEG_SPREAD, &LEG_SPREAD },
              { &ARM_SLAM,   nullptr,     &ARM_SLAM,   nullptr,      &LEG_SPREAD, &LEG_SPREAD },
              { &ARM_REST,   nullptr,     &ARM_REST,   &ARM_CANNON,  &LEG_STAND,  &LEG_STAND  },
              { &ARM_SLAM,   nullptr,     &ARM_SLAM,   nullptr,      &LEG_STAND,  &LEG_STAND  },
        };
        Grid g;
        for (int f = 0; f < B_COUNT; ++f) { BuildBoss(g, poses[f]); PaintFrame(g, img, glow, f * N, BOSS_MATS); }
        boss = ToTexture(img);
        bossGlow = ToTexture(glow);
    }
    {
        const int F = A_COUNT * L_COUNT;
        Image img = GenImageColor(N * F, N, BLANK);
        Image glow = GenImageColor(N * F, N, BLANK);
        Grid g;
        for (int a = 0; a < A_COUNT; ++a)
            for (int l = 0; l < L_COUNT; ++l) {
                BuildHero(g, a, l);
                PaintFrame(g, img, glow, HeroFrame(a, l) * N, HERO_MATS);
            }
        hero = ToTexture(img);
        heroGlow = ToTexture(glow);
    }
    {
        Image g = GenImageColor(64, 64, BLANK);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                const float d = std::sqrt((float)((x - 32) * (x - 32) + (y - 32) * (y - 32))) / 32.f;
                if (d >= 1.f) continue;
                const float a = (1.f - d) * (1.f - d);
                ImageDrawPixel(&g, x, y, Color{255, 255, 255, (unsigned char)(a * 255.f)});
            }
        glow = LoadTextureFromImage(g);
        UnloadImage(g);
        SetTextureFilter(glow, TEXTURE_FILTER_BILINEAR);
    }
}

void Sprites::Shutdown() {
    UnloadTexture(boss); UnloadTexture(bossGlow);
    UnloadTexture(hero); UnloadTexture(heroGlow);
    UnloadTexture(glow);
}

void Sprites::DrawGlow(Vector2 p, float radius, Color c) const {
    DrawTexturePro(glow, { 0.f, 0.f, 64.f, 64.f }, { p.x, p.y, radius * 2.f, radius * 2.f }, { radius, radius }, 0.f, c);
}

void Sprites::DrawSheet(const Texture2D& tex, const Texture2D& glowTex, int frame, Vector2 feet, float size,
                        bool faceRight, float lean, float sx, float sy, Color tint, float glow) {
    const int maxF = tex.width / CELL;
    if (maxF > 0) frame = std::max(0, std::min(frame, maxF - 1));
    const float W = size * sx, H = size * sy;
    const Rectangle src = { (float)(frame * CELL), 0.f, faceRight ? (float)CELL : -(float)CELL, (float)CELL };
    const Rectangle dst = { std::round(feet.x), std::round(feet.y), std::round(W), std::round(H) };
    const Vector2 origin = { dst.width * 0.5f, dst.height };
    DrawTexturePro(tex, src, dst, origin, lean, tint);

    if (glow > 0.f) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawTexturePro(glowTex, src, dst, origin, lean, Fade(WHITE, glow));
        const Rectangle d2 = { dst.x, dst.y - dst.height * 0.5f, dst.width * 1.2f, dst.height * 1.2f };
        DrawTexturePro(glowTex, src, d2, { d2.width * 0.5f, d2.height * 0.5f }, lean, Fade(WHITE, glow * 0.35f));
        EndBlendMode();
    }
}

void Sprites::DrawBoss(int frame, Vector2 feet, float size, bool faceRight, float lean,
                       float sx, float sy, Color tint, float glow) const {
    DrawSheet(boss, bossGlow, frame, feet, size, faceRight, lean, sx, sy, tint, glow);
}

void Sprites::DrawHero(int frame, Vector2 feet, float size, bool faceRight, float lean,
                       float sx, float sy, Color tint, float glow) const {
    DrawSheet(hero, heroGlow, frame, feet, size, faceRight, lean, sx, sy, tint, glow);
}
