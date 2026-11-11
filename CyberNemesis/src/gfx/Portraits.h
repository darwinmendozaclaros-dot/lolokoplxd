#pragma once
#include "raylib.h"
#include <string>

// Quien habla: el jugador (NEMESIS-X), la IA (Heroe / Serafin) o la guia (GUIA).
enum class Role { Player, AI, Guide };
Role RoleOf(const std::string& speaker);

constexpr int PORTRAIT_STRIP_W = 86;   // 3 retratos de 26 px + separaciones

// Dibuja los tres retratos a la vez; el que habla se ilumina (y salta un poco) y los otros se atenuan.
// Debajo del retrato activo escribe el nombre. seraphMode: el icono de la IA es el Serafin en vez del Heroe.
void DrawPortraitStrip(int x, int y, const std::string& speaker, bool seraphMode, float time, float alpha);
