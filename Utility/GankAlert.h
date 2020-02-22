#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventHandler.h"
#include "unordered_map"

namespace GankAlert
{
	class GankInfo
	{
	public:
		std::string ElementName;
		std::string SkinName;
		Vector LastPosition;
		bool IsEnemy;
		bool IsJungler;
		bool Reset;
		int LastPing;
		int StartTick;
		int Id;
	};

	bool IsEnabled(GankInfo* gank);

	uint32_t LineColor(GankInfo* gank);

	bool IsJungler(IGameObject* e);

	PingType GetPingType(int value);

	void DrawLine(GankInfo* gank);

	void Ping(GankInfo* gank);

	void OnGameUpdate();

	void OnHudDraw();

	void Load(IMenu* mainMenu, IMenuElement* toggle);

	void Unload();
}