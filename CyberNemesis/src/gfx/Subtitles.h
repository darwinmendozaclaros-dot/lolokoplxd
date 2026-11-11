#pragma once
#include "gfx/Dialogue.h"
#include <deque>
#include <string>
#include <vector>

// Dialogos "en vivo": aparecen abajo mientras se pelea, sin pausar el juego ni pedir ninguna tecla.
// Se encolan, se escriben letra por letra y desaparecen solos (ENTER salta la linea actual).
class Subtitles {
public:
    void Clear();
    void Say(const std::string& speaker, const std::string& text, Color c, float delay = 0.f, float dur = -1.f);
    void SayAll(const std::vector<DialogueLine>& lines, float firstDelay = 0.f);
    void Skip();
    void Update(float dt);
    bool Busy() const { return has_ || !queue_.empty(); }
    bool ConsumeBlip() { bool r = blip_; blip_ = false; return r; }
    void SetSeraph(bool on) { seraph_ = on; }          // el icono de la IA pasa a ser el Serafin
    void Draw(int y = 302) const;

private:
    struct Item { DialogueLine line; float delay; float dur; };
    void Begin(const Item& it);

    std::deque<Item>          queue_;
    DialogueLine              cur_{};
    std::vector<std::string>  rows_;
    bool  has_ = false, blip_ = false, seraph_ = false;
    float t_ = 0.f, dur_ = 0.f, shown_ = 0.f, gap_ = 0.f;
    int   total_ = 0, lastBlip_ = 0;
};
