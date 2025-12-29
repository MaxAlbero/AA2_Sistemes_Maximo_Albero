#pragma once
class IDamageable {
public:
	virtual ~IDamageable() = default;
	virtual void ReceiveDamage(int damageToAdd) = 0;
};