#vertex

out vec4 frag_color;
out vec2 world_pos;

/*
uniform vec4 u_light_origin;
uniform float u_light_radius;
*/

void main()
{
    set_position();

    frag_color = a_color;
    world_pos = a_world_pos;
}

#fragment

in vec4 frag_color;
in vec2 world_pos;

out vec4 final_color;

uniform vec4 u_light_origin;
uniform float u_light_radius;

void main()
{
    // TODO: it would be more efficient to use a stencil buffer to clip
    // a circle around the light origin
    // TODO: improve the look of this
    float dist = distance(u_light_origin.xy, world_pos);
    vec2 light_origin = u_light_origin.xy;
    float d = sqrt(dist / u_light_radius);

    final_color = frag_color;
    final_color.a *= 1.0f - d;
}
