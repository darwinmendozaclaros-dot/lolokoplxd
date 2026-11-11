#pragma once
#include "raylib.h"
#include <vector>

#include "entities/Arena.h"
#include "entities/Boss.h"
#include "entities/Hero.h"
#include "entities/Hazards.h"
#include "entities/Drones.h"
#include "entities/Pickups.h"
#include "entities/Projectile.h"
#include "ai/PatternTracker.h"
#include "ai/AdaptiveBrain.h"
#include "ai/FinalDirector.h"
#include "entities/Bullets.h"
#include "entities/Summons.h"
#include "gfx/Dialogue.h"
#include "gfx/Subtitles.h"
#include "gfx/ChoiceBox.h"
#include <cstdint>
#include "core/Story.h"
#include "gfx/Sprites.h"
#include "gfx/Particles.h"
#include "gfx/Background.h"
#include "gfx/PostFX.h"
#include "gfx/HUD.h"
#include "gfx/ShardField.h"
#include "gfx/MainMenu.h"
#include "audio/AudioManager.h"

class Game {
public:
    Game();
    ~Game();
    void Run();
    void SetStartFullscreen(bool on) { startFullscreen_ = on; } // --fullscreen

private:
    enum class State { Title, Prologue, Codex, Playing, HeroDying, Reboot, Choice, FinalIntro, BossDown, Victory };
    struct Shockwave { Vector2 origin; float t; };
    enum class CEvent { None, Bombard, Drones, LowGrav, Core };
    void StartCombatEvent(CEvent e, bool fromPhase);
    void EndCombatEvent();
    CEvent PickCombatEvent() const;

    void Init();
    void Shutdown();
    void Update(float dt);
    void Draw();

    void UpdateTitle(float dt);
    void UpdatePlaying(float dt);
    void UpdateHeroDying(float dt);
    void UpdateReboot(float dt);
    void UpdateChoice(float dt);
    void UpdateCamera(float dt);
    void UpdateEffects(float dt);
    void DrawScreenEffects() const;
    void DrawSpawnBeam() const;
    void DrawSeraphTrail() const;
    void SpawnDataTag();
    Vector2 CamFocus(float zoom) const;
    Vector2 WorldToScreen(Vector2 p) const;
    void BeginReboot();
    void DrawRebuildFx() const;
    void UpdateFinalIntro(float dt);
    void ResetSession();
    void DrawVictorySummary(int cx) const;
    void UpdateDialogueTriggers(float dt);
    bool SayEvent(Story::Ev ev);
    void DebugJumpToFinal();
    void UpdatePrologue(float dt);
    void UpdateCodex(float dt);
    void TickShards(float dt);
    void UpdateFinalPhase(float dt);
    void HandleSummonEvents();
    void StartFinalRound();
    void UpdateBossDown(float dt);
    void UpdateVictory(float dt);

    void StartRound(bool newGeneration);
    void ResetRound();
    void OnHeroDefeated();
    void OnBossDefeated();
    void FinishHeroDeath();
    void ReturnToMenu();

    BossInput ReadBossInput() const;
    BossInput IdleBossInput(float lookAtX) const;
    void HandleBossEvents();
    void HandleHeroEvents();
    void UpdateProjectiles(float dt);
    void UpdateHazards(float dt);
    void UpdateDrones(float dt);
    void UpdateCombatEvents(float dt);
    void TriggerPhase(int phase);
    Projectile MakeHeroProjectile(const ShotCmd& s);
    void SpawnExplosion(Vector2 p, Color a, Color b, int n);

    void DrawWorld();
    void DrawOverlay();
    void DrawCenteredBanner(const char* big, const char* small, Color c) const;

    Rectangle PresentRect() const;
    Vector2   MouseToView() const;

    Arena            arena_;
    Boss             boss_;
    Hero             hero_;
    Hazards          hazards_;
    DroneSwarm       drones_;
    Pickups          pickups_;
    BulletField      bullets_;
    Summons          summons_;
    ChoiceBox        choice_;                // responder a la IA
    Story::ChoiceScene scene_;
    bool             hasScene_ = false;
    int              rebootStage_ = 0;       // 0 dialogos, 1 eleccion, 2 respuesta y cierre
    std::vector<DialogueLine> rebootClosing_, seraphChain_;
    int              sessionChoices_ = 0, sessionInsights_ = 0;
    float            choiceFeedbackT_ = 0.f;
    bool             choiceGood_ = false;
    std::string      pauseFact_, defeatFact_;
    // camara cinematografica y transiciones
    float            camZoom_ = 1.f, camZoomTarget_ = 1.f, camPunch_ = 0.f, flashT_ = 0.f, fadeT_ = 0.f, tagTimer_ = 0.f;
    Vector2          camFocus_{Cfg::VIEW_W * 0.5f, Cfg::VIEW_H * 0.5f}, camFocusTarget_{Cfg::VIEW_W * 0.5f, Cfg::VIEW_H * 0.5f};
    State            prevState_ = State::Title;
    // efectos extra: brasas, peligro, impacto, lineas de dash, aura del Serafin
    float            embersT_ = 0.f, dangerAmt_ = 0.f, dmgEdgeT_ = 0.f, dashLinesT_ = 0.f;
    float            lastBossHp_ = 0.f, lastHeroHp_ = 0.f, trailTimer_ = 0.f, featherT_ = 0.f;
    Vector2          trailPos_[6]{};
    bool             trailFace_[6]{};
    int              trailHead_ = 0;
    struct DataTag { Vector2 from; float t; std::string text; Color c; };
    std::vector<DataTag> dataTags_;
    Dialogue         dlg_;          // cuadro bloqueante (solo el prologo)
    Subtitles        subs_;         // dialogos en vivo durante la pelea
    bool             halfSaid_ = false, lowSaid_ = false, debugFinal_ = false;
    float            fightT_ = 0.f, idleT_ = 0.f, eventCd_ = 0.f;
    int              noteTurn_ = 0;           // alterna consejo de la guia / nota del proyecto
    Story::DeathReport deathLog_;
    FinalDirector    director_;
    std::vector<Projectile> projectiles_;
    std::vector<Shockwave>  shockwaves_;
    ParticleSystem   particles_;
    PatternTracker   tracker_;
    AdaptiveBrain    brain_;
    Sprites          sprites_;
    Background       bg_;
    PostFX           fx_;
    HUD              hud_;
    ShardField       shards_;
    MainMenu         menu_;
    AudioManager     audio_;

    State  state_ = State::Title;
    float  stateTime_ = 0.f, time_ = 0.f;
    float  introTimer_ = 0.f, hintTimer_ = 0.f;
    float  shake_ = 0.f, glitch_ = 0.f, hitStop_ = 0.f, slowmo_ = 1.f;
    Vector2 shakeOff_{0.f, 0.f};
    bool   paused_ = false, quit_ = false, brainLoaded_ = false;
    int    nextId_ = 1;

    int    phase_ = 0;
    CEvent cev_ = CEvent::None, lastCev_ = CEvent::None;
    float  eventTimer_ = 15.f, cevT_ = 0.f, cevDur_ = 0.f, strikeTimer_ = 0.f, gravTarget_ = 1.f;
    int    strikesLeft_ = 0;
    const char* cevLabel_ = "";
    float  cevLabelT_ = 0.f, phaseBannerT_ = 0.f;
    int    phaseBannerNum_ = 1;
    bool   introShrink_ = false, shrinkAnnounced_ = false;
    bool   final_ = false;
    float  pewT_ = 0.f;
    bool   startFullscreen_ = false, victorySummary_ = false;
    int    sessionHeroDeaths_ = 0, sessionSeraphDeaths_ = 0, sessionDodges_ = 0, sessionAbsorbed_ = 0;
    HeroTraits finalTraits_;        // contramedidas con las que llego el Serafin (para el resumen final)
    float  rebootK_ = 0.f;          // 0..1: avance de la reconstruccion del Heroe en la cinematica
    bool   hudDetail_ = false;      // H: HUD detallado (telemetria, habilidades); por defecto, pantalla limpia
    bool   prologueSeen_ = false;
    int    codexPage_ = 0;
    int    seraphCycle_ = 1, rebirthFrom_ = 1;     // Eco actual del Serafin / Eco que acaba de caer
    float  dodgePopT_ = 0.f, finalCoreT_ = 0.f;
    Vector2 dodgePopPos_{0.f, 0.f};
    int    fxLevel_ = 1;
    float  fxMsgT_ = 0.f;

    AdaptationReport report_;
    Telemetry        lastTelemetry_;
};
