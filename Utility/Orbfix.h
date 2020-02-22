#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventHandler.h"
#include "Extentions.h"

namespace Orbfix
{
	int GetAutoAttackSlot(IGameObject* obj);

	float AttackCastDelay();

	void OnIssueOrder(IGameObject* sender, OnIssueOrderEventArgs* args);

	void Load(IMenu* mainMenu, IMenuElement* toggle);

	void Unload();
}
