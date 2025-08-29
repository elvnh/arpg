#vertex
#version 330

// NOTE: these should be shared between all shaders, put in common file
layout (location = 0) in vec2 a_world_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

out vec4 frag_color;

void main()
{
    gl_Position.xy = a_world_pos;
    gl_Position.zw = vec2(0.0f, 1.0f);

    frag_color = a_color;
}

#fragment
#version 330

in vec4 frag_color;

out vec4 final_color;

void main()
{
    final_color = frag_color;
}
