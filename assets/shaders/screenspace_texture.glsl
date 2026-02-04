#vertex

out vec2 tex_coord;

void main()
{
    // NOTE: a_world_pos here should actually be in screen space
    gl_Position = vec4(a_world_pos.x, a_world_pos.y, 0.0f, 1.0f);

    tex_coord = a_uv;
}

#fragment

uniform sampler2D u_texture;

in vec2 tex_coord;

out vec4 final_color;

void main()
{
    final_color = texture(u_texture, tex_coord);
}
