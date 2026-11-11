#pragma once
#include "entities/Entity.h"
#include "entities/Projectile.h"
#include "ai/HeroTraits.h"
#include <algorithm>
#include <vector>

struct Sprites;
class Boss;
class Arena;

class Hero : public Entity {
public:
    Hero();
    void ApplyTraits(const HeroTraits& t);
    void Reset(float x);
    void Update(float dt, const Boss& boss, const std::vector<Projectile>& projs, const Arena& arena);
    void Draw(const Sprites& sp, float time) const;
    void DrawEcho(const Sprites& sp, float time, Vector2 feet, float alpha, bool faceRight,
                  Color tint = Color{255, 225, 120, 255}) const;   // copia holografica (senuelo / reconstruccion)

    float TakeDamage(float dmg, bool isProjectile);

    const HeroTraits& Traits() const { return traits_; }
    bool  IsInvulnerable() const { return invuln_ > 0.f; }
    float DashCooldownFrac() const { return dashCd_ > 0.f ? dashCd_ / traits_.dashCooldown : 0.f; }
    bool  Has(unsigned atk) const { return (traits_.attacks & atk) != 0; }

    void  PhaseShift() {
        hp = std::min(maxHp, hp + maxHp * 0.12f);
        invuln_ = std::max(invuln_, 1.6f);
        overdrive_ = 7.f;
        hitFlash = 0.f;
    }
    bool  IsOverdrive() const { return overdrive_ > 0.f; }

    void  EnterFinalForm(const Seraph::Stats& st);
    bool  IsFinalForm() const { return finalForm_; }
    bool  Shielded() const { return finalForm_ ? finalShield_ : traits_.deflectShield; }
    bool  ConsumeDodge() { bool r = dodgePending_; dodgePending_ = false; return r; }
    bool  IsFlying() const { return flying_; }
    void  FinalCommand(Vector2 target, float speed, bool wantFly) { target_ = target; finalSpeed_ = speed; wantFly_ = wantFly; }
    void  FinalCast() { aimT_ = 0.4f; }
    void  SetThreatColumns(const std::vector<float>& xs) { threats_ = xs; }

    std::vector<ShotCmd> TakeShots() { std::vector<ShotCmd> r; r.swap(shots_); return r; }
    bool ConsumeDash() { bool r = dashPending_; dashPending_ = false; return r; }
    int  ConsumeJump() { int r = jumpPending_; jumpPending_ = 0; return r; }
    bool ConsumeOrbital(float& x, float& dmg) {
        if (!orbitalPending_) return false;
        orbitalPending_ = false; x = orbitalX_; dmg = traits_.damage * 4.5f; return true;
    }
    bool ConsumeMine(Vector2& p, float& dmg) {
        if (!minePending_) return false;
        minePending_ = false; p = { pos.x, pos.y }; dmg = traits_.damage * 3.5f; return true;
    }

private:
    void StartDash(int towardBoss);
    bool Jump();
    bool JetJump();
    Vector2 MuzzlePos() const { return { pos.x + facing * 24.f, pos.y - 38.f }; }
    Vector2 AimDir(const Boss& boss, Vector2 origin, float speed, float spread);
    void FireBolt(const Boss& boss);
    void FireBurst();
    void FireSpread(const Boss& boss);
    void FireMissile();
    void UpdateAttacks(float dt, const Boss& boss, const std::vector<Projectile>& projs, float dist);
    void TryDodgeProjectiles(const std::vector<Projectile>& projs);
    void UpdateFinal(float dt, const Boss& boss, const Arena& arena);
    void UpdateScarf(float dt);
    void DrawWings(float time) const;
    int  PickPerch(const Arena& arena) const;

    HeroTraits traits_;
    float attackCd_ = 1.f, orbitalCd_ = 3.f, mineCd_ = 2.f, bossStill_ = 0.f;
    float dashCd_ = 0.f, jumpCd_ = 0.f, dashT_ = 0.f;
    float invuln_ = 0.f, shieldFlash_ = 0.f, strafeTimer_ = 0.f, desiredDist_ = 250.f;
    float burstTimer_ = 0.f;
    int   burstLeft_ = 0;
    int   dashDir_ = 1, strafeDir_ = 0, airJumps_ = 0;
    bool  reacted_ = false;
    int   jumpPending_ = 0;
    bool  dashPending_ = false, orbitalPending_ = false, minePending_ = false;
    float orbitalX_ = 0.f, overdrive_ = 0.f;
    std::vector<float> threats_;
    int   perchIdx_ = -1;
    float perchCd_ = 3.f, perchStay_ = 0.f, perchTry_ = 0.f, stayMax_ = 4.f, accidental_ = 0.f;
    Vector2 aimPoint_{0.f, 0.f};
    bool    finalForm_ = false, flying_ = false, wantFly_ = false;
    Vector2 target_{0.f, 0.f};
    float   finalSpeed_ = 150.f, wingT_ = 0.f;
    float   aimT_ = 0.f;
    float   finalDodge_ = 0.f, finalMoveMul_ = 1.f;
    bool    finalShield_ = false, dodgePending_ = false;
    Vector2 scarf_[7]{};
    std::vector<ShotCmd> shots_;
    std::vector<int> rolled_;
};
