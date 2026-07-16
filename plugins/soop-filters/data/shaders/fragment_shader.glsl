#version 450

layout(location = 0) in vec2 frag_uv;

layout(location = 0) out vec4 out_color;

uniform sampler2D tex;

void main() {
    out_color = texture(tex, frag_uv);
}
