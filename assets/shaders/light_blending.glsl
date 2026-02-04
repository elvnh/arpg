#vertex

out vec2 tex_coord;

void main()
{
    // NOTE: a_world_pos here should actually be in screen space
    gl_Position = vec4(a_world_pos.x, a_world_pos.y, 0.0f, 1.0f);

    tex_coord = a_uv;
}

#fragment

uniform sampler2D u_texture_a;
uniform sampler2D u_texture_b;

in vec2 tex_coord;

out vec4 final_color;

void main()
{
    vec4 a = texture(u_texture_a, tex_coord);
    vec4 b = texture(u_texture_b, tex_coord);

    final_color = a * b;
}
