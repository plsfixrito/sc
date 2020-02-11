#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/PluginSDK_Enums.h"

class PreCalculatedDamage
{
public:
	int UnitId;
	int LastCheck;
	float Damage;
	SpellSlot Slot;
};