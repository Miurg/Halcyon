#pragma once
#include "../Core/Components/Components.h"
#include <cmath>
#define _USE_MATH_DEFINES

namespace TransformUtils 
{
    inline glm::mat4 CreateTransformMatrix(const TransformComponent& transform) 
    {
        return glm::translate(glm::mat4(1.0f), transform.Position) * 
               glm::mat4_cast(transform.Rotation) * 
               glm::scale(glm::mat4(1.0f), transform.Scale);
    }
    
    inline void SetRotationEuler(TransformComponent& transform, float pitch, float yaw, float roll) 
    {
        transform.Rotation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
    }
    
    inline void SetRotationEuler(TransformComponent& transform, const glm::vec3& eulerAngles) 
    {
        transform.Rotation = glm::quat(glm::radians(eulerAngles));
    }
    
    inline glm::vec3 GetEulerAngles(const TransformComponent& transform) 
    {
        glm::vec3 euler;
        
        // pitch (x-axis rotation)
        float sinr_cosp = 2 * (transform.Rotation.w * transform.Rotation.x + transform.Rotation.y * transform.Rotation.z);
        float cosr_cosp = 1 - 2 * (transform.Rotation.x * transform.Rotation.x + transform.Rotation.y * transform.Rotation.y);
        euler.x = std::atan2(sinr_cosp, cosr_cosp);
        
        // yaw (y-axis rotation)
        float sinp = 2 * (transform.Rotation.w * transform.Rotation.y - transform.Rotation.z * transform.Rotation.x);
        if (std::abs(sinp) >= 1)
            euler.y = std::copysign(3.14159265359f / 2, sinp); // use 90 degrees if out of range
        else
            euler.y = std::asin(sinp);
        
        // roll (z-axis rotation)
        float siny_cosp = 2 * (transform.Rotation.w * transform.Rotation.z + transform.Rotation.x * transform.Rotation.y);
        float cosy_cosp = 1 - 2 * (transform.Rotation.y * transform.Rotation.y + transform.Rotation.z * transform.Rotation.z);
        euler.z = std::atan2(siny_cosp, cosy_cosp);
        
        return glm::degrees(euler);
    }
    
    inline void RotateAroundAxis(TransformComponent& transform, float angle, const glm::vec3& axis) 
    {
        glm::quat rotation = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
        transform.Rotation = rotation * transform.Rotation;
    }
    
    inline void RotateLocalX(TransformComponent& transform, float angle) 
    {
        glm::vec3 right = glm::normalize(transform.Rotation * glm::vec3(1, 0, 0));
        RotateAroundAxis(transform, angle, right);
    }
    
    inline void RotateLocalY(TransformComponent& transform, float angle) 
    {
        glm::vec3 up = glm::normalize(transform.Rotation * glm::vec3(0, 1, 0));
        RotateAroundAxis(transform, angle, up);
    }
    
    inline void RotateLocalZ(TransformComponent& transform, float angle) 
    {
        glm::vec3 forward = glm::normalize(transform.Rotation * glm::vec3(0, 0, 1));
        RotateAroundAxis(transform, angle, forward);
    }
    
    inline glm::vec3 GetForward(const TransformComponent& transform) 
    {
        return glm::normalize(transform.Rotation * glm::vec3(0, 0, 1));
    }
    
    inline glm::vec3 GetRight(const TransformComponent& transform) 
    {
        return glm::normalize(transform.Rotation * glm::vec3(1, 0, 0));
    }
    
    inline glm::vec3 GetUp(const TransformComponent& transform) 
    {
        return glm::normalize(transform.Rotation * glm::vec3(0, 1, 0));
    }
    
    inline TransformComponent CreateFromMatrix(const glm::mat4& matrix) 
    {
        TransformComponent transform;
        transform.Position = glm::vec3(matrix[3]);
        glm::mat3 rotationMatrix = glm::mat3(matrix);
        transform.Rotation = glm::quat_cast(rotationMatrix);
        transform.Scale.x = glm::length(glm::vec3(matrix[0]));
        transform.Scale.y = glm::length(glm::vec3(matrix[1]));
        transform.Scale.z = glm::length(glm::vec3(matrix[2]));
        
        return transform;
    }
} 