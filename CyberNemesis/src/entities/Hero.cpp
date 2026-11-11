#include "entities/Hero.h"
#include "entities/Boss.h"
#include "entities/Arena.h"
#include "gfx/Sprites.h"
#include "core/Util.h"
#include <algorithm>
#include <cmath>

Hero::Hero() : Entity(Cfg::HERO_W, Cfg::HERO_H) { Reset(500.f); }

void Hero::ApplyTraits(const HeroTraits& t) { traits_ = t; maxHp = t.maxHp; }

void Hero::Reset(float x) {
    pos = { x, Cfg::GROUND_Y };
    vel = { 0.f, 0.f };
    ResetPhysics();
    hp = maxHp = traits_.maxHp;
    facing = -1;
    hitFlash = 0.f;
    overdrive_ = 0.f; threats_.clear();
    attackCd_ = 1.2f; orbitalCd_ = 3.f; mineCd_ = 2.5f; bossStill_ = 0.f;
    dashCd_ = jumpCd_ = dashT_ = invuln_ = shieldFlash_ = strafeTimer_ = burstTimer_ = 0.f;
    burstLeft_ = 0;
    airJumps_ = traits_.jumpJets ? 1 : 0;
    reacted_ = dashPending_ = orbitalPending_ = minePending_ = false;
    jumpPending_ = 0;
    perchIdx_ = -1; perchCd_ = Util::RandRange(2.f, 4.f); perchStay_ = perchTry_ = accidental_ = 0.f;
    shots_.clear();
    rolled_.clear();
    aimT_ = 0.f;
    finalForm_ = flying_ = wantFly_ = false;
    finalDodge_ = 0.f; finalMoveMul_ = 1.f; finalShield_ = dodgePending_ = false;
    wingT_ = 0.f;
    for (auto& v : scarf_) v = { pos.x, pos.y - 44.f };
}

void Hero::EnterFinalForm(const Seraph::Stats& st) {
    finalForm_ = true; flying_ = true; wantFly_ = true;
    maxHp = hp = st.hp;
    finalDodge_ = st.dodge;
    finalMoveMul_ = st.moveMul;
    finalShield_ = st.shield && traits_.deflectShield;
    dodgePending_ = false;
    pos = { 320.f, 170.f }; vel = { 0.f, 0.f };
    grounded_ = false; standIdx_ = -1;
    target_ = pos; finalSpeed_ = 120.f;
    invuln_ = 1.5f; hitFlash = 0.f; facing = -1;
    for (auto& v : scarf_) v = { pos.x, pos.y - 44.f };
}

float Hero::TakeDamage(float dmg, bool isProjectile) {
    if (invuln_ > 0.f) return -1.f;
    if (finalForm_ && finalDodge_ > 0.f && Util::Rand01() < finalDodge_) {
        // El Serafin "lee" el golpe y se desplaza hacia el centro de la arena.
        const float side = pos.x < Cfg::VIEW_W * 0.5f ? 1.f : -1.f;
        pos.x = Clamp(pos.x + side * 56.f, w * 0.5f, (float)Cfg::VIEW_W - w * 0.5f);
        invuln_ = 0.35f;
        dodgePending_ = true;
        return -1.f;
    }
    if (isProjectile && Shielded()) {
        dmg *= (1.f - Cfg::DEFLECT_REDUCTION);
        shieldFlash_ = 1.f;
    }
    hp = std::max(0.f, hp - dmg);
    hitFlash = 1.f;
    invuln_ = 0.12f;
    return dmg;
}

bool Hero::Jump() {
    if (!OnGround() || jumpCd_ > 0.f) return false;
    vel.y = Cfg::HERO_JUMP_V;
    jumpCd_ = 0.6f;
    jumpPending_ = 1;
    standIdx_ = -1; grounded_ = false;
    return true;
}

bool Hero::JetJump() {
    if (OnGround() || airJumps_ <= 0 || !traits_.jumpJets) return false;
    vel.y = Cfg::HERO_JET_V;
    --airJumps_;
    jumpPending_ = 2;
    return true;
}

void Hero::StartDash(int towardBoss) {
    int away = -towardBoss;
    float target = pos.x + away * Cfg::DASH_SPEED * Cfg::DASH_TIME;
    dashDir_ = (target < 48.f || target > Cfg::VIEW_W - 48.f) ? towardBoss : away;
    dashT_   = Cfg::DASH_TIME;
    dashCd_  = traits_.dashCooldown;
    invuln_  = Cfg::DASH_TIME + 0.08f;
    dashPending_ = true;
}

Vector2 Hero::AimDir(const Boss& boss, Vector2 origin, float speed, float spread) {
    Vector2 target = boss.Center();
    if (traits_.leadShooting) {
        Vector2 t = target;
        for (int i = 0; i < 3; ++i) {
            float flight = Vector2Distance(origin, t) / speed;
            t = Vector2Add(boss.Center(), Vector2Scale(boss.vel, flight));
            if (!boss.OnGround())
                t.y += 0.5f * Cfg::GRAVITY * worldGravity * flight * flight;
            t.y = std::min(t.y, Cfg::GROUND_Y - 30.f);
        }
        target = t;
    }
    aimPoint_ = target;
    Vector2 dir = Vector2Normalize(Vector2Subtract(target, origin));
    float sp = traits_.leadShooting ? spread * 0.45f : spread;
    return Vector2Rotate(dir, Util::RandRange(-sp, sp));
}

void Hero::FireBolt(const Boss& boss) {
    ShotCmd s;
    s.origin = MuzzlePos();
    s.dir = AimDir(boss, s.origin, Cfg::HERO_BOLT_SPEED, 0.045f);
    s.speed = Cfg::HERO_BOLT_SPEED; s.damage = traits_.damage; s.radius = 4.f; s.kind = ProjKind::Bolt;
    shots_.push_back(s);
    attackCd_ = traits_.fireInterval * Util::RandRange(0.9f, 1.1f);
}

void Hero::FireBurst() {
    burstLeft_ = 3;
    burstTimer_ = 0.f;
    attackCd_ = traits_.fireInterval * 2.4f;
}

void Hero::FireSpread(const Boss& boss) {
    const Vector2 o = MuzzlePos();
    const Vector2 d = AimDir(boss, o, 380.f, 0.03f);
    for (int k = -2; k <= 2; ++k) {
        ShotCmd s;
        s.origin = o; s.dir = Vector2Rotate(d, k * 0.15f);
        s.speed = 380.f; s.damage = traits_.damage * 0.6f; s.radius = 3.f; s.life = 0.55f;
        s.kind = ProjKind::Pellet;
        shots_.push_back(s);
    }
    attackCd_ = traits_.fireInterval * 1.7f;
}

void Hero::FireMissile() {
    ShotCmd s;
    s.origin = MuzzlePos();
    s.dir = Vector2Normalize({ (float)facing * 0.5f, -1.f });
    s.speed = 270.f; s.damage = traits_.damage * 3.2f; s.radius = 5.f; s.life = 4.5f;
    s.kind = ProjKind::Missile; s.homing = true;
    shots_.push_back(s);
    attackCd_ = std::max(1.3f, traits_.fireInterval * 3.f);
}

void Hero::UpdateAttacks(float dt, const Boss& boss, const std::vector<Projectile>& projs, float dist) {
    attackCd_ -= dt; orbitalCd_ -= dt; mineCd_ -= dt;
    bossStill_ = (std::fabs(boss.vel.x) < 25.f && boss.OnGround()) ? bossStill_ + dt : 0.f;

    if (burstLeft_ > 0) {
        burstTimer_ -= dt;
        if (burstTimer_ <= 0.f) {
            ShotCmd s;
            s.origin = MuzzlePos();
            s.dir = AimDir(boss, s.origin, Cfg::HERO_BOLT_SPEED * 1.15f, 0.05f);
            s.speed = Cfg::HERO_BOLT_SPEED * 1.15f; s.damage = traits_.damage * 0.8f; s.radius = 3.5f;
            s.kind = ProjKind::Bolt;
            shots_.push_back(s);
            --burstLeft_;
            burstTimer_ = 0.09f;
        }
        return;
    }
    if (dashT_ > 0.f) return;

    if (Has(Atk::ORBITAL) && orbitalCd_ <= 0.f && dist < 520.f &&
        (bossStill_ > 0.45f || traits_.leadShooting)) {
        float x = boss.pos.x;
        if (traits_.leadShooting) x += boss.vel.x * Cfg::ORBITAL_DELAY * 0.85f;
        orbitalX_ = Clamp(x, 30.f, (float)Cfg::VIEW_W - 30.f);
        orbitalPending_ = true;
        orbitalCd_ = std::max(4.f, 8.5f - 0.7f * (float)traits_.generation);
        attackCd_ = std::max(attackCd_, 0.45f);
        return;
    }

    if (Has(Atk::MINE) && mineCd_ <= 0.f && OnGround() && boss.OnGround() && dist < 240.f) {
        minePending_ = true;
        mineCd_ = 5.5f;
        attackCd_ = std::max(attackCd_, 0.3f);
        return;
    }

    if (attackCd_ > 0.f || dist > 430.f) return;

    int missilesInFlight = 0;
    for (const auto& p : projs)
        if (p.alive && p.owner == Owner::Hero && p.kind == ProjKind::Missile) ++missilesInFlight;

    const bool bossEvasive = !boss.OnGround() || std::fabs(boss.vel.x) > 120.f;
    float wBolt = 3.f;
    float wBurst = Has(Atk::BURST) ? 2.2f : 0.f;
    float wSpread = (Has(Atk::SPREAD) && dist < 215.f) ? 5.f : 0.f;
    float wMissile = (Has(Atk::MISSILE) && dist > 110.f && missilesInFlight < 2) ? (bossEvasive ? 3.5f : 1.8f) : 0.f;

    float roll = Util::Rand01() * (wBolt + wBurst + wSpread + wMissile);
    if ((roll -= wSpread) < 0.f && wSpread > 0.f)       FireSpread(boss);
    else if ((roll -= wMissile) < 0.f && wMissile > 0.f) FireMissile();
    else if ((roll -= wBurst) < 0.f && wBurst > 0.f)    FireBurst();
    else                                                 FireBolt(boss);
}

void Hero::TryDodgeProjectiles(const std::vector<Projectile>& projs) {
    if (!OnGround() || jumpCd_ > 0.f || dashT_ > 0.f) return;
    const Vector2 c = Center();
    for (const auto& p : projs) {
        if (!p.alive || p.owner != Owner::Boss) continue;
        Vector2 rel = Vector2Subtract(c, p.pos);
        if (Vector2Length(rel) > 190.f) continue;
        if (Vector2DotProduct(p.vel, rel) <= 0.f) continue;
        Vector2 vn = Vector2Normalize(p.vel);
        float perp = std::fabs(vn.x * rel.y - vn.y * rel.x);
        if (perp > Cfg::HERO_H * 0.7f) continue;
        if (std::find(rolled_.begin(), rolled_.end(), p.id) != rolled_.end()) continue;

        rolled_.push_back(p.id);
        if (rolled_.size() > 24) rolled_.erase(rolled_.begin());
        if (Util::Rand01() < traits_.dodgeSkill) { Jump(); return; }
    }
}

int Hero::PickPerch(const Arena& arena) const {
    const float reach = traits_.jumpJets ? 185.f : 98.f;
    int best = -1;
    float bestScore = 1e9f;
    for (int i = 0; i < (int)arena.platforms.size(); ++i) {
        const Platform& p = arena.platforms[i];
        const float above = pos.y - p.rect.y;
        const float cx = p.rect.x + p.rect.width * 0.5f;
        if (above < 30.f || above > reach) continue;
        if (std::fabs(cx - pos.x) > 260.f) continue;
        const float score = std::fabs(cx - pos.x) + Util::RandRange(0.f, 120.f);
        if (score < bestScore) { bestScore = score; best = i; }
    }
    return best;
}

void Hero::Update(float dt, const Boss& boss, const std::vector<Projectile>& projs, const Arena& arena) {
    animTime += dt;
    hitFlash     = std::max(0.f, hitFlash - dt * 4.f);
    shieldFlash_ = std::max(0.f, shieldFlash_ - dt * 3.f);
    invuln_      = std::max(0.f, invuln_ - dt);
    dashCd_     -= dt;
    jumpCd_     -= dt;
    strafeTimer_ -= dt;
    perchCd_    -= dt;
    overdrive_   = std::max(0.f, overdrive_ - dt);
    const float speedMul = overdrive_ > 0.f ? 1.25f : 1.f;
    dashT_      -= dt;
    if (OnGround()) airJumps_ = traits_.jumpJets ? 1 : 0;

    if (finalForm_) { UpdateFinal(dt, boss, arena); return; }

    const float dx = boss.pos.x - pos.x;
    const float dist = std::fabs(dx);
    const int toward = dx >= 0.f ? 1 : -1;
    facing = toward;
    const bool sameLevel = std::fabs(boss.pos.y - pos.y) < 45.f;
    const float dangerDist = boss.w * 0.5f + boss.MeleeReach() + Cfg::HERO_W * 0.5f + 8.f;

    if (dashT_ <= 0.f) {
        if (boss.IsWindingUp()) {
            if (!reacted_) {
                reacted_ = true;
                if (dist < dangerDist && sameLevel) {
                    if (traits_.dashEvasive && dashCd_ <= 0.f) StartDash(toward);
                    else if (OnGround() && jumpCd_ <= 0.f && Util::Rand01() < traits_.dodgeSkill) Jump();
                }
            }
        } else {
            reacted_ = false;
        }
        if (boss.IsDiving() && dist < dangerDist && traits_.dashEvasive && dashCd_ <= 0.f && sameLevel)
            StartDash(toward);
        if (dashT_ <= 0.f && traits_.dashEvasive && dashCd_ <= 0.f && OnGround() &&
            dist < dangerDist - 30.f && sameLevel)
            StartDash(toward);

        TryDodgeProjectiles(projs);
    }

    const int standing = StandingPlatform();
    if (dashT_ <= 0.f) {
        if (perchIdx_ < 0 && perchCd_ <= 0.f && traits_.generation >= 2 && OnGround() &&
            standing < 0 && !boss.IsWindingUp()) {
            perchIdx_ = PickPerch(arena);
            perchStay_ = perchTry_ = 0.f;
            stayMax_ = Util::RandRange(3.f, 5.5f);
            if (perchIdx_ < 0) perchCd_ = 2.f;
        }
        if (perchIdx_ >= 0) {
            const Platform& p = arena.platforms[perchIdx_];
            const float cx = p.rect.x + p.rect.width * 0.5f;
            if (standing == perchIdx_) {
                perchStay_ += dt;
                if (perchStay_ > stayMax_) {
                    perchIdx_ = -1;
                    perchCd_ = Util::RandRange(5.f, 9.f);
                    DropThrough(0.3f);
                }
            } else {
                perchTry_ += dt;
                if (perchTry_ > 4.5f) { perchIdx_ = -1; perchCd_ = 3.f; }
                const float above = pos.y - p.rect.y;
                if (OnGround() && std::fabs(cx - pos.x) < 70.f && above > 20.f && above < 100.f) Jump();
                else if (!OnGround() && traits_.jumpJets && vel.y > -40.f && above > 8.f && above < 110.f &&
                         std::fabs(cx - pos.x) < p.rect.width * 0.5f + 20.f)
                    JetJump();
            }
        } else if (standing >= 0) {
            accidental_ += dt;
            if (accidental_ > 1.8f) { accidental_ = 0.f; DropThrough(0.3f); }
        } else {
            accidental_ = 0.f;
        }
    }

    if (dashT_ > 0.f) {
        vel.x = dashDir_ * Cfg::DASH_SPEED;
    } else {
        if (strafeTimer_ <= 0.f) {
            strafeTimer_ = Util::RandRange(0.8f, 1.8f);
            strafeDir_   = GetRandomValue(-1, 1);
            desiredDist_ = Util::RandRange(220.f, 300.f);
        }
        float moveDir;
        if (boss.IsWindingUp() && sameLevel && dist < dangerDist + 30.f) moveDir = (float)-toward;
        else if (perchIdx_ >= 0 && standing != perchIdx_) {
            const Platform& p = arena.platforms[perchIdx_];
            const float cx = p.rect.x + p.rect.width * 0.5f;
            moveDir = std::fabs(cx - pos.x) > 18.f ? (cx > pos.x ? 1.f : -1.f) : 0.f;
        }
        else if (dist < desiredDist_ - 30.f)                moveDir = (float)-toward;
        else if (dist > desiredDist_ + 40.f)                moveDir = (float)toward;
        else                                                moveDir = (float)strafeDir_;

        for (float cx : threats_) {
            if (std::fabs(pos.x - cx) < 54.f) {
                moveDir = pos.x < cx ? -1.f : 1.f;
                if (traits_.dashEvasive && dashCd_ <= 0.f && std::fabs(pos.x - cx) < 30.f) StartDash(pos.x < cx ? 1 : -1);
            }
        }

        if ((pos.x < 48.f && moveDir < 0.f) || (pos.x > Cfg::VIEW_W - 48.f && moveDir > 0.f))
            moveDir = 0.f;
        if (standing >= 0 && standing == perchIdx_) {
            const Platform& p = arena.platforms[standing];
            if ((pos.x < p.rect.x + 14.f && moveDir < 0.f) ||
                (pos.x > p.rect.x + p.rect.width - 14.f && moveDir > 0.f))
                moveDir = 0.f;
        }
        vel.x = moveDir * traits_.moveSpeed * speedMul;
    }

    UpdateAttacks(dt * (overdrive_ > 0.f ? 1.5f : 1.f), boss, projs, dist);
    aimT_ = (!shots_.empty() || burstLeft_ > 0 || orbitalPending_) ? 0.4f : std::max(0.f, aimT_ - dt);

    ApplyPhysics(dt, arena, dashT_ > 0.f ? 0.15f : 1.f);

    UpdateScarf(dt);
}

void Hero::UpdateScarf(float dt) {
    scarf_[0] = { pos.x - facing * 6.f, pos.y - 44.f };
    for (int i = 1; i < 7; ++i) {
        const Vector2 target = { scarf_[i - 1].x - facing * 4.f - vel.x * 0.014f,
                                 scarf_[i - 1].y + 2.2f - vel.y * 0.012f };
        const float k = std::min(1.f, dt * (18.f - 1.8f * (float)i));
        scarf_[i].x += (target.x - scarf_[i].x) * k;
        scarf_[i].y += (target.y - scarf_[i].y) * k;
    }
}

void Hero::UpdateFinal(float dt, const Boss& boss, const Arena& arena) {
    const float fs = finalSpeed_ * finalMoveMul_;
    wingT_ += dt * (flying_ ? 9.f : 3.f);
    facing = (boss.pos.x >= pos.x) ? 1 : -1;
    aimT_ = std::max(0.f, aimT_ - dt);

    if (wantFly_ && !flying_) {
        vel.y = -430.f; standIdx_ = -1; grounded_ = false; flying_ = true; jumpPending_ = 2;
    }

    if (flying_) {
        const Vector2 goal = wantFly_ ? target_ : Vector2{ target_.x, Cfg::GROUND_Y };
        const Vector2 d = { goal.x - pos.x, goal.y - pos.y };
        const float dist = std::sqrt(d.x * d.x + d.y * d.y);
        Vector2 desired = { 0.f, 0.f };
        if (dist > 1.f) {
            const float sp = std::min(fs, dist * 3.f);
            desired = { d.x / dist * sp, d.y / dist * sp };
        }
        const float k = 1.f - std::exp(-dt * 5.f);
        vel.x += (desired.x - vel.x) * k;
        vel.y += (desired.y - vel.y) * k;
        pos.x += vel.x * dt; pos.y += vel.y * dt;
        pos.x = Clamp(pos.x, w * 0.5f, (float)Cfg::VIEW_W - w * 0.5f);
        pos.y = Clamp(pos.y, h + 56.f, Cfg::GROUND_Y);
        grounded_ = false; standIdx_ = -1;
        if (!wantFly_ && pos.y >= Cfg::GROUND_Y - 3.f) {
            flying_ = false; pos.y = Cfg::GROUND_Y; vel.y = 0.f; grounded_ = true; landSquash_ = 0.6f;
        }
    } else {
        const float dx = target_.x - pos.x;
        vel.x = (std::fabs(dx) > 6.f) ? (dx > 0.f ? 1.f : -1.f) * std::min(fs, std::fabs(dx) * 4.f) : 0.f;
        ApplyPhysics(dt, arena, 1.f);
    }
    UpdateScarf(dt);
}

void Hero::DrawWings(float time) const {
    (void)time;
    const Vector2 anchor = { pos.x - facing * 6.f, pos.y - 46.f };
    const float base = facing > 0 ? 3.14159f : 0.f;
    const float flap = std::sin(wingT_) * (flying_ ? 0.40f : 0.12f);
    const float up = 1.f;

    for (int wing = 0; wing < 2; ++wing) {
        const float scale = wing == 0 ? 1.f : 0.65f;
        Vector2 prevTip = anchor;
        for (int i = 0; i < 6; ++i) {
            const float delta = (wing == 0 ? 0.30f + 0.26f * (float)i + flap * (0.6f + 0.12f * (float)i)
                                           : -(0.15f + 0.20f * (float)i) + flap * 0.5f) * up;
            const float ang = base + facing * delta;
            const float len = (54.f - 4.f * (float)i) * scale;
            const Vector2 tip = { anchor.x + std::cos(ang) * len, anchor.y + std::sin(ang) * len };
            const Color c = (i % 2) ? Color{255, 225, 130, 255} : Color{255, 250, 235, 255};
            DrawLineEx(anchor, tip, 3.4f - 0.35f * (float)i, Fade(c, 0.85f));
            DrawLineEx(anchor, tip, 7.f - 0.7f * (float)i, Fade(Color{255, 200, 80, 255}, 0.16f));
            if (i > 0) {
                Vector2 a = anchor, b = prevTip, cpt = tip;
                const float cr = (b.x - a.x) * (cpt.y - a.y) - (b.y - a.y) * (cpt.x - a.x);
                if (cr > 0.f) DrawTriangle(a, cpt, b, Fade(Color{255, 215, 110, 255}, 0.13f));
                else          DrawTriangle(a, b, cpt, Fade(Color{255, 215, 110, 255}, 0.13f));
            }
            prevTip = tip;
        }
    }
}

void Hero::Draw(const Sprites& sp, float time) const {
    const float air = std::min(1.f, (Cfg::GROUND_Y - pos.y) / 180.f);
    DrawEllipse((int)pos.x, (int)Cfg::GROUND_Y + 2, 26.f * (1.f - 0.4f * air), 5.f, Fade(BLACK, 0.5f * (1.f - 0.5f * air)));

    const bool moving = std::fabs(vel.x) > 8.f;
    int legs = Sprites::L_STAND;
    if (dashT_ > 0.f)           legs = Sprites::L_RUN0;
    else if (!OnGround())       legs = vel.y < 0.f ? Sprites::L_JUMP : Sprites::L_FALL;
    else if (moving) {
        static const int CYCLE[4] = { Sprites::L_RUN0, Sprites::L_RUN1, Sprites::L_RUN2, Sprites::L_RUN3 };
        legs = CYCLE[(int)(walkPhase * 4.f) & 3];
    }
    const int arms = (aimT_ > 0.f || burstLeft_ > 0) ? Sprites::A_AIM : Sprites::A_READY;
    const int frame = Sprites::HeroFrame(arms, legs);

    const bool idle = OnGround() && !moving;
    const float breathe = idle ? 0.02f * std::sin(time * 3.6f + 1.f) : 0.f;
    float sy = 1.f + breathe - 0.22f * LandSquash();
    float sx = 1.f - breathe * 0.6f + 0.18f * LandSquash();
    if (!OnGround() && vel.y < 0.f) { const float st = std::min(1.f, -vel.y / 560.f); sy += 0.09f * st; sx -= 0.05f * st; }
    float lean = Clamp(vel.x / 200.f, -1.f, 1.f) * 7.f;
    if (dashT_ > 0.f) { lean = dashDir_ * 14.f; sx += 0.18f; sy -= 0.08f; }
    const float bob = (OnGround() && moving) ? -std::fabs(std::sin(walkPhase * 6.2832f * 2.f)) * 1.5f : 0.f;
    const Vector2 feet = { pos.x, pos.y + bob };
    const float S = Cfg::HERO_W;

    if (finalForm_) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawWings(time);
        EndBlendMode();
    }

    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 1; i < 7; ++i)
        DrawLineEx(scarf_[i - 1], scarf_[i], 3.2f - 0.4f * (float)i, Fade(i % 2 ? Theme::NEON_YELLOW : Color{255, 150, 30, 255}, 0.85f - 0.09f * (float)i));
    EndBlendMode();

    if (dashT_ > 0.f) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int k = 1; k <= 4; ++k)
            sp.DrawHero(frame, { feet.x - dashDir_ * k * 16.f, feet.y }, S, facing >= 0, lean, sx, sy,
                        Fade(Theme::NEON_CYAN, 0.5f - k * 0.1f), 0.f);
        EndBlendMode();
    }

    Color tint = WHITE;
    if (hitFlash > 0.f) tint = { 255, 110, 110, 255 };
    if (invuln_ > 0.f && dashT_ <= 0.f && std::fmod(time * 30.f, 2.f) < 1.f) tint.a = 110;
    if (finalForm_ && hitFlash <= 0.f) tint = { 255, 238, 190, tint.a };
    const Vector2 hoverFeet = (finalForm_ && flying_) ? Vector2{ feet.x, feet.y + std::sin(time * 3.f) * 3.f } : feet;
    sp.DrawHero(frame, hoverFeet, S, facing >= 0, lean, sx, sy, tint, (finalForm_ ? 0.9f : 0.55f) + 0.2f * std::sin(time * 6.f));
    if (finalForm_) {
        BeginBlendMode(BLEND_ADDITIVE);
        const float hy = hoverFeet.y - 80.f;
        DrawCircleLines((int)pos.x, (int)hy, 12.f, Fade(Color{255, 225, 120, 255}, 0.9f));
        DrawCircleLines((int)pos.x, (int)hy, 11.f, Fade(Color{255, 225, 120, 255}, 0.5f));
        DrawCircleGradient((int)pos.x, (int)(hoverFeet.y - 34.f), 46.f, Fade(Color{255, 215, 110, 255}, 0.16f), Fade(Color{255, 215, 110, 255}, 0.f));
        EndBlendMode();
    }

    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient((int)(pos.x + facing * 4.f), (int)(pos.y - 54.f), 9.f,
                       Fade(Theme::NEON_CYAN, 0.45f), Fade(Theme::NEON_CYAN, 0.f));
    {
        const bool burn = traits_.jumpJets && !OnGround();
        const float fl = 0.6f + 0.4f * std::sin(time * 50.f);
        const float jx = pos.x - facing * 18.f, jy = pos.y - 28.f;
        const float len = burn ? 16.f * fl : 3.f * fl;
        DrawTriangle({ jx - 4.f, jy }, { jx, jy + len }, { jx + 4.f, jy }, Fade(Theme::NEON_YELLOW, burn ? 0.9f : 0.35f));
        if (burn) DrawCircleGradient((int)jx, (int)(jy + 6.f), 12.f * fl, Fade(Theme::NEON_YELLOW, 0.7f), Fade(Theme::NEON_YELLOW, 0.f));
    }
    if (Shielded()) {
        float r = 44.f + std::sin(time * 6.f) * 2.f + shieldFlash_ * 6.f;
        Vector2 c = Center();
        DrawPoly(c, 6, r, time * 20.f, Fade(Theme::NEON_CYAN, 0.07f + 0.25f * shieldFlash_));
        DrawPolyLinesEx(c, 6, r, time * 20.f, 1.5f, Fade(Theme::NEON_CYAN, 0.45f + 0.5f * shieldFlash_));
    }
    if (traits_.leadShooting && Alive()) {
        float pulse = 7.f + std::sin(time * 8.f) * 1.5f;
        DrawCircleLines((int)aimPoint_.x, (int)aimPoint_.y, pulse, Fade(Theme::NEON_YELLOW, 0.55f));
        DrawLine((int)aimPoint_.x - 12, (int)aimPoint_.y, (int)aimPoint_.x + 12, (int)aimPoint_.y,
                 Fade(Theme::NEON_YELLOW, 0.4f));
        DrawLine((int)aimPoint_.x, (int)aimPoint_.y - 12, (int)aimPoint_.x, (int)aimPoint_.y + 12,
                 Fade(Theme::NEON_YELLOW, 0.4f));
    }
    if (overdrive_ > 0.f) {
        const float a = overdrive_ < 1.5f ? 0.2f + 0.2f * std::sin(time * 30.f) : 0.4f;
        DrawCircleGradient((int)pos.x, (int)(pos.y - 32.f), 52.f, Fade(Theme::NEON_RED, a), Fade(Theme::NEON_RED, 0.f));
    }
    if (burstLeft_ > 0)
        DrawCircleGradient((int)MuzzlePos().x, (int)MuzzlePos().y, 12.f, Fade(Theme::NEON_CYAN, 0.7f), Fade(Theme::NEON_CYAN, 0.f));
    EndBlendMode();
}

void Hero::DrawEcho(const Sprites& sp, float time, Vector2 feet, float alpha, bool faceRight, Color tint) const {
    const int frame = Sprites::HeroFrame(Sprites::A_READY, Sprites::L_STAND);
    const float bob = std::sin(time * 3.f + feet.x * 0.05f) * 3.f;
    BeginBlendMode(BLEND_ADDITIVE);
    sp.DrawHero(frame, { feet.x, feet.y + bob }, Cfg::HERO_W, faceRight, 0.f, 1.f, 1.f, Fade(tint, alpha), 0.9f);
    DrawCircleGradient((int)feet.x, (int)(feet.y + bob - 32.f), 40.f, Fade(tint, 0.18f * alpha), Fade(tint, 0.f));
    EndBlendMode();
}
