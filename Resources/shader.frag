#version 460 core
in vec2 TexCoord;

out vec4 color;

uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;
uniform bool hasTexture1;
uniform bool hasTexture2;

void main()
{
    if (hasTexture1) 
{
        color = texture(ourTexture1, TexCoord);
    } else {
        color = vec4(1.0, 0.0, 0.0, 1.0); // Red color for testing
    }
}