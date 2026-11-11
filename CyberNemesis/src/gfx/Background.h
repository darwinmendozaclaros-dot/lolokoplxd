#pragma once
#include "raylib.h"
#include <vector>

class Background {
public:
    enum class Event { None, Storm, DataRain, Meteors, Airship, Blackout };

    void Init(unsigned seed);
    void Update(float dt);
    void SetTheme(int generation);
    void Draw(float time) const;
    void DrawFront(float time) const;

    bool  ConsumeLightning() { bool r = lightningPending_; lightningPending_ = false; return r; }
    Event Current() const { return ev_; }
    Color GridColor() const { return cur_.grid; }
    void  SetEventsEnabled(bool on) { eventsEnabled_ = on; }
    const char* EventLabel(float& alpha) const;

private:
    struct Building { float x, w, h; unsigned seed; int accent; bool sign, billboard, antenna; };
    struct Star   { float x, y, phase; };
    struct Palette { Color skyTop, skyBot, sunTop, sunBot, grid, acc[3]; };
    struct Drone  { float x, y, speed, phase; };
    struct Meteor { Vector2 pos, vel; float life; };
    struct Drop   { float x, y, speed; };
    struct Column { float x, y, speed; int len; };

    static Palette ThemePalette(int idx);
    static void LerpPalette(Palette& cur, const Palette& tgt, float k);
    void StartEvent(Event e);
    void EndEvent();
    void DrawLayer(int layer, float time) const;
    void DrawGround(float time) const;
    void DrawSun() const;
    void DrawDecor(float time) const;
    void DrawAirship(float time) const;
    void DrawLightning() const;

    std::vector<Building> layers_[3];
    float wrap_[3]   = {0.f, 0.f, 0.f};
    float scroll_[3] = {0.f, 0.f, 0.f};
    std::vector<Star>   stars_;
    std::vector<Drone>  drones_;
    std::vector<Meteor> meteors_;
    std::vector<Drop>   drops_;
    std::vector<Column> columns_;
    std::vector<Vector2> bolt_;

    Palette cur_{}, target_{};

    Event ev_ = Event::None, lastEv_ = Event::None;
    float evT_ = 0.f, evDur_ = 0.f, evTimer_ = 8.f, eventFade_ = 0.f;
    float flash_ = 0.f, boltT_ = 0.f, lightTimer_ = 2.f, meteorTimer_ = 0.f;
    float lightFactor_ = 1.f, flickerT_ = 0.f, flicker_ = 1.f;
    bool  lightningPending_ = false, eventsEnabled_ = true;
};
