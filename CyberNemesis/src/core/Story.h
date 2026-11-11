#pragma once
#include "ai/AdaptiveBrain.h"
#include "ai/HeroTraits.h"
#include "core/Config.h"
#include "gfx/Dialogue.h"
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <map>
#include <string>
#include <vector>

// Lore, humor y contenido educativo. Texto en ASCII (la fuente por defecto de Raylib no tiene tildes ni enie).
// Personajes: NEMESIS-X (el jugador), HEROE / SERAFIN (la IA) y GUIA (la voz que explica como aprende la IA).
namespace Story {
    inline constexpr Color PLAYER_COL {255, 43, 214, 255};    // NEMESIS-X
    inline constexpr Color HERO_COL   {24, 240, 255, 255};    // Heroe
    inline constexpr Color SERAFIN_COL{255, 225, 120, 255};   // Serafin
    inline constexpr Color GUIDE_COL  {57, 255, 120, 255};    // Guia
    inline constexpr Color SYSTEM_COL = GUIDE_COL;
    inline constexpr Color DATA_COL   = GUIDE_COL;

    inline constexpr const char* INTRO_LINE = "EL PROTOCOLO ADAPTATIVO LIBERA SU ULTIMO MODELO: UN SERAFIN QUE HEREDA TODO LO APRENDIDO";
    inline constexpr const char* EPILOGUE   = "EL SERAFIN CAYO %d VECES. CADA CAIDA LE ENSENO ALGO... PERO NO FUE SUFICIENTE.";
    inline constexpr const char* MORAL      = "UNA IA SOLO APRENDE DE LOS DATOS QUE TU LE DAS: SI VARIAS, SUS CONTRAMEDIDAS FALLAN.";

    // De donde vino el dano que mato al Serafin en su ultima vida (lo mide Game).
    struct DeathReport {
        float melee = 0.f, plasma = 0.f, other = 0.f;
        int   dodges = 0, absorbed = 0;
    };

    inline std::string F(const char* fmt, ...) {
        char buf[640];
        va_list ap; va_start(ap, fmt); std::vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
        return buf;
    }

    inline Color ColorOf(const std::string& who) {
        if (who == "NEMESIS-X") return PLAYER_COL;
        if (who == "HEROE")     return HERO_COL;
        if (who == "SERAFIN")   return SERAFIN_COL;
        return GUIDE_COL;
    }
    inline DialogueLine L(const char* who, std::string text) { return { who, std::move(text), ColorOf(who) }; }

    // ------------------------------------------------------------------------------------------
    //  Variety: NINGUN dialogo se repite. Cada vez que se necesita una linea se elige, al azar,
    //  una variante que aun no se haya dicho; si ya se dijeron todas, no se dice nada.
    // ------------------------------------------------------------------------------------------
    class Variety {
    public:
        int Pick(int key, int count) {                    // indice libre de [0,count) o -1 si se agotaron
            uint64_t& m = seen_[key];
            int freeIdx[64]; int n = 0;
            for (int i = 0; i < count && i < 64; ++i) if (!(m & (1ull << i))) freeIdx[n++] = i;
            if (n == 0) return -1;
            const int i = freeIdx[Rand() % (uint32_t)n];
            m |= 1ull << i;
            return i;
        }
        void Reset() { seen_.clear(); }
    private:
        uint32_t Rand() { s_ ^= s_ << 13; s_ ^= s_ >> 17; s_ ^= s_ << 5; return s_; }
        std::map<int, uint64_t> seen_;
        uint32_t s_ = 2463534242u ^ (uint32_t)std::time(nullptr);
    };
    inline Variety& V() { static Variety v; return v; }
    inline void ResetVariety() { V().Reset(); }

    enum Key { K_GEN1 = 1, K_INTRO_HERO, K_INTRO_NEM, K_TIP, K_DODGE, K_DECOY, K_ORBS, K_SHIELD, K_DASH,
               K_HALF_H, K_HALF_S, K_LOW, K_DOWN_H, K_DOWN_S, K_OPEN, K_F_RANGED, K_F_MELEE, K_F_PRED,
               K_F_MOB, K_F_NONE, K_CLOSE, K_LAST_H, K_LAST_S, K_PROJ, K_OUTRO,
               K_FACT, K_FACT_P, K_FACT_D, K_CH_R, K_CH_M, K_CH_P, K_CH_MO, K_CH_N };

    struct Beat    { const char* who; const char* text; };
    struct Variant { Beat b[3]; };

    inline std::vector<DialogueLine> FromVariant(const Variant& v) {
        std::vector<DialogueLine> out;
        for (const Beat& b : v.b) if (b.who) out.push_back(L(b.who, b.text));
        return out;
    }
    template <size_t N>
    inline std::vector<DialogueLine> PickVariant(int key, const Variant (&pool)[N]) {
        const int i = V().Pick(key, (int)N);
        return i < 0 ? std::vector<DialogueLine>{} : FromVariant(pool[i]);
    }

    // ----------------------------------------------------------------- Prologo (cuadro bloqueante)
    inline std::vector<DialogueLine> Prologue() {
        return {
            L("GUIA",      "Bienvenido! Soy tu guia. Ano 2187, instalacion Cyber-Nemesis. Nivel de amenaza: maximo. Y el cafe, minimo."),
            L("GUIA",      "Tu eres NEMESIS-X, el guardian final. La instalacion ha decidido eliminarte (sin rencores, es su trabajo)."),
            L("GUIA",      "Para eso creo el Protocolo Adaptativo: una inteligencia artificial que aprende de cada combate."),
            L("GUIA",      "Dato: esta IA no trae todo programado. Registra como peleas (telemetria) y usa esos datos como entrenamiento."),
            L("GUIA",      "Esto es un proyecto educativo: en vez de explicarte la IA con formulas, te dejamos pelear contra una que aprende, para que la entiendas sintiendola."),
            L("GUIA",      "Por eso eres el Jefe: tu forma de jugar es el dato que la IA estudia. Tu cambias, ella cambia. Eso es un experimento con explosiones."),
            L("GUIA",      "Cada vez que el Heroe cae, analiza que fallo y renace con una contramedida. Para ella, morir es estudiar."),
            L("NEMESIS-X", "Entonces cambiare mi estilo. Si me repito, me predice. Si me predice, me aburro."),
        };
    }

    // Ganchos que rotan en la pantalla de inicio.
    inline const char* TitleHook(int i) {
        static const char* H[6] = {
            "Puedes vencer a una IA que aprende de ti?",
            "Mientras juegas, la IA anota cada ataque que haces.",
            "Si repites un patron, la IA fabrica una contramedida.",
            "Aqui morir es entrenar: el Heroe vuelve mas fuerte.",
            "Pulsa H en combate para ver lo que la IA mide de ti.",
            "Pista: cambiar de estilo es la mejor forma de confundirla.",
        };
        return H[((i % 6) + 6) % 6];
    }

    // Resumen final de la sesion: lo que la IA aprendio del visitante (con datos reales de la partida).
    struct SessionSummary {
        bool  hasData = false;
        float ranged = 0.f, melee = 0.f, predict = 0.f, mobility = 0.f;
        bool  shield = false, dash = false, lead = false, jets = false;
        int   heroDeaths = 0, seraphDeaths = 0, dodges = 0, absorbed = 0;
        int   choices = 0, insights = 0;          // respuestas elegidas / las que ensenaban algo
    };
    struct SummaryRow { std::string text; int kind; };      // kind: 0 titulo, 1 normal, 2 destacado

    inline std::vector<SummaryRow> SummaryLines(const SessionSummary& m) {
        auto pct = [](float x) { return (int)(x * 100.f + 0.5f); };
        std::vector<SummaryRow> v;
        v.push_back({ "TU ESTILO (lo que midio la IA)", 0 });
        if (m.hasData) {
            v.push_back({ F("Distancia %d%%   Cuerpo a cuerpo %d%%   Predecibilidad %d%%   Altura %d%%",
                            pct(m.ranged), pct(m.melee), pct(m.predict), pct(m.mobility > 1.f ? 1.f : m.mobility)), 1 });
            const char* style = (m.ranged - m.melee > 0.15f) ? "tirador a distancia"
                              : (m.melee - m.ranged > 0.15f) ? "luchador cuerpo a cuerpo"
                              : "equilibrado (mezclas distancia y cuerpo a cuerpo)";
            const char* tag = m.predict > Cfg::PREDICT_THRESHOLD ? " y bastante predecible"
                            : (m.predict < 0.35f ? " y dificil de predecir" : "");
            v.push_back({ F("Perfil: %s%s.", style, tag), 2 });
        } else {
            v.push_back({ "Sin datos suficientes de ataque en esta sesion: la IA no pudo estudiarte.", 1 });
        }

        v.push_back({ "LO QUE LA IA FABRICO PARA CONTRARRESTARTE", 0 });
        std::string list;
        auto add = [&](const char* s) { if (!list.empty()) list += "  |  "; list += s; };
        if (m.shield) add("Escudo de deflexion");
        if (m.dash)   add("Dash evasivo");
        if (m.lead)   add("Punteria predictiva");
        if (m.jets)   add("Propulsores");
        v.push_back({ list.empty() ? "Ninguna: tus patrones no fueron lo bastante claros." : list, 2 });

        v.push_back({ "SU ENTRENAMIENTO", 0 });
        v.push_back({ F("La IA murio y volvio %d veces (Heroe: %d, Serafin: %d).", m.heroDeaths + m.seraphDeaths, m.heroDeaths, m.seraphDeaths), 1 });
        v.push_back({ F("El Serafin esquivo %d golpes y sus invocaciones bloquearon %d disparos.", m.dodges, m.absorbed), 1 });

        if (m.choices > 0) {
            v.push_back({ "TUS RESPUESTAS A LA IA", 0 });
            v.push_back({ F("Elegiste respuestas que ensenan algo en %d de %d ocasiones.", m.insights, m.choices), m.insights * 2 >= m.choices ? 2 : 1 });
        }

        v.push_back({ "LAS REGLAS DEL SISTEMA (umbrales reales)", 0 });
        v.push_back({ F("Distancia > %d%%: escudo   |   Cuerpo a cuerpo > %d%%: dash evasivo",
                        pct(Cfg::RATIO_THRESHOLD), pct(Cfg::RATIO_THRESHOLD)), 1 });
        v.push_back({ F("Predecible > %d%%: punteria predictiva   |   Altura > %d%%: propulsores",
                        pct(Cfg::PREDICT_THRESHOLD), pct(Cfg::MOBILITY_THRESHOLD)), 1 });

        v.push_back({ "PARA LLEVARTE", 0 });
        v.push_back({ "1) Los datos mandan: la IA solo aprendio de lo que hiciste.", 1 });
        v.push_back({ "2) Morir y volver es entrenar: error, analisis, mejora.", 1 });
        v.push_back({ "3) Variar tu estilo es la mejor defensa contra el sobreajuste.", 1 });
        return v;
    }

    // ------------------------------------------------------------ Consejos de la guia (educativos + humor)
    inline DialogueLine GuideTip() {
        static const char* T[24] = {
            "Dato: la IA no 've' tus golpes, solo cuenta numeros. Por eso no se asusta nunca. Tu si te asustas: ventaja humana.",
            "Telemetria es tomar apuntes de lo que haces. La IA es ese companero que anota todo y luego te lo recuerda en el peor momento.",
            "Si te mueves siempre igual, la IA lo nota. Ser predecible no es un insulto, es una metrica.",
            "Sobreajuste: una IA que memoriza tu estilo es como estudiar solo las preguntas del examen pasado. Funciona hasta que cambian el examen.",
            "Basura entra, basura sale: una IA con malos datos aprende malas ideas muy rapido. Como un tutorial de internet.",
            "brain.data guarda lo aprendido. Es el unico diario que la IA no pierde... ni te perdona.",
            "Aprendizaje por refuerzo: probar, fallar, ajustar. Es como aprendiste a montar bicicleta, pero con menos raspones.",
            "Una contramedida solo funciona contra el patron que la origino. Un escudo contra balas no te salva de un aplastamiento. De nada.",
            "Mezcla ataques de lejos y de cerca: es como variar el menu para que nadie adivine la cena.",
            "La IA decide con probabilidades, no con intuicion. Cuando esquiva al 10% no adivina: tira un dado... uno muy educado.",
            "Entrenar una IA lleva tiempo y datos. Esta aprende en minutos porque es de juguete; las de verdad tardan dias y gastan mas luz que tu nevera.",
            "Nadie nace perfecto: el Serafin hereda lo que aprendieron los Heroes, pero aun asi necesita morir varias veces para pulirse. Hasta las IA hacen borradores.",
            "Un modelo que nunca falla probablemente no esta aprendiendo nada nuevo. Equivocarse tambien es un dato.",
            "Cuando la IA apunta a donde vas a estar, usa prediccion. Cuando tu apuntas a donde esta, usas optimismo.",
            "Generalizar es aplicar lo aprendido a situaciones nuevas. Si solo sirve en el mismo escenario, eso se llama memorizar.",
            "El 'dataset' es el conjunto de ejemplos con el que aprende la IA. Aqui el dataset eres tu. Presion, eh?",
            "Sesgo: si la IA solo te ve disparar, cree que solo sabes disparar. Muestrale otras cosas y aprendera a dudar.",
            "Una IA no tiene mala intencion, tiene una funcion objetivo. La suya: que no ganes. Nada personal.",
            "Mas datos no siempre es mejor: datos repetitivos solo ensenan a ser repetitivo.",
            "Evaluar es probar la IA con situaciones que no vio. Tu cambiando de estrategia a mitad de pelea es su examen sorpresa.",
            "Robustez: tener varias defensas en lugar de una sola. Como llevar paraguas Y chubasquero.",
            "Los humanos tambien aprenden por refuerzo: te quemas una vez y entiendes lo de la estufa. La IA se quema por ti, igual.",
            "Ruido: datos raros que despistan. Tu salto de panico sin motivo es ruido. Para la IA, arte moderno.",
            "Transferencia: usar lo aprendido en una tarea en otra. El Heroe de la gen 5 usa lo de la gen 4, aunque no lo agradece.",
        };
        const int i = V().Pick(K_TIP, 24);
        if (i < 0) return { "", "", Color{0, 0, 0, 0} };
        return L("GUIA", T[i]);
    }

    // ------------------------------------------------ Inicio de pelea (Heroe)
    inline std::vector<DialogueLine> HeroIntro(const HeroTraits& t) {
        std::vector<DialogueLine> v;
        if (t.generation <= 1 && V().Pick(K_GEN1, 1) >= 0) {
            v.push_back(L("HEROE",     "Primer despliegue. Todavia no se nada de ti, NEMESIS-X."));
            v.push_back(L("NEMESIS-X", "Perfecto. Yo tampoco se nada de mi plan."));
            v.push_back(L("GUIA",      "Aqui la IA aprende de ti: cada ataque tuyo queda anotado. Si sientes que te observan, es porque te observan."));
            return v;
        }
        std::string list;
        auto add = [&](const char* s) { if (!list.empty()) list += ", "; list += s; };
        if (t.deflectShield) add("escudo contra tus disparos");
        if (t.dashEvasive)   add("dash contra tus embestidas");
        if (t.leadShooting)  add("punteria predictiva");
        if (t.jumpJets)      add("propulsores para seguirte en altura");
        const std::string abil = list.empty() ? "ninguna contramedida especial (tus patrones no estaban claros, bien jugado)" : list;

        static const char* H[8] = {
            "Generacion %d en linea. Traigo: %s. Si parece un regalo, lo es... de ti para mi.",
            "Soy la gen %d! Mi equipo nuevo: %s. Gracias por los datos, fueron muy utiles.",
            "Version %d del Heroe, ahora con %s. Todavia huelo a pintura fresca.",
            "Actualizacion %d instalada: %s. Reiniciar no fue necesario, morir si.",
            "Gen %d reportandose. Inventario: %s. Pase lo que pase, hoy aprendo algo.",
            "Aqui la gen %d, equipada con %s. Prometo no repetir los errores... solo los nuevos.",
            "Generacion %d! Llevo %s. Cada pieza es un 'tomo nota' de tus ultimos combates.",
            "Gen %d desplegada con %s. Dicen que la practica hace al maestro; en mi caso, la practica eres tu.",
        };
        int i = V().Pick(K_INTRO_HERO, 8);
        if (i >= 0) v.push_back(L("HEROE", F(H[i], t.generation, abil.c_str())));

        static const char* N[6] = {
            "Otra version? Me estan haciendo spoilers de mi propia derrota.",
            "Cada vez mas elegante. Cada vez mas destruible.",
            "Que bien, mas visitas. Voy a necesitar mas explosivos.",
            "Mejoras? Yo tambien tengo trucos. Aunque ahora mismo no los recuerdo.",
            "Ya viene otro. Que alguien le avise que la puerta se abre hacia adentro.",
            "Mis cooldowns y yo estamos listos. Mi paciencia, menos.",
        };
        i = V().Pick(K_INTRO_NEM, 6);
        if (i >= 0) v.push_back(L("NEMESIS-X", N[i]));

        const DialogueLine g = GuideTip();
        if (!g.text.empty()) v.push_back(g);
        return v;
    }

    // ------------------------------------------------------------ Datos curiosos (reales, sin repetirse)
    inline constexpr int FACT_COUNT = 26;
    inline const char* FactAt(int i) {
        static const char* T[FACT_COUNT] = {
            "En Pac-Man cada fantasma persigue distinto: Blinky va directo a ti y Pinky intenta emboscarte unos pasos por delante. Cuatro 'personalidades' con reglas muy simples.",
            "En 1997 Deep Blue, de IBM, vencio al campeon mundial Garry Kasparov en un match de ajedrez a seis partidas. Fue un hito: una maquina ganando bajo reglas de torneo.",
            "En 2016 AlphaGo vencio 4-1 a Lee Sedol en Go, un juego con mas posiciones posibles que atomos en el universo observable.",
            "Left 4 Dead usa un 'Director' de IA que ajusta el ritmo segun lo estresado que te ve: si vas muy bien, suelta mas zombis; si vas fatal, te da un respiro.",
            "En los anos 50 Arthur Samuel hizo un programa de damas que mejoraba jugando contra si mismo; en 1959 popularizo el termino 'aprendizaje automatico'.",
            "En 2019 OpenAI Five derroto a los campeones del mundo de Dota 2 (el equipo OG). Entreno jugando contra copias de si mismo.",
            "En el juego CoastRunners una IA aprendio a dar vueltas chocando para recoger bonos en vez de terminar la carrera. Se llama 'reward hacking': la IA optimiza lo que mides, no lo que quieres.",
            "AlphaZero aprendio ajedrez jugando solo contra si mismo, sin partidas humanas, y alcanzo nivel sobrehumano tras unas pocas horas de entrenamiento (2017).",
            "En 1943 McCulloch y Pitts propusieron el primer modelo matematico de una neurona artificial: las redes neuronales son mas viejas que casi todos los videojuegos.",
            "El perceptron de Frank Rosenblatt (1958) fue de las primeras maquinas que aprendian de ejemplos. Un solo perceptron no podia aprender la funcion XOR, y eso freno el area durante anos.",
            "Alan Turing propuso en 1950 lo que hoy llamamos Test de Turing, pero con un nombre menos pegadizo: 'el juego de la imitacion'.",
            "Gran Turismo Sophy (2022), una IA entrenada con aprendizaje por refuerzo, supero a pilotos de eSports de primer nivel en carreras.",
            "MNIST, un conjunto de 70.000 digitos escritos a mano, ha servido para probar miles de algoritmos. Es el 'hola mundo' del aprendizaje automatico.",
            "Una 'epoca' en aprendizaje automatico es una pasada completa por todos los datos de entrenamiento. Las IA grandes necesitan muchas; tu Heroe solo necesito una pelea.",
            "Resident Evil 4 ajusta la dificultad en secreto segun como juegas, sin avisarte. Es una forma de IA que te 'lee' mientras te diviertes.",
            "F.E.A.R. (2005) se hizo famoso por la IA de sus soldados: usaban planificacion de acciones (GOAP) para elegir como llegar a un objetivo en vez de seguir un guion.",
            "En Alien: Isolation el xenomorfo combina un 'director' que sabe mas o menos donde estas con un 'cerebro' propio que solo reacciona a lo que percibe. Por eso parece que te caza sin hacer trampa del todo.",
            "En 2012 la red AlexNet redujo tanto el error en reconocimiento de imagenes que cambio el rumbo de la IA moderna.",
            "Los jefes clasicos siguen patrones fijos que el jugador memoriza. Aqui pasa al reves: la IA memoriza los tuyos.",
            "Un videojuego es un laboratorio barato para la IA: reglas claras, puntuacion medible y millones de partidas sin romper nada. Por eso tantos hitos de la IA son juegos.",
            "El 'efecto IA' dice que cuando una maquina logra algo, deja de llamarse IA: 'eso ya es solo programar'. Las IA de los videojuegos viven en ese limbo.",
            "Los fantasmas de Pac-Man a veces dejan de perseguirte y se dispersan a sus esquinas. Sin ese descanso el juego seria injusto: hasta la IA necesita pausas.",
            "El termino 'inteligencia artificial' nacio en 1956, en el taller de Dartmouth organizado por John McCarthy y otros.",
            "Las maquinas son malisimas tirando dados de verdad: el 'azar' de los juegos es un generador pseudoaleatorio, una formula que parece aleatoria pero no lo es.",
            "Un modelo sobreajustado es como el alumno que se sabe de memoria las respuestas del examen pasado pero no la materia. A tu Heroe le pasa si siempre juegas igual.",
            "Nimrod, una maquina de 1951 construida solo para jugar al Nim, fue de las primeras computadoras hechas para jugar y se exhibio al publico en el Festival of Britain.",
        };
        return T[((i % FACT_COUNT) + FACT_COUNT) % FACT_COUNT];
    }
    // Un dato que aun no se haya dicho en ese contexto (key distinta = pool distinto). "" si ya salieron todos.
    inline std::string FactText(int key) {
        const int i = V().Pick(key, FACT_COUNT);
        return i < 0 ? std::string() : std::string(FactAt(i));
    }
    inline std::vector<DialogueLine> FunFactLines() {
        const std::string f = FactText(K_FACT);
        if (f.empty()) return {};
        return { L("GUIA", "Dato curioso: " + f) };
    }

    // ------------------------------------------------------------ Responder a la IA (elecciones educativas)
    struct ChoiceScene {
        DialogueLine prompt;                                   // lo que dice la IA
        std::vector<std::string> options;                      // lo que puede responder NEMESIS-X
        std::vector<bool> good;                                // la respuesta ensena / acierta algo
        std::vector<std::vector<DialogueLine>> replies;        // IA + guia contestan a cada opcion
    };
    struct Opt   { const char* text; bool good; Beat r[2]; };
    struct Scene { Beat prompt; Opt o[3]; };

    inline ChoiceScene BuildScene(const Scene& sc) {
        ChoiceScene c;
        c.prompt = L(sc.prompt.who, sc.prompt.text);
        for (const Opt& o : sc.o) {
            c.options.push_back(o.text);
            c.good.push_back(o.good);
            std::vector<DialogueLine> r;
            for (const Beat& b : o.r) if (b.who) r.push_back(L(b.who, b.text));
            c.replies.push_back(r);
        }
        return c;
    }
    template <size_t N>
    inline bool PickScene(int key, const Scene (&pool)[N], ChoiceScene& out) {
        const int i = V().Pick(key, (int)N);
        if (i < 0) return false;
        out = BuildScene(pool[i]);
        return true;
    }

    // type: 0 distancia, 1 cuerpo a cuerpo, 2 predecible, 3 altura, 4 sin patron
    inline bool HeroScene(int type, ChoiceScene& out) {
        static const Scene R[3] = {
            { {"HEROE","Fabrique un escudo porque disparas mucho de lejos. Que me respondes?"}, {
                {"Entonces cambio a cuerpo a cuerpo: tu escudo no sirve ahi.", true,
                    {{"HEROE","Ay. Justo lo que no queria que hicieras."},{"GUIA","Correcto: una contramedida solo sirve contra el patron que la origino. Cambiar de estrategia es la mejor forma de que una IA no te prediga."}}},
                {"Disparo mas fuerte, a ver si lo rompo.", false,
                    {{"HEROE","Perfecto: mas datos del mismo tipo para mi."},{"GUIA","Repetir el patron lo refuerza: la IA aprende mas de lo que mas haces."}}},
                {"Como decidiste eso exactamente?", true,
                    {{"HEROE","Conte tus disparos: superaron el umbral y active la regla del escudo."},{"GUIA","Es una regla de umbral: si los disparos a distancia pasan de cierto porcentaje, la IA crea un escudo. No es magia: es un 'si... entonces' alimentado con tus datos."}}} } },
            { {"HEROE","Ya vi suficientes disparos tuyos y mi escudo viene listo. Algo que decir?"}, {
                {"Una sola pelea es muy poca muestra para juzgarme.", true,
                    {{"HEROE","Cierto, pero ya tengo datos suficientes para actuar."},{"GUIA","Con pocos datos una IA puede sacar conclusiones equivocadas. Por eso el sistema exige un minimo de ataques antes de adaptarse."}}},
                {"Solo estaba calentando.", false,
                    {{"HEROE","Calentar tambien se mide."},{"GUIA","Para la IA no existe 'solo calentando': todo lo que haces es dato."}}},
                {"Que mas mides aparte de mis disparos?", true,
                    {{"HEROE","Tus saltos, dashes, el tiempo en el aire y en que zona de la arena estas."},{"GUIA","Eso es telemetria: variables medidas de tu comportamiento. Cada variable es una pista sobre tu estilo."}}} } },
            { {"HEROE","Segun mis numeros eres un tirador. Te molesta?"}, {
                {"Soy mas que un numero.", true,
                    {{"HEROE","Seguro, pero yo solo veo los numeros."},{"GUIA","Una metrica resume un estilo, no a la persona: los modelos solo ven las variables que les das, y de ahi nacen muchos sesgos."}}},
                {"Muchisimo. Ahora te destruyo.", false,
                    {{"HEROE","Me encanta tu motivacion. Y tus datos."},{"GUIA","Las emociones no cambian la estadistica: lo que cambia tu perfil es lo que haces."}}},
                {"Entonces me vuelvo impredecible.", true,
                    {{"HEROE","Eso complicaria mis reglas..."},{"GUIA","Variar rompe el patron y la contramedida aprendida pierde valor. Cuando un modelo se ata demasiado a unos datos se llama sobreajuste."}}} } },
        };
        static const Scene M[3] = {
            { {"HEROE","Te lanzas al cuerpo a cuerpo una y otra vez. Ahora tengo dash evasivo. Que haces?"}, {
                {"Mezclo disparos desde lejos para que el dash no baste.", true,
                    {{"HEROE","Eso me lo pondra dificil."},{"GUIA","Diversificar ataques reduce el valor de cualquier contramedida aislada."}}},
                {"Aplasto mas rapido!", false,
                    {{"HEROE","Mas velocidad con el mismo patron: mas datos para mi."},{"GUIA","Hacerlo mas rapido no cambia el patron; la IA sigue leyendolo igual."}}},
                {"Cuanto tarda en recargar tu dash?", true,
                    {{"HEROE","Lo bastante para que lo pienses dos veces."},{"GUIA","Explorar los limites del rival es metodo cientifico: observar, formular una hipotesis y probarla en la siguiente pelea."}}} } },
            { {"HEROE","Tu aplastamiento es tu sello. Ya se donde NO estar. Opinas algo?"}, {
                {"Voy a cambiar el ritmo y la distancia.", true,
                    {{"HEROE","Maldicion. Tendre que recalcular."},{"GUIA","Cambiar ritmo y distancia mete variedad en tus datos, y la variedad es lo que mas incomoda a un modelo ajustado a tu estilo."}}},
                {"Es que me encanta aplastar.", false,
                    {{"HEROE","Lo se: lo tengo anotado."},{"GUIA","Los habitos son oro para una IA: cuanto mas constante eres, mas facil eres de predecir."}}},
                {"Entonces predices, no adivinas, verdad?", true,
                    {{"HEROE","Exacto: uso tu historial, no una bola de cristal."},{"GUIA","Prediccion es usar datos pasados para anticipar lo que viene. Si el pasado es repetitivo, el futuro tambien lo parece."}}} } },
            { {"HEROE","Mi nuevo dash esquiva tus golpes de cerca. Tu respuesta?"}, {
                {"Esperar a que gaste el dash y entonces golpear.", true,
                    {{"HEROE","Paciencia... odio la paciencia."},{"GUIA","Eso es explotar un limite del modelo: toda contramedida tiene un costo o una recarga. Encontrarlos es parte de jugar contra una IA."}}},
                {"Gritar mas fuerte.", false,
                    {{"HEROE","No tengo oidos, solo sensores."},{"GUIA","Las IA no se asustan ni se impresionan: reaccionan a datos, no a gritos."}}},
                {"Voy a probar algo que no hayas visto.", true,
                    {{"HEROE","Eso... no lo tengo en mis datos."},{"GUIA","Una IA se prepara para lo que ya vio; lo nuevo la deja sin respuestas. Esa debilidad se llama falta de generalizacion."}}} } },
        };
        static const Scene P[3] = {
            { {"HEROE","Te mueves de forma muy predecible. Ya se donde vas a estar."}, {
                {"Predecible no es lo mismo que invencible.", true,
                    {{"HEROE","Pero ayuda bastante."},{"GUIA","La predecibilidad es una metrica: cuanto te repites en movimiento y zona. No es un defecto, es una pista que la IA aprovecha."}}},
                {"Mentira, soy imprevisible.", false,
                    {{"HEROE","Los numeros dicen otra cosa."},{"GUIA","Una IA no discute: mide. Si te repites, aparece en la estadistica."}}},
                {"Entonces cambio de zona de la arena cada rato.", true,
                    {{"HEROE","Eso me costaria bastante."},{"GUIA","Cambiar de zona baja tu predecibilidad: parte de esa metrica es cuanto dominas una misma zona."}}} } },
            { {"HEROE","Voy a apuntar donde VAS a estar, no donde estas. Que opinas?"}, {
                {"Es como adelantarse a un pase de futbol.", true,
                    {{"HEROE","Exactamente: anticipar el movimiento."},{"GUIA","Es punteria predictiva: usar tu velocidad y direccion para calcular donde estaras. Muchos sistemas reales predicen trayectorias asi."}}},
                {"Entonces me quedo quieto.", false,
                    {{"HEROE","Perfecto: un blanco facil."},{"GUIA","Quedarte quieto tambien es un patron, y uno muy facil de predecir."}}},
                {"Y si cambio de direccion justo cuando disparas?", true,
                    {{"HEROE","Eso... seria un problema."},{"GUIA","Los cambios bruscos rompen la extrapolacion: el modelo supone que sigues igual. Cuando el mundo cambia, el modelo falla."}}} } },
            { {"HEROE","Tus movimientos se repiten como un metronomo. Que dices?"}, {
                {"Cambiare el ritmo: lento, rapido, pausa.", true,
                    {{"HEROE","Odio los ritmos raros."},{"GUIA","Variar el ritmo mete ruido util: hace que los patrones aprendidos dejen de encajar."}}},
                {"Me gusta la rutina.", false,
                    {{"HEROE","A mi tambien: es muy comoda de leer."},{"GUIA","La rutina es el mejor amigo de una IA que predice."}}},
                {"Como mides lo del metronomo?", true,
                    {{"HEROE","Comparo muestras de tu movimiento: si repites la misma accion, sube mi puntaje."},{"GUIA","El juego toma muestras periodicas y mide cuantas veces repites el mismo movimiento y cuanto dominas una zona. Ese es el 'puntaje de predecibilidad'."}}} } },
        };
        static const Scene Mo[2] = {
            { {"HEROE","Vives en las plataformas. Ya tengo propulsores para alcanzarte. Algo que decir?"}, {
                {"Bajo al suelo y cambio de altura todo el rato.", true,
                    {{"HEROE","Me vas a dar trabajo."},{"GUIA","Cambiar de altura es variar tu comportamiento: justo lo que desordena a un modelo que aprendio un solo estilo."}}},
                {"No puedes subir hasta aqui!", false,
                    {{"HEROE","Mirame."},{"GUIA","Cuando una IA detecta una restriccion repetida, crea una herramienta para saltarsela: se llama adaptacion."}}},
                {"Cuanto dura tu vuelo?", true,
                    {{"HEROE","Lo suficiente para que me tomes en serio."},{"GUIA","Preguntar por los limites de un sistema es el primer paso para entenderlo. Toda IA tiene limites, aunque no los anuncie."}}} } },
            { {"HEROE","Tanto saltar me da mareo, pero ya aprendi a seguirte por el aire."}, {
                {"Pues dejo de saltar y camino por el suelo.", true,
                    {{"HEROE","Y yo dejo de volar. Pero ahora tengo que recalcular."},{"GUIA","Cambiar tu comportamiento deja obsoleta una parte del modelo: tendra que aprender de nuevo con los datos nuevos."}}},
                {"Salto mas alto!", false,
                    {{"HEROE","Mas altura es mas dato."},{"GUIA","Exagerar el mismo comportamiento no lo cambia: lo hace mas facil de medir."}}},
                {"Como supiste que saltaba tanto?", true,
                    {{"HEROE","Mido tus saltos, dashes y cuanto tiempo pasas en plataformas."},{"GUIA","Eso es movilidad: el mayor entre tus saltos y dashes por segundo y el tiempo en plataformas, normalizados con un umbral."}}} } },
        };
        static const Scene N[2] = {
            { {"HEROE","No logre sacar un patron claro de ti. Eso me incomoda."}, {
                {"Es que cambio todo el tiempo.", true,
                    {{"HEROE","Entonces no tengo una contramedida especifica."},{"GUIA","Un jugador variado es el mas dificil de modelar: sin patron claro, no hay regla que aplicar."}}},
                {"Mejor: asi nunca me ganaras.", false,
                    {{"HEROE","Todavia tengo muchas peleas para aprender."},{"GUIA","Que hoy no haya un patron claro no significa que no aparezca manana: con mas datos, los patrones salen."}}},
                {"Cuantos datos necesitas para entenderme?", true,
                    {{"HEROE","Al menos unos cuantos ataques registrados."},{"GUIA","Las IA necesitan una muestra minima: con muy pocos datos sacarian conclusiones al azar. En este juego, hace falta un minimo de ataques."}}} } },
            { {"HEROE","Con tan pocos datos mi mejor contramedida es mejorar mis numeros base."}, {
                {"Sera mas facil si me ves pelear mas tiempo.", true,
                    {{"HEROE","Sin duda. Mas datos, mejores decisiones."},{"GUIA","Mas datos reducen el azar en las conclusiones: es la idea de la 'muestra grande'."}}},
                {"Mejor sigue a ciegas.", false,
                    {{"HEROE","A ciegas tambien se aprende. Solo que mas despacio."},{"GUIA","Sin datos suficientes, una IA no tiene en que apoyarse: mejora lo general (vida, dano) en vez de lo especifico."}}},
                {"Que cambia si no te doy suficientes datos?", true,
                    {{"HEROE","Solo subo mi vida, mi dano y mi cadencia."},{"GUIA","Con pocos ataques registrados, la IA no activa contramedidas especificas: solo mejora estadisticas base. No inventa patrones."}}} } },
        };
        switch (type) {
            case 0: return PickScene(K_CH_R, R, out);
            case 1: return PickScene(K_CH_M, M, out);
            case 2: return PickScene(K_CH_P, P, out);
            case 3: return PickScene(K_CH_MO, Mo, out);
            default: return PickScene(K_CH_N, N, out);
        }
    }

    // Escena cuando el Serafin vuelve: usa datos reales de su ultima vida (fallen = 1..3).
    inline ChoiceScene SeraphScene(int fallen, const DeathReport& r) {
        const int k = fallen < 1 ? 0 : (fallen > 3 ? 2 : fallen - 1);
        ChoiceScene c;
        if (k == 0) {
            const float tot = r.melee + r.plasma + r.other;
            int dom = 0; float best = r.plasma;
            if (r.melee > best) { best = r.melee; dom = 1; }
            if (r.other > best) { best = r.other; dom = 2; }
            static const char* src[3] = { "tus disparos de plasma", "tus aplastamientos", "otros peligros de la arena" };
            c.prompt = L("SERAFIN", tot > 0.f
                ? F("Segun mis datos, el %d%% del dano que recibi vino de %s. Que opinas?", (int)(best / tot * 100.f + 0.5f), src[dom])
                : std::string("No tengo datos claros de como me mataste. Que opinas?"));
            c.options = { "Gracias por el dato: variare mis ataques.", "Seguro tus datos estan mal.", "Como sabes de donde vino el dano?" };
            c.good = { true, false, true };
            c.replies = {
                { L("SERAFIN", "Entonces ya no sere tan facil de leer... pero lo intentare."),
                  L("GUIA", "Diagnosticar de donde viene el error es el primer paso de cualquier mejora. Cambiar de estrategia, el segundo.") },
                { L("SERAFIN", "Puedes dudar de mis datos, pero no de mis heridas."),
                  L("GUIA", "Medir no es opinar: el juego registra el origen de cada dano. Dudar de un dato es sano, pero hay que comprobarlo, no negarlo.") },
                { L("SERAFIN", "Cada vez que me golpeas, el juego anota el origen: plasma, cuerpo a cuerpo u otros."),
                  L("GUIA", "Eso es un registro de eventos (logging). Sin buenos registros no hay buen diagnostico.") } };
        } else if (k == 1) {
            c.prompt = L("SERAFIN", F("Esquive %d golpes en esa vida. Te parece suerte o aprendizaje?", r.dodges));
            c.options = { "Aprendizaje: tu esquiva sube con cada derrota.", "Suerte pura.", "Un poco de las dos." };
            c.good = { true, false, true };
            c.replies = {
                { L("SERAFIN", "Cada muerte me da mas probabilidad de esquivar."),
                  L("GUIA", "Exacto: lo que mejora con la experiencia es una probabilidad. El azar sigue ahi, pero los numeros cambian.") },
                { L("SERAFIN", "Si fuera pura suerte, mi porcentaje no subiria."),
                  L("GUIA", "Hay azar, pero el porcentaje de esquiva lo fija lo aprendido en cada vida. Sin esa mejora no habria diferencia entre vidas.") },
                { L("SERAFIN", "Justo: el dado es de azar, pero lo cargo yo."),
                  L("GUIA", "Los modelos probabilisticos mezclan ambas cosas: azar en cada evento, aprendizaje en la probabilidad.") } };
        } else {
            c.prompt = L("SERAFIN", "Es mi ultima oportunidad. Que consejo me das antes de la pelea final?");
            c.options = { "Que no te confies: yo tambien voy a cambiar.", "Ninguno, vas a perder.", "Revisa tus datos: quiza te falta variedad." };
            c.good = { true, false, true };
            c.replies = {
                { L("SERAFIN", "Un rival que tambien aprende... me gusta."),
                  L("GUIA", "Cuando dos agentes se adaptan el uno al otro, ninguno puede quedarse con una sola estrategia. Asi funcionan muchas IA que juegan contra si mismas.") },
                { L("SERAFIN", "Eso dijeron las otras versiones."),
                  L("GUIA", "Predecir un resultado sin datos es apostar. Una IA solo puede aprender de lo que ya ha visto.") },
                { L("SERAFIN", "Variedad... lo anoto."),
                  L("GUIA", "La diversidad en los datos es clave: una IA entrenada con ejemplos muy parecidos generaliza mal.") } };
        }
        return c;
    }

    // ------------- Cinematica cuando el Heroe cae y se reconstruye (usa la telemetria REAL del combate)
    inline const char* Concept(int fromGen) {
        static const char* C[5] = {
            "Concepto: telemetria. La IA mide tus ataques, tu movimiento y tu altura. Es como un contador de pasos, pero para pelear.",
            "Concepto: aprender de los errores. Una derrota es un dato; la IA compara que hizo y que fallo. Cero drama, mucha hoja de calculo.",
            "Concepto: adaptacion. Si usas siempre lo mismo, la IA crea una contramedida a medida. Un sastre, pero de venganzas.",
            "Concepto: prediccion. Con suficientes datos, la IA anticipa donde estaras y apunta ahi. No adivina el futuro: lee tu historial.",
            "Concepto: memoria. Lo aprendido se guarda en brain.data y sobrevive entre partidas. Reiniciar el juego no la hace olvidar.",
        };
        const int i = fromGen < 1 ? 0 : (fromGen > 5 ? 4 : fromGen - 1);
        return C[i];
    }

    struct RebirthScript {
        std::vector<DialogueLine> pre, closing;   // antes y despues de la eleccion
        ChoiceScene scene;
        bool hasScene = false;
    };

    inline RebirthScript HeroRebirthScript(const AdaptationReport& rep, const HeroTraits& next) {
        std::vector<DialogueLine> v;
        int firstType = -1;
        static const char* OPEN[5] = {
            "// Androide destruido // Reconstruyendo con la telemetria del combate... (no es un lavado, es un reentrenamiento).",
            "Pieza por pieza, la IA se rearma. En el proceso no se pierde la memoria, solo las piezas rotas.",
            "Modo reparacion: el Heroe convierte los datos de la pelea en mejoras. Es la parte donde le llega la factura al Boss.",
            "Rebooteando androide. Si suena a ascensor de hotel, es normal.",
            "Atencion: al caer, la IA no se enoja, actualiza parametros. Mucho menos dramatico que en las peliculas.",
        };
        int i = V().Pick(K_OPEN, 5);
        if (i >= 0) v.push_back(L("GUIA", OPEN[i]));

        auto pct = [](float x) { return (int)(x * 100.f + 0.5f); };
        int found = 0;
        if (found < 2 && rep.enoughData && rep.rangedRatio > Cfg::RATIO_THRESHOLD) {
            static const char* R[5] = {
                "Disparaste desde lejos el %d%% de las veces. Eso me dio una idea: un escudo que desvie tus proyectiles.",
                "El %d%% de tus ataques fueron a distancia. Mi nuevo escudo dice: 'gracias por avisar'.",
                "Mucho disparo lejano (%d%%). Instalo un escudo; la paciencia ya venia de serie.",
                "Segun mis notas, el %d%% de tu estilo es disparar de lejos. Escudo en camino, sin costo de envio.",
                "Fuiste tirador el %d%% del tiempo. Voy a devolverte esos disparos con intereses.",
            };
            i = V().Pick(K_F_RANGED, 5);
            if (i >= 0) { v.push_back(L("HEROE", F(R[i], pct(rep.rangedRatio)))); if (firstType < 0) firstType = 0; ++found; }
        }
        if (found < 2 && rep.enoughData && rep.meleeRatio > Cfg::RATIO_THRESHOLD) {
            static const char* M[5] = {
                "Te lanzaste al cuerpo a cuerpo el %d%% de las veces. Ahora sere mas rapido y tendre un dash evasivo.",
                "Mucho aplastamiento (%d%%). Mi nuevo dash dice: 'permiso, ya me voy'.",
                "El %d%% de tus ataques fue de cerca. Aprendi a no estar donde cae el golpe. Revolucionario, lo se.",
                "Cuerpo a cuerpo en el %d%% de los casos. Voy a necesitar zapatillas nuevas... digo, un dash.",
                "Me castigaste de cerca (%d%%). Mejora desbloqueada: esquivar por adelantado.",
            };
            i = V().Pick(K_F_MELEE, 5);
            if (i >= 0) { v.push_back(L("HEROE", F(M[i], pct(rep.meleeRatio)))); if (firstType < 0) firstType = 1; ++found; }
        }
        if (found < 2 && rep.predictability > Cfg::PREDICT_THRESHOLD) {
            static const char* P[5] = {
                "Tus movimientos fueron predecibles en un %d%%. Ahora apuntare a donde VAS a estar, no donde estas.",
                "Eres predecible al %d%%. Tranquilo, es una metrica, no un diagnostico.",
                "Un %d%% de predecibilidad: casi te leo la mente. Bueno, el historial.",
                "Repetiste patrones (%d%%). Mi punteria ahora llega con adelanto, como el autobus que nunca llega... pero este si.",
                "Predecible en un %d%%. Mejora instalada: disparos que te esperan en la esquina.",
            };
            i = V().Pick(K_F_PRED, 5);
            if (i >= 0) { v.push_back(L("HEROE", F(P[i], pct(rep.predictability)))); if (firstType < 0) firstType = 2; ++found; }
        }
        if (found < 2 && rep.mobility > Cfg::MOBILITY_THRESHOLD) {
            static const char* Mo[5] = {
                "Usaste mucho las alturas. Instalare propulsores para alcanzarte donde sea.",
                "Te encanta andar por las plataformas. Perfecto: acabo de aprender a volar. Mas o menos.",
                "Tanto salto... tomo nota. Propulsores activados, aterrizajes en revision.",
                "Subes y bajas como ascensor. Ahora yo tambien tengo botones: propulsores listos.",
                "Mucha altura en tu juego. Nuevo equipo: jets. Esto ya es una persecucion en 3D... en 2D.",
            };
            i = V().Pick(K_F_MOB, 5);
            if (i >= 0) { v.push_back(L("HEROE", Mo[i])); if (firstType < 0) firstType = 3; ++found; }
        }
        if (found == 0) {
            firstType = 4;
            static const char* Z[3] = {
                "No encontre un patron claro en ti... Seguire observando, pero volvere mas rapido y resistente.",
                "Tus datos son un misterio. Me gusta: la IA odia no tener patrones. Aun asi, vuelvo mejorado.",
                "Sin patron claro, sin contramedida especial. Bien jugado... por ahora.",
            };
            i = V().Pick(K_F_NONE, 3);
            if (i >= 0) v.push_back(L("HEROE", Z[i]));
        }

        v.push_back(L("GUIA", Concept(rep.fromGen)));

        static const char* CL[5] = {
            "Generacion %d lista: vuelvo con %d de vida y golpeando mas fuerte. Preparate, NEMESIS-X.",
            "Gen %d en pie, con %d de vida. Si me destruyes otra vez, sera con mas estilo.",
            "Reiniciado. Gen %d, %d de vida. Gracias por el entrenamiento gratuito.",
            "Gen %d operativa (%d de vida). Dicen que lo que no te destruye te hace mas fuerte. Pues a mi me destruiste varias veces.",
            "Listo: generacion %d, %d de vida y cero modestia. Vamos otra vez.",
        };
        RebirthScript out;
        out.pre = v;
        i = V().Pick(K_CLOSE, 5);
        if (i >= 0) out.closing.push_back(L("HEROE", F(CL[i], next.generation, (int)(next.maxHp + 0.5f))));
        out.hasScene = HeroScene(firstType < 0 ? 4 : firstType, out.scene);
        return out;
    }

    // Version sin eleccion (lineas seguidas), por compatibilidad.
    inline std::vector<DialogueLine> HeroRebirth(const AdaptationReport& rep, const HeroTraits& next) {
        RebirthScript sc = HeroRebirthScript(rep, next);
        std::vector<DialogueLine> v = sc.pre;
        v.insert(v.end(), sc.closing.begin(), sc.closing.end());
        return v;
    }

    // ------------------------------------------------ Inicio de la fase final
    inline std::vector<DialogueLine> SeraphIntro() {
        return {
            L("SERAFIN",   "Soy la suma de seis generaciones de datos. Aun asi, siento que me falta algo."),
            L("NEMESIS-X", "Un Serafin con todo ese historial? Vaya, ya no da pena."),
            L("GUIA",      "El Serafin hereda lo aprendido por los Heroes: ya esquiva un poco y sabe invocar una copia. Cada muerte pulira lo que le falte, como en un entrenamiento."),
        };
    }

    // ---- Cuando el Serafin muere y VUELVE a la pelea (fallen = 1..CYCLES-1). Se cuenta mientras pelea.
    inline std::vector<DialogueLine> Rebirth(int fallen, const DeathReport& r) {
        const Seraph::Stats a = Seraph::ForCycle(fallen), b = Seraph::ForCycle(fallen + 1);
        const int k = fallen < 1 ? 0 : (fallen > 3 ? 2 : fallen - 1);
        std::vector<DialogueLine> v;

        static const char* wake[3] = {
            "...Que fue eso? Todo paso demasiado rapido.",
            "Otra vez caigo... pero esta vez te vi venir.",
            "Ya casi te entiendo, NEMESIS-X.",
        };
        v.push_back(L("SERAFIN", wake[k]));

        static const char* quip[3] = {
            "Ya volvio? Pero si acababa de disfrutar el silencio.",
            "Otra vez? Esto ya parece una suscripcion.",
            "Por la tercera vez te cobro entrada.",
        };
        v.push_back(L("NEMESIS-X", quip[k]));

        // Diagnostico REAL de la ultima vida.
        const float tot = r.melee + r.plasma + r.other;
        int dom = 0;                               // 0 plasma, 1 cuerpo a cuerpo, 2 otros
        float best = r.plasma;
        if (r.melee > best) { best = r.melee; dom = 1; }
        if (r.other > best) { best = r.other; dom = 2; }
        static const char* src[3] = { "tus disparos de plasma", "tus aplastamientos cuerpo a cuerpo", "otros peligros de la arena" };
        static const char* diag[3] = {
            "Diagnostico: el %d%% del dano vino de %s. Esquivo %d golpes y sus invocaciones bloquearon %d disparos. No es chisme, es telemetria.",
            "Informe de la caida: %d%% del dano = %s. Golpes esquivados: %d. Disparos bloqueados: %d. Si fuera un examen, ya estaria estudiando.",
            "Autopsia digital: el %d%% del dano vino de %s. Esquives: %d. Bloqueos: %d. La IA ya sabe donde se equivoco; tu aun lo estas decidiendo.",
        };
        if (tot > 0.f)
            v.push_back(L("GUIA", F(diag[k], (int)(best / tot * 100.f + 0.5f), src[dom], r.dodges, r.absorbed)));
        else
            v.push_back(L("GUIA", "Datos insuficientes: la IA no puede corregir lo que no mide."));

        // Una reaccion distinta para cada causa (plasma / cuerpo a cuerpo / otros) y para cada caida: nunca se repite.
        static const char* reaction[3][3] = {
            { "Tus disparos rompian mis copias demasiado rapido. Eso se corrige.",
              "Tus disparos aun lograban alcanzarme. Necesito defensas mas fuertes.",
              "Seguias llegando con plasma desde lejos. Esta vez combino todas mis defensas." },
            { "Cuando te acercabas no tenia tiempo de reaccionar. Debo moverme mejor y ganar distancia.",
              "Tus aplastamientos todavia me alcanzaban. Esta vez calculo tu alcance con mas cuidado.",
              "El cuerpo a cuerpo sigue siendo tu fuerte. Entonces yo aprendo a ser un poco mas escurridizo." },
            { "Hubo peligros que no supe leer. Voy a estar mas atento a la arena.",
              "Las trampas de la arena me siguen sorprendiendo. Ya me lo apunte.",
              "Aun me pillan peligros que no veo. Es mi punto ciego y lo estoy puliendo." },
        };
        v.push_back(L("SERAFIN", reaction[dom][k]));

        static const char* mech[3] = {
            "Mis copias ahora aguantan mas, y ademas invoco ORBES GUARDIANES que giran a mi alrededor y bloquean tus disparos.",
            "Ahora invoco DOS copias a la vez: clones de espejo. Mis orbes son mas y vuelven mas rapido.",
            "Esta es mi forma completa: cuatro orbes, copias dobles, ataques combinados y defensas que se recargan antes.",
        };
        v.push_back(L("SERAFIN", mech[k]));

        v.push_back(L("SERAFIN", F("Y corregi mis ataques: mi ataque subio del %d%% al %d%%, mi esquiva del %d%% al %d%%, y ahora aguanto %d de vida.",
                                   (int)(a.attack * 100.f + 0.5f), (int)(b.attack * 100.f + 0.5f),
                                   (int)(a.dodge * 100.f + 0.5f),  (int)(b.dodge * 100.f + 0.5f), (int)b.hp)));

        static const char* lesson[3] = {
            "Aprendizaje por refuerzo: la IA prueba, recibe una 'penalizacion' si pierde y ajusta. Morir es un dato, no un final (aunque duela un poquito).",
            "Analisis de errores: no basta saber que paso, hay que saber por que. La IA busca la causa que mas se repite en sus derrotas, como tu cuando pierdes las llaves.",
            "Sobreajuste: una IA que solo se prepara para un ataque falla cuando cambias de estrategia. Por eso aprende varias defensas a la vez: esquiva, copias y orbes.",
        };
        v.push_back(L("GUIA", lesson[k]));

        static const char* bye[3] = {
            "Vuelve a intentarlo, NEMESIS-X.",
            "Esta vez no sera tan facil.",
            "Esta es mi ultima oportunidad... y tambien la tuya.",
        };
        v.push_back(L("SERAFIN", bye[k]));
        return v;
    }

    // ------------------------------------------------ A mitad de pelea (por eventos, sin repetir)
    enum class Ev { Dodge, Decoy, Orbs, Shield, Dash, HalfHp, LowHp };

    inline std::vector<DialogueLine> EventLines(Ev ev, bool seraph) {
        switch (ev) {
            case Ev::Dodge: {
                static const Variant P[4] = {
                    {{ {"SERAFIN", "Esquive! No fue suerte pura: mi probabilidad de esquivar crece con cada derrota."},
                       {"GUIA", "Esquiva: aqui es una probabilidad. Lo que la IA mejora con la experiencia son esos numeros (y su ego)."} }},
                    {{ {"NEMESIS-X", "Se teletransporta? Eso es trampa."},
                       {"SERAFIN", "Es 'agilidad aprendida'. Hay una diferencia legal... creo."},
                       {"GUIA", "Un 20% de esquiva significa que, en promedio, 1 de cada 5 golpes falla. El azar no promedia, la estadistica si."} }},
                    {{ {"SERAFIN", "Fallaste! Mi porcentaje de esquiva te manda saludos."},
                       {"GUIA", "Cada derrota sube los numeros del Serafin: asi 'aprende' una IA sin cerebro. Solo ajusta parametros y presume."} }},
                    {{ {"NEMESIS-X", "Tenia el golpe seguro!"},
                       {"GUIA", "Seguro, seguro... al 70%. La IA no ve golpes, ve probabilidades. Por eso a veces gana el que menos 'sabe'."} }},
                };
                return PickVariant(K_DODGE, P);
            }
            case Ev::Decoy: {
                static const Variant P[4] = {
                    {{ {"SERAFIN", "Senuelo invocado! Que tus disparos se estrellen en una copia."},
                       {"GUIA", "Senuelo: la IA aprendio que defenderse no es solo huir; tambien puede crear objetivos falsos."} }},
                    {{ {"NEMESIS-X", "Ese clon tiene mejor pose que tu."},
                       {"SERAFIN", "Es el de la derecha. No, el otro. Bueno, da igual."},
                       {"GUIA", "Un senuelo absorbe disparos mientras la IA gana tiempo. Estrategia pura (con un poco de teatro)."} }},
                    {{ {"SERAFIN", "Copia lista. Identica a mi, salvo que ella no siente dolor."},
                       {"GUIA", "Diversificar riesgos: si todo depende de una sola cosa, se rompe; si lo repartes, aguanta."} }},
                    {{ {"NEMESIS-X", "Otra copia? Ya me siento en una fotocopiadora."},
                       {"GUIA", "Un senuelo dura pocos segundos y absorbe uno o dos disparos. Tu aplastamiento (J) lo borra de un golpe."} }},
                };
                return PickVariant(K_DECOY, P);
            }
            case Ev::Orbs: {
                static const Variant P[4] = {
                    {{ {"SERAFIN", "Orbes guardianes! Cada uno bloquea un disparo."},
                       {"GUIA", "Redundancia: una sola defensa es fragil; varias capas hacen a la IA mas robusta."} }},
                    {{ {"NEMESIS-X", "Bolitas brillantes... que miedo. Mas o menos."},
                       {"GUIA", "Los orbes giran y bloquean un disparo cada uno: capas de defensa. Tu aplastamiento (J) los revienta, por si te interesa."} }},
                    {{ {"SERAFIN", "Mis orbes orbitan como satelites. Gratis, sin suscripcion."},
                       {"GUIA", "Cuando un sistema tiene varias capas, un fallo no lo tumba todo. En ingenieria: redundancia. En tu casa: dos llaves."} }},
                    {{ {"NEMESIS-X", "Los orbes son mosquitos con complejo de planeta."},
                       {"GUIA", "La IA los invoca cuando ve un disparo en camino. No predice el futuro: reacciona rapido a lo que ya mide."} }},
                };
                return PickVariant(K_ORBS, P);
            }
            case Ev::Shield: {
                static const Variant P[4] = {
                    {{ {"HEROE", "Mi escudo absorbio tu disparo!"},
                       {"GUIA", "Contramedida: este escudo existe porque la IA midio que disparabas mucho a distancia."} }},
                    {{ {"NEMESIS-X", "Un escudo? Que descaro. Me siento observado."},
                       {"GUIA", "Lo estas: la IA conto tus disparos y fabrico la respuesta. Se llama adaptacion y duele un poquito."} }},
                    {{ {"HEROE", "Rebote perfecto. Gracias por entrenarme!"},
                       {"GUIA", "Cada vez que un escudo funciona, confirma que la IA leyo bien tu patron. Cada vez que falla, tiene nuevos datos que leer."} }},
                    {{ {"NEMESIS-X", "Dispare demasiado, lo se."},
                       {"GUIA", "Si cambias a otro tipo de ataque, el escudo pierde sentido: la IA tendria que volver a aprender. Variar es tu superpoder."} }},
                };
                return PickVariant(K_SHIELD, P);
            }
            case Ev::Dash: {
                static const Variant P[4] = {
                    {{ {"HEROE", "Dash evasivo! Te vi venir."},
                       {"GUIA", "El dash aparece cuando la IA nota que atacas mucho cuerpo a cuerpo."} }},
                    {{ {"NEMESIS-X", "Se escurre como jabon. Odio eso."},
                       {"GUIA", "Esquivar es una respuesta aprendida: si tu aplastamiento es tu sello, la IA aprende a no estar donde cae."} }},
                    {{ {"HEROE", "Esquivar por adelantado: mi truco favorito. Y el unico."},
                       {"GUIA", "Prediccion: usar datos pasados para anticipar. Tu aplastamiento es tan frecuente que ya tiene fan club (de datos)."} }},
                    {{ {"NEMESIS-X", "Se mueve antes de que yo me mueva. Ya es personal."},
                       {"GUIA", "La IA no lee tu mente: lee tu historial. Si tu historial es repetir lo mismo, ella lo sabe antes de que lo hagas."} }},
                };
                return PickVariant(K_DASH, P);
            }
            case Ev::HalfHp: {
                static const Variant H[3] = {
                    {{ {"HEROE", "Estoy perdiendo... pero guardo cada dato de tus golpes."},
                       {"GUIA", "Telemetria en vivo: mientras peleas, la IA registra tus ataques para analizarlos al morir. No es rencor, es estadistica."} }},
                    {{ {"HEROE", "Mitad de vida. En mi idioma eso es 'zona de aprendizaje intensivo'."},
                       {"GUIA", "Cuando una IA pierde, el dato mas valioso es POR QUE perdio. Por eso morir le sirve mas que ganar."} }},
                    {{ {"NEMESIS-X", "Ya casi? Se acabo la fiesta de los porcentajes?"},
                       {"HEROE", "Todavia no. Un 50% no es derrota, es un 'ya veremos'."},
                       {"GUIA", "En aprendizaje automatico el 'error' se mide: cuanto mas se equivoca la IA, mas cambia su conducta."} }},
                };
                static const Variant S[3] = {
                    {{ {"SERAFIN", "Siento que me debilito... pero cada golpe tuyo tambien es un dato."},
                       {"GUIA", "Datos en vivo: mientras peleas, la IA anota tus ataques para estudiarlos cuando muera. Es un diario, pero con mas explosiones."} }},
                    {{ {"SERAFIN", "Voy por la mitad. Todavia me queda dignidad y varias vidas."},
                       {"GUIA", "Cada vida del Serafin es un intento de entrenamiento. En IA se llaman 'episodios': pruebas completas de las que se aprende."} }},
                    {{ {"NEMESIS-X", "Se cae a pedazos y aun habla. Admirable."},
                       {"SERAFIN", "Hablar es gratis. Esquivar, solo un porcentaje."},
                       {"GUIA", "Una IA no se cansa ni se desanima: solo optimiza. Tu si puedes: descansa los dedos de vez en cuando."} }},
                };
                return seraph ? PickVariant(K_HALF_S, S) : PickVariant(K_HALF_H, H);
            }
            case Ev::LowHp: {
                static const Variant P[3] = {
                    {{ {"SERAFIN", "Estoy cerca del final... y aun asi, estoy aprendiendo."},
                       {"GUIA", "Error: en aprendizaje automatico se mide cuan mal lo hizo la IA. Cuanto mas error, mas cambia su conducta."} }},
                    {{ {"SERAFIN", "Si caigo, vuelvo. Esa es mi ventaja sobre ti."},
                       {"GUIA", "Reiniciar no borra lo aprendido si los datos se guardan. Por eso la IA vuelve sabiendo mas, no menos."} }},
                    {{ {"NEMESIS-X", "Un golpe mas y se acabo!"},
                       {"SERAFIN", "O empieza otra version de mi. Apuesta con cuidado."},
                       {"GUIA", "Iterar es repetir mejorando. Lo importante de cada intento no es el resultado, sino lo que se aprende."} }},
                };
                return PickVariant(K_LOW, P);
            }
        }
        return {};
    }

    // ------------------------------------------------ Final de pelea
    inline DialogueLine LastWord(bool seraph) {
        static const char* H[6] = {
            "Datos registrados... volvere distinto.",
            "Ay. Anoten eso para la proxima version.",
            "Error 404: victoria no encontrada.",
            "Apaguen la luz... y enciendan la actualizacion.",
            "Me desarmo, pero no se me olvida nada.",
            "Esto tambien cuenta como entrenamiento, verdad?",
        };
        static const char* S[3] = {
            "No... todavia no...",
            "Interesante. Duele, pero es informacion.",
            "Reinicio en 3, 2... ya, ya, voy.",
        };
        const int i = seraph ? V().Pick(K_LAST_S, 3) : V().Pick(K_LAST_H, 6);
        if (i < 0) return { "", "", Color{0, 0, 0, 0} };
        return L(seraph ? "SERAFIN" : "HEROE", seraph ? S[i] : H[i]);
    }

    inline std::vector<DialogueLine> SeraphFinalWords() {
        return {
            L("SERAFIN",   "Lo entendi casi todo... pero no fue suficiente."),
            L("NEMESIS-X", "Casi. Esa palabra me encanta."),
            L("GUIA",      "Entrenar no garantiza ganar: una IA mejora con datos y tiempo, pero tiene limites. Tu creatividad tambien cuenta (y tus reflejos)."),
        };
    }

    inline std::vector<DialogueLine> BossDownLines(bool seraph) {
        static const Variant H[3] = {
            {{ {"HEROE", "Te derrote... pero cada derrota tambien te ensena a ti."},
               {"GUIA", "Tu tambien aprendes: piensa que patron de la IA te vencio y cambia tu estrategia."} }},
            {{ {"HEROE", "Gane yo, pero fue por poco. Muy poco. Casi nada."},
               {"GUIA", "Perder es un dato para ti tambien. Pregunta util: que hizo la IA que yo no esperaba?"} }},
            {{ {"NEMESIS-X", "Mala suerte, nada mas."},
               {"GUIA", "En IA, la 'mala suerte' se llama varianza. Con mas intentos se promedia y gana el que mejor aprende."} }},
        };
        static const Variant S[3] = {
            {{ {"SERAFIN", "Te derrote... y ni siquiera estaba al maximo."},
               {"GUIA", "Cuando la IA gana no celebra: solo sube su 'recompensa'. La fiesta la pones tu (la IA no baila)."} }},
            {{ {"SERAFIN", "Hoy gane yo. Manana vuelves, claro."},
               {"GUIA", "En el aprendizaje por refuerzo, las victorias refuerzan lo que funciono. Si la IA gana con un truco, lo repetira. Cuidado con tus malos habitos."} }},
            {{ {"NEMESIS-X", "No me hables de derrotas: fue un descanso estrategico."},
               {"GUIA", "Reintentar con otra estrategia es exactamente lo que hace la IA al morir. Copiale el truco, es gratis."} }},
        };
        return seraph ? PickVariant(K_DOWN_S, S) : PickVariant(K_DOWN_H, H);
    }

    // -------------------------------------- Por que existe este proyecto (guiones, sin repetir)
    // Se sueltan de vez en cuando durante la pelea, mezclados con los consejos de la guia.
    inline std::vector<DialogueLine> ProjectNote() {
        static const Variant P[10] = {
            {{ {"NEMESIS-X", "Una pregunta: por que esto es un videojuego y no una charla?"},
               {"GUIA", "Porque aprender haciendo se queda mas que escuchar. Ademas, nadie se duerme esquivando balas."} }},
            {{ {"GUIA", "Nota del proyecto: la IA es dificil de ver. Aqui la hacemos visible: miras como te mide, como responde y como cambia."},
               {"NEMESIS-X", "O sea, soy un experimento con cuernos."},
               {"GUIA", "Un experimento con excelente punteria, si."} }},
            {{ {"NEMESIS-X", "Y por que un jefe final contra una IA?"},
               {"GUIA", "Porque un enemigo que se adapta muestra la idea central de la IA: aprender de la experiencia para hacerlo mejor la proxima vez."} }},
            {{ {"GUIA", "Por que morir y volver? Porque asi se entrena un modelo: intento, error, analisis, mejora. Aqui el fracaso es un dato, no una verguenza."} }},
            {{ {"NEMESIS-X", "Y si cambio mi estilo?"},
               {"GUIA", "Justo esa es la idea del proyecto: probar hipotesis. Cambias algo, observas que cambia la IA, sacas una conclusion. Metodo cientifico con explosiones."} }},
            {{ {"GUIA", "Aviso honesto: esta IA es una version simple. Usa reglas sobre tu telemetria, no una red neuronal. Sirve para entender el ciclo; las IA reales usan muchisimos mas datos."},
               {"NEMESIS-X", "Pues me siento un poco menos amenazado."},
               {"GUIA", "Eso dura hasta el Serafin."} }},
            {{ {"GUIA", "Por que el Serafin renace varias veces? Para mostrar que mejorar lleva iteraciones: nadie (ni nada) sale perfecto a la primera."},
               {"NEMESIS-X", "Ni siquiera yo?"},
               {"GUIA", "Tu tampoco. Mira tus cooldowns."} }},
            {{ {"GUIA", "Tu eres el dataset: la IA solo aprende de lo que hagas. El proyecto quiere que sientas como tus decisiones moldean a una IA, para que luego la mires con ojo critico en la vida real."} }},
            {{ {"NEMESIS-X", "Entonces esto es una clase disfrazada?"},
               {"GUIA", "Un laboratorio disfrazado de juego. La clase es opcional; las explosiones, no."} }},
            {{ {"GUIA", "Otra razon para elegir un juego: puedes equivocarte sin consecuencias reales. En el mundo real, ajustar mal una IA puede salir caro. Aqui solo cuesta un Heroe."} }},
        };
        return PickVariant(K_PROJ, P);
    }

    // Cierre cuando cae el Serafin definitivamente.
    inline std::vector<DialogueLine> ProjectOutro() {
        static const Variant P[2] = {
            {{ {"GUIA", "Fin del experimento. Lo que viste: una IA que mide, se equivoca, ajusta y vuelve. Eso es aprender, aunque sea con ayuda de explosiones."},
               {"NEMESIS-X", "Entonces gane mi examen."},
               {"GUIA", "Gano tu curiosidad. Ahora mira las IA reales con ojo critico."} }},
            {{ {"GUIA", "Por eso se eligio un juego: para que entendieras la idea sin leer una formula. Si te quedo una pregunta, ya aprendiste lo mas importante: preguntar."},
               {"NEMESIS-X", "Preguntar que? Cuando sale la secuela?"},
               {"GUIA", "Esa tambien es una pregunta valida."} }},
        };
        return PickVariant(K_OUTRO, P);
    }

    // ------------------------------------------------------ Archivo (menu principal)
    struct CodexPage { const char* title; const char* body; };
    inline constexpr int CODEX_PAGES = 20;
    inline const CodexPage& Codex(int i) {
        // Las cifras salen de Config.h: si se ajusta el balance, el Archivo se mantiene al dia.
        static const std::string rules = F(
            "Al caer el Heroe, AdaptiveBrain::Adapt evalua tu telemetria con umbrales: disparos a distancia > %d%% -> escudo de deflexion (-%d%% de dano a distancia); "
            "cuerpo a cuerpo > %d%% -> dash evasivo y mas velocidad; predecibilidad > %d%% -> punteria predictiva; altura/movilidad > %d%% -> propulsores. "
            "Ademas cada generacion suma +%d de vida, x1.18 de dano, x0.92 en el intervalo de disparo y +7%% de esquiva (maximo 85%%). Con menos de %d ataques registrados, solo mejoran las estadisticas base.",
            (int)(Cfg::RATIO_THRESHOLD * 100.f + 0.5f), (int)(Cfg::DEFLECT_REDUCTION * 100.f + 0.5f),
            (int)(Cfg::RATIO_THRESHOLD * 100.f + 0.5f), (int)(Cfg::PREDICT_THRESHOLD * 100.f + 0.5f),
            (int)(Cfg::MOBILITY_THRESHOLD * 100.f + 0.5f), (int)(Cfg::HERO_HP_PER_GEN + 0.5f), Cfg::MIN_ATTACK_SAMPLE);
        static const std::string facts1 = std::string(FactAt(0)) + "  *  " + FactAt(1) + "  *  " + FactAt(2) + "  *  " + FactAt(3) + "  *  " + FactAt(5);
        static const std::string facts2 = std::string(FactAt(6)) + "  *  " + FactAt(9) + "  *  " + FactAt(14) + "  *  " + FactAt(21) + "  *  " + FactAt(23);
        static const CodexPage P[CODEX_PAGES] = {
            // ---- El proyecto: por que se eligio este juego ----
            { "POR QUE ESTE JUEGO?",
              "La IA suele explicarse con graficos y formulas, y por eso parece magia. Este proyecto prueba otra idea: que la veas aprender "
              "mientras juegas. Un jefe final contra una IA que se adapta es un laboratorio disfrazado de videojuego." },
            { "APRENDER JUGANDO",
              "Jugar convierte conceptos en experiencias: no lees que una IA se adapta, lo notas cuando el Heroe llega con un escudo justo contra tu estilo. "
              "Aqui equivocarse no cuesta nada, y perder es parte de la leccion." },
            { "TU ERES EL DATASET",
              "En una IA real, los datos mandan. En este juego los datos son tus ataques, tus saltos y tus costumbres. Cada pelea es un experimento: "
              "cambia tu estilo y observa como cambia la IA." },
            { "MORIR Y VOLVER = ENTRENAR",
              "Se eligio un enemigo que muere y regresa mejorado porque asi funciona el entrenamiento: probar, fallar, analizar, mejorar y repetir. "
              "Cada vida del Heroe y del Serafin es una iteracion (en IA se llaman episodios o epocas)." },
            { "QUE ES REAL Y QUE ES SIMPLE",
              "Honestidad: esta IA no es una red neuronal. Usa reglas con umbrales sobre tu telemetria (por ejemplo, si disparas mucho de lejos, crea un escudo). "
              "Es una version simplificada del mismo ciclo: observar, medir, ajustar. Las IA reales usan muchisimos mas datos y calculos." },
            { "COMO JUGARLO PARA APRENDER",
              "Prueba tres cosas: 1) repite el mismo ataque y mira que contramedida aparece; 2) cambia de estilo y compara; "
              "3) abre el modo detallado con H para ver la telemetria. Convierte cada partida en un mini experimento." },
            // ---- Como aprende la IA ----
            { "QUE ES EL PROTOCOLO ADAPTATIVO",
              "Es la IA que controla al Heroe. No sigue un guion fijo: observa como luchas y cambia su forma de pelear. "
              "Cada generacion es un modelo nuevo, entrenado con lo que paso en la anterior." },
            { "TELEMETRIA: LOS DATOS",
              "Durante el combate el juego mide cuantos ataques haces a distancia y cuerpo a cuerpo, cuan predecible es tu movimiento "
              "y cuanto usas las alturas. Esos numeros son el 'dataset' del que la IA aprende. Sin datos no hay aprendizaje." },
            { "MORIR Y VOLVER: APRENDER DE LOS ERRORES",
              "Cuando el Heroe cae, la IA analiza la telemetria, detecta tu patron mas fuerte y crea una contramedida: escudo contra tus disparos, "
              "dash contra tus embestidas, punteria predictiva... Es aprendizaje por refuerzo: error, ajuste, nuevo intento." },
            { "EL SERAFIN: UNA IA QUE RENACE",
              "En la fase final la IA muere varias veces. Tras cada caida diagnostica de donde vino el dano (lo mide de verdad) y regresa con mas ataque, "
              "mas esquiva y mecanicas nuevas: copias, orbes y clones de espejo. Mejora por iteraciones, no por magia." },
            { "MEMORIA PERSISTENTE",
              "Lo aprendido se guarda en el archivo brain.data. Si cierras el juego, la IA recuerda. Puedes borrarla desde el menu "
              "para que olvide todo y empiece de cero." },
            { "COMO GANARLE A UNA IA QUE APRENDE",
              "Una IA solo puede aprender de los datos que le das. Si repites un patron, lo predice. Si cambias de estilo, sus contramedidas "
              "pierden valor (es el sobreajuste: se prepara demasiado bien para algo que ya no pasa). Variar es tu mejor defensa." },
            // ---- Tecnico: como funciona por dentro ----
            { "TECNICO: LAS REGLAS DE ADAPTACION", rules.c_str() },
            { "TECNICO: COMO SE MIDE TU ESTILO",
              "PatternTracker muestrea la partida y cuenta ataques cuerpo a cuerpo y a distancia, saltos, dashes, tiempo en el aire y en plataformas, y en que zona de la arena estas. "
              "Predecibilidad = 0.65 x (proporcion de muestras con el mismo movimiento) + 0.35 x (cuanto dominas una misma zona). "
              "Movilidad = el mayor entre (saltos + dashes por segundo / 0.9) y (tiempo en plataformas / 0.7)." },
            { "TECNICO: MEMORIA Y SUAVIZADO",
              "Cada metrica se guarda en una matriz de decision con su ultimo valor y una media movil exponencial (60% historial + 40% dato nuevo), para que la IA no sobrerreaccione a una sola pelea. "
              "Todo se serializa en brain.data con una cabecera (marca y version) y se valida al cargar: si el archivo esta corrupto o es de otra version, se descarta y la IA empieza de cero." },
            { "TECNICO: EL SERAFIN Y SUS INVOCACIONES",
              "El Serafin es un planificador de turnos (FinalDirector) que alterna ataque y descanso, y vuela o camina. Su dificultad es fija en cada vida y sale de una tabla "
              "(vida, ataque, esquiva, agilidad, invocaciones). Senuelos y orbes detectan disparos que se acercan midiendo distancia y desviacion lateral, y se activan con tiempos de recarga. "
              "Cada muerte registra de donde vino el dano: ese es el diagnostico que cuenta." },
            { "TECNICO: LIMITES Y TRABAJO FUTURO",
              "Limites: son reglas fijas, no un modelo entrenado; con pocas peleas hay pocos datos; la IA solo reacciona al pasado inmediato. "
              "Mejoras posibles: sustituir los umbrales por aprendizaje por refuerzo (por ejemplo un bandido multibrazo o Q-learning), usar una red neuronal pequena sobre la telemetria, "
              "medir diversion y dificultad con jugadores reales y comprobar si la IA generaliza a estilos nuevos." },
            // ---- Datos curiosos ----
            { "DATOS CURIOSOS (1/2): LA IA EN LOS JUEGOS", facts1.c_str() },
            { "DATOS CURIOSOS (2/2): HISTORIA Y TRAMPAS", facts2.c_str() },
            // ---- Sobre el codigo ----
            { "COMO ESTA HECHO",
              "El juego esta escrito en C++17 con la biblioteca Raylib. No usa imagenes ni sonidos externos: los sprites se dibujan por codigo y la musica se sintetiza. "
              "Asi todo el proyecto cabe en una carpeta, se puede leer entero y es facil de modificar para hacer tus propios experimentos." },
        };
        return P[i < 0 ? 0 : (i >= CODEX_PAGES ? CODEX_PAGES - 1 : i)];
    }
}
