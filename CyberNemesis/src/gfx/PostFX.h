#pragma once
#include "raylib.h"

class PostFX {
public:
    bool Init();
    void Shutdown();
    void BeginScene();
    void EndScene();
    void SetIntensity(float v) { intensity_ = v; }
    float Intensity() const { return intensity_; }
    void Present(Rectangle dest, float time, float glitch) const;

private:
    RenderTexture2D target_{};
    Shader shader_{};
    bool   shaderOk_ = false;
    int    locRes_ = -1, locTime_ = -1, locGlitch_ = -1, locFx_ = -1;
    float  intensity_ = 0.35f;
};
