#vertex
#version 330

// NOTE: these should be shared between all shaders, put in common file
layout (location = 0) in vec2 a_world_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

out vec2 tex_coord;
out vec4 frag_color;

uniform mat4 u_proj;

void main()
{
    gl_Position.xy = (u_proj * vec4(a_world_pos.xy, 0.0f, 1.0f)).xy;
    gl_Position.zw = vec2(0.0f, 1.0f);

    tex_coord = a_uv;
    frag_color = a_color;
}

#fragment
#version 330

in vec2 tex_coord;
in vec4 frag_color;

out vec4 final_color;

uniform sampler2D u_texture;

void main()
{
    final_color = texture(u_texture, tex_coord) * frag_color;
}
