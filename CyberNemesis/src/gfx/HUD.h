#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include "ai/AdaptiveBrain.h"

class Boss;
class Hero;

class HUD {
public:
    void Reset(const Boss& boss, const Hero& hero);
    void Update(float dt, const Boss& boss, const Hero& hero);
    void DrawBattle(const Boss& boss, const Hero& hero, const Telemetry& tel, float hintAlpha) const;
    void DrawCodex(int page, float t) const;

    void SetDetail(bool on) { detail_ = on; }      // H: muestra los datos de la IA (telemetria, habilidades...)
    void SetFinal(bool on, const char* pattern, bool flying) { final_ = on; pattern_ = pattern; flying_ = flying; }

    void SetSeraph(int cycle, int cycles, float atk, float dodge, float decoyReady, float guardReady) {
        sCycle_ = cycle; sCycles_ = cycles; sAtk_ = atk; sDodge_ = dodge; sDecoy_ = decoyReady; sGuard_ = guardReady;
    }

    static std::vector<std::string> Wrap(const std::string& s, int maxW, int size);   // parte un texto en lineas por ancho
    static void Text(const char* txt, int x, int y, int size, Color c);
    static void TextCentered(const char* txt, int cx, int y, int size, Color c);

private:
    static void Bar(int x, int y, int w, int h, float value, float ghost, Color fill);
    static void MiniBar(int x, int y, int w, int h, float value, Color fill);

    bool final_ = false, flying_ = false, detail_ = false;
    const char* pattern_ = "";
    int  sCycle_ = 1, sCycles_ = 1;
    float sAtk_ = 1.f, sDodge_ = 0.f, sDecoy_ = -1.f, sGuard_ = -1.f;
    float bossShown_ = 1.f, bossGhost_ = 1.f, heroShown_ = 1.f, heroGhost_ = 1.f;
};
