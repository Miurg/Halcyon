#pragma once
#include <memory>
#include <unordered_map>

class ContextManager
{
private:
	std::unordered_map<size_t, std::shared_ptr<void>> contexts;

public:
	template <typename TContext>
	void registerContext(std::shared_ptr<TContext> ctx)
	{
		contexts[typeid(TContext).hash_code()] = ctx;
	}

	template <typename TContext>
	std::shared_ptr<TContext> getContext()
	{
		auto it = contexts.find(typeid(TContext).hash_code());
		if (it != contexts.end())
		{
			return std::static_pointer_cast<TContext>(it->second);
		}
		return nullptr;
	}
};