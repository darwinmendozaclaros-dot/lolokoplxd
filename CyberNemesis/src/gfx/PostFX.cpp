#include "gfx/PostFX.h"
#include "gfx/ShaderSource.h"
#include "core/Config.h"
#include "rlgl.h"
#include <string>
#include <vector>

bool PostFX::Init() {
    target_ = LoadRenderTexture(Cfg::VIEW_W, Cfg::VIEW_H);
    SetTextureFilter(target_.texture, TEXTURE_FILTER_POINT);

    const std::string appDir = GetApplicationDirectory();
    const std::vector<std::string> paths = {
        appDir + "assets/shaders/postfx.fs",
        "assets/shaders/postfx.fs",
    };
    for (const auto& p : paths) {
        if (!FileExists(p.c_str())) continue;
        shader_ = LoadShader(nullptr, p.c_str());
        shaderOk_ = (shader_.id != rlGetShaderIdDefault());
        if (shaderOk_) break;
    }
    if (!shaderOk_) {
        shader_ = LoadShaderFromMemory(nullptr, Embedded::POSTFX_FRAGMENT);
        shaderOk_ = (shader_.id != rlGetShaderIdDefault());
    }

    if (shaderOk_) {
        locRes_    = GetShaderLocation(shader_, "resolution");
        locTime_   = GetShaderLocation(shader_, "time");
        locGlitch_ = GetShaderLocation(shader_, "glitch");
        locFx_     = GetShaderLocation(shader_, "fx");
        float res[2] = { (float)Cfg::VIEW_W, (float)Cfg::VIEW_H };
        SetShaderValue(shader_, locRes_, res, SHADER_UNIFORM_VEC2);
    } else {
        TraceLog(LOG_WARNING, "CYBER-NEMESIS: no se pudo compilar el shader (el juego sigue sin efectos)");
    }
    return shaderOk_;
}

void PostFX::Shutdown() {
    if (shaderOk_) UnloadShader(shader_);
    UnloadRenderTexture(target_);
}

void PostFX::BeginScene() { BeginTextureMode(target_); }
void PostFX::EndScene()   { EndTextureMode(); }

void PostFX::Present(Rectangle dest, float time, float glitch) const {
    if (shaderOk_) {
        SetShaderValue(shader_, locTime_, &time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader_, locGlitch_, &glitch, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader_, locFx_, &intensity_, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(shader_);
    }
    Rectangle src = { 0.f, 0.f, (float)target_.texture.width, -(float)target_.texture.height };
    DrawTexturePro(target_.texture, src, dest, { 0.f, 0.f }, 0.f, WHITE);
    if (shaderOk_) EndShaderMode();
}
