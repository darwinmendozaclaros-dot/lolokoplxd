#include "core/Game.h"
#include "core/Story.h"
#include "core/Util.h"
#include "gfx/IconData.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <ctime>

namespace {
    const float       FX_VALUES[4] = { 0.f, 0.35f, 0.65f, 1.f };
    const char* const FX_NAMES[4]  = { "APAGADOS", "SUAVES", "MEDIOS", "INTENSOS" };
}

Game::Game() {}
Game::~Game() {}

void Game::Init() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(Cfg::WINDOW_W, Cfg::WINDOW_H, "CYBER-NEMESIS : Adaptive Boss Protocol");
    {
        Image icon{};
        icon.data = const_cast<unsigned char*>(Embedded::ICON_RGBA);
        icon.width = 32; icon.height = 32; icon.mipmaps = 1;
        icon.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        SetWindowIcon(icon);
    }
    SetWindowMinSize(Cfg::VIEW_W, Cfg::VIEW_H);
    SetExitKey(KEY_NULL);
    SetTargetFPS(144);

    sprites_.Init();
    fx_.Init();
    audio_.Init();
    fx_.SetIntensity(FX_VALUES[fxLevel_]);
    bg_.Init(1337u);
    shards_.Init(2024u);
    SetRandomSeed((unsigned)std::time(nullptr));

    brainLoaded_ = brain_.Load();
    hero_.ApplyTraits(brain_.Traits());
    boss_.SetScale(Cfg::BossScaleForGeneration(brain_.Traits().generation), true);
    bg_.SetTheme(brain_.Traits().generation);
    ResetRound();
    state_ = State::Title;
    if (startFullscreen_) ToggleFullscreen();      // --fullscreen
}

void Game::Shutdown() {
    brain_.Save();
    audio_.Shutdown();
    fx_.Shutdown();
    sprites_.Shutdown();
    CloseWindow();
}

void Game::Run() {
    Init();
    while (!WindowShouldClose() && !quit_) {
        const float dt = std::min(GetFrameTime(), 1.f / 30.f);
        Update(dt);
        Draw();
    }
    Shutdown();
}

Rectangle Game::PresentRect() const {
    const float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
    const float s = std::min(w / Cfg::VIEW_W, h / Cfg::VIEW_H);
    return { (w - Cfg::VIEW_W * s) * 0.5f, (h - Cfg::VIEW_H * s) * 0.5f, Cfg::VIEW_W * s, Cfg::VIEW_H * s };
}

Vector2 Game::MouseToView() const {
    const Rectangle r = PresentRect();
    const float s = r.width / Cfg::VIEW_W;
    const Vector2 m = GetMousePosition();
    return { (m.x - r.x) / s, (m.y - r.y) / s };
}

void Game::SpawnExplosion(Vector2 p, Color a, Color b, int n) {
    particles_.Burst(p, n, a, 260.f, 4.f, 0.9f, true);
    particles_.Burst(p, n / 2, b, 160.f, 3.f, 1.2f, false);
    particles_.Ring(p, 24, WHITE, 220.f, 2.f, 0.5f);
}

BossInput Game::IdleBossInput(float lookAtX) const {
    BossInput in;
    in.aim = { lookAtX, boss_.pos.y - 50.f };
    return in;
}

void Game::Update(float dt) {
    time_ += dt;
    if (state_ != prevState_) {                                  // transicion suave entre pantallas
        const State s = state_;
        const bool fade = s == State::Title || s == State::Prologue || s == State::Codex || s == State::Reboot ||
                          s == State::FinalIntro || (s == State::Playing && prevState_ != State::Choice);
        if (fade) fadeT_ = 1.f;
        prevState_ = s;
    }
    if (IsKeyPressed(KEY_M)) audio_.ToggleMusic();
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
    if (IsKeyPressed(KEY_F10)) {
        fxLevel_ = (fxLevel_ + 1) % 4;
        fx_.SetIntensity(FX_VALUES[fxLevel_]);
        fxMsgT_ = 2.f;
    }
    fxMsgT_ = std::max(0.f, fxMsgT_ - dt);
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (state_ == State::Title) quit_ = true;
        else ReturnToMenu();
        return;
    }
    // ---- Teclas de debug ----
    if (IsKeyPressed(KEY_F8) && (state_ == State::Title || state_ == State::Playing || state_ == State::BossDown)) {
        DebugJumpToFinal();                                   // F8: ir directo a la fase final (Serafin)
        return;
    }
    if (IsKeyPressed(KEY_F7) && state_ == State::Playing && introTimer_ <= 0.f && hero_.Alive())
        hero_.hp = 0.f;                                       // F7: matar al Heroe / Serafin (para ver su muerte y regreso)

    cevLabelT_ = std::max(0.f, cevLabelT_ - dt);
    phaseBannerT_ = std::max(0.f, phaseBannerT_ - dt);
    dodgePopT_    = std::max(0.f, dodgePopT_ - dt);
    Entity::worldGravity = Lerp(Entity::worldGravity, gravTarget_, 1.f - std::exp(-dt * 3.f));
    if (IsKeyPressed(KEY_P) && state_ == State::Playing) {
        paused_ = !paused_;
        if (paused_) pauseFact_ = Story::FactText(Story::K_FACT_P);    // un dato curioso distinto en cada pausa
    }

    glitch_ = std::max(0.f, glitch_ - dt * 2.5f);
    shake_  = std::max(0.f, shake_ - dt * 28.f);
    shakeOff_ = { Util::RandRange(-1.f, 1.f) * shake_, Util::RandRange(-1.f, 1.f) * shake_ };
    hintTimer_ += dt;
    flashT_ = std::max(0.f, flashT_ - dt * 2.2f);
    fadeT_  = std::max(0.f, fadeT_  - dt * 3.2f);
    camPunch_ = std::max(0.f, camPunch_ - dt * 1.6f);
    choiceFeedbackT_ = std::max(0.f, choiceFeedbackT_ - dt);
    UpdateCamera(dt);

    const float gen = (float)brain_.Traits().generation;
    audio_.UpdateMusic(final_ ? 1.6f : Util::Clamp01((gen - 1.f) / (float)(Cfg::MAX_GENERATION - 1)));

    if (paused_) return;
    subs_.Update(dt);
    if (subs_.ConsumeBlip()) audio_.Play(AudioManager::Blip, 0.06f, 1.9f);
    UpdateEffects(dt);
    bg_.Update(dt);
    if (bg_.ConsumeLightning()) {
        audio_.Play(AudioManager::Thunder, 0.8f);
        glitch_ = std::max(glitch_, 0.5f);
    }

    switch (state_) {
        case State::Title:     UpdateTitle(dt);     break;
        case State::Prologue:  UpdatePrologue(dt);  break;
        case State::Codex:     UpdateCodex(dt);     break;
        case State::Playing:   UpdatePlaying(dt);   break;
        case State::HeroDying: UpdateHeroDying(dt); break;
        case State::Reboot:    UpdateReboot(dt);    break;
        case State::Choice:    UpdateChoice(dt);    break;
        case State::FinalIntro: UpdateFinalIntro(dt); break;
        case State::BossDown:  UpdateBossDown(dt);  break;
        case State::Victory:   UpdateVictory(dt);   break;
    }
}

void Game::UpdateTitle(float dt) {
    stateTime_ += dt;
    const Vector2 m = MouseToView();
    shards_.SetTint(Util::LerpColor(Color{150, 105, 225, 255}, bg_.GridColor(), 0.35f));
    shards_.Update(dt, { Clamp((m.x / Cfg::VIEW_W - 0.5f) * 2.f, -1.f, 1.f),
                         Clamp((m.y / Cfg::VIEW_H - 0.5f) * 2.f, -1.f, 1.f) });

    const MainMenu::Action act = menu_.Update(dt, m, IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
    if (menu_.ConsumeMoved()) audio_.Play(AudioManager::Blip, 0.35f, 1.4f);

    switch (act) {
        case MainMenu::Action::Start:
            audio_.Play(AudioManager::Blip, 0.6f, 1.8f);
            ResetSession();
            if (!prologueSeen_ && !(brainLoaded_ || brain_.HasLearned())) {
                prologueSeen_ = true;                 // primer arranque: se cuenta el lore
                dlg_.Start(Story::Prologue());
                state_ = State::Prologue;
                stateTime_ = 0.f;
            } else {
                StartRound(false);
            }
            break;
        case MainMenu::Action::Codex:
            audio_.Play(AudioManager::Blip, 0.6f, 1.6f);
            codexPage_ = 0;
            state_ = State::Codex;
            stateTime_ = 0.f;
            break;
        case MainMenu::Action::Effects:
            fxLevel_ = (fxLevel_ + 1) % 4;
            fx_.SetIntensity(FX_VALUES[fxLevel_]);
            audio_.Play(AudioManager::Blip, 0.5f, 1.2f);
            break;
        case MainMenu::Action::Music:
            audio_.ToggleMusic();
            audio_.Play(AudioManager::Blip, 0.5f, 1.2f);
            break;
        case MainMenu::Action::Fullscreen:
            ToggleFullscreen();
            break;
        case MainMenu::Action::Wipe:
            brain_.Reset();
            hero_.ApplyTraits(brain_.Traits());
            boss_.SetScale(1.f, true);
            bg_.SetTheme(1);
            brainLoaded_ = false;
            prologueSeen_ = false;
            Story::ResetVariety();
            eventCd_ = 0.f;
            audio_.Play(AudioManager::Explosion, 0.4f, 1.5f);
            glitch_ = 0.8f;
            break;
        case MainMenu::Action::Quit:
            quit_ = true;
            break;
        default: break;
    }
}

void Game::ReturnToMenu() {
    victorySummary_ = false;
    brain_.Save();
    hero_.ApplyTraits(brain_.Traits());
    boss_.SetScale(Cfg::BossScaleForGeneration(brain_.Traits().generation), true);
    bg_.SetTheme(brain_.Traits().generation);
    final_ = false;
    seraphCycle_ = 1;
    debugFinal_ = false;
    subs_.Clear();
    bg_.SetEventsEnabled(true);
    ResetRound();
    state_ = State::Title;
    stateTime_ = 0.f;
    paused_ = false;
    introTimer_ = 0.f;
}

BossInput Game::ReadBossInput() const {
    BossInput in;
    in.moveDir = (float)((IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) - (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)));
    in.jumpPressed = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_SPACE);
    in.jumpHeld    = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_SPACE);
    in.dash        = IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_L);
    in.down        = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
    in.melee       = IsKeyDown(KEY_J) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    in.shoot       = IsKeyDown(KEY_K) || IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    in.aim         = MouseToView();
    return in;
}

void Game::ResetRound() {
    arena_.Reset();
    boss_.Reset(150.f);
    hero_.Reset(500.f);
    projectiles_.clear();
    shockwaves_.clear();
    hazards_.Clear();
    bullets_.Clear();
    summons_.Clear();
    drones_.Clear();
    pickups_.Clear();
    particles_.Clear();
    phase_ = 0;
    cev_ = lastCev_ = CEvent::None;
    eventTimer_ = Cfg::EVENT_FIRST_DELAY + Util::RandRange(0.f, 6.f);
    gravTarget_ = 1.f;
    Entity::worldGravity = 1.f;
    cevLabelT_ = phaseBannerT_ = 0.f;
    shrinkAnnounced_ = false;
    hero_.SetThreatColumns({});
    tracker_.Reset();
    hud_.Reset(boss_, hero_);
    nextId_ = 1;
    hitStop_ = 0.f;
    slowmo_ = 1.f;
    shake_ = 0.f;
}

void Game::StartRound(bool newGeneration) {
    final_ = false;
    bg_.SetEventsEnabled(true);
    hero_.ApplyTraits(brain_.Traits());
    const float oldScale = boss_.Scale();
    const float newScale = Cfg::BossScaleForGeneration(brain_.Traits().generation);
    ResetRound();
    introShrink_ = newGeneration && newScale < oldScale - 0.001f;
    if (introShrink_) boss_.BeginShrink(oldScale, newScale);
    else boss_.SetScale(newScale, true);
    bg_.SetTheme(brain_.Traits().generation);
    state_ = State::Playing;
    stateTime_ = 0.f;
    introTimer_ = 2.2f;
    hintTimer_ = 0.f;
    audio_.Play(AudioManager::HeroSpawn);
    particles_.Ring({ hero_.pos.x, hero_.pos.y - 32.f }, 30, Theme::NEON_CYAN, 200.f, 3.f, 0.7f);
    if (newGeneration) glitch_ = 1.f;
    debugFinal_ = false;
    halfSaid_ = lowSaid_ = false; fightT_ = idleT_ = 0.f;
    subs_.Clear();
    subs_.SayAll(Story::HeroIntro(brain_.Traits()), 2.6f);     // inicio de la pelea
    particles_.Ring(hero_.Center(), 30, Theme::NEON_CYAN, 240.f, 3.f, 0.8f);               // aparicion del Heroe
    particles_.Ring(boss_.Center(), 22, Theme::NEON_MAGENTA, 200.f, 3.f, 0.7f);
}

void Game::UpdatePlaying(float dt) {
    stateTime_ += dt;
    if (IsKeyPressed(KEY_ENTER)) subs_.Skip();
    if (IsKeyPressed(KEY_H)) hudDetail_ = !hudDetail_;
    hitStop_ = std::max(0.f, hitStop_ - dt);

    if (introTimer_ > 0.f) {
        introTimer_ -= dt;
        if (introTimer_ < 1.7f) {
            if (boss_.IsShrinking() && !shrinkAnnounced_) {
                shrinkAnnounced_ = true;
                audio_.Play(AudioManager::Dash, 0.5f, 0.6f);
                particles_.Ring(boss_.Center(), 20, Theme::NEON_MAGENTA, 160.f, 2.f, 0.5f);
            }
            if (boss_.IsShrinking()) particles_.Burst(boss_.Center(), 2, Theme::NEON_MAGENTA, 80.f, 2.f, 0.4f);
            boss_.UpdateScale(dt);
        }
        arena_.Update(dt);
        particles_.Update(dt);
        hud_.Update(dt, boss_, hero_);
        return;
    }

    const float sdt = dt * (hitStop_ > 0.f ? 0.05f : 1.f);
    arena_.Update(sdt);
    boss_.Update(sdt, ReadBossInput(), arena_);
    hero_.SetThreatColumns(hazards_.NeutralColumns());
    hero_.Update(sdt, boss_, projectiles_, arena_);
    tracker_.Sample(boss_, sdt);

    HandleBossEvents();
    HandleHeroEvents();
    UpdateDialogueTriggers(dt);
    if (final_) UpdateFinalPhase(sdt);
    else { UpdateCombatEvents(sdt); UpdateDrones(sdt); }
    if (pickups_.Update(sdt, arena_, boss_.HurtBox())) {
        boss_.Heal(Cfg::CORE_HEAL);
        boss_.Overcharge(Cfg::OVERCHARGE_TIME);
        particles_.Ring(boss_.Center(), 28, Theme::NEON_YELLOW, 220.f, 3.f, 0.7f);
        particles_.Burst(boss_.Center(), 20, WHITE, 200.f, 2.f, 0.5f);
        audio_.Play(AudioManager::ShieldHit, 0.8f, 1.6f);
        cevLabel_ = "SOBRECARGA ACTIVADA: +40% DANO Y RECARGAS RAPIDAS"; cevLabelT_ = 3.f;
        glitch_ = std::max(glitch_, 0.3f);
    }
    UpdateProjectiles(sdt);
    UpdateHazards(sdt);
    particles_.Update(sdt);
    hud_.Update(dt, boss_, hero_);

    for (auto& s : shockwaves_) s.t += sdt;
    shockwaves_.erase(std::remove_if(shockwaves_.begin(), shockwaves_.end(),
                      [](const Shockwave& s) { return s.t > 0.4f; }), shockwaves_.end());

    if (!hero_.Alive()) OnHeroDefeated();
    else if (!boss_.Alive()) OnBossDefeated();
    else {
        const float frac = hero_.hp / hero_.maxHp;
        if (final_) {   }
        else if (phase_ == 0 && frac < Cfg::PHASE1_FRAC) TriggerPhase(1);
        else if (phase_ == 1 && frac < Cfg::PHASE2_FRAC) TriggerPhase(2);
    }
}

// Polvo al aterrizar: mas grande cuanto mas fuerte fue la caida.
static void LandingDust(ParticleSystem& pt, Vector2 feet, float speed, Color c) {
    if (speed < 260.f) return;
    if (speed > 560.f) pt.Ring({ feet.x, feet.y - 2.f }, 14, c, 150.f, 2.f, 0.3f);          // onda de impacto en caidas fuertes
    const int n = (int)Clamp(speed / 110.f, 3.f, 9.f);
    for (int sgn = -1; sgn <= 1; sgn += 2)
        pt.Directional({ feet.x + sgn * 10.f, feet.y - 1.f }, { (float)sgn, -0.12f }, 0.25f, n, c, 120.f + speed * 0.08f, 2.f, 0.35f);
}

void Game::HandleBossEvents() {
    Vector2 o, d;
    { float ls; if (boss_.ConsumeLand(ls)) LandingDust(particles_, { boss_.pos.x, boss_.pos.y }, ls, Color{170, 160, 220, 255}); }
    if (boss_.DashActive() && GetRandomValue(0, 1) == 0)           // estela de chispas mientras dura el dash
        particles_.Burst({ boss_.pos.x - boss_.facing * 8.f, boss_.pos.y - boss_.h * 0.4f }, 1, Theme::NEON_MAGENTA, 40.f, 2.f, 0.25f);
    if (boss_.ConsumeShot(o, d)) {
        Projectile p;
        p.pos = o; p.vel = Vector2Scale(d, Cfg::BOSS_SHOT_SPEED);
        p.radius = 10.f; p.damage = Cfg::BOSS_SHOT_DAMAGE * boss_.DamageMul(); p.owner = Owner::Boss; p.kind = ProjKind::Plasma;
        p.id = nextId_++;
        projectiles_.push_back(p);
        tracker_.RecordRanged();
        audio_.Play(AudioManager::BossShot, 0.8f);
        particles_.Directional(o, d, 0.5f, 8, Theme::NEON_MAGENTA, 180.f, 3.f, 0.3f);
        shake_ = std::max(shake_, 1.5f);
    }

    if (const int j = boss_.ConsumeJump()) {
        tracker_.RecordJump();
        const Vector2 feet = { boss_.pos.x, boss_.pos.y };
        if (j == 2) {
            particles_.Ring({ feet.x, feet.y - 4.f }, 16, Theme::NEON_CYAN, 140.f, 2.f, 0.4f);
            audio_.Play(AudioManager::Jump, 0.5f, 1.4f);
        } else {
            particles_.Directional(feet, { 0.f, -1.f }, 1.4f, 10, Theme::NEON_CYAN, 120.f, 3.f, 0.4f);
            audio_.Play(AudioManager::Jump, 0.4f, 0.8f);
        }
    }

    if (boss_.ConsumeDash()) {
        tracker_.RecordDash();
        audio_.Play(AudioManager::Dash, 0.6f, 0.8f);
        particles_.Burst(boss_.Center(), 16, Theme::NEON_MAGENTA, 220.f, 3.f, 0.4f);
        shake_ = std::max(shake_, 2.f);
    }

    if (boss_.ConsumeSlam()) {
        tracker_.RecordMelee();
        audio_.Play(AudioManager::Slam);
        shake_ = std::max(shake_, 8.f);
        const float gy = boss_.pos.y;
        const float edge = boss_.w * 0.5f + 8.f;
        shockwaves_.push_back({ { boss_.pos.x, gy }, 0.f });
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            particles_.Directional({ boss_.pos.x + sgn * edge, gy - 2.f }, { (float)sgn, -0.35f },
                                   0.5f, 16, Theme::NEON_MAGENTA, 330.f, 3.f, 0.5f, true);
            particles_.Directional({ boss_.pos.x + sgn * edge, gy - 2.f }, { (float)sgn, -0.15f },
                                   0.3f, 10, Theme::NEON_CYAN, 380.f, 2.f, 0.4f, false);
        }
        std::vector<HazardEvent> ev;
        hazards_.DestroyMinesIn(boss_.MeleeZone(), ev);
        for (const auto& e : ev) particles_.Burst(e.pos, 10, Theme::NEON_YELLOW, 160.f, 2.f, 0.4f);
        drones_.DestroyIn(boss_.MeleeZone());
        summons_.DestroyIn(boss_.MeleeZone());
        if (bullets_.ClearIn(boss_.MeleeZone()) > 0)
            particles_.Ring({ boss_.pos.x, boss_.pos.y - 10.f }, 20, Theme::NEON_CYAN, 200.f, 2.f, 0.4f);

        if (hero_.Alive() && CheckCollisionRecs(boss_.MeleeZone(), hero_.Bounds())) {
            float dealt = hero_.TakeDamage(Cfg::BOSS_MELEE_DAMAGE * boss_.DamageMul(), false);
            if (dealt >= 0.f) {
                if (final_) deathLog_.melee += dealt;
                SpawnExplosion(hero_.Center(), Theme::NEON_RED, Theme::NEON_YELLOW, 18);
                hitStop_ = 0.09f; glitch_ = 1.f; shake_ = 11.f;
                audio_.Play(AudioManager::Hit);
            }
        }
    }
}

void Game::HandleHeroEvents() {
    { float ls; if (hero_.ConsumeLand(ls)) LandingDust(particles_, { hero_.pos.x, hero_.pos.y }, ls, Color{150, 200, 230, 255}); }
    bool firstPellet = true, firstMissile = true;
    for (const ShotCmd& s : hero_.TakeShots()) {
        projectiles_.push_back(MakeHeroProjectile(s));
        if (s.kind == ProjKind::Missile) {
            if (firstMissile) audio_.Play(AudioManager::Missile, 0.6f);
            firstMissile = false;
        } else if (s.kind == ProjKind::Pellet) {
            if (firstPellet) audio_.Play(AudioManager::HeroShot, 0.6f, 0.6f);
            firstPellet = false;
            particles_.Directional(s.origin, s.dir, 0.3f, 2, Theme::NEON_YELLOW, 160.f, 2.f, 0.2f);
        } else {
            audio_.Play(AudioManager::HeroShot, 0.4f);
            particles_.Directional(s.origin, s.dir, 0.4f, 3, Theme::NEON_CYAN, 120.f, 2.f, 0.2f);
        }
    }

    float x, dmg; Vector2 mp;
    if (hero_.ConsumeOrbital(x, dmg)) {
        hazards_.SpawnOrbital(x, dmg);
        audio_.Play(AudioManager::Analyze, 0.5f, 1.2f);
    }
    if (hero_.ConsumeMine(mp, dmg)) {
        hazards_.SpawnMine(mp, dmg);
        audio_.Play(AudioManager::Blip, 0.7f, 0.7f);
        particles_.Burst({ mp.x, mp.y - 4.f }, 6, Theme::NEON_YELLOW, 80.f, 2.f, 0.3f);
    }
    if (hero_.ConsumeDodge()) {
        const Color gold{255, 225, 120, 255};
        audio_.Play(AudioManager::Dash, 0.7f, 1.6f);
        particles_.Ring(hero_.Center(), 18, gold, 220.f, 2.f, 0.4f);
        particles_.Burst(hero_.Center(), 10, WHITE, 160.f, 2.f, 0.3f);
        dodgePopT_ = 0.8f;
        dodgePopPos_ = hero_.Center();
        if (final_) { ++deathLog_.dodges; SayEvent(Story::Ev::Dodge); }
    }
    if (hero_.ConsumeDash()) {
        if (!final_) SayEvent(Story::Ev::Dash);
        audio_.Play(AudioManager::Dash, 0.7f);
        particles_.Burst(hero_.Center(), 14, Theme::NEON_CYAN, 180.f, 3.f, 0.4f);
    }
    if (const int j = hero_.ConsumeJump()) {
        audio_.Play(AudioManager::Jump, 0.4f, j == 2 ? 1.5f : 1.f);
        if (j == 2) particles_.Burst({ hero_.pos.x, hero_.pos.y }, 10, Theme::NEON_YELLOW, 140.f, 2.f, 0.35f);
        else particles_.Directional({ hero_.pos.x, hero_.pos.y }, { 0.f, -1.f }, 1.2f, 6, Theme::NEON_CYAN, 90.f, 2.f, 0.3f);
    }
}

void Game::UpdateProjectiles(float dt) {
    const Vector2 bossTarget = boss_.Center();
    for (auto& p : projectiles_) {
        if (!p.alive) continue;
        p.Update(dt, bossTarget);
        if (!p.alive) {
            if (p.pos.y >= Cfg::GROUND_Y - 1.f) {
                const Color c = p.owner == Owner::Boss ? Theme::NEON_MAGENTA : Theme::NEON_CYAN;
                if (p.kind == ProjKind::Missile) { SpawnExplosion({ p.pos.x, Cfg::GROUND_Y - 4.f }, Theme::NEON_YELLOW, Theme::NEON_RED, 16); audio_.Play(AudioManager::Explosion, 0.4f); }
                else particles_.Directional({ p.pos.x, Cfg::GROUND_Y }, { 0.f, -1.f }, 1.2f, 8, c, 150.f, 2.f, 0.3f, true);
            }
            continue;
        }

        if (p.owner == Owner::Boss) {
            if (drones_.HitCircle(p.pos, p.radius, p.damage)) {
                p.alive = false;
                particles_.Burst(p.pos, 6, Theme::NEON_MAGENTA, 140.f, 2.f, 0.3f);
                continue;
            }
            if (summons_.Absorb(p.pos, p.radius)) {
                p.alive = false;
                particles_.Burst(p.pos, 6, Color{255, 225, 120, 255}, 140.f, 2.f, 0.3f);
                continue;
            }
            if (hero_.Alive() && CheckCollisionCircleRec(p.pos, p.radius, hero_.Bounds())) {
                const bool shielded = hero_.Shielded();
                float dealt = hero_.TakeDamage(p.damage, true);
                if (dealt >= 0.f) {
                    if (final_) deathLog_.plasma += dealt;
                    else if (shielded) SayEvent(Story::Ev::Shield);
                    p.alive = false;
                    SpawnExplosion(p.pos, shielded ? Theme::NEON_CYAN : Theme::NEON_MAGENTA, WHITE, shielded ? 10 : 16);
                    audio_.Play(shielded ? AudioManager::ShieldHit : AudioManager::Hit);
                    shake_ = std::max(shake_, 4.f);
                    hitStop_ = shielded ? 0.f : 0.05f;
                    glitch_ = std::max(glitch_, shielded ? 0.2f : 0.7f);
                }
            }
        } else {
            if (boss_.Alive() && CheckCollisionCircleRec(p.pos, p.radius, boss_.HurtBox())) {
                boss_.TakeDamage(p.damage);
                p.alive = false;
                if (p.kind == ProjKind::Missile) {
                    SpawnExplosion(p.pos, Theme::NEON_YELLOW, Theme::NEON_RED, 22);
                    audio_.Play(AudioManager::Explosion, 0.7f);
                    shake_ = std::max(shake_, 6.f); glitch_ = std::max(glitch_, 0.4f);
                } else {
                    particles_.Burst(p.pos, 8, Theme::NEON_CYAN, 150.f, 2.f, 0.3f);
                    particles_.Burst(p.pos, 4, WHITE, 100.f, 2.f, 0.2f);
                    audio_.Play(AudioManager::Hit, 0.35f, 1.4f);
                    shake_ = std::max(shake_, 1.5f);
                }
            }
        }
    }

    for (auto& bp : projectiles_) {
        if (!bp.alive || bp.owner != Owner::Boss) continue;
        for (auto& mp : projectiles_) {
            if (!mp.alive || mp.owner != Owner::Hero || mp.kind != ProjKind::Missile) continue;
            if (CheckCollisionCircles(bp.pos, bp.radius, mp.pos, mp.radius + 4.f)) {
                bp.alive = mp.alive = false;
                SpawnExplosion(mp.pos, Theme::NEON_YELLOW, Theme::NEON_MAGENTA, 18);
                audio_.Play(AudioManager::Explosion, 0.5f);
                break;
            }
        }
    }

    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(),
                       [](const Projectile& p) { return !p.alive; }), projectiles_.end());
}

void Game::UpdateHazards(float dt) {
    std::vector<HazardEvent> events;
    hazards_.Update(dt, boss_.HurtBox(), boss_.pos.y, events);
    for (const auto& e : events) {
        switch (e.type) {
            case HazardEvent::MineBlast:
                SpawnExplosion(e.pos, Theme::NEON_RED, Theme::NEON_YELLOW, 26);
                audio_.Play(AudioManager::Explosion);
                shake_ = std::max(shake_, 8.f); glitch_ = std::max(glitch_, 0.6f);
                if (boss_.Alive() && CheckCollisionCircleRec(e.pos, e.radius, boss_.HurtBox())) {
                    boss_.TakeDamage(e.damage);
                    hitStop_ = std::max(hitStop_, 0.05f);
                }
                break;
            case HazardEvent::MineFizzle:
                particles_.Burst(e.pos, 8, Theme::NEON_YELLOW, 120.f, 2.f, 0.3f);
                break;
            case HazardEvent::OrbitalFire: {
                audio_.Play(AudioManager::Beam);
                shake_ = std::max(shake_, 9.f); glitch_ = std::max(glitch_, 0.8f);
                for (int i = 0; i < 6; ++i)
                    particles_.Burst({ e.pos.x, Cfg::GROUND_Y - 4.f }, 6, Theme::NEON_CYAN, 260.f, 3.f, 0.5f, true);
                const Rectangle col = { e.pos.x - e.radius, 0.f, e.radius * 2.f, Cfg::GROUND_Y };
                if (e.neutral && hero_.Alive() && CheckCollisionRecs(col, hero_.Bounds())) {
                    const float dealt = hero_.TakeDamage(e.damage, false);
                    if (dealt >= 0.f) {
                        if (final_) deathLog_.other += dealt;
                        SpawnExplosion(hero_.Center(), Theme::NEON_YELLOW, Theme::NEON_RED, 16);
                        audio_.Play(AudioManager::Hit);
                    }
                }
                if (boss_.Alive() && CheckCollisionRecs(col, boss_.HurtBox())) {
                    boss_.TakeDamage(e.damage);
                    SpawnExplosion(boss_.Center(), Theme::NEON_CYAN, WHITE, 22);
                    hitStop_ = std::max(hitStop_, 0.08f);
                }
                break;
            }
        }
    }
}

Projectile Game::MakeHeroProjectile(const ShotCmd& s) {
    Projectile p;
    p.pos = s.origin; p.owner = Owner::Hero; p.kind = s.kind; p.homing = s.homing;
    p.maxSpeed = s.speed;
    p.vel = Vector2Scale(s.dir, s.homing ? s.speed * 0.55f : s.speed);
    p.radius = s.radius; p.damage = s.damage; p.life = s.life; p.id = nextId_++;
    return p;
}

Game::CEvent Game::PickCombatEvent() const {
    float wB = 3.f, wD = 2.f, wG = 2.f, wC = pickups_.Active() ? 0.f : (boss_.hp < boss_.maxHp * 0.7f ? 4.f : 1.5f);
    if (drones_.Count() > 0) wD = 0.f;
    if (lastCev_ == CEvent::Bombard) wB = 0.f;
    if (lastCev_ == CEvent::Drones)  wD = 0.f;
    if (lastCev_ == CEvent::LowGrav) wG = 0.f;
    if (lastCev_ == CEvent::Core)    wC = 0.f;
    float roll = Util::Rand01() * (wB + wD + wG + wC);
    if ((roll -= wB) < 0.f && wB > 0.f) return CEvent::Bombard;
    if ((roll -= wD) < 0.f && wD > 0.f) return CEvent::Drones;
    if ((roll -= wG) < 0.f && wG > 0.f) return CEvent::LowGrav;
    return wC > 0.f ? CEvent::Core : CEvent::Bombard;
}

void Game::StartCombatEvent(CEvent e, bool fromPhase) {
    lastCev_ = e;
    cevT_ = 0.f;
    audio_.Play(AudioManager::Analyze, 0.5f, 0.8f);
    glitch_ = std::max(glitch_, 0.35f);
    cevLabelT_ = 3.2f;
    eventTimer_ = Util::RandRange(17.f, 27.f);
    switch (e) {
        case CEvent::Bombard:
            cev_ = CEvent::Bombard;
            strikesLeft_ = fromPhase ? 6 : 4;
            strikeTimer_ = 0.4f;
            cevDur_ = (float)strikesLeft_ * 0.75f + 1.5f;
            cevLabel_ = "EVENTO DE COMBATE: BOMBARDEO - HUYE DE LAS ZONAS NARANJAS";
            break;
        case CEvent::Drones:
            drones_.Spawn(fromPhase ? 3 : 2, hero_.Center(), hero_.Traits().damage * 0.8f);
            cevLabel_ = "EVENTO DE COMBATE: REFUERZOS - DRONES DEL ANDROIDE";
            break;
        case CEvent::LowGrav:
            cev_ = CEvent::LowGrav;
            cevDur_ = 9.f;
            gravTarget_ = 0.6f;
            cevLabel_ = "EVENTO DE COMBATE: FLUJO DE GRAVEDAD REDUCIDA";
            break;
        case CEvent::Core:
            pickups_.SpawnCore(arena_);
            cevLabel_ = "EVENTO DE COMBATE: NUCLEO DE ENERGIA - RECOGELO";
            break;
        default: break;
    }
}

void Game::EndCombatEvent() {
    cev_ = CEvent::None;
    gravTarget_ = 1.f;
    eventTimer_ = Util::RandRange(17.f, 27.f);
}

void Game::UpdateCombatEvents(float dt) {
    eventTimer_ -= dt;
    if (cev_ == CEvent::None && eventTimer_ <= 0.f) StartCombatEvent(PickCombatEvent(), false);

    if (cev_ == CEvent::Bombard) {
        cevT_ += dt;
        strikeTimer_ -= dt;
        if (strikesLeft_ > 0 && strikeTimer_ <= 0.f) {
            strikeTimer_ = 0.75f;
            float x = (strikesLeft_ % 2 == 0) ? boss_.pos.x + Util::RandRange(-40.f, 40.f) : Util::RandRange(40.f, 600.f);
            x = Clamp(x, 30.f, (float)Cfg::VIEW_W - 30.f);
            hazards_.SpawnStrike(x, 38.f);
            --strikesLeft_;
            audio_.Play(AudioManager::Blip, 0.6f, 0.5f);
        }
        if (strikesLeft_ <= 0 && cevT_ > cevDur_) EndCombatEvent();
    } else if (cev_ == CEvent::LowGrav) {
        cevT_ += dt;
        if (GetRandomValue(0, 5) == 0)
            particles_.Directional({ Util::RandRange(10.f, 630.f), Cfg::GROUND_Y - 2.f }, { 0.f, -1.f }, 0.2f, 1,
                                   Theme::NEON_CYAN, 50.f, 2.f, 1.4f);
        if (cevT_ > cevDur_) EndCombatEvent();
    }
}

void Game::UpdateDrones(float dt) {
    std::vector<ShotCmd> shots;
    drones_.Update(dt, boss_.Center(), shots);
    for (const ShotCmd& s : shots) {
        projectiles_.push_back(MakeHeroProjectile(s));
        audio_.Play(AudioManager::HeroShot, 0.25f, 1.7f);
    }
    drones_.DestroyIn(boss_.HurtBox());
    for (const Vector2& d : drones_.TakeDeaths()) {
        SpawnExplosion(d, Theme::NEON_RED, Theme::NEON_YELLOW, 10);
        audio_.Play(AudioManager::Explosion, 0.35f, 1.3f);
    }
}

void Game::TriggerPhase(int phase) {
    phase_ = phase;
    hero_.PhaseShift();
    boss_.Knock(boss_.pos.x < hero_.pos.x ? -520.f : 520.f, -240.f);
    particles_.Ring(hero_.Center(), 36, Theme::NEON_RED, 320.f, 3.f, 0.7f);
    particles_.Ring(hero_.Center(), 24, WHITE, 200.f, 2.f, 0.5f);
    audio_.Play(AudioManager::Beam, 0.8f);
    shake_ = std::max(shake_, 10.f);
    glitch_ = std::max(glitch_, 0.8f);
    hitStop_ = std::max(hitStop_, 0.1f);
    phaseBannerT_ = 2.4f;
    phaseBannerNum_ = phase + 1;
    StartCombatEvent(phase == 1 ? CEvent::Drones : CEvent::Bombard, true);
}

void Game::OnHeroDefeated() {
    bullets_.Clear();
    deathLog_.absorbed = summons_.Absorbed();
    if (final_) { ++sessionSeraphDeaths_; sessionDodges_ += deathLog_.dodges; sessionAbsorbed_ += deathLog_.absorbed; }
    else        { ++sessionHeroDeaths_; }
    summons_.Clear();
    camPunch_ = 0.22f; flashT_ = 0.5f;                          // golpe de camara al caer
    subs_.Clear();                                              // final de la pelea: ultimas palabras
    if (final_) {
        if (seraphCycle_ >= Seraph::CYCLES) { subs_.SayAll(Story::SeraphFinalWords(), 0.3f); subs_.SayAll(Story::ProjectOutro(), 0.5f); }
        else { const DialogueLine w = Story::LastWord(true);  if (!w.text.empty()) subs_.Say(w.speaker, w.text, w.color, 0.2f, 2.0f); }
    } else {
        const DialogueLine w = Story::LastWord(false);
        if (!w.text.empty()) subs_.Say(w.speaker, w.text, w.color, 0.2f, 2.0f);
    }
    gravTarget_ = 1.f;
    cev_ = CEvent::None;
    state_ = State::HeroDying;
    stateTime_ = 0.f;
    slowmo_ = 0.3f;
    shake_ = 14.f;
    glitch_ = 1.f;
    SpawnExplosion(hero_.Center(), Theme::NEON_CYAN, Theme::NEON_MAGENTA, 70);
    audio_.Play(AudioManager::HeroDeath);
}

void Game::OnBossDefeated() {
    defeatFact_ = Story::FactText(Story::K_FACT_D);            // dato curioso en la pantalla de derrota
    camPunch_ = 0.3f; flashT_ = 0.7f;
    subs_.Clear();
    subs_.SayAll(Story::BossDownLines(final_), 1.4f);
    state_ = State::BossDown;
    stateTime_ = 0.f;
    slowmo_ = 0.25f;
    shake_ = 16.f;
    glitch_ = 1.f;
    SpawnExplosion(boss_.Center(), Theme::NEON_MAGENTA, Theme::NEON_YELLOW, 120);
    audio_.Play(AudioManager::BossDeath);
}

void Game::UpdateHeroDying(float dt) {
    stateTime_ += dt;
    const float sdt = dt * slowmo_;
    slowmo_ = Lerp(slowmo_, 1.f, dt * 0.8f);
    arena_.Update(sdt);
    boss_.Update(sdt, IdleBossInput(hero_.pos.x), arena_);
    UpdateProjectiles(sdt);
    UpdateHazards(sdt);
    particles_.Update(sdt);
    hud_.Update(dt, boss_, hero_);
    if (stateTime_ > 2.2f) FinishHeroDeath();
}

void Game::FinishHeroDeath() {
    lastTelemetry_ = tracker_.Snapshot();
    slowmo_ = 1.f;

    if (!final_ && brain_.Traits().generation >= Cfg::MAX_GENERATION) {
        finalTraits_ = brain_.Traits();
        subs_.Clear();
        seraphCycle_ = 1;
        state_ = State::FinalIntro;
        stateTime_ = 0.f;
        audio_.Play(AudioManager::Roar);
        glitch_ = 1.f; shake_ = 12.f;
        return;
    }
    if (final_ && seraphCycle_ < Seraph::CYCLES) {
        // El Serafin cae... y VUELVE enseguida a la pelea, ya con sus mejoras. Lo cuenta mientras pelea.
        rebirthFrom_ = seraphCycle_;
        const std::vector<DialogueLine> lines = Story::Rebirth(rebirthFrom_, deathLog_);
        const Story::ChoiceScene sc = Story::SeraphScene(rebirthFrom_, deathLog_);
        ++seraphCycle_;
        StartFinalRound();
        subs_.Clear();
        camPunch_ = 0.3f; flashT_ = 0.8f;
        seraphChain_ = lines;
        scene_ = sc; hasScene_ = true;
        choice_.Start(scene_.prompt, scene_.options);
        state_ = State::Choice;                                   // la pelea espera mientras eliges
        stateTime_ = 0.f;
        return;
    }
    if (final_ || brain_.Traits().generation >= Cfg::MAX_GENERATION) {
        final_ = false;
        bg_.SetEventsEnabled(true);
        state_ = State::Victory;
        stateTime_ = 0.f;
        if (!debugFinal_) brain_.Reset();                       // en debug no se borra tu progreso real
        audio_.Play(AudioManager::Victory);
        return;
    }

    report_ = brain_.Adapt(lastTelemetry_);
    brain_.Save();
    BeginReboot();
}

// Cinematica: el Heroe cae, se reconstruye con lo aprendido y vuelve hablando (sin pantalla de estadisticas).
void Game::BeginReboot() {
    hero_.ApplyTraits(brain_.Traits());
    ResetRound();                       // arena limpia; el Heroe ya tiene sus mejoras pero aun no se ve
    subs_.Clear();
    Story::RebirthScript sc = Story::HeroRebirthScript(report_, brain_.Traits());
    subs_.SayAll(sc.pre, 0.8f);
    scene_ = sc.scene; hasScene_ = sc.hasScene; rebootClosing_ = sc.closing;
    rebootStage_ = 0;
    dataTags_.clear(); tagTimer_ = 0.f;
    rebootK_ = 0.f;
    state_ = State::Reboot;
    stateTime_ = 0.f;
    audio_.Play(AudioManager::Analyze);
}

// Etiquetas con tus numeros reales de telemetria que vuelan hacia el Heroe mientras se reconstruye.
void Game::SpawnDataTag() {
    auto pct = [](float x) { return (int)(Clamp(x, 0.f, 1.f) * 100.f + 0.5f); };
    static const char* GEN[6] = { "SALTOS", "DASH", "ZONA", "ATAQUES", "TIEMPO EN AIRE", "DISPAROS" };
    std::string t; Color c = Theme::NEON_CYAN;
    const int r = GetRandomValue(0, 5);
    if (report_.enoughData && r == 0)      { t = Story::F("DISTANCIA %d%%", pct(report_.rangedRatio));  c = Theme::NEON_YELLOW; }
    else if (report_.enoughData && r == 1) { t = Story::F("CUERPO A CUERPO %d%%", pct(report_.meleeRatio)); c = Theme::NEON_RED; }
    else if (r == 2)                       { t = Story::F("PREDECIBLE %d%%", pct(report_.predictability)); c = Theme::NEON_GREEN; }
    else if (r == 3)                       { t = Story::F("ALTURA %d%%", pct(report_.mobility));        c = Theme::NEON_MAGENTA; }
    else                                   t = GEN[GetRandomValue(0, 5)];
    Vector2 from;
    switch (GetRandomValue(0, 3)) {
        case 0:  from = { Util::RandRange(30.f, 610.f), 40.f };  break;
        case 1:  from = { Util::RandRange(30.f, 610.f), 320.f }; break;
        case 2:  from = { 30.f,  Util::RandRange(50.f, 310.f) }; break;
        default: from = { 610.f, Util::RandRange(50.f, 310.f) }; break;
    }
    dataTags_.push_back({ from, 0.f, t, c });
}

void Game::UpdateReboot(float dt) {
    stateTime_ += dt;
    arena_.Update(dt);
    boss_.Update(dt, IdleBossInput(hero_.pos.x), arena_);
    particles_.Update(dt);
    hud_.Update(dt, boss_, hero_);

    const float prevK = rebootK_;
    rebootK_ = Util::Clamp01((stateTime_ - 2.0f) / 2.0f);          // se arma entre los 2 y los 4 s
    if (prevK <= 0.f && rebootK_ > 0.f) audio_.Play(AudioManager::Beam, 0.5f, 1.4f);
    if (rebootK_ > 0.f && rebootK_ < 1.f) shake_ = std::max(shake_, 1.5f);
    if (prevK < 1.f && rebootK_ >= 1.f) {                           // el Heroe vuelve
        const Vector2 c = hero_.Center();
        particles_.Ring(c, 36, Theme::NEON_CYAN, 260.f, 3.f, 0.8f);
        particles_.Burst(c, 24, WHITE, 220.f, 2.f, 0.6f);
        audio_.Play(AudioManager::HeroSpawn);
        glitch_ = 1.f; shake_ = 8.f; camPunch_ = 0.28f; flashT_ = 0.9f;
    }

    if (rebootStage_ == 0 && stateTime_ > 0.8f && rebootK_ < 1.f) {
        tagTimer_ -= dt;
        if (tagTimer_ <= 0.f) { tagTimer_ = 0.13f; SpawnDataTag(); }
    }
    for (auto& t : dataTags_) t.t += dt / 1.1f;
    for (const auto& t : dataTags_) if (t.t >= 1.f) particles_.Burst(hero_.Center(), 3, t.c, 90.f, 2.f, 0.3f);
    dataTags_.erase(std::remove_if(dataTags_.begin(), dataTags_.end(), [](const DataTag& t) { return t.t >= 1.f; }), dataTags_.end());

    if (rebootStage_ == 0) {                                        // 0: primeras lineas
        if (stateTime_ > 4.5f && !subs_.Busy()) {
            if (hasScene_) { choice_.Start(scene_.prompt, scene_.options); rebootStage_ = 1; }
            else { subs_.SayAll(rebootClosing_, 0.4f); rebootStage_ = 2; }
        }
    } else if (rebootStage_ == 1) {                                 // 1: el jugador elige que responderle a la IA
        choice_.Update(dt);
        if (choice_.ConsumeBlip()) audio_.Play(AudioManager::Blip, 0.12f, 1.9f);
        if (choice_.ConsumeMove()) audio_.Play(AudioManager::Blip, 0.25f, 1.5f);
        int idx = 0;
        if (choice_.Chosen(idx)) {
            ++sessionChoices_;
            choiceGood_ = scene_.good[(size_t)idx];
            if (choiceGood_) ++sessionInsights_;
            choiceFeedbackT_ = 2.6f;
            audio_.Play(choiceGood_ ? AudioManager::HeroSpawn : AudioManager::Blip, 0.35f, choiceGood_ ? 1.6f : 1.0f);
            subs_.Clear();
            subs_.SayAll(scene_.replies[(size_t)idx], 0.4f);
            subs_.SayAll(rebootClosing_, 0.7f);
            rebootStage_ = 2;
        }
    }

    if (rebootStage_ != 1 && IsKeyPressed(KEY_ENTER)) subs_.Skip();
    if (IsKeyPressed(KEY_TAB) || (rebootStage_ == 2 && !subs_.Busy() && stateTime_ > 6.f) || stateTime_ > 150.f) StartRound(true);
}

// El Serafin vuelve: antes de seguir peleando, el jugador elige que responderle.
void Game::UpdateChoice(float dt) {
    stateTime_ += dt;
    arena_.Update(dt);
    boss_.Update(dt, IdleBossInput(hero_.pos.x), arena_);
    particles_.Update(dt);
    hud_.Update(dt, boss_, hero_);
    if (GetRandomValue(0, 6) == 0)
        particles_.Burst(hero_.Center(), 2, Color{255, 220, 120, 255}, 80.f, 2.f, 0.7f);
    choice_.Update(dt);
    if (choice_.ConsumeBlip()) audio_.Play(AudioManager::Blip, 0.12f, 1.9f);
    if (choice_.ConsumeMove()) audio_.Play(AudioManager::Blip, 0.25f, 1.5f);
    int idx = 0;
    if (choice_.Chosen(idx)) {
        ++sessionChoices_;
        choiceGood_ = scene_.good[(size_t)idx];
        if (choiceGood_) ++sessionInsights_;
        choiceFeedbackT_ = 2.6f;
        audio_.Play(choiceGood_ ? AudioManager::HeroSpawn : AudioManager::Blip, 0.35f, choiceGood_ ? 1.6f : 1.0f);
        subs_.Clear();
        subs_.SayAll(scene_.replies[(size_t)idx], 0.5f);          // la IA y la guia contestan...
        subs_.SayAll(seraphChain_, 0.8f);                         // ...y luego cuenta lo que mejoro
        state_ = State::Playing;
        stateTime_ = 0.f;
    }
}

void Game::DrawRebuildFx() const {
    const float k = rebootK_;
    const Vector2 c = hero_.Center();
    const Color cy = Theme::NEON_CYAN;
    hero_.DrawEcho(sprites_, time_, { hero_.pos.x, hero_.pos.y }, 0.15f + 0.75f * k, false, cy);
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 48; ++i) {                                  // fragmentos que convergen
        const float ang = (float)i * 2.39996f + time_ * 0.6f;
        const float r0 = 30.f + (float)((i * 37) % 110);
        const float r = r0 * (1.f - k) * (1.f - k);
        DrawRectangle((int)(c.x + std::cos(ang) * r) - 1, (int)(c.y + std::sin(ang) * r) - 1, 3, 3, Fade(cy, 0.4f + 0.6f * k));
    }
    DrawRectangle((int)(hero_.pos.x - 34.f), (int)(hero_.pos.y - 64.f * k), 68, 2, Fade(WHITE, 0.8f - 0.4f * k));   // barrido de escaneo
    EndBlendMode();
}

void Game::UpdateFinalIntro(float dt) {
    stateTime_ += dt;
    arena_.Update(dt);
    boss_.Update(dt, IdleBossInput(boss_.pos.x + 100.f), arena_);
    boss_.hp = Lerp(boss_.hp, boss_.maxHp, 1.f - std::exp(-dt * 2.5f));
    particles_.Update(dt);
    hud_.Update(dt, boss_, hero_);
    shake_ = std::max(shake_, stateTime_ < 3.f ? 2.5f : 0.f);
    if (stateTime_ > 1.0f && GetRandomValue(0, 3) == 0)
        particles_.Burst({ Util::RandRange(60.f, 580.f), Util::RandRange(60.f, 290.f) }, 4, Color{255, 220, 120, 255}, 160.f, 2.f, 0.9f);
    if (stateTime_ > 3.6f && stateTime_ < 3.7f) { glitch_ = 1.f; audio_.Play(AudioManager::Beam, 0.8f); }
    if (stateTime_ > 4.6f) {
        StartFinalRound();
        subs_.Clear();
        subs_.SayAll(Story::SeraphIntro(), 2.8f);              // inicio de la fase final
    }
}

// ----------------------------------------------------------------------------------------------
//  Efectos extra. Todos escalan con F10 (EFECTOS DE PANTALLA); en "APAGADOS" no se muestra ninguno.
// ----------------------------------------------------------------------------------------------
void Game::UpdateEffects(float dt) {
    const float fx = FX_VALUES[fxLevel_];
    const bool scene = state_ == State::Playing || state_ == State::HeroDying || state_ == State::Choice ||
                       state_ == State::Reboot || state_ == State::FinalIntro;
    const bool live = state_ == State::Playing && introTimer_ <= 0.f;

    // Brasas: motas de energia que suben lentamente por la arena (con el color del tema de la generacion).
    if (fx > 0.f && scene) {
        embersT_ -= dt;
        if (embersT_ <= 0.f) {
            embersT_ = 0.30f / (0.5f + fx);
            particles_.Directional({ Util::RandRange(10.f, (float)Cfg::VIEW_W - 10.f), Cfg::GROUND_Y - Util::RandRange(0.f, 90.f) },
                                   { 0.15f, -1.f }, 0.5f, 1, Fade(bg_.GridColor(), 0.9f), 18.f, 2.f, 3.4f);
        }
    }

    // Peligro: la vida de NEMESIS-X esta baja (pulso rojo en los bordes).
    const float bf = boss_.hp / std::max(1.f, boss_.maxHp);
    const float dangerTarget = (live && bf < 0.3f) ? (1.f - bf / 0.3f) : 0.f;
    dangerAmt_ += (dangerTarget - dangerAmt_) * std::min(1.f, dt * 4.f);

    // Impactos: anillo y destello cuando alguien recibe dano.
    if (live) {
        const float lost = lastBossHp_ - boss_.hp;
        if (lastBossHp_ > 0.f && lost > 2.f) {
            particles_.Ring(boss_.Center(), 16, Theme::NEON_RED, 200.f, 2.f, 0.35f);
            dmgEdgeT_ = std::max(dmgEdgeT_, Util::Clamp01(lost / 50.f));
        }
        const float hl = lastHeroHp_ - hero_.hp;
        if (lastHeroHp_ > 0.f && hl > 1.f && hero_.Alive())
            particles_.Ring(hero_.Center(), 10, Color{200, 255, 255, 255}, 150.f, 2.f, 0.28f);
    }
    lastBossHp_ = boss_.hp;
    lastHeroHp_ = hero_.hp;
    dmgEdgeT_ = std::max(0.f, dmgEdgeT_ - dt * 2.5f);

    // Lineas de velocidad mientras dura el dash.
    dashLinesT_ = (live && boss_.DashActive()) ? 1.f : std::max(0.f, dashLinesT_ - dt * 5.f);

    // Aura del Serafin: plumas doradas que caen y una estela de imagenes residuales.
    if (final_ && scene && state_ != State::HeroDying && hero_.Alive()) {
        featherT_ -= dt;
        if (fx > 0.f && featherT_ <= 0.f) {
            featherT_ = 0.09f;
            const Vector2 c = hero_.Center();
            particles_.Directional({ c.x + Util::RandRange(-16.f, 16.f), c.y + Util::RandRange(-10.f, 14.f) }, { 0.2f, 1.f }, 1.0f, 1,
                                   Color{255, 225, 120, 255}, 26.f, 2.5f, 1.3f);
        }
        trailTimer_ -= dt;
        if (trailTimer_ <= 0.f) {
            trailTimer_ = 0.045f;
            trailHead_ = (trailHead_ + 1) % 6;
            trailPos_[trailHead_] = { hero_.pos.x, hero_.pos.y };
            trailFace_[trailHead_] = hero_.facing >= 0;
        }
    } else if (!final_) {
        for (int i = 0; i < 6; ++i) trailPos_[i] = { 0.f, 0.f };
    }
}

void Game::DrawSeraphTrail() const {
    if (FX_VALUES[fxLevel_] <= 0.f || !final_ || !hero_.Alive()) return;
    if (state_ != State::Playing && state_ != State::Choice) return;
    for (int k = 1; k < 6; ++k) {
        const int idx = (trailHead_ + 6 - k) % 6;
        const Vector2 p = trailPos_[idx];
        if (p.x == 0.f && p.y == 0.f) continue;
        if (std::fabs(p.x - hero_.pos.x) + std::fabs(p.y - hero_.pos.y) < 8.f) continue;       // quieto: sin estela
        hero_.DrawEcho(sprites_, time_, p, 0.30f * (1.f - (float)k / 6.f) * FX_VALUES[fxLevel_], trailFace_[idx], Color{255, 225, 120, 255});
    }
}

// Columna de luz sobre el Heroe / Serafin mientras aparece (durante el cartel de inicio de ronda).
void Game::DrawSpawnBeam() const {
    const float fx = FX_VALUES[fxLevel_];
    if (fx <= 0.f || state_ != State::Playing || introTimer_ <= 0.f || !hero_.Alive()) return;
    const float k = Util::Clamp01(introTimer_ / 2.4f);
    const float a = k * k * 0.6f * fx;
    const Color cc = final_ ? Color{255, 225, 120, 255} : Theme::NEON_CYAN;
    const float x = hero_.pos.x, y = hero_.pos.y;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleGradientH((int)(x - 36.f), 0, 36, (int)y, Fade(cc, 0.f), Fade(cc, a));
    DrawRectangleGradientH((int)x, 0, 36, (int)y, Fade(cc, a), Fade(cc, 0.f));
    DrawCircleGradient((int)x, (int)y, 60.f * k + 14.f, Fade(cc, a), Fade(cc, 0.f));
    EndBlendMode();
}

// Efectos de pantalla: peligro, dano recibido, sobrecarga y lineas de velocidad.
void Game::DrawScreenEffects() const {
    const float fx = FX_VALUES[fxLevel_];
    if (fx <= 0.f) return;
    const int W = Cfg::VIEW_W, H = Cfg::VIEW_H;
    auto edges = [&](Color c, float a) {
        const Color e = Fade(c, std::min(1.f, a)), z = Fade(c, 0.f);
        DrawRectangleGradientV(0, 0, W, 70, e, z);
        DrawRectangleGradientV(0, H - 70, W, 70, z, e);
        DrawRectangleGradientH(0, 0, 90, H, e, z);
        DrawRectangleGradientH(W - 90, 0, 90, H, z, e);
    };
    if (dangerAmt_ > 0.01f)
        edges(Theme::NEON_RED, (0.10f + 0.20f * (0.5f + 0.5f * std::sin(time_ * 7.f))) * dangerAmt_ * (0.4f + 0.6f * fx));
    if (dmgEdgeT_ > 0.f) edges(Theme::NEON_RED, dmgEdgeT_ * 0.5f * fx);
    if (boss_.IsOvercharged() && state_ == State::Playing)
        edges(Color{255, 220, 90, 255}, (0.08f + 0.05f * std::sin(time_ * 9.f)) * fx);
    if (dashLinesT_ > 0.f) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = 0; i < 18; ++i) {
            const float ang = (float)i * 0.349f + std::sin((float)i * 3.1f) * 0.12f;
            const float r1 = 150.f + (float)((i * 37) % 70), r2 = r1 + 50.f + 60.f * dashLinesT_;
            DrawLineEx({ W * 0.5f + std::cos(ang) * r1, H * 0.5f + std::sin(ang) * r1 },
                       { W * 0.5f + std::cos(ang) * r2, H * 0.5f + std::sin(ang) * r2 }, 2.f, Fade(WHITE, 0.22f * dashLinesT_ * fx));
        }
        EndBlendMode();
    }
}

// Camara cinematografica: zoom suave hacia el punto de interes en las cinematicas.
void Game::UpdateCamera(float dt) {
    const Vector2 mid = { Cfg::VIEW_W * 0.5f, Cfg::VIEW_H * 0.5f };
    float zt = 1.f; Vector2 ft = mid;
    switch (state_) {
        case State::Reboot: {
            const Vector2 c = hero_.Center();
            if (rebootStage_ == 1)      { zt = 1.10f; ft = { Lerp(mid.x, c.x, 0.3f), Lerp(mid.y, c.y, 0.3f) }; }
            else if (rebootK_ < 1.f && stateTime_ > 1.f) {
                const float k = Util::Clamp01((stateTime_ - 1.f) / 3.f);          // empuja hacia donde se arma el Heroe
                zt = 1.f + 0.6f * k; ft = { Lerp(mid.x, c.x, k), Lerp(mid.y, c.y, k) };
            } else zt = 1.04f;
            break;
        }
        case State::FinalIntro: {
            const float k = Util::Clamp01(stateTime_ / 4.4f);                      // lento empuje hacia el Boss
            zt = 1.f + 0.20f * k; ft = { Lerp(mid.x, boss_.Center().x, 0.35f * k), mid.y };
            break;
        }
        case State::HeroDying: {
            zt = 1.f + 0.20f * Util::Clamp01(1.f - slowmo_);                       // acercamiento durante la camara lenta
            ft = hero_.Center();
            break;
        }
        case State::BossDown: if (stateTime_ > 0.2f) { zt = 1.12f; ft = boss_.Center(); } break;
        default: break;
    }
    camZoomTarget_ = zt; camFocusTarget_ = ft;
    const float k = 1.f - std::exp(-dt * 4.f);
    camZoom_ += (camZoomTarget_ - camZoom_) * k;
    camFocus_.x += (camFocusTarget_.x - camFocus_.x) * k;
    camFocus_.y += (camFocusTarget_.y - camFocus_.y) * k;
}

Vector2 Game::CamFocus(float zoom) const {
    const float hw = Cfg::VIEW_W * 0.5f / zoom, hh = Cfg::VIEW_H * 0.5f / zoom;      // no mostrar nunca fuera del mundo
    return { Clamp(camFocus_.x, hw, Cfg::VIEW_W - hw), Clamp(camFocus_.y, hh, Cfg::VIEW_H - hh) };
}

Vector2 Game::WorldToScreen(Vector2 p) const {
    const float z = std::max(1.f, camZoom_ + camPunch_);
    const Vector2 f = CamFocus(z);
    return { (p.x - f.x) * z + Cfg::VIEW_W * 0.5f, (p.y - f.y) * z + Cfg::VIEW_H * 0.5f };
}

void Game::TickShards(float dt) {
    const Vector2 m = MouseToView();
    shards_.SetTint(Util::LerpColor(Color{150, 105, 225, 255}, bg_.GridColor(), 0.35f));
    shards_.Update(dt, { Clamp((m.x / Cfg::VIEW_W - 0.5f) * 2.f, -1.f, 1.f),
                         Clamp((m.y / Cfg::VIEW_H - 0.5f) * 2.f, -1.f, 1.f) });
}

void Game::ResetSession() {
    sessionHeroDeaths_ = sessionSeraphDeaths_ = sessionDodges_ = sessionAbsorbed_ = 0;
    sessionChoices_ = sessionInsights_ = 0;
    finalTraits_ = HeroTraits{};
    report_ = AdaptationReport{};
    victorySummary_ = false;
}

// Resumen del final de la sesion: lo que la IA aprendio del visitante (datos reales de su partida).
void Game::DrawVictorySummary(int cx) const {
    DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(BLACK, 0.86f));
    HUD::TextCentered("LO QUE LA IA APRENDIO DE TI", cx, 14, 20, Theme::NEON_CYAN);

    Story::SessionSummary m;
    m.hasData = report_.enoughData;
    m.ranged = report_.rangedRatio; m.melee = report_.meleeRatio;
    m.predict = report_.predictability; m.mobility = report_.mobility;
    m.shield = finalTraits_.deflectShield; m.dash = finalTraits_.dashEvasive;
    m.lead = finalTraits_.leadShooting;    m.jets = finalTraits_.jumpJets;
    m.heroDeaths = sessionHeroDeaths_; m.seraphDeaths = sessionSeraphDeaths_;
    m.dodges = sessionDodges_; m.absorbed = sessionAbsorbed_;
    m.choices = sessionChoices_; m.insights = sessionInsights_;

    int y = 46;
    for (const Story::SummaryRow& r : Story::SummaryLines(m)) {
        if (r.kind == 0) y += 4;
        const Color c = r.kind == 0 ? Theme::NEON_YELLOW : (r.kind == 2 ? Theme::NEON_GREEN : WHITE);
        for (const std::string& row : HUD::Wrap(r.text, Cfg::VIEW_W - 100, 10)) {
            HUD::Text(row.c_str(), 50, y, 10, c);
            y += 13;
        }
    }
    HUD::TextCentered("Gracias por jugar! En ARCHIVO (menu principal) puedes ver como funciona por dentro.", cx, Cfg::VIEW_H - 32, 10, Theme::NEON_CYAN);
    if (stateTime_ > 1.f && std::fmod(time_, 1.f) < 0.65f)
        HUD::TextCentered("[ENTER] VOLVER AL MENU", cx, Cfg::VIEW_H - 16, 10, WHITE);
}

void Game::UpdatePrologue(float dt) {
    stateTime_ += dt;
    TickShards(dt);
    dlg_.Update(dt);
    if (dlg_.ConsumeBlip()) audio_.Play(AudioManager::Blip, 0.12f, 1.9f);
    const bool next = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (IsKeyPressed(KEY_TAB) || (next && stateTime_ > 0.4f && dlg_.Advance())) StartRound(false);
}

void Game::UpdateCodex(float dt) {
    stateTime_ += dt;
    TickShards(dt);
    const int n = Story::CODEX_PAGES;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { codexPage_ = (codexPage_ + 1) % n; stateTime_ = 0.f; audio_.Play(AudioManager::Blip, 0.3f, 1.5f); }
    if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) { codexPage_ = (codexPage_ + n - 1) % n; stateTime_ = 0.f; audio_.Play(AudioManager::Blip, 0.3f, 1.5f); }
    if (IsKeyPressed(KEY_ENTER) && stateTime_ > 0.2f) { state_ = State::Title; stateTime_ = 0.f; }
}

bool Game::SayEvent(Story::Ev ev) {
    if (subs_.Busy() || eventCd_ > 0.f) return false;        // si ya hay un dialogo, se intentara en el proximo evento
    const std::vector<DialogueLine> lines = Story::EventLines(ev, final_);
    if (lines.empty()) return false;                          // ya se dijeron todas las variantes: nada se repite
    eventCd_ = 18.f;
    subs_.SayAll(lines, 0.4f);
    return true;
}

// Dialogos a mitad de pelea: al bajar la vida y, si hace rato que nadie habla, un consejo nuevo de la guia.
void Game::UpdateDialogueTriggers(float dt) {
    fightT_ += dt;
    eventCd_ = std::max(0.f, eventCd_ - dt);
    const float frac = hero_.hp / std::max(1.f, hero_.maxHp);
    if (!subs_.Busy()) {
        if (!halfSaid_ && frac < 0.5f)           { halfSaid_ = true; subs_.SayAll(Story::EventLines(Story::Ev::HalfHp, final_), 0.3f); return; }
        if (final_ && !lowSaid_ && frac < 0.22f) { lowSaid_ = true;  subs_.SayAll(Story::EventLines(Story::Ev::LowHp, true), 0.3f);   return; }
        idleT_ += dt;
        if (idleT_ > 24.f && fightT_ > 14.f) {
            idleT_ = 0.f;
            // Alterna: consejo sobre IA y nota sobre por que existe el proyecto (si una se agota, usa la otra).
            std::vector<DialogueLine> note;
            const int turn = noteTurn_++ % 3;
            if (turn == 1) note = Story::ProjectNote();
            else if (turn == 2) note = Story::FunFactLines();
            if (note.empty()) { const DialogueLine g = Story::GuideTip(); if (!g.text.empty()) note.push_back(g); }
            if (note.empty()) note = Story::FunFactLines();
            if (note.empty()) note = Story::ProjectNote();
            if (!note.empty()) subs_.SayAll(note, 0.f);
        }
    } else {
        idleT_ = 0.f;
    }
}

// Debug (F8): salta directo a la fase final sin jugar las 6 generaciones.
void Game::DebugJumpToFinal() {
    debugFinal_ = true;
    finalTraits_ = brain_.Traits();
    seraphCycle_ = 1;
    paused_ = false;
    StartFinalRound();
    subs_.Clear();
    subs_.SayAll(Story::SeraphIntro(), 2.8f);
}

void Game::StartFinalRound() {
    const Seraph::Stats st = Seraph::ForCycle(seraphCycle_);
    hero_.ApplyTraits(brain_.Traits());
    ResetRound();
    hero_.EnterFinalForm(st);
    boss_.SetScale(Cfg::BossScaleForGeneration(Cfg::MAX_GENERATION), true);
    final_ = true;
    director_.Start(st);
    particles_.Ring(hero_.Center(), 30, Color{255, 225, 120, 255}, 240.f, 3.f, 0.8f);      // aparicion del Serafin
    summons_.Configure(st);
    deathLog_ = Story::DeathReport{};
    finalCoreT_ = 0.f;
    halfSaid_ = lowSaid_ = false; fightT_ = idleT_ = 0.f;
    bg_.SetTheme(Cfg::MAX_GENERATION);
    bg_.SetEventsEnabled(false);
    state_ = State::Playing;
    stateTime_ = 0.f;
    introTimer_ = 2.4f;
    introShrink_ = false;
    hintTimer_ = 0.f;
    audio_.Play(AudioManager::HeroSpawn, 0.8f, 0.6f);
    particles_.Ring({ hero_.pos.x, hero_.pos.y - 32.f }, 40, Color{255, 220, 120, 255}, 240.f, 3.f, 0.9f);
    glitch_ = 1.f;
}

void Game::HandleSummonEvents() {
    const Color gold{255, 225, 120, 255};
    for (const SummonEvent& e : summons_.TakeEvents()) {
        switch (e.kind) {
            case SummonEvent::DecoySpawn:
                SayEvent(Story::Ev::Decoy);
                particles_.Ring(e.pos, 16, gold, 180.f, 2.f, 0.4f);
                particles_.Burst(e.pos, 8, WHITE, 120.f, 2.f, 0.3f);
                audio_.Play(AudioManager::HeroSpawn, 0.35f, 1.6f);
                break;
            case SummonEvent::DecoyHit:
                particles_.Burst(e.pos, 8, gold, 160.f, 2.f, 0.3f);
                audio_.Play(AudioManager::ShieldHit, 0.6f, 1.3f);
                break;
            case SummonEvent::DecoyBreak:
                SpawnExplosion(e.pos, gold, WHITE, 14);
                audio_.Play(AudioManager::ShieldHit, 0.8f, 0.8f);
                break;
            case SummonEvent::GuardSpawn:
                SayEvent(Story::Ev::Orbs);
                particles_.Ring(e.pos, 14, gold, 160.f, 2.f, 0.4f);
                audio_.Play(AudioManager::Blip, 0.4f, 1.5f);
                break;
            case SummonEvent::GuardBreak:
                particles_.Burst(e.pos, 10, gold, 170.f, 2.f, 0.3f);
                audio_.Play(AudioManager::ShieldHit, 0.5f, 1.6f);
                break;
        }
    }
}

void Game::UpdateFinalPhase(float dt) {
    director_.Update(dt, hero_, boss_, bullets_, hazards_);
    bullets_.Update(dt);
    summons_.Update(dt, hero_, boss_, projectiles_);
    HandleSummonEvents();

    // Sin fases por vida: el Boss recibe un nucleo de energia cada ~30 s para sostener la pelea.
    finalCoreT_ += dt;
    if (finalCoreT_ > 30.f && !pickups_.Active()) {
        finalCoreT_ = 0.f;
        pickups_.SpawnCore(arena_);
        cevLabel_ = "NUCLEO DE ENERGIA: RECOGELO (CURA + SOBRECARGA)"; cevLabelT_ = 3.2f;
    }

    const BulletHit h = bullets_.Collide(boss_.HurtBox(), !boss_.IsInvulnerable());
    if (h.hit) {
        boss_.TakeDamage(h.damage);
        boss_.GrantInvuln(Cfg::BOSS_HIT_INVULN);
        particles_.Burst(h.pos, 10, Theme::NEON_RED, 180.f, 3.f, 0.4f);
        particles_.Burst(boss_.Center(), 6, WHITE, 140.f, 2.f, 0.3f);
        audio_.Play(AudioManager::Hit, 0.7f, 0.8f);
        shake_ = std::max(shake_, 4.f);
        glitch_ = std::max(glitch_, 0.5f);
        hitStop_ = std::max(hitStop_, 0.05f);
    }

    if (bullets_.ConsumeCharged() > 0) audio_.Play(AudioManager::Charge, 0.45f);
    if (bullets_.ConsumeFired() > 0) { audio_.Play(AudioManager::Beam, 0.6f); shake_ = std::max(shake_, 4.f); }
    pewT_ -= dt;
    if (director_.ConsumeBursts() > 0 && pewT_ <= 0.f) {
        audio_.Play(AudioManager::Pew, 0.22f, Util::RandRange(0.9f, 1.3f));
        pewT_ = 0.12f;
    }
}

void Game::UpdateBossDown(float dt) {
    stateTime_ += dt;
    const float sdt = dt * slowmo_;
    slowmo_ = Lerp(slowmo_, 1.f, dt * 0.8f);
    arena_.Update(sdt);
    particles_.Update(sdt);
    hero_.Update(sdt, boss_, projectiles_, arena_);
    hero_.TakeShots();
    projectiles_.clear();

    if (stateTime_ > 2.5f) {
        if (IsKeyPressed(KEY_ENTER)) { if (final_) { StartFinalRound(); subs_.Clear(); } else StartRound(false); }
        if (IsKeyPressed(KEY_F9)) { brain_.Reset(); StartRound(false); }
    }
}

void Game::UpdateVictory(float dt) {
    stateTime_ += dt;
    arena_.Update(dt);
    particles_.Update(dt);
    if (stateTime_ > 0.2f && std::fmod(stateTime_, 0.35f) < dt)
        particles_.Burst({ Util::RandRange(80.f, 560.f), Util::RandRange(40.f, 200.f) },
                         30, (GetRandomValue(0, 1) ? Theme::NEON_CYAN : Theme::NEON_MAGENTA), 180.f, 3.f, 1.f);
    if (stateTime_ > (victorySummary_ ? 1.f : 2.f) && IsKeyPressed(KEY_ENTER)) {
        if (!victorySummary_) { victorySummary_ = true; stateTime_ = 0.f; return; }    // 1er ENTER: resumen de lo que aprendio la IA
        victorySummary_ = false;
        seraphCycle_ = 1;
        debugFinal_ = false;
        subs_.Clear();
        Story::ResetVariety();                                  // nueva partida: los dialogos pueden volver a salir
        eventCd_ = 0.f;
        hero_.ApplyTraits(brain_.Traits());
        boss_.SetScale(1.f, true);
        bg_.SetTheme(1);
        ResetRound();
        state_ = State::Title;
        stateTime_ = 0.f;
        brainLoaded_ = false;
    }
}

void Game::DrawCenteredBanner(const char* big, const char* small, Color c) const {
    HUD::TextCentered(big, Cfg::VIEW_W / 2, 120, 40, c);
    if (small) HUD::TextCentered(small, Cfg::VIEW_W / 2, 170, 10, WHITE);
}

void Game::DrawWorld() {
    if (state_ == State::Title || state_ == State::Prologue || state_ == State::Codex) { shards_.Draw(); return; }

    Camera2D cam{};
    cam.zoom = std::max(1.f, camZoom_ + camPunch_);
    const Vector2 foc = CamFocus(cam.zoom);
    cam.offset = { Cfg::VIEW_W * 0.5f, Cfg::VIEW_H * 0.5f };
    cam.target = { foc.x + shakeOff_.x, foc.y + shakeOff_.y };
    BeginMode2D(cam);

    bg_.Draw(time_);
    arena_.Draw(time_);
    pickups_.Draw(time_);

    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& s : shockwaves_) {
        const float k = s.t / 0.4f;
        const float reach = (boss_.w * 0.5f + boss_.MeleeReach()) * std::min(1.f, k * 2.5f);
        const int a = (int)(s.origin.x), y = (int)s.origin.y;
        const Color c = Fade(Theme::NEON_MAGENTA, 1.f - k);
        for (int i = 0; i < 6; ++i)
            DrawLine(a - (int)reach, y - i * 4, a + (int)reach, y - i * 4, Fade(c, 0.5f - i * 0.07f));
        DrawCircleLines(a, y, reach, Fade(Theme::NEON_CYAN, 1.f - k));
    }
    EndBlendMode();

    hazards_.Draw(time_);
    drones_.Draw(time_);

    const bool bossVisible = !(state_ == State::BossDown && stateTime_ > 0.9f);
    if (bossVisible) boss_.Draw(sprites_, time_);
    const bool rebuilding = (state_ == State::Reboot && rebootK_ < 1.f);
    if (hero_.Alive() && state_ != State::Victory && !rebuilding) hero_.Draw(sprites_, time_);
    if (rebuilding && rebootK_ > 0.f) DrawRebuildFx();
    DrawSeraphTrail();
    DrawSpawnBeam();
    summons_.Draw(hero_, sprites_, time_);

    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& p : projectiles_) p.Draw(time_);
    particles_.Draw();
    EndBlendMode();
    bullets_.Draw(sprites_, time_);

    bg_.DrawFront(time_);
    const float lg = Clamp((1.f - Entity::worldGravity) / 0.4f, 0.f, 1.f);
    if (lg > 0.02f) {
        DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(Theme::NEON_CYAN, 0.05f * lg));
    }
    EndMode2D();
}

void Game::DrawOverlay() {
    const int cx = Cfg::VIEW_W / 2;
    const Telemetry tel = tracker_.Snapshot();
    const float hint = std::max(0.f, 1.f - std::max(0.f, hintTimer_ - 5.f) / 2.f);

    if (state_ == State::Playing || state_ == State::HeroDying || state_ == State::BossDown ||
        state_ == State::Reboot || state_ == State::FinalIntro || state_ == State::Choice) {
        const Color vc = Fade(BLACK, 0.42f), vz = Fade(BLACK, 0.f);                  // vineta: da profundidad y mejora la lectura del HUD
        DrawRectangleGradientV(0, 0, Cfg::VIEW_W, 48, vc, vz);
        DrawRectangleGradientV(0, Cfg::VIEW_H - 48, Cfg::VIEW_W, 48, vz, vc);
        DrawRectangleGradientH(0, 0, 64, Cfg::VIEW_H, vc, vz);
        DrawRectangleGradientH(Cfg::VIEW_W - 64, 0, 64, Cfg::VIEW_H, vz, vc);
        DrawScreenEffects();                                                         // peligro, impactos, sobrecarga, dash
    }

    float evAlpha = 0.f;
    const char* evText = bg_.EventLabel(evAlpha);
    if (evAlpha > 0.01f && state_ != State::Reboot && state_ != State::Title && !subs_.Busy())
        HUD::TextCentered(evText, cx, 50, 10, Fade(Theme::NEON_YELLOW, evAlpha));        // debajo de las barras, no encima

    if (cevLabelT_ > 0.f && state_ == State::Playing && !subs_.Busy())      // mientras se habla, la pantalla se mantiene limpia
        HUD::TextCentered(cevLabel_, cx, 62, 10, Fade(Theme::NEON_RED, std::min(1.f, cevLabelT_ / 0.5f)));
    if (phaseBannerT_ > 0.f && state_ == State::Playing) {
        const float a = std::min(1.f, phaseBannerT_ / 0.5f);
        HUD::TextCentered(TextFormat("FASE %d / 3", phaseBannerNum_), cx, 84, 20, Fade(Theme::NEON_RED, a));
        HUD::TextCentered("EL ANDROIDE ENTRA EN SOBRECARGA", cx, 108, 10, Fade(WHITE, a));
    }
    if (fxMsgT_ > 0.f)
        HUD::TextCentered(TextFormat("EFECTOS DE PANTALLA: %s  (F10)", FX_NAMES[fxLevel_]), cx, Cfg::VIEW_H - 28, 10,
                          Fade(Theme::NEON_CYAN, std::min(1.f, fxMsgT_)));

    hud_.SetDetail(hudDetail_);
    hud_.SetFinal(final_, director_.PatternName(), director_.IsFlyingMode());
    {
        const Seraph::Stats st = Seraph::ForCycle(seraphCycle_);
        hud_.SetSeraph(seraphCycle_, Seraph::CYCLES, st.attack, st.dodge, summons_.DecoyReady(), summons_.GuardReady());
    }
    if (dodgePopT_ > 0.f && state_ == State::Playing) {
        const float k = 1.f - dodgePopT_ / 0.8f;
        HUD::TextCentered("ESQUIVA!", (int)dodgePopPos_.x, (int)(dodgePopPos_.y - 46.f - 22.f * k), 10,
                          Fade(Color{255, 225, 120, 255}, std::min(1.f, dodgePopT_ / 0.3f)));
    }
    switch (state_) {
        case State::Title: {
            {   // ganchos educativos que rotan + controles siempre visibles
                const float hk = std::fmod(time_, 5.f);
                const float ha = std::min(1.f, std::min(hk / 0.4f, (5.f - hk) / 0.4f));
                HUD::TextCentered(Story::TitleHook((int)(time_ / 5.f)), cx, Cfg::VIEW_H - 30, 10, Fade(Theme::NEON_CYAN, std::max(0.f, ha)));
                HUD::TextCentered("A/D MOVER   W SALTAR   SHIFT DASH   J APLASTAR   K/CLIC DISPARAR   H DATOS DE LA IA", cx, Cfg::VIEW_H - 20, 10, Fade(WHITE, 0.6f));
                HUD::Text("DEBUG:  F8 = ir al Serafin    F7 = matar al Heroe / Serafin", Cfg::VIEW_W - 290, 6, 10, Fade(WHITE, 0.28f));
            }
            MenuInfo info;
            info.fxName = FX_NAMES[fxLevel_];
            info.musicOn = audio_.MusicOn();
            info.hasMemory = brainLoaded_ || brain_.HasLearned();
            info.traits = brain_.Traits();
            info.bossScale = boss_.Scale();
            menu_.Draw(time_, info, sprites_, Util::LerpColor(Color{150, 105, 225, 255}, bg_.GridColor(), 0.35f));
            break;
        }

        case State::Playing:
            hud_.DrawBattle(boss_, hero_, tel, hint);
            if (introTimer_ > 0.f) {
                if (final_) {
                    const char* head = seraphCycle_ == 1 ? "FASE FINAL" : "EL SERAFIN HA VUELTO";
                    const char* sub = introTimer_ <= 0.9f ? "ESQUIVA!"
                                    : (seraphCycle_ == 1 ? "HEREDA TODO LO QUE APRENDIERON LOS HEROES..."
                                                         : "RECUERDA TODO LO QUE APRENDIO AL MORIR...");
                    DrawCenteredBanner(head, sub, Color{255, 225, 120, 255});
                } else {
                const char* sub = introTimer_ > 0.9f ? "EL ANDROIDE HA SIDO RECONSTRUIDO..." : "COMBATE!";
                DrawCenteredBanner(TextFormat("GENERACION %02d", brain_.Traits().generation), sub,
                                   introTimer_ > 0.9f ? Theme::NEON_CYAN : Theme::NEON_RED);
                }
                if (introShrink_)
                    HUD::TextCentered(TextFormat("NEMESIS-X SE COMPACTA: TAMANO %d%%",
                                      (int)(Cfg::BossScaleForGeneration(brain_.Traits().generation) * 100.f + 0.5f)),
                                      Cfg::VIEW_W / 2, 186, 10, Theme::NEON_MAGENTA);
            }
            if (paused_) {
                DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(BLACK, 0.72f));
                const int px = 90, pw = Cfg::VIEW_W - 180, py = 30, ph = 300;
                DrawRectangle(px, py, pw, ph, Fade(Color{8, 6, 22, 255}, 0.92f));
                DrawRectangleLines(px, py, pw, ph, Fade(Theme::NEON_CYAN, 0.7f));
                HUD::TextCentered("PAUSA", cx, py + 12, 30, WHITE);
                DrawRectangle(px + 16, py + 52, pw - 32, 1, Fade(Theme::NEON_CYAN, 0.4f));
                const char* ctl[6] = { "A / D  ...  MOVERTE", "W / ESPACIO  ...  SALTAR (doble salto en el aire)", "SHIFT  ...  DASH",
                                       "J  ...  APLASTAR (en el aire: PICADO)", "K / CLIC  ...  DISPARAR", "H  ...  DATOS DE LA IA   |   M  ...  MUSICA" };
                for (int i = 0; i < 6; ++i) HUD::Text(ctl[i], px + 28, py + 62 + i * 14, 10, Fade(WHITE, 0.85f));
                DrawRectangle(px + 16, py + 154, pw - 32, 1, Fade(Theme::NEON_CYAN, 0.4f));
                HUD::Text("SABIAS QUE...", px + 28, py + 164, 10, Theme::NEON_GREEN);
                const std::string& pf = pauseFact_.empty() ? std::string("Todo lo que haces en esta pelea es un dato para la IA.") : pauseFact_;
                int fy = py + 180;
                for (const std::string& row : HUD::Wrap(pf, pw - 56, 10)) { HUD::Text(row.c_str(), px + 28, fy, 10, WHITE); fy += 13; }
                HUD::TextCentered("P: CONTINUAR      ESC: VOLVER AL MENU", cx, py + ph - 20, 10, Fade(WHITE, 0.8f));
            }
            break;

        case State::HeroDying:
            hud_.DrawBattle(boss_, hero_, tel, 0.f);
            if (stateTime_ > 0.5f) HUD::TextCentered("ANDROIDE NEUTRALIZADO", cx, 120, 20, Theme::NEON_RED);
            break;

        case State::Reboot: {
            const float e = Util::Clamp01(stateTime_ / 0.6f);
            const int bar = (int)(28.f * e);                         // barras de cine
            DrawRectangle(0, 0, Cfg::VIEW_W, bar, BLACK);
            DrawRectangle(0, Cfg::VIEW_H - bar, Cfg::VIEW_W, bar, BLACK);
            if (!dataTags_.empty()) {                                    // tus numeros de telemetria vuelan hacia el Heroe
                const Vector2 hs = WorldToScreen(hero_.Center());
                for (const DataTag& t : dataTags_) {
                    const float k = Util::Clamp01(t.t);
                    const float ek = k * k * (3.f - 2.f * k);
                    const float a = Util::Clamp01(k / 0.15f) * (1.f - Util::Clamp01((k - 0.82f) / 0.18f));
                    HUD::TextCentered(t.text.c_str(), (int)Lerp(t.from.x, hs.x, ek), (int)Lerp(t.from.y, hs.y, ek), 10, Fade(t.c, a));
                }
            }
            if (rebootStage_ == 1) choice_.Draw(time_, 40, false);       // el jugador elige que responderle a la IA
            if (e >= 1.f) {
                HUD::TextCentered("// RECONSTRUYENDO ANDROIDE //", cx, 9, 10, Fade(Theme::NEON_CYAN, 0.75f + 0.25f * std::sin(time_ * 6.f)));
                HUD::TextCentered("[ENTER] SALTAR LINEA     [TAB] OMITIR CINEMATICA", cx, Cfg::VIEW_H - 19, 10, Fade(WHITE, 0.6f));
            }
            break;
        }

        case State::Choice:
            DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(BLACK, 0.35f));
            choice_.Draw(time_, 40, true);                               // el Serafin vuelve y le respondes
            break;

        case State::FinalIntro: {
            const float t = stateTime_;
            DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(BLACK, std::min(0.78f, t * 0.4f)));
            { const int bar = (int)(28.f * Util::Clamp01(t / 0.7f));                  // barras de cine
              DrawRectangle(0, 0, Cfg::VIEW_W, bar, BLACK); DrawRectangle(0, Cfg::VIEW_H - bar, Cfg::VIEW_W, bar, BLACK); }
            if (t > 0.4f) HUD::TextCentered("...", cx, 100, 20, Fade(WHITE, std::min(1.f, (t - 0.4f))));
            if (t > 1.2f) HUD::TextCentered("EL ANDROIDE SE NIEGA A CAER", cx, 130, 10, Fade(WHITE, std::min(1.f, (t - 1.2f))));
            if (t > 2.2f) HUD::TextCentered("SU ENERGIA SE REUNE...", cx, 148, 10, Fade(Color{255, 225, 120, 255}, std::min(1.f, (t - 2.2f))));
            if (t > 2.7f) HUD::TextCentered(Story::INTRO_LINE, cx, 160, 10, Fade(Theme::NEON_CYAN, std::min(1.f, (t - 2.7f) * 2.f)));
            if (t > 3.3f) {
                const float j = (std::fmod(t * 30.f, 2.f) < 1.f) ? 2.f : 0.f;
                DrawText("FASE FINAL", cx - MeasureText("FASE FINAL", 40) / 2 + (int)j, 170, 40, Fade(Theme::NEON_MAGENTA, 0.8f));
                HUD::TextCentered("FASE FINAL", cx, 171, 40, Color{255, 225, 120, 255});
            }
            break;
        }

        case State::Prologue:
            DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(BLACK, 0.55f));
            HUD::TextCentered("// PROLOGO //", Cfg::VIEW_W / 2, 40, 20, Theme::NEON_CYAN);
            HUD::TextCentered("[ENTER] SIGUIENTE      [TAB] OMITIR", Cfg::VIEW_W / 2, 330, 10, Fade(WHITE, 0.6f));
            dlg_.Draw(time_, 200);
            break;

        case State::Codex:
            hud_.DrawCodex(codexPage_, stateTime_);
            break;

        case State::BossDown:
            hud_.DrawBattle(boss_, hero_, tel, 0.f);
            if (stateTime_ > 1.2f) {
                DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(BLACK, 0.6f));
                DrawCenteredBanner("FALLO DEL SISTEMA", "EL HEROE HA APRENDIDO LO SUFICIENTE: NEMESIS-X FUE DESTRUIDO", Theme::NEON_RED);
                if (stateTime_ > 2.5f && std::fmod(time_, 1.f) < 0.65f) {
                    HUD::TextCentered("[ENTER] REINTENTAR (el Heroe conserva su memoria)", cx, 200, 10, Theme::NEON_GREEN);
                    HUD::TextCentered("[F9] BORRAR MEMORIA Y REINICIAR", cx, 216, 10, Theme::NEON_CYAN);
                }
                if (stateTime_ > 2.5f && !defeatFact_.empty()) {
                    HUD::TextCentered("SABIAS QUE...", cx, 246, 10, Theme::NEON_GREEN);
                    int fy = 260;
                    for (const std::string& row : HUD::Wrap(defeatFact_, Cfg::VIEW_W - 120, 10)) { HUD::TextCentered(row.c_str(), cx, fy, 10, Fade(WHITE, 0.9f)); fy += 13; }
                }
            }
            break;

        case State::Victory:
            if (victorySummary_) {
                DrawVictorySummary(cx);
                break;
            }
            DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(BLACK, 0.55f));
            DrawCenteredBanner("NEMESIS-X TRIUNFA", "EL SERAFIN HA CAIDO. EL PROTOCOLO ADAPTATIVO FUE SUPERADO.", Theme::NEON_GREEN);
            HUD::TextCentered(TextFormat("GENERACIONES DERROTADAS: %d", Cfg::MAX_GENERATION), cx, 192, 10, Theme::NEON_YELLOW);
            HUD::TextCentered(TextFormat(Story::EPILOGUE, Seraph::CYCLES), cx, 208, 10, Color{255, 225, 120, 255});
            HUD::TextCentered(Story::MORAL, cx, 222, 10, Theme::NEON_GREEN);
            if (stateTime_ > 2.f && std::fmod(time_, 1.f) < 0.65f)
                HUD::TextCentered("[ENTER] VER LO QUE LA IA APRENDIO DE TI", cx, 246, 10, WHITE);
            break;
    }

    if (debugFinal_ && (state_ == State::Playing || state_ == State::HeroDying))
        HUD::Text("[DEBUG]  F7: matar", Cfg::VIEW_W - 118, Cfg::VIEW_H - 28, 10, Fade(Theme::NEON_YELLOW, 0.7f));
    subs_.SetSeraph(final_);
    subs_.Draw(state_ == State::Reboot ? 34 : 302);
    if (choiceFeedbackT_ > 0.f) {
        const float a = std::min(1.f, choiceFeedbackT_ / 0.4f);
        HUD::TextCentered(choiceGood_ ? "+1   RESPUESTA QUE ENSENA" : "...Esa no era la mejor. Mira la explicacion.", cx, 312, 10,
                          Fade(choiceGood_ ? Theme::NEON_GREEN : Theme::NEON_YELLOW, a));
    }
    if (flashT_ > 0.f) DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(WHITE, std::min(1.f, flashT_) * 0.55f));
    if (fadeT_ > 0.f)  DrawRectangle(0, 0, Cfg::VIEW_W, Cfg::VIEW_H, Fade(BLACK, std::min(1.f, fadeT_)));              // dialogos en vivo (en la cinematica van arriba)
}

void Game::Draw() {
    BeginDrawing();
    fx_.BeginScene();
        ClearBackground(BLACK);
        DrawWorld();
        DrawOverlay();
    fx_.EndScene();

    ClearBackground(BLACK);
    fx_.Present(PresentRect(), time_, glitch_);
    EndDrawing();
}
