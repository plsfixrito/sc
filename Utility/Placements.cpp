#include "Placements.h"

namespace Placements
{
	std::vector<Placement> Tracked = std::vector<Placement>();

	std::vector<PlacementInfo> Info = std::vector<PlacementInfo>
	{
		PlacementInfo("Ward", "SightWard", "sharedwardbuff", PlacementType::SightWard),
		PlacementInfo("Control Ward", "JammerDevice", "", PlacementType::VisionWard),
		PlacementInfo("Jhin E", "JhinTrap", "JhinETrap", PlacementType::JhinTrap),
		PlacementInfo("Teemo R", "TeemoMushroom", "BantamTrap", PlacementType::Shroom),
		PlacementInfo("Shaco W", "ShacoBox", "JackInTheBox", PlacementType::ShacoTrap),
		PlacementInfo("Caitlyn W", "CaitlynTrap", "CaitlynYordleTrap", PlacementType::CaitlynTrap),
		//PlacementInfo("NidaleeTrap", "Trap", PlacementType::NidaleeTrap),
	};

	std::map<ChampionId, SpellSlot> CastSlots = std::map<ChampionId, SpellSlot>
	{
		{ ChampionId::Caitlyn, SpellSlot::W },
		{ ChampionId::Jhin, SpellSlot::E },
		{ ChampionId::Teemo, SpellSlot::R },
		{ ChampionId::Shaco, SpellSlot::W },
		//{ ChampionId::Nidalee, SpellSlot::W },
	};

	std::map<ChampionId, PlacementType> Champlacemnt = std::map<ChampionId, PlacementType>
	{
		{ ChampionId::Caitlyn, PlacementType::CaitlynTrap },
		{ ChampionId::Jhin, PlacementType::JhinTrap },
		{ ChampionId::Teemo, PlacementType::Shroom },
		{ ChampionId::Shaco, PlacementType::ShacoTrap },
		//{ ChampionId::Nidalee, SpellSlot::W },
	};

	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;

	IMenuElement* Ally = nullptr;
	IMenuElement* Enemy = nullptr;

	IMenuElement* ShowVisionToggle = nullptr;
	IMenuElement* ShowVision = nullptr;
	IMenuElement* VisionSegments = nullptr;
	IMenuElement* WallChecks = nullptr;

	IMenuElement* Green = nullptr;
	IMenuElement* Red = nullptr;
	IMenuElement* White = nullptr;
	IMenu* PTT = nullptr;

	bool LastToggle = false;
	bool LastValue = false;
	int LastSegments = 0;
	int LastChecks = 0;

	std::vector<Geometry::Polygon> AllyPolys;
	std::vector<Geometry::Polygon> EnemyPolys;

	void DrawPolygon(Geometry::Polygon poly, uint32_t color)
	{
		DrawPoints(poly.Points, color);
	}

	std::vector<Geometry::Polygon> ClipWards(std::vector<Geometry::Polygon> polygs)
	{
		return Geometry::Geometry::ToPolygons(Geometry::Geometry::ClipPolygons(polygs));
	}

	void UpdateWardsClip(bool enemy)
	{
		auto polys = std::vector<Geometry::Polygon>();

		for (auto const& track : Tracked)
		{
			if (!track.IsWard)
				return;

			if ((enemy && track.IsEnemy) || (!enemy && !track.IsEnemy))
			{
				auto poly = Geometry::Circle(track.Position, 1000).ToPolygon();
				poly.Points = track.Points;
				polys.push_back(poly);
			}
		}

		(enemy ? EnemyPolys : AllyPolys) = ClipWards(polys);
	}

	void DrawWards(bool enemy, uint32_t color)
	{
		const auto polys = (enemy ? EnemyPolys : AllyPolys);
		if (polys.empty())
			return;

		for (auto const& poly : (enemy ? EnemyPolys : AllyPolys))
			DrawPolygon(poly, color);
	}

	std::vector<Vector> GetWardVision(Vector pos)
	{
		std::vector<Vector> result = std::vector<Vector>();
		const auto inGrass = g_NavMesh->HasFlag(pos, kNavFlagsGrass);
		const auto range = 1000;
		const auto ppc = range / LastChecks;
		for (auto i = 0; i < LastSegments; i++)
		{
			auto added = false;
			float outeradius = 0;
			float angle = 0;
			int notGrass = 0;
			Vector LastPoint = Vector();
			for (auto y = 1; y <= LastChecks; y++)
			{
				const auto exrange = ppc * y;
				outeradius = exrange / (float)cos(2 * M_PI / LastSegments);
				angle = i * 2 * M_PI / LastSegments;
				const auto point = Vector(
					pos.x + outeradius * (float)cos(angle), pos.y + outeradius * (float)sin(angle));

				if (point.IsWall())
				{
					added = true;
					result.push_back(point);
					break;
				}

				if (!g_NavMesh->HasFlag(point, kNavFlagsGrass))
				{
					notGrass++;
					LastPoint = point;
				}
				else if (notGrass > 1 && LastPoint.Distance(pos) >= 200)
				{
					added = true;
					result.push_back(!LastPoint.IsZero() ? LastPoint : point);
					break;
				}
			}

			if (!added)
			{
				const auto outeradius = range / (float)cos(2 * M_PI / LastSegments);
				const auto angle = i * 2 * M_PI / LastSegments;
				const auto point = Vector(
					pos.x + outeradius * (float)cos(angle), pos.y + outeradius * (float)sin(angle));
				result.push_back(point);
			}
		}

		return result;
	}
	
	void OnCreateObject(IGameObject* sender)
	{
		if (sender->IsMissileClient())
			return;

		PlacementInfo info;
		for (auto const& i : Info)
		{
			if (sender->Name() == i.ObjectName || sender->BaseSkinName() == i.ObjectName)
			{
				info = i;
				break;
			}
		}

		if (info.Type == PlacementType::Unknown)
			return;
		
		auto to = Placement();

		to.Info = info;
		to.Position = sender->Position();
		to.IsEnemy = sender->IsEnemy();
		to.Object = sender;
		to.Position = sender->Position();
		to.StartTime = g_Common->Time();
		to.Display = info.DisplayName;
		to.IsWard = info.Type == PlacementType::SightWard || info.Type == PlacementType::VisionWard;

		if (to.IsWard)
			to.Points = GetWardVision(to.Position);

		if (to.Info.BuffName != "")
		{
			const auto buff = sender->GetBuff(to.Info.BuffName.c_str());
			if (buff.Valid)
			{
				to.EndTime = buff.EndTime;
			}
		}

		Tracked.push_back(to);

		if (!to.IsWard)
			return;

		UpdateWardsClip(to.IsEnemy);
	}

	void OnDeleteObject(IGameObject* sender)
	{
		if (sender->IsMissileClient())
			return;

		Tracked.erase(std::remove_if(Tracked.begin(), Tracked.end(), [&sender](Placement x)
			{
				if (sender->NetworkId() == x.Object->NetworkId() || x.Ended())
				{
					x.Points.clear();
					x.Points.shrink_to_fit();
					if (x.IsWard)
						UpdateWardsClip(x.IsEnemy);
					return true;
				}

				return false;
			}), Tracked.end());

		//Tracked.shrink_to_fit();
	}

	void OnBuff(IGameObject* sender, OnBuffEventArgs* args)
	{
		if (sender == nullptr || !args->IsBuffGain || sender->IsAIHero())
			return;

		for (auto& t : Tracked)
		{
			if (sender->NetworkId() == t.Object->NetworkId() && t.Info.BuffName == args->Buff.Name)
			{
				t.EndTime = args->Buff.EndTime;
				break;
			}
		}
	}

	void OnHudDraw()
	{
		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		const auto drawEnemy = Enemy->GetBool();
		const auto drawAlly = Ally->GetBool();

		const auto enemyColor = Red->GetColor();
		const auto allyColor = Green->GetColor();

		const auto textColor = White->GetColor();

		for (auto& track : Tracked)
		{
			if (track.IsEnemy && !drawEnemy)
				continue;

			if (!track.IsEnemy && !drawAlly)
				continue;

			if (!PTT->GetElement(track.Info.ObjectName)->GetBool())
				continue;
			
			if (!OnScreen(track.Position.WorldToScreen()))
				continue;

			const auto color = (track.IsEnemy ? enemyColor : allyColor);
			g_Drawing->AddCircle(track.Position, 100, (track.IsEnemy ? enemyColor : allyColor));
			g_Drawing->AddText(track.Position, textColor, 16, track.DisplayString().c_str());
		}

		if (ShowVision->GetBool())
		{
			if (drawEnemy)
				DrawWards(true, enemyColor);
			if (drawAlly)
				DrawWards(false, allyColor);
		}
	}

	void GameUpdate()
	{
		if (LastToggle != ShowVisionToggle->GetBool())
		{
			LastToggle = ShowVisionToggle->GetBool();
			LastValue = LastToggle;
			ShowVision->SetBool(LastToggle);
		}
		else if (LastValue != ShowVision->GetBool())
		{
			LastValue = ShowVision->GetBool();
			LastToggle = LastValue;
			ShowVisionToggle->SetBool(LastValue);
		}

		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		if (LastSegments != VisionSegments->GetInt() || LastChecks != WallChecks->GetInt())
		{
			LastSegments = VisionSegments->GetInt();
			LastChecks = WallChecks->GetInt();

			for (auto& track : Tracked)
				if (track.IsWard)
					track.Points = GetWardVision(track.Position);

			UpdateWardsClip(true);
			UpdateWardsClip(false);
		}
	}
	
	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;

		const auto menu = mainMenu->AddSubMenu("Placements", "placements");
		Enabled = menu->AddCheckBox("Enabled", "enable", true);

		PTT = menu->AddSubMenu("Tracked", "tracked");

		for (const auto& p : Info)
			PTT->AddCheckBox(p.DisplayName, p.ObjectName, true);

		const auto ward = menu->AddSubMenu("Ward Settings", "wards");
		ward->AddLabel("This Can Impact Performance", "notice");
		ShowVision = ward->AddCheckBox("Show Ward Vision Area", "ShowVision", false);
		ShowVisionToggle = ward->AddKeybind("Show Ward Vision Toggle", "ShowVisionToggle", 0x4c, false, KeybindType_Toggle);
		LastToggle = ShowVisionToggle->GetBool();
		LastValue = ShowVision->GetBool();
		VisionSegments = ward->AddSlider("Vision Segments", "VisionSegments", 16, 16, 180);
		WallChecks = ward->AddSlider("Vision Accuracy", "WallChecks", 10, 8, 40);
		LastSegments = VisionSegments->GetInt();
		LastChecks = WallChecks->GetInt();

		Ally = menu->AddCheckBox("Track Allied", "Ally", false);
		Enemy = menu->AddCheckBox("Track Enemy", "Enemy", true);

		Green = menu->AddColorPicker("Ally Color", "Green", 0, 255, 0, 255);
		Red = menu->AddColorPicker("Enemy Color", "Red", 255, 0, 0, 255);
		White = menu->AddColorPicker("Text Color", "White", 255, 255, 255, 255);

		for (const auto& obj : g_ObjectManager->GetByType())
			OnCreateObject(obj);

		EventHandler<Events::OnCreateObject>::AddEventHandler(OnCreateObject);
		EventHandler<Events::OnDeleteObject>::AddEventHandler(OnDeleteObject);
		EventHandler<Events::OnBuff>::AddEventHandler(OnBuff);
		EventHandler<Events::OnHudDraw>::AddEventHandler(OnHudDraw);
		EventHandler<Events::GameUpdate>::AddEventHandler(GameUpdate);
	}

	void Unload()
	{
		EventHandler<Events::OnCreateObject>::RemoveEventHandler(OnCreateObject);
		EventHandler<Events::OnDeleteObject>::RemoveEventHandler(OnDeleteObject);
		EventHandler<Events::OnBuff>::RemoveEventHandler(OnBuff);
		EventHandler<Events::OnHudDraw>::RemoveEventHandler(OnHudDraw);
		EventHandler<Events::GameUpdate>::RemoveEventHandler(GameUpdate);

		if (!Tracked.empty())
		{
			Tracked.clear();
			Tracked.shrink_to_fit();
		}

		Info.clear();
		Info.shrink_to_fit();
	}
}