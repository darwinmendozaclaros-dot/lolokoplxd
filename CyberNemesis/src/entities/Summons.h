#pragma once
#include "raylib.h"
#include "core/Config.h"
#include "entities/Projectile.h"
#include <vector>

struct Sprites;
class Hero;
class Boss;

// Eventos para que Game muestre efectos y sonido.
struct SummonEvent {
    enum Kind { DecoySpawn, DecoyHit, DecoyBreak, GuardSpawn, GuardBreak };
    Kind    kind;
    Vector2 pos;
};

// Cosas que el Serafin invoca para esquivar:
//  - Senuelos: copias holograficas que absorben disparos del Boss (clones de espejo = 2 a la vez).
//  - Orbes guardianes: orbitan al Serafin y cada uno bloquea un disparo.
// Que sabe invocar depende del Eco (ver Seraph::ForCycle).
class Summons {
public:
    void Clear();
    void Configure(const Seraph::Stats& st);
    void Update(float dt, const Hero& hero, const Boss& boss, const std::vector<Projectile>& projs);

    bool Absorb(Vector2 p, float r);              // true si una invocacion detuvo el disparo del Boss
    void DestroyIn(const Rectangle& zone);        // el aplastamiento del Boss las destruye
    std::vector<SummonEvent> TakeEvents() { std::vector<SummonEvent> r; r.swap(events_); return r; }

    // -1 = aun no sabe invocarlo; 0..1 = carga de la recarga (1 = listo).
    float DecoyReady() const { return st_.decoys > 0 ? 1.f - decoyCd_ / st_.decoyCd : -1.f; }
    float GuardReady() const { return st_.guardOrbs > 0 ? 1.f - guardCd_ / st_.guardCd : -1.f; }

    int  Absorbed() const { return absorbed_; }   // disparos bloqueados desde Configure()

    void Draw(const Hero& hero, const Sprites& sp, float time) const;

private:
    struct Decoy { Vector2 pos; float age, life; int hp; bool faceRight; };
    struct Orb   { float off; bool alive; };

    void SpawnDecoys(Vector2 c, Vector2 aim);
    void SpawnGuards(Vector2 c, Vector2 aim);
    Vector2 OrbPos(size_t i) const;

    Seraph::Stats st_{};
    std::vector<Decoy> decoys_;
    std::vector<Orb>   orbs_;
    std::vector<SummonEvent> events_;
    Vector2 c_{0.f, 0.f};
    float decoyCd_ = 0.f, guardCd_ = 0.f, guardAng_ = 0.f, guardAge_ = 0.f;
    int   absorbed_ = 0;
};
