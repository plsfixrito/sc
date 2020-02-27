#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventArgs.h"
#include "../SDK/EventHandler.h"
#include "Extentions.h"

using namespace Extentions;

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
		std::string DisplayName = "";
		std::string ObjectName = "";
		std::string BuffName = "";

		PlacementInfo()
		{

		}

		PlacementInfo(std::string displayName, std::string objName, std::string buffName, PlacementType type)
		{
			DisplayName = displayName;
			ObjectName = objName;
			BuffName = buffName;
			Type = type;
		}
	};

	class Placement
	{
	public:
		PlacementInfo Info;
		std::string Display;
		Vector Position;
		bool IsEnemy;
		bool IsWard;
		bool DetectedByCast;
		bool Removed;
		IGameObject* Object = nullptr;
		int EndTime;
		int StartTime;
		std::vector<Vector> Points;

		std::string DisplayString()
		{
			if (EndTime > 0)
				return Display + " " + std::to_string((int)(EndTime - g_Common->Time()));

			return Display;
		}

		bool Ended()
		{
			return (EndTime > 0 && g_Common->Time() >= EndTime) ||
				Object == nullptr || !Object->IsValid() || Object->IsDead() || (IsWard && Object->Health() <= 0);// || g_ObjectManager->GetEntityByNetworkID(Id)->IsDead();
		}
	};

	void DrawPolygon(Geometry::Polygon poly, uint32_t color);

	std::vector<Geometry::Polygon> ClipWards(std::vector<Geometry::Polygon> polygs);

	void UpdateWardsClip(bool enemy);

	void DrawWards(bool enemy, uint32_t color);

	std::vector<Vector> GetWardVision(Vector pos);

	void OnCreateObject(IGameObject* sender);

	void OnDeleteObject(IGameObject* sender);

	void OnBuff(IGameObject* sender, OnBuffEventArgs* args);

	void OnHudDraw();

	void GameUpdate();

	void Load(IMenu* mainMenu, IMenuElement* toggle);

	void Unload();
}