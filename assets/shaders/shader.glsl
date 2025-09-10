#vertex

out vec2 tex_coord;
out vec4 frag_color;

void main()
{
    set_position();

    tex_coord = a_uv;
    frag_color = a_color;
}

#fragment

in vec2 tex_coord;
in vec4 frag_color;

out vec4 final_color;

uniform sampler2D u_texture;

void main()
{
    final_color = texture(u_texture, tex_coord) * frag_color;
}
