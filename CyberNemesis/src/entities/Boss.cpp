#include "entities/Boss.h"
#include "entities/Arena.h"
#include "gfx/Sprites.h"
#include <algorithm>
#include <cmath>

namespace {
    float Approach(float v, float target, float maxDelta) {
        if (v < target) return std::min(v + maxDelta, target);
        return std::max(v - maxDelta, target);
    }
}

Boss::Boss() : Entity(Cfg::BOSS_W, Cfg::BOSS_H) { Reset(150.f); }

void Boss::Reset(float x) {
    ApplySize();
    pos = { x, Cfg::GROUND_Y };
    vel = { 0.f, 0.f };
    ResetPhysics();
    hp = maxHp = Cfg::BOSS_MAX_HP;
    facing = 1;
    hitFlash = 0.f;
    meleeCd_ = shotCd_ = windup_ = dashCd_ = dashT_ = coyote_ = jumpBuf_ = dashBuf_ = overcharge_ = knockT_ = 0.f;
    recoilT_ = slamT_ = invuln_ = 0.f;
    airJumps_ = 1; jumpPending_ = 0;
    winding_ = diving_ = slamPending_ = shotPending_ = dashPending_ = false;
}

void Boss::UpdateScale(float dt) {
    if (!IsShrinking()) { curScale_ = targetScale_; ApplySize(); return; }
    curScale_ += (targetScale_ - curScale_) * std::min(1.f, dt * 3.5f);
    ApplySize();
}

Rectangle Boss::MeleeZone() const {
    const float reach = MeleeReach();
    return { pos.x - w * 0.5f - reach, pos.y - Cfg::BOSS_MELEE_HEIGHT, w + 2.f * reach, Cfg::BOSS_MELEE_HEIGHT };
}

Rectangle Boss::HurtBox() const {
    return { pos.x - w * 0.5f + 0.104f * w, pos.y - h + 0.0625f * h, w * 0.792f, h * 0.9375f };
}

void Boss::Update(float dt, const BossInput& in, const Arena& arena) {
    UpdateScale(dt);
    animTime += dt;
    hitFlash = std::max(0.f, hitFlash - dt * 5.f);

    const float cd = dt * (overcharge_ > 0.f ? 1.8f : 1.f);
    overcharge_ = std::max(0.f, overcharge_ - dt);
    knockT_     = std::max(0.f, knockT_ - dt);
    recoilT_    = std::max(0.f, recoilT_ - dt);
    invuln_     = std::max(0.f, invuln_ - dt);
    slamT_      = std::max(0.f, slamT_ - dt);
    meleeCd_ = std::max(0.f, meleeCd_ - cd);
    shotCd_  = std::max(0.f, shotCd_  - cd);
    dashCd_  = std::max(0.f, dashCd_  - cd);
    dashT_   = std::max(0.f, dashT_   - dt);

    facing = (in.aim.x >= pos.x) ? 1 : -1;

    if (OnGround()) { coyote_ = Cfg::BOSS_COYOTE_TIME; airJumps_ = 1; }
    else coyote_ -= dt;
    jumpBuf_ = in.jumpPressed ? Cfg::BOSS_JUMP_BUFFER : jumpBuf_ - dt;

    dashBuf_ = in.dash ? Cfg::BOSS_JUMP_BUFFER : dashBuf_ - dt;       // si pulsas el dash un instante antes de que recargue, sale igual
    if (dashBuf_ > 0.f && dashCd_ <= 0.f && !winding_ && !diving_) {
        dashBuf_ = 0.f;
        dashDir_ = (in.moveDir != 0.f) ? (in.moveDir > 0.f ? 1 : -1) : facing;
        dashT_ = Cfg::BOSS_DASH_TIME;
        dashCd_ = Cfg::BOSS_DASH_COOLDOWN;
        dashPending_ = true;
        knockT_ = 0.f;
        if (vel.y > 0.f) vel.y = 0.f;
    }

    const float speed = Cfg::BOSS_SPEED * (1.f + 0.4f * (1.f - curScale_));
    if (dashT_ > 0.f) {
        vel.x = dashDir_ * Cfg::BOSS_DASH_SPEED;
        if (vel.y > 0.f) vel.y = 0.f;
    } else if (knockT_ > 0.f) {
        vel.x = Approach(vel.x, 0.f, 900.f * dt);
    } else {
        float target = in.moveDir * speed * (winding_ ? 0.2f : 1.f);
        if (diving_) target = vel.x * 0.98f;
        float accel = OnGround() ? Cfg::BOSS_ACCEL_GROUND : Cfg::BOSS_ACCEL_AIR;
        if (in.moveDir == 0.f && OnGround()) accel *= 1.4f;
        if (in.moveDir != 0.f && vel.x * in.moveDir < -20.f) accel *= 1.6f;      // giro mas agil al cambiar de direccion
        vel.x = Approach(vel.x, target, accel * dt);
    }

    if (jumpBuf_ > 0.f && !winding_ && !diving_) {
        if (coyote_ > 0.f) {
            vel.y = Cfg::BOSS_JUMP_V;
            coyote_ = 0.f; jumpBuf_ = 0.f; jumpPending_ = 1;
            standIdx_ = -1; grounded_ = false;
        } else if (airJumps_ > 0) {
            vel.y = Cfg::BOSS_AIRJUMP_V;
            --airJumps_; jumpBuf_ = 0.f; jumpPending_ = 2;
        }
    }
    if (!in.jumpHeld && vel.y < -180.f && !diving_) vel.y += Cfg::GRAVITY * worldGravity * 1.1f * dt;

    if (in.down && OnGround() && StandingPlatform() >= 0 && !winding_) DropThrough();

    if (in.melee && meleeCd_ <= 0.f && !winding_ && !diving_) {
        if (OnGround()) {
            winding_ = true;
            windup_  = Cfg::BOSS_MELEE_WINDUP;
            meleeCd_ = Cfg::BOSS_MELEE_COOLDOWN;
        } else {
            diving_ = true;
            dashT_ = 0.f;
            vel.y = Cfg::BOSS_DIVE_SPEED;
            vel.x *= 0.3f;
            meleeCd_ = Cfg::BOSS_MELEE_COOLDOWN;
        }
    }
    if (winding_) {
        windup_ -= dt;
        if (windup_ <= 0.f) { winding_ = false; slamPending_ = true; slamT_ = 0.25f; }
    }

    if (in.shoot && shotCd_ <= 0.f && !winding_) {
        shotCd_ = Cfg::BOSS_SHOT_COOLDOWN;
        shotPending_ = true;
        recoilT_ = 0.2f;
        Vector2 o = MuzzlePos();
        Vector2 d = Vector2Subtract(in.aim, o);
        if (Vector2Length(d) < 1.f) d = { (float)facing, 0.f };
        shotDir_ = Vector2Normalize(d);
    }

    float gs = dashT_ > 0.f ? 0.f : 1.f;
    if (gs > 0.f && !OnGround() && !diving_ && in.jumpHeld && std::fabs(vel.y) < 80.f) gs = 0.72f;   // "hang time": flotas un poco en lo alto del salto
    ApplyPhysics(dt, arena, gs);

    if (diving_ && OnGround()) { diving_ = false; slamPending_ = true; slamT_ = 0.25f; }
}

void Boss::Draw(const Sprites& sp, float time) const {
    const float S  = w;
    const float sc = S / Cfg::BOSS_W;

    const float air = std::min(1.f, (Cfg::GROUND_Y - pos.y) / 180.f);
    DrawEllipse((int)pos.x, (int)Cfg::GROUND_Y + 2, 46.f * sc * (1.f - 0.4f * air), 7.f * (1.f - 0.4f * air),
                Fade(BLACK, 0.55f * (1.f - 0.5f * air)));

    float k = 0.f;
    if (winding_) {
        k = 1.f - windup_ / Cfg::BOSS_MELEE_WINDUP;
        Rectangle z = MeleeZone();
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRectangleRec(z, Fade(Theme::NEON_RED, 0.10f + 0.25f * k));
        DrawRectangleLinesEx(z, 1.f, Fade(Theme::NEON_RED, 0.8f));
        EndBlendMode();
    }

    const bool moving = std::fabs(vel.x) > 8.f;
    int frame = Sprites::B_IDLE;
    if (diving_)                         frame = Sprites::B_DIVE;
    else if (winding_)                   frame = Sprites::B_WINDUP;
    else if (slamT_ > 0.f)               frame = Sprites::B_SLAM;
    else if (recoilT_ > 0.f)             frame = Sprites::B_SHOOT;
    else if (!OnGround())                frame = vel.y < 0.f ? Sprites::B_JUMP : Sprites::B_FALL;
    else if (moving) {
        static const int CYCLE[4] = { Sprites::B_WALK_A, Sprites::B_IDLE, Sprites::B_WALK_B, Sprites::B_IDLE };
        frame = CYCLE[(int)(walkPhase * 4.f) & 3];
    }

    const bool idle = OnGround() && !moving && !winding_;
    const float breathe = idle ? 0.022f * std::sin(time * 3.2f) : 0.f;
    float sy = 1.f + breathe - 0.24f * LandSquash();
    float sx = 1.f - breathe * 0.6f + 0.20f * LandSquash();
    if (!OnGround() && vel.y < 0.f) { const float st = std::min(1.f, -vel.y / 540.f); sy += 0.09f * st; sx -= 0.05f * st; }
    if (diving_) { sy += 0.12f; sx -= 0.08f; }
    float lean = Clamp(vel.x / (Cfg::BOSS_SPEED * 1.2f), -1.f, 1.f) * 6.f;
    if (dashT_ > 0.f) lean = dashDir_ * 11.f;
    if (winding_) lean = std::sin(time * 90.f) * 2.5f * (0.4f + k);
    const float bob = (OnGround() && moving) ? -std::fabs(std::sin(walkPhase * 6.2832f * 2.f)) * 1.8f * sc : 0.f;
    const float raise = winding_ ? k * 6.f * sc : 0.f;
    const Vector2 feet = { pos.x, pos.y + bob - raise };

    if (dashT_ > 0.f) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int g = 1; g <= 4; ++g) {
            const Vector2 gf = { feet.x - dashDir_ * g * 18.f * sc, feet.y };
            sp.DrawBoss(frame, gf, S, facing >= 0, lean, 1.f, 1.f, Fade(Theme::NEON_MAGENTA, 0.5f - g * 0.1f), 0.f);
        }
        EndBlendMode();
    }
    if (diving_) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int g = 1; g <= 5; ++g)
            DrawRectangle((int)(pos.x - 30.f * sc + g * 9.f * sc), (int)(pos.y - S - g * 16), 3, 26, Fade(Theme::NEON_MAGENTA, 0.5f - g * 0.08f));
        EndBlendMode();
    }

    Color tint = hitFlash > 0.f ? Color{255, 120, 120, 255} : WHITE;
    if (invuln_ > 0.f && std::fmod(time * 24.f, 2.f) < 1.f) tint.a = 110;
    const float glow = 0.55f + 0.20f * std::sin(time * 5.f) + 0.30f * k + (overcharge_ > 0.f ? 0.25f : 0.f);
    sp.DrawBoss(frame, feet, S, facing >= 0, lean, sx, sy, tint, glow);

    BeginBlendMode(BLEND_ADDITIVE);
    const float top = feet.y - S * sy;
    const float pulse = 0.6f + 0.4f * std::sin(time * 5.f);
    DrawCircleGradient((int)pos.x, (int)(top + S * sy * 0.575f), (20.f + 6.f * k) * sc,
                       Fade(Theme::NEON_MAGENTA, 0.45f * pulse + 0.3f * k), Fade(Theme::NEON_MAGENTA, 0.f));
    DrawCircleGradient((int)pos.x, (int)(top + S * sy * 0.27f), 26.f * sc,
                       Fade(Theme::NEON_RED, 0.35f + 0.1f * std::sin(time * 7.f)), Fade(Theme::NEON_RED, 0.f));
    if (overcharge_ > 0.f) {
        const float a = (overcharge_ < 2.f) ? 0.25f + 0.25f * std::sin(time * 25.f) : 0.45f;
        DrawCircleGradient((int)pos.x, (int)(pos.y - S * 0.5f), S * 0.85f, Fade(Theme::NEON_YELLOW, a), Fade(Theme::NEON_YELLOW, 0.f));
    }
    EndBlendMode();
}
