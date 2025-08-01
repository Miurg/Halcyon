#pragma once
#include <cstdint>

using Entity = uint32_t;

class EntityManager
{
public:
   EntityManager() : _nextEntity(1) {}
   Entity CreateEntity()
   {
	  return _nextEntity++;
   }

private:
   Entity _nextEntity;
};
