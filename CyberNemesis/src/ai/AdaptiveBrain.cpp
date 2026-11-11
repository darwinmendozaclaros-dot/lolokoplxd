#include "ai/AdaptiveBrain.h"
#include "core/Paths.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <type_traits>

static_assert(std::is_trivially_copyable<HeroTraits>::value,  "HeroTraits debe ser plano");
static_assert(std::is_trivially_copyable<BrainMemory>::value, "BrainMemory debe ser plano");

namespace {
    constexpr std::uint32_t MAGIC   = 0x52424E43u;
    constexpr std::uint32_t VERSION = 3u;

    struct Header { std::uint32_t magic, version, traitsSize, memSize; };

    std::string Fmt(const char* f, float a, float b = 0.f) {
        char buf[200];
        std::snprintf(buf, sizeof(buf), f, a, b);
        return buf;
    }

    const char* AttackName(unsigned bit) {
        switch (bit) {
            case Atk::BURST:   return "RAFAGA DE PLASMA (3 disparos)";
            case Atk::SPREAD:  return "ESCOPETA DE PLASMA (corta distancia)";
            case Atk::MISSILE: return "MISILES TELEDIRIGIDOS";
            case Atk::ORBITAL: return "ATAQUE ORBITAL (columna de energia)";
            case Atk::MINE:    return "MINAS DE PROXIMIDAD";
            default:           return "???";
        }
    }
}

bool AdaptiveBrain::Load() {
    std::ifstream in(Paths::BrainPath(), std::ios::binary);
    if (!in) return false;

    Header h{};
    in.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (!in || h.magic != MAGIC || h.version != VERSION ||
        h.traitsSize != sizeof(HeroTraits) || h.memSize != sizeof(BrainMemory))
        return false;

    HeroTraits  t;
    BrainMemory m;
    in.read(reinterpret_cast<char*>(&t), sizeof(t));
    in.read(reinterpret_cast<char*>(&m), sizeof(m));
    if (!in) return false;
    if (t.generation < 1 || t.generation > Cfg::MAX_GENERATION) return false;

    traits_ = t;
    memory_ = m;
    ++memory_.sessions;
    return true;
}

bool AdaptiveBrain::Save() const {
    std::ofstream out(Paths::BrainPath(), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    Header h{MAGIC, VERSION, (std::uint32_t)sizeof(HeroTraits), (std::uint32_t)sizeof(BrainMemory)};
    out.write(reinterpret_cast<const char*>(&h), sizeof(h));
    out.write(reinterpret_cast<const char*>(&traits_), sizeof(traits_));
    out.write(reinterpret_cast<const char*>(&memory_), sizeof(memory_));
    return static_cast<bool>(out);
}

void AdaptiveBrain::Reset() {
    traits_ = HeroTraits{};
    memory_ = BrainMemory{};
    Save();
}

AdaptationReport AdaptiveBrain::Adapt(const Telemetry& tel) {
    AdaptationReport rep;
    rep.fromGen = traits_.generation;
    rep.toGen   = traits_.generation + 1;
    rep.rangedRatio     = tel.RangedRatio();
    rep.meleeRatio      = tel.MeleeRatio();
    rep.predictability  = tel.predictability;
    rep.mobility        = tel.duration >= 8.f ? tel.Mobility() : 0.f;
    rep.enoughData      = tel.TotalAttacks() >= Cfg::MIN_ATTACK_SAMPLE;

    ++memory_.deaths;
    HeroTraits& t = traits_;
    ++t.generation;

    t.maxHp        += Cfg::HERO_HP_PER_GEN;
    t.damage       *= 1.18f;
    t.fireInterval  = std::max(0.22f, t.fireInterval * 0.92f);
    t.dodgeSkill    = std::min(0.85f, t.dodgeSkill + 0.07f);

    const float observed[4] = { rep.rangedRatio, rep.meleeRatio, rep.predictability, std::min(1.f, rep.mobility) };
    for (int r = 0; r < 4; ++r) {
        float& last = memory_.decisionMatrix[r][0];
        float& ema  = memory_.decisionMatrix[r][1];
        last = observed[r];
        ema  = (memory_.deaths == 1) ? observed[r] : ema * 0.6f + observed[r] * 0.4f;
    }

    auto unlock = [&](unsigned bit, const char* reason) {
        if (t.attacks & bit) return;
        t.attacks |= bit;
        rep.lines.push_back(std::string(reason) + " -> NUEVO ATAQUE: " + AttackName(bit));
    };

    bool any = false;

    if (rep.enoughData) {
        if (rep.rangedRatio > Cfg::RATIO_THRESHOLD) {
            any = true;
            memory_.decisionMatrix[0][2] += 1.f;
            if (!t.deflectShield) {
                t.deflectShield = true;
                rep.lines.push_back(Fmt("DISPAROS %.0f%% -> ESCUDO DE DEFLEXION NEON (-%.0f%% dano a distancia)",
                                        rep.rangedRatio * 100.f, Cfg::DEFLECT_REDUCTION * 100.f));
            } else {
                rep.lines.push_back(Fmt("DISPAROS %.0f%% -> escudo recalibrado (%.0f%% de reduccion)",
                                        rep.rangedRatio * 100.f, Cfg::DEFLECT_REDUCTION * 100.f));
            }
            unlock(Atk::MISSILE, "DISPAROS");
        }
        if (rep.meleeRatio > Cfg::RATIO_THRESHOLD) {
            any = true;
            memory_.decisionMatrix[1][2] += 1.f;
            t.speedLevel  = std::min(3, t.speedLevel + 1);
            t.moveSpeed   = Cfg::HERO_BASE_SPEED * (1.f + 0.12f * (float)t.speedLevel);
            t.dashCooldown = std::max(0.8f, Cfg::DASH_COOLDOWN - 0.25f * (float)t.speedLevel);
            if (!t.dashEvasive) {
                t.dashEvasive = true;
                rep.lines.push_back(Fmt("CUERPO A CUERPO %.0f%% -> DASH EVASIVO + velocidad +%.0f%%",
                                        rep.meleeRatio * 100.f, 12.f * (float)t.speedLevel));
            } else {
                rep.lines.push_back(Fmt("CUERPO A CUERPO %.0f%% -> velocidad +%.0f%% y dash mas rapido",
                                        rep.meleeRatio * 100.f, 12.f * (float)t.speedLevel));
            }
            unlock(Atk::SPREAD, "CUERPO A CUERPO");
            unlock(Atk::MINE,   "CUERPO A CUERPO");
        }
    } else {
        rep.lines.push_back("DATOS DE ATAQUE INSUFICIENTES: solo mejora de estadisticas base");
    }

    if (tel.predictability > Cfg::PREDICT_THRESHOLD) {
        any = true;
        memory_.decisionMatrix[2][2] += 1.f;
        if (!t.leadShooting) {
            t.leadShooting = true;
            rep.lines.push_back(Fmt("PREDECIBILIDAD %.0f%% -> APUNTADO DE PREDICCION (lead shooting)",
                                    tel.predictability * 100.f));
        } else {
            rep.lines.push_back(Fmt("PREDECIBILIDAD %.0f%% -> prediccion de trayectoria afinada",
                                    tel.predictability * 100.f));
        }
        unlock(Atk::ORBITAL, "PREDECIBLE");
    }

    if (rep.mobility > Cfg::MOBILITY_THRESHOLD) {
        any = true;
        memory_.decisionMatrix[3][2] += 1.f;
        if (!t.jumpJets) {
            t.jumpJets = true;
            rep.lines.push_back(Fmt("MOVILIDAD/ALTURA %.0f%% -> PROPULSORES DE SALTO (doble salto)",
                                    std::min(1.f, rep.mobility) * 100.f));
        } else {
            rep.lines.push_back(Fmt("MOVILIDAD/ALTURA %.0f%% -> persecucion aerea reforzada",
                                    std::min(1.f, rep.mobility) * 100.f));
        }
        unlock(Atk::MISSILE, "MOVIL");
    }

    if (!any && rep.enoughData)
        rep.lines.push_back("SIN PATRON DOMINANTE: el Heroe refuerza sus estadisticas generales");

    const struct { int gen; unsigned bit; } ladder[] = {
        { 2, Atk::BURST }, { 3, Atk::SPREAD }, { 4, Atk::MISSILE }, { 5, Atk::ORBITAL }, { 6, Atk::MINE }
    };
    for (const auto& step : ladder)
        if (t.generation >= step.gen) unlock(step.bit, Fmt("EVOLUCION GEN %.0f", (float)step.gen).c_str());
    if (t.generation >= 3 && !t.jumpJets) {
        t.jumpJets = true;
        rep.lines.push_back("EVOLUCION GEN 3 -> PROPULSORES DE SALTO (doble salto)");
    }

    rep.lines.push_back(Fmt("+%.0f HP maximo, +18%% dano, cadencia +8%%", Cfg::HERO_HP_PER_GEN));
    return rep;
}
