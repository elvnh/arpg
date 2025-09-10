#vertex

out vec4 frag_color;

void main()
{
    set_position();

    frag_color = a_color;
}

#fragment

in vec4 frag_color;

out vec4 final_color;

void main()
{
    final_color = frag_color;
}
