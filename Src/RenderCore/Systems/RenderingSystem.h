#pragma once
#include <glad/glad.h>
#include "../../RenderCore/Camera.h"
#include "../../RenderCore/Shader.h"
#include "../../Core/Components/Components.h"
#include "../TransformUtils.h"
#include "../../RenderCore/MeshAsset.h"
#include "../../RenderCore/MaterialAsset.h"
#include "../../Core/Systems/System.h"
#include <map>
#include <vector>
#include <glm/gtc/type_ptr.hpp>


class RenderingSystem : public System<RenderingSystem, TransformComponent, RenderableComponent>
{
private:
    Shader& _shader;
    Camera& _camera;
    GLuint* _screenWidth;
    GLuint* _screenHeight;

    std::map<MeshAsset*, std::map<MaterialAsset*, std::vector<Entity>>> _renderGroups;

public:
    RenderingSystem(Shader& shader, Camera& camera, GLuint* width, GLuint* height)
        : _shader(shader), _camera(camera), _screenWidth(width), _screenHeight(height) {}

    void ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime) override
    {
        try 
        {
            TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
            RenderableComponent* renderable = cm.GetComponent<RenderableComponent>(entity);

            _renderGroups[renderable->Mesh][renderable->Material].push_back(entity);
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "ERROR::RENDER SYSTEM::Entity " << entity << ": " << e.what() << std::endl;
        }
    }

    void Update(float deltaTime, ComponentManager& cm, const std::vector<Entity>& entities) override
    {
        try 
        {
            _renderGroups.clear();
            System::Update(deltaTime, cm, entities);
            _shader.BindShader();
            MeshAsset* lastMesh = nullptr;
            MaterialAsset* lastMaterial = nullptr;

            glm::mat4 view = _camera.GetViewMatrix();
            glUniformMatrix4fv(glGetUniformLocation(_shader.ShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

            glm::mat4 projection = glm::perspective(glm::radians(_camera.Fov), (GLfloat)*_screenWidth / (GLfloat)*_screenHeight, 0.1f, 1000.0f);
            glUniformMatrix4fv(glGetUniformLocation(_shader.ShaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            GLint hasTexture1Loc = glGetUniformLocation(_shader.ShaderID, "hasTexture1");
            GLint hasTexture2Loc = glGetUniformLocation(_shader.ShaderID, "hasTexture2");
            GLint shaderModel = glGetUniformLocation(_shader.ShaderID, "model");
            for (auto& [mesh, materialGroups] : _renderGroups)
            {
                if (mesh != lastMesh) 
                {
                    mesh->BindMesh();
                    lastMesh = mesh;
                }
                for (auto& [material, entities] : materialGroups)
                {
                    if (material != lastMaterial) 
                    {
                        material->BindTextures();
                        lastMaterial = material;
                    }
                    for (size_t i = 0; i < material->GetTextureCount(); ++i)
                    {
                        std::string uniformName = "ourTexture" + std::to_string(i + 1);
                        glUniform1i(glGetUniformLocation(_shader.ShaderID, uniformName.c_str()), i);
                    }

                    

                    if (hasTexture1Loc != -1)
                    {
                        glUniform1i(hasTexture1Loc, material->GetTextureCount() > 0 ? 1 : 0);
                    }
                    if (hasTexture2Loc != -1)
                    {
                        glUniform1i(hasTexture2Loc, material->GetTextureCount() > 1 ? 1 : 0);
                    }

                    for (Entity entity : entities)
                    {
                        TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
                        glm::mat4 model = TransformUtils::CreateTransformMatrix(*transform);
                        glUniformMatrix4fv(shaderModel, 1, GL_FALSE, glm::value_ptr(model));

                        glDrawElements(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
                    }
                    material->UnbindTextures();
                }
                mesh->UnbindMesh();
            }
            _shader.UnbindShader();
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "ERROR::RENDER SYSTEM::" << e.what() << std::endl;
        }
    }
};
