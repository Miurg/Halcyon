#pragma once

#include "HalcyonExport.hpp"
#include <Orhescyon/GeneralManager.hpp>

inline constexpr Orhescyon::Entity NULL_ENTITY = Orhescyon::Entity::invalid();
// Intrusive linked-list scene hierarchy. addChild() prepends to sibling list.
struct HALCYON_API RelationshipComponent
{
	Orhescyon::Entity parent = NULL_ENTITY;
	Orhescyon::Entity firstChild = NULL_ENTITY;
	Orhescyon::Entity nextSibling = NULL_ENTITY;
	Orhescyon::Entity prevSibling = NULL_ENTITY;

	void addChild(Orhescyon::Entity myEntity, Orhescyon::Entity childEntity, Orhescyon::GeneralManager& gm)
	{
		auto* childRel = gm.hasComponent<RelationshipComponent>(childEntity)
		                     ? gm.getComponent<RelationshipComponent>(childEntity)
		                     : gm.addComponentImmediate<RelationshipComponent>(childEntity);
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