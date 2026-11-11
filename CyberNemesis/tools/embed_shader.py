import pathlib

root = pathlib.Path(__file__).resolve().parent.parent
src = (root / "assets/shaders/postfx.fs").read_text(encoding="utf-8")
assert ')CNSHADER"' not in src
out = (
    "#pragma once\n"
    "namespace Embedded {\n"
    'inline constexpr const char* POSTFX_FRAGMENT = R"CNSHADER(' + src + ')CNSHADER";\n'
    "}\n"
)
(root / "src/gfx/ShaderSource.h").write_text(out, encoding="utf-8")
print("OK", len(src), "bytes")
