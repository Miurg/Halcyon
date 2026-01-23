#pragma once
#include "../../Core/Entitys/EntityManager.hpp"

const Entity NULL_ENTITY = 0;
struct RelationshipComponent
{
	Entity parent = NULL_ENTITY;
	Entity firstChild = NULL_ENTITY;
	Entity nextSibling = NULL_ENTITY;
	Entity prevSibling = NULL_ENTITY; 
};