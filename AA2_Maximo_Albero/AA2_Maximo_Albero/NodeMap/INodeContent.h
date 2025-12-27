#pragma once
#include "Vector2.h"
class INodeContent
{
public:
	virtual void Draw(Vector2 offset) = 0;
};