#include "entities/Entity.h"
#include "entities/Arena.h"
#include <algorithm>
#include <cmath>

void Entity::ApplyPhysics(float dt, const Arena& arena, float gravityScale) {
    dropT_ = std::max(0.f, dropT_ - dt);
    const int n = (int)arena.platforms.size();

    if (standIdx_ >= 0 && standIdx_ < n) {
        const Platform& p = arena.platforms[standIdx_];
        pos.x += p.delta.x;
        pos.y  = p.rect.y;
    } else {
        standIdx_ = -1;
    }
    const float prevFeet = pos.y;
    const float vyIn = vel.y;

    vel.y += Cfg::GRAVITY * gravityScale * worldGravity * dt;
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    grounded_ = false;
    const float hw = w * 0.35f;

    if (standIdx_ >= 0) {
        const Platform& p = arena.platforms[standIdx_];
        const bool over = pos.x + hw > p.rect.x && pos.x - hw < p.rect.x + p.rect.width;
        if (vel.y >= 0.f && over && dropT_ <= 0.f) {
            pos.y = p.rect.y; vel.y = 0.f; grounded_ = true;
        } else {
            standIdx_ = -1;
        }
    }

    if (standIdx_ < 0 && vel.y >= 0.f && dropT_ <= 0.f) {
        for (int i = 0; i < n; ++i) {
            const Platform& p = arena.platforms[i];
            const bool over = pos.x + hw > p.rect.x && pos.x - hw < p.rect.x + p.rect.width;
            const float prevTop = p.rect.y - p.delta.y;
            if (over && prevFeet <= prevTop + 4.f && pos.y >= p.rect.y) {
                pos.y = p.rect.y; vel.y = 0.f; standIdx_ = i; grounded_ = true;
                break;
            }
        }
    }

    if (pos.y >= Cfg::GROUND_Y) {
        pos.y = Cfg::GROUND_Y; vel.y = 0.f; grounded_ = true; standIdx_ = -1;
    }

    pos.x = Clamp(pos.x, w * 0.5f, (float)Cfg::VIEW_W - w * 0.5f);

    landSquash_ = std::max(0.f, landSquash_ - dt * 6.f);
    if (grounded_ && !wasGrounded_ && vyIn > 120.f) {
        landSquash_ = Clamp(vyIn / 900.f, 0.15f, 1.f);
        landPending_ = true; landSpeed_ = vyIn;
    }
    wasGrounded_ = grounded_;
    if (grounded_ && std::fabs(vel.x) > 8.f) {
        walkPhase += std::fabs(vel.x) * dt / (w * 0.8f);
        walkPhase -= std::floor(walkPhase);
    }
}
