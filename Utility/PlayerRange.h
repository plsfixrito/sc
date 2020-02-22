#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventHandler.h"
#include "Extentions.h"

namespace PlayerRange
{
	void OnHudDraw();

	void Load(IMenu* mainMenu, IMenuElement* toggle);

	void Unload();
}

