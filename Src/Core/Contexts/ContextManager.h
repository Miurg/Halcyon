#pragma once
#include <memory>
#include <unordered_map>

class ContextManager
{
private:
	std::unordered_map<size_t, std::shared_ptr<void>> _contexts;

public:
	template <typename TContext>
	void RegisterContext(std::shared_ptr<TContext> ctx)
	{
		_contexts[typeid(TContext).hash_code()] = ctx;
	}

	template <typename TContext>
	std::shared_ptr<TContext> GetContext()
	{
		auto it = _contexts.find(typeid(TContext).hash_code());
		if (it != _contexts.end())
		{
			return std::static_pointer_cast<TContext>(it->second);
		}
		return nullptr;
	}
};