#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventHandler.h"

namespace AntiAfk
{
	void OnIssueOrder(IGameObject* sender, OnIssueOrderEventArgs* args);

	void Load(IMenu* mainMenu, IMenuElement* toggle);

	void Unload();
}