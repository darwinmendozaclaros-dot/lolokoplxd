#pragma once
#include "raylib.h"
#include "ai/HeroTraits.h"

struct Sprites;

struct MenuInfo {
    const char* fxName = "";
    bool        musicOn = true;
    bool        hasMemory = false;
    HeroTraits  traits;
    float       bossScale = 1.f;
};

class MainMenu {
public:
    enum class Action { None, Start, Effects, Music, Fullscreen, Wipe, Codex, Quit };

    Action Update(float dt, Vector2 mouse, bool click);
    void   Draw(float time, const MenuInfo& info, const Sprites& sp, Color tint) const;
    bool   ConsumeMoved() { bool r = moved_; moved_ = false; return r; }

private:
    static constexpr int COUNT = 7;
    Action Activate(int i);

    int     sel_ = 0;
    float   confirmT_ = 0.f, anim_[COUNT] = {};
    bool    moved_ = false;
    Vector2 lastMouse_{-1.f, -1.f};
};
