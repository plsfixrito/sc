#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventHandler.h"

namespace GankAlert
{
	bool IsJungler(IGameObject* e);

	PingType GetPingType(int value);

	//void DrawLine(GankInfo* gank);

	void Ping(Vector location, bool isEnemy);

	void OnGameUpdate();

	void OnHudDraw();

	void Load(IMenu* mainMenu, IMenuElement* toggle);

	void Unload();
}