#pragma once
#include "entities/Entity.h"
#include <cmath>

struct Sprites;
class Arena;

struct BossInput {
    float   moveDir = 0.f;
    bool    jumpPressed = false, jumpHeld = false;
    bool    dash = false, down = false;
    bool    melee = false, shoot = false;
    Vector2 aim{0.f, 0.f};
};

class Boss : public Entity {
public:
    Boss();
    void Reset(float x);
    void Update(float dt, const BossInput& in, const Arena& arena);
    void Draw(const Sprites& sp, float time) const;

    void TakeDamage(float dmg) { hp -= dmg; if (hp < 0.f) hp = 0.f; hitFlash = 1.f; }
    void Heal(float amount) { hp = std::fmin(maxHp, hp + amount); }

    void  SetScale(float s, bool instant) { targetScale_ = s; if (instant) { curScale_ = s; ApplySize(); } }
    void  BeginShrink(float from, float to) { curScale_ = from; targetScale_ = to; ApplySize(); }
    void  UpdateScale(float dt);
    bool  IsShrinking() const { return std::fabs(curScale_ - targetScale_) > 0.004f; }
    float Scale() const { return curScale_; }
    float MeleeReach() const { return Cfg::BOSS_MELEE_REACH * (1.f + (1.f - curScale_) * 0.6f); }

    void  Overcharge(float t) { overcharge_ = t; }
    bool  IsOvercharged() const { return overcharge_ > 0.f; }
    float DamageMul() const { return overcharge_ > 0.f ? 1.4f : 1.f; }
    void  GrantInvuln(float t) { invuln_ = invuln_ > t ? invuln_ : t; }
    bool  IsInvulnerable() const { return invuln_ > 0.f || dashT_ > 0.f; }
    void  Knock(float vx, float vy) { knockT_ = 0.28f; vel.x = vx; vel.y = vy; grounded_ = false; standIdx_ = -1; }

    bool  DashActive() const { return dashT_ > 0.f; }
    bool  IsWindingUp() const { return winding_; }
    bool  IsDiving() const { return diving_; }
    Rectangle MeleeZone() const;
    Rectangle HurtBox() const;
    Vector2   MuzzlePos() const { return { pos.x + facing * 0.27f * w, pos.y - 0.72f * h }; }
    float MeleeCooldownFrac() const { return meleeCd_ / Cfg::BOSS_MELEE_COOLDOWN; }
    float ShotCooldownFrac()  const { return shotCd_  / Cfg::BOSS_SHOT_COOLDOWN; }
    float DashCooldownFrac()  const { return dashCd_  / Cfg::BOSS_DASH_COOLDOWN; }
    bool  AirJumpAvailable()  const { return airJumps_ > 0; }

    bool ConsumeSlam() { bool r = slamPending_; slamPending_ = false; return r; }
    bool ConsumeDash() { bool r = dashPending_; dashPending_ = false; return r; }
    int  ConsumeJump() { int r = jumpPending_; jumpPending_ = 0; return r; }
    bool ConsumeShot(Vector2& origin, Vector2& dir) {
        if (!shotPending_) return false;
        shotPending_ = false; origin = MuzzlePos(); dir = shotDir_; return true;
    }

private:
    void ApplySize() { w = h = Cfg::BOSS_W * curScale_; }

    float curScale_ = 1.f, targetScale_ = 1.f;
    float meleeCd_ = 0.f, shotCd_ = 0.f, windup_ = 0.f, dashCd_ = 0.f, dashT_ = 0.f;
    float coyote_ = 0.f, jumpBuf_ = 0.f, dashBuf_ = 0.f, overcharge_ = 0.f, knockT_ = 0.f;
    float recoilT_ = 0.f, slamT_ = 0.f, invuln_ = 0.f;
    int   airJumps_ = 1, dashDir_ = 1, jumpPending_ = 0;
    bool  winding_ = false, diving_ = false;
    bool  slamPending_ = false, shotPending_ = false, dashPending_ = false;
    Vector2 shotDir_{1.f, 0.f};
};
