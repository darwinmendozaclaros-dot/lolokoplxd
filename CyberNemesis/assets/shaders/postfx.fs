#version 330
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2  resolution;
uniform float time;
uniform float glitch;
uniform float fx;

out vec4 finalColor;

vec2 curve(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    vec2 off = abs(uv.yx) / vec2(8.0, 6.5) * fx;
    uv = uv + uv * off * off;
    return uv * 0.5 + 0.5;
}

void main() {
    vec2 uv = curve(fragTexCoord);
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float g = glitch * fx;
    float band = floor(uv.y * 40.0);
    float n = fract(sin(band * 12.9898 + floor(time * 18.0) * 78.233) * 43758.5453);
    uv.x += (n - 0.5) * 0.030 * g * step(0.55, n);

    float ca = (0.0010 + g * 0.004) * fx;
    vec3 col;
    col.r = texture(texture0, uv + vec2(ca, 0.0)).r;
    col.g = texture(texture0, uv).g;
    col.b = texture(texture0, uv - vec2(ca, 0.0)).b;

    if (fx > 0.01) {
        vec2 px = 1.0 / resolution;
        vec3 bloom = vec3(0.0);
        for (int i = -3; i <= 3; i++) {
            for (int j = -3; j <= 3; j++) {
                vec3 s = texture(texture0, uv + vec2(float(i), float(j)) * px * 1.6).rgb;
                float w = exp(-float(i * i + j * j) * 0.16);
                bloom += max(s - 0.55, 0.0) * w;
            }
        }
        col += bloom * 0.24 * fx;
    }

    float sl = 0.90 + 0.10 * sin(uv.y * resolution.y * 6.28318);
    col *= mix(1.0, sl, fx);
    col *= mix(1.0, 0.985 + 0.015 * sin(time * 60.0), fx);

    vec2 v = uv * (1.0 - uv);
    float vig = pow(clamp(v.x * v.y * 18.0, 0.0, 1.0), 0.28);
    col *= mix(1.0, mix(0.55, 1.0, vig), fx);

    finalColor = vec4(col, 1.0);
}
