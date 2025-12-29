#pragma once
#include "IDamageable.h"

class IAttacker {
public:
	virtual ~IAttacker() = default;
	virtual void Attack(IDamageable* entity) const = 0; //in the case of player - entity = enemy/chest / in the case of the enemy - entity = player
};	