#version 460 core

in vec2 TexCoord;

out vec4 color;

uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;
uniform bool hasTexture1;
uniform bool hasTexture2;

void main()
{
    if (hasTexture1) {
        if (hasTexture2) {
            // Смешиваем две текстуры
            color = mix(texture(ourTexture1, TexCoord), texture(ourTexture2, TexCoord), 0.2);
        } else {
            // Используем только первую текстуру
            color = texture(ourTexture1, TexCoord);
        }
    } else {
        // Базовый цвет для объектов без текстур
        color = vec4(0.8, 0.8, 0.8, 1.0);
    }
}