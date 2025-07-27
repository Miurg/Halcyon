#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>
#include <stb_image/stb_image.h>
#include <iostream>

class MaterialAsset 
{
private:
    std::string _name;
    std::vector<GLuint> _textures;
    std::vector<std::string> _textureNames;
    bool _initialized;

public:
    MaterialAsset(const std::string& materialName) : _name(materialName), _initialized(false) {}
    
    ~MaterialAsset() 
    {
        if (_initialized) 
        {
            for (GLuint texture : _textures) 
            {
                glDeleteTextures(1, &texture);
            }
        }
    }

    void AddTexture(const std::string& texturePath, const std::string& textureName = "") 
    {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        int width, height, nrChannels;
        unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
        
        if (data) 
        {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } 
        else 
        {
            std::cout << "ERROR::MATERIAL::Failed to load texture: " << texturePath << std::endl;
        }
        
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        _textures.push_back(textureID);
        _textureNames.push_back(textureName.empty() ? texturePath : textureName);
        _initialized = true;
    }

    void BindTextures() const 
    {
        for (size_t i = 0; i < _textures.size(); ++i) 
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, _textures[i]);
        }
    }

    void UnbindTextures() const 
    {
        for (size_t i = 0; i < _textures.size(); ++i) 
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    const std::string& GetName() const { return _name; }
    const std::vector<GLuint>& GetTextures() const { return _textures; }
    const std::vector<std::string>& GetTextureNames() const { return _textureNames; }
    bool IsInitialized() const { return _initialized; }
    size_t GetTextureCount() const { return _textures.size(); }
}; 