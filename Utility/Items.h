#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventHandler.h"
#include "../SDK/EventArgs.h"

namespace Items
{
	void OnBeforeAttackOrbwalker(BeforeAttackOrbwalkerArgs* args);

	void Load(IMenu* mainMenu, IMenuElement* toggle);

	void Unload();
}