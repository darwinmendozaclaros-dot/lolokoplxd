#include "gfx/Portraits.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr int S = 26, GAP = 4;
    const Color PLAYER_C{255, 43, 214, 255};
    const Color HERO_C  {24, 240, 255, 255};
    const Color SERAPH_C{255, 225, 120, 255};
    const Color GUIDE_C {57, 255, 120, 255};

    void Line(int x1, int y1, int x2, int y2, Color c) { DrawLineEx({ (float)x1, (float)y1 }, { (float)x2, (float)y2 }, 2.f, c); }

    // NEMESIS-X: casco con cuernos y visor.
    void IconPlayer(int cx, int cy, Color c, float a) {
        DrawCircle(cx, cy + 1, 9.f, Fade(c, 0.25f * a));
        DrawCircleLines(cx, cy + 1, 9.f, Fade(c, a));
        DrawRectangle(cx - 6, cy, 12, 3, Fade(WHITE, a));
        Line(cx - 8, cy - 5, cx - 12, cy - 11, Fade(c, a));
        Line(cx + 8, cy - 5, cx + 12, cy - 11, Fade(c, a));
    }
    // Heroe: casco con visor ancho y antena.
    void IconHero(int cx, int cy, Color c, float a) {
        DrawCircle(cx, cy + 1, 9.f, Fade(c, 0.25f * a));
        DrawCircleLines(cx, cy + 1, 9.f, Fade(c, a));
        DrawRectangle(cx - 7, cy - 1, 14, 4, Fade(c, a));
        DrawLine(cx, cy - 8, cx, cy - 11, Fade(c, a));
        DrawCircle(cx, cy - 12, 1.5f, Fade(WHITE, a));
    }
    // Serafin: cabeza con aureola y alas.
    void IconSeraph(int cx, int cy, Color c, float a) {
        DrawCircle(cx, cy + 2, 7.f, Fade(c, 0.25f * a));
        DrawCircleLines(cx, cy + 2, 7.f, Fade(c, a));
        DrawCircleLines(cx, cy - 8, 4.f, Fade(c, a));
        DrawRectangle(cx - 4, cy + 1, 2, 2, Fade(WHITE, a));
        DrawRectangle(cx + 2, cy + 1, 2, 2, Fade(WHITE, a));
        Line(cx - 7, cy + 1, cx - 12, cy - 5, Fade(c, a));
        Line(cx - 7, cy + 4, cx - 12, cy + 5, Fade(c, a));
        Line(cx + 7, cy + 1, cx + 12, cy - 5, Fade(c, a));
        Line(cx + 7, cy + 4, cx + 12, cy + 5, Fade(c, a));
    }
    // Guia: bombilla (la idea / el consejo).
    void IconGuide(int cx, int cy, Color c, float a) {
        DrawCircle(cx, cy - 3, 7.f, Fade(c, 0.25f * a));
        DrawCircleLines(cx, cy - 3, 7.f, Fade(c, a));
        DrawRectangle(cx - 3, cy + 4, 6, 2, Fade(c, a));
        DrawRectangle(cx - 2, cy + 7, 4, 2, Fade(c, a));
        DrawLine(cx - 3, cy - 4, cx, cy + 1, Fade(c, a));
        DrawLine(cx, cy + 1, cx + 3, cy - 4, Fade(c, a));
        DrawLine(cx - 12, cy - 3, cx - 10, cy - 3, Fade(c, a));
        DrawLine(cx + 10, cy - 3, cx + 12, cy - 3, Fade(c, a));
    }
}

Role RoleOf(const std::string& speaker) {
    if (speaker == "NEMESIS-X") return Role::Player;
    if (speaker == "GUIA")      return Role::Guide;
    return Role::AI;
}

void DrawPortraitStrip(int x, int y, const std::string& speaker, bool seraphMode, float time, float alpha) {
    const Role active = RoleOf(speaker);
    const Role roles[3] = { Role::Player, Role::AI, Role::Guide };
    int activeCx = x + S / 2;
    Color activeCol = WHITE;

    for (int i = 0; i < 3; ++i) {
        const Role r = roles[i];
        const bool on = (r == active);
        const Color col = r == Role::Player ? PLAYER_C : (r == Role::Guide ? GUIDE_C : (seraphMode ? SERAPH_C : HERO_C));
        const float k = (on ? 1.f : 0.28f) * alpha;
        const int fx = x + i * (S + GAP);
        const int bob = on ? (int)std::lround(std::sin(time * 8.f) * 1.5f) : 0;

        DrawRectangle(fx, y, S, S, Fade(Color{6, 4, 18, 255}, 0.9f * alpha));
        DrawRectangleLines(fx, y, S, S, Fade(col, (on ? 1.f : 0.35f) * alpha));
        if (on) DrawRectangleLines(fx - 1, y - 1, S + 2, S + 2, Fade(col, 0.45f * alpha));

        const int cx = fx + S / 2, cy = y + S / 2 + bob;
        if (r == Role::Player)     IconPlayer(cx, cy, col, k);
        else if (r == Role::Guide) IconGuide(cx, cy, col, k);
        else if (seraphMode)       IconSeraph(cx, cy, col, k);
        else                       IconHero(cx, cy, col, k);

        if (on) { activeCx = cx; activeCol = col; }
    }

    const int tw = MeasureText(speaker.c_str(), 10);
    const int lx = std::max(x, std::min(activeCx - tw / 2, x + PORTRAIT_STRIP_W - tw));
    DrawText(speaker.c_str(), lx, y + S + 2, 10, Fade(activeCol, alpha));
}
