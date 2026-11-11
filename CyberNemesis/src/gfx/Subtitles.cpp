#include "gfx/Subtitles.h"
#include "gfx/HUD.h"
#include "gfx/Portraits.h"
#include "core/Config.h"
#include "core/Util.h"
#include <algorithm>

void Subtitles::Clear() {
    queue_.clear();
    has_ = false; blip_ = false;
    gap_ = 0.f;
}

void Subtitles::Say(const std::string& speaker, const std::string& text, Color c, float delay, float dur) {
    if (!has_ && queue_.empty()) gap_ = delay;
    queue_.push_back({ { speaker, text, c }, delay, dur });
}

void Subtitles::SayAll(const std::vector<DialogueLine>& lines, float firstDelay) {
    bool first = true;
    for (const auto& l : lines) {
        Say(l.speaker, l.text, l.color, first ? firstDelay : 0.5f);
        first = false;
    }
}

void Subtitles::Begin(const Item& it) {
    cur_ = it.line;
    rows_ = HUD::Wrap(cur_.text, Cfg::VIEW_W - 48 - PORTRAIT_STRIP_W - 30, 10);
    total_ = 0;
    for (const auto& r : rows_) total_ += (int)r.size();
    shown_ = 0.f; t_ = 0.f; lastBlip_ = 0;
    dur_ = it.dur > 0.f ? it.dur : std::max(3.2f, 1.8f + (float)cur_.text.size() * 0.04f);
    has_ = true;
}

void Subtitles::Skip() {
    if (!has_) return;
    if ((int)shown_ < total_) shown_ = (float)total_;     // primero completa el texto
    else t_ = dur_;                                       // luego pasa a la siguiente
}

void Subtitles::Update(float dt) {
    if (!has_) {
        if (queue_.empty()) return;
        gap_ -= dt;
        if (gap_ > 0.f) return;
        const Item it = queue_.front();
        queue_.pop_front();
        Begin(it);
        return;
    }
    t_ += dt;
    if ((int)shown_ < total_) {
        shown_ = std::min((float)total_, shown_ + dt * 55.f);
        if ((int)shown_ - lastBlip_ >= 4) { blip_ = true; lastBlip_ = (int)shown_; }
    }
    if (t_ >= dur_) {
        has_ = false;
        gap_ = queue_.empty() ? 0.f : queue_.front().delay;
    }
}

void Subtitles::Draw(int y) const {
    if (!has_) return;
    const float a = Util::Clamp01(t_ / 0.25f) * Util::Clamp01((dur_ - t_) / 0.3f);
    if (a <= 0.f) return;
    const int x = 24, w = Cfg::VIEW_W - 48, h = 42;
    const int textX = x + 8 + PORTRAIT_STRIP_W + 12;

    DrawRectangle(x, y, w, h, Fade(Color{6, 4, 18, 255}, 0.84f * a));
    DrawRectangleLines(x, y, w, h, Fade(cur_.color, 0.85f * a));
    DrawPortraitStrip(x + 8, y + 2, cur_.speaker, seraph_, t_, a);       // jugador, IA y guia a la vez

    int remaining = (int)shown_;
    for (size_t i = 0; i < rows_.size() && i < 3; ++i) {
        const int n = std::min((int)rows_[i].size(), remaining);
        if (n <= 0) break;
        HUD::Text(rows_[i].substr(0, (size_t)n).c_str(), textX, y + 6 + (int)i * 12, 10, Fade(WHITE, a));
        remaining -= n;
    }
}
