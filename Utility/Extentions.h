#pragma once
#include "../SDK/PluginSDK.h"

namespace Extentions
{
	bool IsKillable(IGameObject* target);

	float GetAutoAttackRange(IGameObject* source, IGameObject* target);

	bool IsInRange(Vector source, Vector target, float range);

	bool IsInRange(IGameObject* source, IGameObject* target, float range);

	bool IsInAutoAttackRange(IGameObject* source, IGameObject* target);
}