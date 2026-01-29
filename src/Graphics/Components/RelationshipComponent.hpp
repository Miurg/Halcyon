#pragma once
#include "../../Core/Entitys/EntityManager.hpp"
#include "../../Core/GeneralManager.hpp"

const Entity NULL_ENTITY = 0;
struct RelationshipComponent
{
	Entity parent = NULL_ENTITY;
	Entity firstChild = NULL_ENTITY;
	Entity nextSibling = NULL_ENTITY;
	Entity prevSibling = NULL_ENTITY;

	void AddChild(Entity myEntity, Entity childEntity, GeneralManager& gm)
	{
		auto* childRel = gm.getComponent<RelationshipComponent>(childEntity);
		if (childRel == nullptr)
		{
			childRel = gm.addComponent<RelationshipComponent>(
			    childEntity); // If the child entity does not have a RelationshipComponent, add one
		}
		childRel->parent = myEntity;
		childRel->nextSibling = firstChild;
		childRel->prevSibling = NULL_ENTITY;

		if (firstChild != NULL_ENTITY)
		{
			gm.getComponent<RelationshipComponent>(firstChild)->prevSibling = childEntity;
		}
		firstChild = childEntity;
	}
};