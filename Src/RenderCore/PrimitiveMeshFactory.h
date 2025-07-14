#pragma once
#include "MeshAsset.h"
#include "AssetManager.h"
#include <glm/glm.hpp>
#include <vector>

class PrimitiveMeshFactory 
{
public:
    //Create cube
    static MeshAsset* CreateCube(AssetManager* assetManager, const std::string& name = "Cube");
    
    //Create sphere
    static MeshAsset* CreateSphere(AssetManager* assetManager, const std::string& name = "Sphere", 
                                  int segments = 32, int rings = 16);
    
    //Create plane
    static MeshAsset* CreatePlane(AssetManager* assetManager, const std::string& name = "Plane",
                                 float width = 1.0f, float height = 1.0f);
    
    //Create cylinder
    static MeshAsset* CreateCylinder(AssetManager* assetManager, const std::string& name = "Cylinder",
                                    float radius = 0.5f, float height = 1.0f, int segments = 32);
    
    //Create cone
    static MeshAsset* CreateCone(AssetManager* assetManager, const std::string& name = "Cone",
                                float radius = 0.5f, float height = 1.0f, int segments = 32);

private:
    //Utility methods for vertex and index generation
    static void GenerateCubeData(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
    static void GenerateSphereData(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, 
                                  int segments, int rings);
    static void GeneratePlaneData(std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                                 float width, float height);
    static void GenerateCylinderData(std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                                    float radius, float height, int segments);
    static void GenerateConeData(std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                                float radius, float height, int segments);
}; 