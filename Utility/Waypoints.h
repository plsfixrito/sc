#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventHandler.h"
#include "Extentions.h"

using namespace Extentions;

namespace Waypoints
{
	class TrackedWaypoints
	{
	public:
		IGameObject* sender = nullptr;
		std::vector<Vector> path;
		float TravelTime = 0.f;
		float ArriveTime = 0.f;
		float DistanceRemoved = 0.f;
		bool Remove = false;

		TrackedWaypoints(IGameObject* s, std::vector<Vector> a, float tt)
		{
			sender = s;
			path = a;
			TravelTime = tt;
			ArriveTime = g_Common->Time() + TravelTime;
		}
	};

	void OnNewPath(IGameObject* sender, OnNewPathEventArgs* args);

	void DrawPath(IGameObject* sender, OnNewPathEventArgs* args);

	void OnHudDraw();

	void GameUpdate();

	void Load(IMenu* mainMenu, IMenuElement* toggle);

	void Unload();
}