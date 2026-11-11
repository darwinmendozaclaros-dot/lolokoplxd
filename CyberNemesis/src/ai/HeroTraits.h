#pragma once
#include "core/Config.h"

struct HeroTraits {
    int      generation    = 1;
    float    maxHp         = Cfg::HERO_BASE_HP;
    float    moveSpeed     = Cfg::HERO_BASE_SPEED;
    float    damage        = Cfg::HERO_BASE_DAMAGE;
    float    fireInterval  = Cfg::HERO_BASE_FIRE;
    float    dodgeSkill    = Cfg::HERO_BASE_DODGE;
    float    dashCooldown  = Cfg::DASH_COOLDOWN;
    int      speedLevel    = 0;
    unsigned attacks       = Atk::BOLT;
    bool     deflectShield = false;
    bool     dashEvasive   = false;
    bool     leadShooting  = false;
    bool     jumpJets      = false;
};
