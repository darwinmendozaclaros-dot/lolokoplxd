#pragma once
#include "raylib.h"
#include <string>
#include <vector>

struct DialogueLine {
    std::string speaker;   // "SERAFIN", "SISTEMA", "DATO DE IA"...
    std::string text;
    Color       color;
};

// Cuadro de dialogo con efecto maquina de escribir. ENTER completa el texto o pasa a la siguiente linea.
class Dialogue {
public:
    void Start(std::vector<DialogueLine> lines);
    void Update(float dt);
    bool Advance();                       // true cuando ya no quedan lineas
    bool Active() const { return idx_ < (int)lines_.size(); }
    bool ConsumeBlip() { bool r = blip_; blip_ = false; return r; }
    void Draw(float time, int boxY) const;

private:
    int  CountChars(int i) const;

    std::vector<DialogueLine>               lines_;
    std::vector<std::vector<std::string>>   wrapped_;
    int   idx_ = 0, total_ = 0, lastBlip_ = 0;
    float shown_ = 0.f;
    bool  blip_ = false;
};
