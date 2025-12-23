#include "ComponentManager.hpp"

std::unordered_map<std::type_index, std::function<void(Entity)>> ComponentManager::_removeCallbacks;
