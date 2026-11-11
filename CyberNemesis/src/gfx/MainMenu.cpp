#include "gfx/MainMenu.h"
#include "gfx/Sprites.h"
#include "gfx/HUD.h"
#include "core/Config.h"
#include "core/Util.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr int ITEM_X = 58, ITEM_Y = 150, ITEM_STEP = 26, ITEM_W = 275, ITEM_H = 22;
}

MainMenu::Action MainMenu::Activate(int i) {
    switch (i) {
        case 0: return Action::Start;
        case 1: return Action::Effects;
        case 2: return Action::Music;
        case 3: return Action::Fullscreen;
        case 4:
            if (confirmT_ > 0.f) { confirmT_ = 0.f; return Action::Wipe; }
            confirmT_ = 3.f;
            moved_ = true;
            return Action::None;
        case 5: return Action::Codex;
        default: return Action::Quit;
    }
}

MainMenu::Action MainMenu::Update(float dt, Vector2 mouse, bool click) {
    confirmT_ = std::max(0.f, confirmT_ - dt);
    for (int i = 0; i < COUNT; ++i) {
        const float target = (i == sel_) ? 1.f : 0.f;
        anim_[i] += (target - anim_[i]) * std::min(1.f, dt * 12.f);
    }

    const int old = sel_;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) sel_ = (sel_ + 1) % COUNT;
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) sel_ = (sel_ + COUNT - 1) % COUNT;

    bool overItem = false;
    for (int i = 0; i < COUNT; ++i) {
        const Rectangle r = { (float)ITEM_X - 6.f, (float)(ITEM_Y + i * ITEM_STEP), (float)ITEM_W, (float)ITEM_H };
        if (CheckCollisionPointRec(mouse, r)) {
            overItem = true;
            if (mouse.x != lastMouse_.x || mouse.y != lastMouse_.y) sel_ = i;
        }
    }
    lastMouse_ = mouse;

    if (sel_ != old) { confirmT_ = 0.f; moved_ = true; }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) return Activate(sel_);
    if ((IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) && (sel_ == 1 || sel_ == 2)) return Activate(sel_);
    if (click && overItem) return Activate(sel_);
    return Action::None;
}

void MainMenu::Draw(float time, const MenuInfo& info, const Sprites& sp, Color tint) const {
    const int W = Cfg::VIEW_W, H = Cfg::VIEW_H;
    const Color hi = Util::LerpColor(tint, WHITE, 0.45f);

    const float j = (std::sin(time * 17.f) > 0.965f) ? 3.f : 0.f;
    const char* title = "CYBER-NEMESIS";
    DrawText(title, 54 + 2 + (int)j, 34, 40, Fade(Theme::NEON_MAGENTA, 0.85f));
    DrawText(title, 54 - 2 - (int)j, 36, 40, Fade(Theme::NEON_CYAN, 0.85f));
    DrawText(title, 54, 35, 40, WHITE);
    HUD::Text("ADAPTIVE BOSS PROTOCOL", 58, 84, 20, hi);
    DrawRectangle(58, 112, 300, 1, Fade(hi, 0.6f));
    DrawRectangle(58, 114, 120, 1, Fade(hi, 0.3f));
    HUD::Text("ERES EL JEFE FINAL. EL HEROE APRENDE DE CADA GOLPE.", 58, 122, 10, Fade(WHITE, 0.7f));

    const char* confirm = "SEGURO? PULSA DE NUEVO";
    const char* labels[COUNT] = {
        "INICIAR COMBATE",
        TextFormat("EFECTOS: %s", info.fxName),
        TextFormat("MUSICA: %s", info.musicOn ? "SI" : "NO"),
        "PANTALLA COMPLETA",
        confirmT_ > 0.f ? confirm : "BORRAR MEMORIA DE LA IA",
        "ARCHIVO: COMO APRENDE LA IA",
        "SALIR",
    };
    for (int i = 0; i < COUNT; ++i) {
        const int y = ITEM_Y + i * ITEM_STEP;
        const float a = anim_[i];
        const bool sel = (i == sel_);
        if (a > 0.02f) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawRectangleGradientH(ITEM_X - 8, y - 1, ITEM_W, ITEM_H, Fade(tint, 0.30f * a), Fade(tint, 0.f));
            EndBlendMode();
            DrawRectangle(ITEM_X - 8, y - 1, 2, ITEM_H, Fade(hi, a));
        }
        Color c = Fade(WHITE, 0.50f + 0.45f * a);
        if (i == 4 && confirmT_ > 0.f) c = Fade(Theme::NEON_RED, 0.6f + 0.4f * std::fmod(time * 4.f, 1.f));
        if (sel && !(i == 4 && confirmT_ > 0.f)) c = WHITE;
        HUD::Text(labels[i], ITEM_X + (int)(6.f * a), y + 1, 20, c);
        if (sel) HUD::Text(">", ITEM_X - 6 + (int)(2.f * std::sin(time * 8.f)), y + 1, 20, hi);
    }

    {
        const int px = 392, py = 112, pw = 224, ph = 108;
        DrawRectangle(px, py, pw, ph, Fade(BLACK, 0.40f));
        DrawRectangleLines(px, py, pw, ph, Fade(hi, 0.55f));
        DrawRectangle(px, py, 36, 1, hi);
        HUD::Text("MEMORIA DEL ANDROIDE", px + 8, py + 6, 10, hi);
        if (info.hasMemory) {
            HUD::Text(TextFormat("GENERACION %d / %d", info.traits.generation, Cfg::MAX_GENERATION), px + 8, py + 20, 10, Theme::NEON_RED);
            HUD::Text(TextFormat("VIDA %d   DANO %.1f", (int)info.traits.maxHp, info.traits.damage), px + 8, py + 32, 10, WHITE);
        } else {
            HUD::Text("SIN DATOS PREVIOS", px + 8, py + 20, 10, Fade(WHITE, 0.7f));
            HUD::Text("PRIMER ARRANQUE DE LA IA", px + 8, py + 32, 10, Fade(WHITE, 0.5f));
        }
        int tx = px + 8;
        auto pill = [&](int y, const char* label, bool on, Color col) {
            const int w = MeasureText(label, 10) + 8;
            if (tx + w > px + pw - 4) return;
            DrawRectangle(tx, y, w, 12, on ? Fade(col, 0.25f) : Fade(GRAY, 0.10f));
            DrawRectangleLines(tx, y, w, 12, on ? col : Fade(GRAY, 0.30f));
            DrawText(label, tx + 4, y + 2, 10, on ? col : Fade(GRAY, 0.45f));
            tx += w + 3;
        };
        HUD::Text("HABILIDADES", px + 8, py + 48, 10, Fade(WHITE, 0.6f));
        tx = px + 8; const int y1 = py + 60;
        pill(y1, "ESCUDO", info.traits.deflectShield, Theme::NEON_CYAN);
        pill(y1, "DASH", info.traits.dashEvasive, Theme::NEON_YELLOW);
        pill(y1, "PRED", info.traits.leadShooting, Theme::NEON_GREEN);
        pill(y1, "JETS", info.traits.jumpJets, Theme::NEON_MAGENTA);
        HUD::Text("ARSENAL", px + 8, py + 76, 10, Fade(WHITE, 0.6f));
        tx = px + 8; const int y2 = py + 88;
        pill(y2, "RAF", (info.traits.attacks & Atk::BURST) != 0, Theme::NEON_CYAN);
        pill(y2, "ESC", (info.traits.attacks & Atk::SPREAD) != 0, Theme::NEON_GREEN);
        pill(y2, "MIS", (info.traits.attacks & Atk::MISSILE) != 0, Theme::NEON_YELLOW);
        pill(y2, "ORB", (info.traits.attacks & Atk::ORBITAL) != 0, Theme::NEON_RED);
        pill(y2, "MIN", (info.traits.attacks & Atk::MINE) != 0, Theme::NEON_MAGENTA);
    }

    {
        const float fy = (float)H - 22.f;
        DrawEllipse(470, (int)fy + 2, 40.f * info.bossScale + 8.f, 6.f, Fade(BLACK, 0.5f));
        DrawEllipse(572, (int)fy + 2, 26.f, 5.f, Fade(BLACK, 0.5f));
        const float br = 0.022f * std::sin(time * 3.2f);
        sp.DrawBoss(Sprites::B_IDLE, { 470.f, fy }, Cfg::BOSS_W * info.bossScale, true, 0.f, 1.f - br * 0.6f, 1.f + br, WHITE, 0.6f + 0.2f * std::sin(time * 5.f));
        const float hr = 0.02f * std::sin(time * 3.6f + 1.f);
        sp.DrawHero(Sprites::HeroFrame(Sprites::A_READY, Sprites::L_STAND), { 572.f, fy }, Cfg::HERO_W, false, 0.f, 1.f - hr * 0.6f, 1.f + hr, WHITE, 0.6f);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRectangle(430, (int)fy + 1, 180, 1, Fade(hi, 0.35f));
        EndBlendMode();
    }

    HUD::Text("W/S O FLECHAS: ELEGIR    ENTER / CLIC: ACEPTAR    ESC: SALIR", 58, H - 9, 10, Fade(WHITE, 0.55f));

    // Descripcion de la opcion seleccionada
    static const char* DESC[COUNT] = {
        "Empieza el experimento: tu forma de jugar entrena a la IA.",
        "Cambia la intensidad de glitch, scanlines y vibracion.",
        "Activa o apaga la musica.",
        "Alterna pantalla completa (tambien con F11).",
        "Borra lo que la IA aprendio de ti y empieza de cero.",
        "Como aprende la IA, por que existe el proyecto y como funciona por dentro.",
        "Cierra el juego.",
    };
    HUD::Text(DESC[std::max(0, std::min(sel_, COUNT - 1))], 58, 136, 10, Fade(Theme::NEON_CYAN, 0.75f));
    (void)W;
}
