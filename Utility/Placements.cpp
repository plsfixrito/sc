#include "Placements.h"

namespace Placements
{
	std::vector<Placement*> Tracked = std::vector<Placement*>();
	std::vector<PlacementInfo*> Info = std::vector<PlacementInfo*>
	{
		new PlacementInfo("SightWard", "sharedwardbuff", PlacementType::SightWard),
		new PlacementInfo("JammerDevice", "", PlacementType::VisionWard),
		new PlacementInfo("JhinTrap", "JhinETrap", PlacementType::JhinTrap),
		new PlacementInfo("TeemoMushroom", "BantamTrap", PlacementType::Shroom),
		new PlacementInfo("ShacoBox", "JackInTheBox", PlacementType::ShacoTrap),
		new PlacementInfo("CaitlynTrap", "CaitlynYordleTrap", PlacementType::CaitlynTrap),
		//new PlacementInfo("NidaleeTrap", "Trap", PlacementType::NidaleeTrap),
	};

	std::map<PlacementType, std::string> DisplayNames = std::map<PlacementType, std::string>{
		{ PlacementType::SightWard, "Ward" },
		{ PlacementType::VisionWard, "Control Ward" },
		{ PlacementType::JhinTrap, "Jhin E" },
		{ PlacementType::Shroom, "Teemo R" },
		{ PlacementType::ShacoTrap, "Shaco W" },
		{ PlacementType::CaitlynTrap, "Caitlyn W" },
		//{ PlacementType::NidaleeTrap, "Trap" },
	};

	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;

	IMenuElement* Ally = nullptr;
	IMenuElement* Enemy = nullptr;

	IMenuElement* Green = nullptr;
	IMenuElement* Red = nullptr;
	IMenuElement* White = nullptr;
	IMenu* PTT = nullptr;

	void OnCreateObject(IGameObject* sender)
	{
		PlacementType type = PlacementType::Unknown;
		std::string buffName = "";
		for (auto const& info : Info)
		{
			if (sender->Name() == info->ObjectName || sender->BaseSkinName() == info->ObjectName)
			{
				buffName = info->BuffName;
				type = info->Type;
				break;
			}
		}

		if (type == PlacementType::Unknown)
			return;

		auto to = new Placement();

		to->Id = sender->NetworkId();
		to->Type = type;
		to->BuffName = buffName;
		to->Display = DisplayNames[type];
		to->Position = sender->Position();
		to->IsEnemy = sender->IsEnemy();

		if (to->BuffName != "")
		{
			const auto buff = sender->GetBuff(to->BuffName.c_str());
			if (buff.Valid)
			{
				to->EndTime = buff.EndTime;
			}
		}

		Tracked.push_back(to);
	}

	void OnDeleteObject(IGameObject* sender)
	{
		Tracked.erase(std::remove_if(Tracked.begin(), Tracked.end(), [&sender](Placement* x)
			{
				if (sender->NetworkId() == x->Id || x->Ended())
				{
					delete x;
					return true;
				}

				return false;
			}), Tracked.end());

		if (Tracked.empty())
		{
			Tracked.clear();
			Tracked.shrink_to_fit();
		}
	}

	void OnBuff(IGameObject* sender, OnBuffEventArgs* args)
	{
		if (sender == nullptr || !args->IsBuffGain || sender->IsAIHero())
			return;

		for (auto& t : Tracked)
		{
			if (sender->NetworkId() == t->Id && t->BuffName == args->Buff.Name)
			{
				t->EndTime = args->Buff.EndTime;
				break;
			}
		}
	}

	bool OnScreen(Vector2 pos)
	{
		if (pos.x <= 0 || pos.y <= 0)
			return false;

		if (pos.x > g_Renderer->ScreenWidth() || pos.y > g_Renderer->ScreenHeight())
			return false;

		return true;
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

		for (auto const& track : Tracked)
		{
			if (track->IsEnemy && !drawEnemy)
				continue;

			if (!track->IsEnemy && !drawAlly)
				continue;

			if (!OnScreen(track->Position.WorldToScreen()))
				continue;

			g_Drawing->AddCircle(track->Position, 100, (track->IsEnemy ? enemyColor : allyColor));
			g_Drawing->AddText(track->Position, textColor, 16, track->DisplayString().c_str());
		}
	}

	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;

		const auto menu = mainMenu->AddSubMenu("Placements", "placements");
		Enabled = menu->AddCheckBox("Enabled", "enable", true);

		PTT = menu->AddSubMenu("Tracked", "tracked");

		for (const auto& p : Info)
			PTT->AddCheckBox(DisplayNames[p->Type], p->ObjectName, true);

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
	}

	void Unload()
	{
		EventHandler<Events::OnCreateObject>::RemoveEventHandler(OnCreateObject);
		EventHandler<Events::OnDeleteObject>::RemoveEventHandler(OnDeleteObject);
		EventHandler<Events::OnBuff>::RemoveEventHandler(OnBuff);
		EventHandler<Events::OnHudDraw>::RemoveEventHandler(OnHudDraw);

		if (!Tracked.empty())
		{
			for (auto i = 0; i < Tracked.size(); i++)
				delete Tracked[i];

			Tracked.clear();
			Tracked.shrink_to_fit();
		}

		for (auto i : Info)
			delete i;

		Info.clear();
		Info.shrink_to_fit();
		DisplayNames.clear();
	}
}