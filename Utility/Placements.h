#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventArgs.h"
#include "../SDK/EventHandler.h"

namespace Placements
{
	enum class PlacementType
	{
		Unknown,
		SightWard,
		VisionWard,
		JhinTrap,
		Shroom,
		CaitlynTrap,
		NidaleeTrap,
		ShacoTrap
	};

	class PlacementInfo
	{
	public:
		PlacementType Type = PlacementType::Unknown;
		std::string ObjectName = "";
		std::string BuffName = "";

		PlacementInfo(std::string objName, std::string buffName, PlacementType type)
		{
			ObjectName = objName;
			BuffName = buffName;
			Type = type;
		}
	};

	class Placement
	{
	public:
		PlacementType Type;
		std::string Display;
		std::string BuffName;
		Vector Position;
		bool IsEnemy;
		int Id;
		int EndTime;

		std::string DisplayString()
		{
			if (EndTime > 0)
				return Display + " " + std::to_string((int)(EndTime - g_Common->Time()));

			return Display;
		}

		bool Ended()
		{
			return (EndTime > 0 && g_Common->Time() >= EndTime);// || g_ObjectManager->GetEntityByNetworkID(Id)->IsDead();
		}
	};

	void OnCreateObject(IGameObject* sender);

	void OnDeleteObject(IGameObject* sender);

	void OnBuff(IGameObject* sender, OnBuffEventArgs* args);

	bool OnScreen(Vector2 pos);

	void OnHudDraw();

	void Load(IMenu* mainMenu, IMenuElement* toggle);

	void Unload();
}