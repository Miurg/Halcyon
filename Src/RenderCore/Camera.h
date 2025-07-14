#pragma once

#include <vector>
#include <cmath>

// GL Includes
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glad/glad.h>

enum Camera_Movement 
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

const GLfloat SPEED = 5.0f;
const GLfloat SENSITIVITY = 0.8f;
const GLfloat FOV = 60.0f;
const GLfloat YAW = 45.0f;
const GLfloat PITCH = 30.0f;

class Camera
{
public:
    glm::vec3 Position;
    glm::quat Rotation; 
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    GLfloat MovementSpeed;
    GLfloat MouseSensitivity;
    GLfloat Fov;
    float Yaw;
    float Pitch;

    Camera(
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = YAW,
        float pitch = PITCH
    ) : Position(position), WorldUp(up), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Fov(FOV), Yaw(yaw), Pitch(pitch)
    {
        _updateCameraVectors();
    }

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime)
    {
        GLfloat velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }
        _updateCameraVectors();
    }

    void ResetCamera()
    {
        Position = glm::vec3(10.0f, 10.0f, 10.0f);
        Yaw = YAW;
        Pitch = PITCH;
        _updateCameraVectors();
    }

private:
    void _updateCameraVectors()
    {
        //Convert angles to radians
        float yawRad = glm::radians(Yaw);
        float pitchRad = glm::radians(Pitch);
        //Corrected view direction formula
        glm::vec3 front;
        front.x = cos(pitchRad) * sin(yawRad);
        front.y = sin(pitchRad);
        front.z = -cos(pitchRad) * cos(yawRad);
        Front = glm::normalize(front);
        //Quaternion for compatibility(if needed)
        Rotation = glm::quat(glm::vec3(pitchRad, yawRad, 0.0f));
        //Proper Right and Up
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};