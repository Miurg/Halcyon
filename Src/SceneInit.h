#pragma once

#include "Core/GeneralManager.h"
#include "RenderCore/PrimitiveMeshFactory.h"

class SceneInit
{
public:
	static void Run(GeneralManager& gm, AssetManager& assetManager)
	{
		std::cout << "SCENEINIT::RUN::Start init" << std::endl;

		//=== Create meshes ===
		MeshAsset* cubeMesh = PrimitiveMeshFactory::CreateCube(&assetManager, "cube");
		MeshAsset* sphereMesh = PrimitiveMeshFactory::CreateSphere(&assetManager, "sphere", 16, 16);
		MeshAsset* planeMesh = PrimitiveMeshFactory::CreatePlane(&assetManager, "plane", 1.0f, 2.0f);
		MeshAsset* cylinderMesh = PrimitiveMeshFactory::CreateCylinder(&assetManager, "cylinder", 0.5f, 1.0f, 16);
		MeshAsset* coneMesh = PrimitiveMeshFactory::CreateCone(&assetManager, "cone", 0.5f, 1.0f, 16);

		//=== Materials ===
		MaterialAsset* cubeMaterial = assetManager.CreateMaterial("cube_material");
		cubeMaterial->AddTexture(RESOURCES_PATH "awesomeface.png", "diffuse");

		MaterialAsset* altMaterial = assetManager.CreateMaterial("alt_material");
		altMaterial->AddTexture(RESOURCES_PATH "container.jpg", "diffuse");

		//=== Create entities ===
		for (int i = 0; i < 100; ++i)
		{
			for (int j = 0; j < 100; ++j)
			{
				for (int k = 0; k < 100; ++k)
				{
					Entity entity = gm.CreateEntity();
					gm.AddComponent<TransformComponent>(entity, glm::vec3(i * 2.0f, k * 2.0f, j * 2.0f));

					// Mesh and material variety
					MeshAsset* currentMesh = cubeMesh;
					MaterialAsset* currentMaterial = cubeMaterial;

					int meshType = (i + j + k) % 5;
					switch (meshType)
					{
					case 0:
						currentMesh = cubeMesh;
						break;
					case 1:
						currentMesh = sphereMesh;
						break;
					case 2:
						currentMesh = planeMesh;
						break;
					case 3:
						currentMesh = cylinderMesh;
						break;
					case 4:
						currentMesh = coneMesh;
						break;
					}

					if ((i + j + k) % 2 == 0)
					{
						currentMaterial = altMaterial;
					}

					gm.AddComponent<RenderableComponent>(entity, currentMesh, currentMaterial);

					if ((i + j + k) % 2 == 0)
					{
						gm.AddComponent<RotationSpeedComponent>(entity, 500.0f, glm::vec3(0.0f, 1.0f, 0.0f));
						gm.SubscribeEntity<RotationSystem>(entity);
						gm.AddComponent<VelocityComponent>(entity, glm::vec3(0.5f, 0.0f, 0.5f));
						gm.SubscribeEntity<MovementSystem>(entity);
					}

					gm.SubscribeEntity<MultiDrawIndirectRenderingSystem>(entity);
				}
			}
		}
		std::cout << "SCENEINIT::RUN::Succes!" << std::endl;
	};
};