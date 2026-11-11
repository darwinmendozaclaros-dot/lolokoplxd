#pragma once
#include "gfx/Dialogue.h"
#include <string>
#include <vector>

// Caja de eleccion: la IA dice algo y el jugador (NEMESIS-X) elige que responder.
// W/S o flechas para moverse, 1-3 para elegir directo, ENTER para confirmar.
class ChoiceBox {
public:
    void Start(const DialogueLine& prompt, const std::vector<std::string>& options);
    bool Active() const { return active_; }
    void Update(float dt);
    bool Chosen(int& idx);                       // true UNA sola vez cuando el jugador elige
    bool ConsumeBlip() { bool r = blip_; blip_ = false; return r; }
    bool ConsumeMove() { bool r = moved_; moved_ = false; return r; }
    void Draw(float time, int y, bool seraphMode) const;

private:
    DialogueLine                          prompt_;
    std::vector<std::string>              promptRows_;
    std::vector<std::vector<std::string>> rows_;
    bool  active_ = false, blip_ = false, moved_ = false;
    int   sel_ = 0, picked_ = -1, total_ = 0, lastBlip_ = 0;
    float t_ = 0.f, shown_ = 0.f;
};
