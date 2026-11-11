#include "gfx/ChoiceBox.h"
#include "gfx/HUD.h"
#include "gfx/Portraits.h"
#include "core/Config.h"
#include <algorithm>
#include <cmath>

namespace {
    const Color CHOICE_MAG{255, 43, 214, 255};
    constexpr int BOX_X = 24;
}

void ChoiceBox::Start(const DialogueLine& prompt, const std::vector<std::string>& options) {
    prompt_ = prompt;
    const int w = Cfg::VIEW_W - 48;
    promptRows_ = HUD::Wrap(prompt.text, w - PORTRAIT_STRIP_W - 40, 10);
    rows_.clear();
    for (const auto& o : options) rows_.push_back(HUD::Wrap(o, w - 64, 10));
    total_ = 0;
    for (const auto& r : promptRows_) total_ += (int)r.size();
    sel_ = 0; picked_ = -1; lastBlip_ = 0;
    t_ = 0.f; shown_ = 0.f;
    active_ = true; blip_ = moved_ = false;
}

void ChoiceBox::Update(float dt) {
    if (!active_) return;
    t_ += dt;
    if ((int)shown_ < total_) {
        shown_ = std::min((float)total_, shown_ + dt * 55.f);
        if ((int)shown_ - lastBlip_ >= 4) { blip_ = true; lastBlip_ = (int)shown_; }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) shown_ = (float)total_;   // ENTER completa el texto
        return;
    }
    if (t_ < 0.6f) return;
    const int n = (int)rows_.size();
    if (n == 0) { picked_ = 0; return; }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { sel_ = (sel_ + 1) % n; moved_ = true; }
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) { sel_ = (sel_ + n - 1) % n; moved_ = true; }
    for (int i = 0; i < n && i < 9; ++i)
        if (IsKeyPressed(KEY_ONE + i)) { sel_ = i; picked_ = i; }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) picked_ = sel_;
}

bool ChoiceBox::Chosen(int& idx) {
    if (!active_ || picked_ < 0) return false;
    idx = picked_;
    active_ = false;
    return true;
}

void ChoiceBox::Draw(float time, int y, bool seraphMode) const {
    if (!active_) return;
    const int x = BOX_X, w = Cfg::VIEW_W - 48;
    const bool opts = (int)shown_ >= total_;

    int h = 58 + 16;                                            // cabecera con el dialogo de la IA + titulo de opciones
    for (const auto& r : rows_) h += (int)r.size() * 12 + 7;
    h += 8;

    DrawRectangle(x, y, w, h, Fade(Color{6, 4, 18, 255}, 0.93f));
    DrawRectangleLines(x, y, w, h, Fade(opts ? CHOICE_MAG : prompt_.color, 0.9f));
    DrawPortraitStrip(x + 8, y + 4, opts ? std::string("NEMESIS-X") : prompt_.speaker, seraphMode, time, 1.f);

    int remaining = (int)shown_;
    for (size_t i = 0; i < promptRows_.size() && i < 3; ++i) {
        const int n = std::min((int)promptRows_[i].size(), remaining);
        if (n <= 0) break;
        HUD::Text(promptRows_[i].substr(0, (size_t)n).c_str(), x + 8 + PORTRAIT_STRIP_W + 12, y + 8 + (int)i * 12, 10, WHITE);
        remaining -= n;
    }
    if (!opts) return;

    DrawRectangle(x + 8, y + 56, w - 16, 1, Fade(CHOICE_MAG, 0.5f));
    HUD::Text("ELIGE TU RESPUESTA   (W/S + ENTER, o pulsa 1-3)", x + 10, y + 61, 10, Fade(CHOICE_MAG, 0.95f));

    int oy = y + 76;
    for (size_t i = 0; i < rows_.size(); ++i) {
        const bool on = ((int)i == sel_);
        const int hh = (int)rows_[i].size() * 12 + 4;
        if (on) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawRectangleGradientH(x + 6, oy - 2, w - 12, hh, Fade(CHOICE_MAG, 0.32f), Fade(CHOICE_MAG, 0.f));
            EndBlendMode();
            DrawRectangle(x + 6, oy - 2, 2, hh, CHOICE_MAG);
        }
        HUD::Text(TextFormat("%d)", (int)i + 1), x + 16 + (on ? (int)(2.f * std::sin(time * 9.f)) : 0), oy, 10, on ? WHITE : Fade(CHOICE_MAG, 0.7f));
        for (size_t r = 0; r < rows_[i].size(); ++r)
            HUD::Text(rows_[i][r].c_str(), x + 40, oy + (int)r * 12, 10, on ? WHITE : Fade(WHITE, 0.62f));
        oy += (int)rows_[i].size() * 12 + 7;
    }
}
