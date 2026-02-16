#pragma once
#include <memory>
#include <unordered_map>
#include <utility>
#include <typeinfo>
#include "../Entitys/EntityManager.hpp"

// Maps a context type (via typeid hash) → Entity for singleton-like global resource access.
class ContextManager
{
private:
	std::unordered_map<size_t, Entity> _contexts;

public:
	template <typename TContext>
	void registerContext(Entity entity)
	{
		_contexts[typeid(TContext).hash_code()] = entity;
	}

	template <typename TContext>
	Entity getContext()
	{
		return _contexts[typeid(TContext).hash_code()];
	}
};