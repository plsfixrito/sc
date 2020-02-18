#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventHandler.h"

namespace TurretRange
{
	void DrawRange(IGameObject* turret, bool enemyTurret);

	void OnHudDraw();
	
	void Load(IMenu* mainMenu, IMenuElement* toggle);

	void Unload();
}