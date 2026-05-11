#pragma once
#include <Orhescyon/GeneralManager.hpp>

const Orhescyon::Entity NULL_ENTITY = 0;
// Intrusive linked-list scene hierarchy. addChild() prepends to sibling list.
struct RelationshipComponent
{
	Orhescyon::Entity parent = NULL_ENTITY;
	Orhescyon::Entity firstChild = NULL_ENTITY;
	Orhescyon::Entity nextSibling = NULL_ENTITY;
	Orhescyon::Entity prevSibling = NULL_ENTITY;

	void addChild(Orhescyon::Entity myEntity, Orhescyon::Entity childEntity, GeneralManager& gm)
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