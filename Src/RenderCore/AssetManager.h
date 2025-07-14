#pragma once
#include "MeshAsset.h"
#include "MaterialAsset.h"
#include <unordered_map>
#include <memory>
#include <string>

class AssetManager 
{
private:
    std::unordered_map<std::string, std::unique_ptr<MeshAsset>> _meshes;
    std::unordered_map<std::string, std::unique_ptr<MaterialAsset>> _materials;

public:
    //=== Mesh management ===
    MeshAsset* CreateMesh(const std::string& name) 
    {
        auto mesh = std::make_unique<MeshAsset>(name);
        MeshAsset* meshPtr = mesh.get();
        _meshes[name] = std::move(mesh);
        return meshPtr;
    }

    MeshAsset* GetMesh(const std::string& name)
    {
        auto it = _meshes.find(name);
        if (it != _meshes.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    bool HasMesh(const std::string& name) const 
    {
        return _meshes.find(name) != _meshes.end();
    }

    const std::unordered_map<std::string, std::unique_ptr<MeshAsset>>& GetAllMeshes() const 
    {
        return _meshes;
    }

    //=== Material management ===
    MaterialAsset* CreateMaterial(const std::string& name) 
    {
        auto material = std::make_unique<MaterialAsset>(name);
        MaterialAsset* materialPtr = material.get();
        _materials[name] = std::move(material);
        return materialPtr;
    }

    MaterialAsset* GetMaterial(const std::string& name)
    {
        auto it = _materials.find(name);
        if (it != _materials.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    bool HasMaterial(const std::string& name) const 
    {
        return _materials.find(name) != _materials.end();
    }

    const std::unordered_map<std::string, std::unique_ptr<MaterialAsset>>& GetAllMaterials() const 
    {
        return _materials;
    }

    void Cleanup() 
    {
        _meshes.clear();
        _materials.clear();
    }
}; 