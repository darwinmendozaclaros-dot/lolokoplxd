#include "gfx/Dialogue.h"
#include "gfx/HUD.h"
#include "gfx/Portraits.h"
#include "core/Config.h"
#include <algorithm>
#include <cmath>

int Dialogue::CountChars(int i) const {
    int n = 0;
    for (const auto& ln : wrapped_[(size_t)i]) n += (int)ln.size();
    return n;
}

void Dialogue::Start(std::vector<DialogueLine> lines) {
    lines_ = std::move(lines);
    wrapped_.clear();
    for (const auto& l : lines_) wrapped_.push_back(HUD::Wrap(l.text, Cfg::VIEW_W - 32 - PORTRAIT_STRIP_W - 40, 10));
    idx_ = 0; shown_ = 0.f; lastBlip_ = 0; blip_ = false;
    total_ = lines_.empty() ? 0 : CountChars(0);
}

void Dialogue::Update(float dt) {
    if (!Active() || (int)shown_ >= total_) return;
    shown_ = std::min((float)total_, shown_ + dt * 48.f);
    if ((int)shown_ - lastBlip_ >= 3) { blip_ = true; lastBlip_ = (int)shown_; }
}

bool Dialogue::Advance() {
    if (!Active()) return true;
    if ((int)shown_ < total_) { shown_ = (float)total_; return false; }   // primero completa el texto
    ++idx_;
    shown_ = 0.f; lastBlip_ = 0;
    if (!Active()) return true;
    total_ = CountChars(idx_);
    return false;
}

void Dialogue::Draw(float time, int boxY) const {
    if (!Active()) return;
    const DialogueLine& L = lines_[(size_t)idx_];
    const int x = 16, w = Cfg::VIEW_W - 32, h = 104;

    DrawRectangle(x, boxY, w, h, Fade(Color{6, 4, 18, 255}, 0.90f));
    DrawRectangleLines(x, boxY, w, h, Fade(L.color, 0.9f));
    DrawRectangle(x, boxY, 3, h, L.color);

    DrawPortraitStrip(x + 10, boxY + 30, L.speaker, false, time, 1.f);      // jugador, IA y guia a la vez

    int remaining = (int)shown_;
    const auto& rows = wrapped_[(size_t)idx_];
    for (size_t i = 0; i < rows.size(); ++i) {
        const int n = std::min((int)rows[i].size(), remaining);
        if (n <= 0) break;
        HUD::Text(rows[i].substr(0, (size_t)n).c_str(), x + PORTRAIT_STRIP_W + 26, boxY + 16 + (int)i * 13, 10, WHITE);
        remaining -= n;
    }

    HUD::Text(TextFormat("%d/%d", idx_ + 1, (int)lines_.size()), x + w - 40, boxY + h - 14, 10, Fade(L.color, 0.6f));
    if ((int)shown_ >= total_ && std::fmod(time, 1.f) < 0.65f)
        HUD::Text("[ENTER]", x + w - 100, boxY + h - 14, 10, WHITE);
}
