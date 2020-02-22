#include "GankAlert.h"

namespace GankAlert
{
	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;

	IMenuElement* FoW = nullptr;
	IMenuElement* JunglerOnly = nullptr;
	IMenuElement* PingDelay = nullptr;
	IMenuElement* PreviewDetectRange = nullptr;
	IMenuElement* DetectRange = nullptr;
	IMenuElement* FontSize = nullptr;
	IMenuElement* LineWidth = nullptr;
	IMenuElement* TextColor = nullptr;

	IMenu* EnemyMenu = nullptr;
	IMenuElement* PingEnemyGanks = nullptr;
	IMenuElement* EnemyGanks = nullptr;
	IMenuElement* EnemyPingType = nullptr;
	IMenuElement* EnemyColor = nullptr;

	IMenu* AllyMenu = nullptr;
	IMenuElement* PingAllyGanks = nullptr;
	IMenuElement* AllyGanks = nullptr;
	IMenuElement* AllyPingType = nullptr;
	IMenuElement* AllyColor = nullptr;

	std::unordered_map<int, GankInfo*> Ganks = std::unordered_map<int, GankInfo*>();
	std::unordered_map<int, int> GankCooldown = std::unordered_map<int, int>();

	int LastDetectRange = 0;
	int LastPreviewStart = 0;

	bool IsEnabled(GankInfo* gank)
	{
		if (JunglerOnly->GetBool() && !gank->IsJungler)
		{
			return false;
		}

		// expired
		if (g_Common->TickCount() - gank->StartTick > 5000)
		{
			return false;
		}

		if (gank->IsEnemy && !EnemyGanks->GetBool())
		{
			return false;
		}

		if (!gank->IsEnemy && !AllyGanks->GetBool())
		{
			return false;
		}

		return (gank->IsEnemy ? EnemyMenu : AllyMenu)->GetElement(gank->ElementName)->GetBool();
	}

	bool IsJungler(IGameObject* e)
	{
		for (auto& spell : e->GetSpellbook()->GetSpells())
		{
			if (spell->SData().SpellName.find("SummonerSmite") != std::string::npos)
			{
				return true;
			}
		}

		return false;
	}

	PingType GetPingType(int value)
	{
		if (value > 0)
			value += 1;

		if (value > 4)
			value += 1;

		return static_cast<PingType>(value);
	}

	uint32_t LineColor(GankInfo* gank)
	{
		if (gank->IsEnemy)
			return EnemyColor->GetColor();

		return AllyColor->GetColor();
	}

	void DrawLine(GankInfo* gank)
	{
		auto start = g_LocalPlayer->Position();
		auto end = gank->LastPosition;

		g_Drawing->AddLine(start, end, LineColor(gank), LineWidth->GetInt());

		auto mod = 1000 * (start.Distance(end) / DetectRange->GetInt());
		auto extend = start.Extend(end, mod);

		std::string s = gank->SkinName + (gank->IsJungler ? " (Jungler)" : "");
		g_Drawing->AddText(extend, TextColor->GetColor(), FontSize->GetInt(), s.c_str());
	}

	void Ping(GankInfo* gank)
	{
		if (g_Common->TickCount() - gank->LastPing < PingDelay->GetInt() * 1000)
		{
			return;
		}

		gank->LastPing = g_Common->TickCount();
		g_Common->CastLocalPing(gank->LastPosition, GetPingType(gank->IsEnemy ? EnemyPingType->GetInt() : AllyPingType->GetInt()), true);
	}

	void OnGameUpdate()
	{
		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		if (PreviewDetectRange->GetBool())
		{
			if (LastDetectRange != DetectRange->GetInt())
			{
				LastPreviewStart = g_Common->TickCount();
				LastDetectRange = DetectRange->GetInt();
			}
		}

		for (auto& t : g_ObjectManager->GetChampions())
		{
			if (t->IsMe() || t->IsDead())
				continue;

			if (!t->IsVisible() && !FoW->GetBool())
				continue;

			// on cooldown
			if (GankCooldown.count(t->NetworkId()) > 0)
			{
				if (g_Common->TickCount() - GankCooldown[t->NetworkId()] < 15000)
					continue;
			}

			// already detected
			if (Ganks.count(t->NetworkId()) > 0)
			{
				auto gank1 = Ganks[t->NetworkId()];

				if (t->Distance(g_LocalPlayer) > DetectRange->GetInt())
				{
					// Expired Gank
					if (g_Common->TickCount() - gank1->StartTick > 5000)
					{
						gank1->Reset = true;
						//Ganks.erase(t->NetworkId());
						GankCooldown[t->NetworkId()] = g_Common->TickCount();
					}
				}
				else
				{
					gank1->LastPosition = t->Position();
					// error
					if (IsEnabled(gank1))
					{
						Ping(gank1);
					}
				}

				Ganks[t->NetworkId()] = gank1;
				if (!gank1->Reset)
					continue;
			}

			if (t->Distance(g_LocalPlayer) > DetectRange->GetInt())
				continue;
			
			auto gank = Ganks.count(t->NetworkId()) > 0 ? Ganks[t->NetworkId()] : new GankInfo();
			gank->Id = t->NetworkId();
			gank->IsEnemy = t->IsEnemy();
			gank->IsJungler = IsJungler(t);
			gank->LastPosition = t->Position();
			gank->ElementName = t->BaseSkinName();// +std::to_string(t->NetworkId());
			gank->SkinName = t->BaseSkinName();
			gank->StartTick = g_Common->TickCount();
			Ping(gank);

			if (gank->Reset)
				gank->Reset = false;

			Ganks[t->NetworkId()] = gank;
		}
	}

	void OnHudDraw()
	{
		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		if (PreviewDetectRange->GetBool())
		{
			if (g_Common->TickCount() - LastPreviewStart < 3000)
			{
				g_Drawing->AddCircle(g_LocalPlayer->Position(), LastDetectRange, EnemyColor->GetColor());
			}
		}

		for (auto& gank : Ganks)
		{
			if (IsEnabled(gank.second))
				DrawLine(gank.second);
		}
	}

	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;

		const auto menu = mainMenu->AddSubMenu("Gank Alert", "ganks");
		Enabled = menu->AddCheckBox("Enabled", "enable", true);
		FoW = menu->AddCheckBox("Detect Ganks From FoW", "fow", true);
		JunglerOnly = menu->AddCheckBox("Detect Ganks From Junglers Only", "JunglerOnly", false);
		PingDelay = menu->AddSlider("Ping Delay in Seconds", "PingDelay", 10, 1, 30);
		PreviewDetectRange = menu->AddCheckBox("Preview Detect Range", "PreviewDetectRange", true);
		DetectRange = menu->AddSlider("Detect Range", "DetectRange", 3000, 1000, 25000);
		LineWidth = menu->AddSlider("Line Width", "LineWidth", 5, 1, 10);
		FontSize = menu->AddSlider("Font Size", "FontSize", 20, 10, 36);
		TextColor = menu->AddColorPicker("Text Color", "TextColor", 255, 255, 255, 255);

		std::vector<std::string> pings = std::vector<std::string>{ "Normal", "Danger", "Enemy Missing", "On My Way", "Assist Me", "Vision Here" };
		EnemyMenu = menu->AddSubMenu("Enemy Team", "enemy");
		EnemyGanks = EnemyMenu->AddCheckBox("Detect Enemy Ganks", "EnemyGanks", true);
		PingEnemyGanks = EnemyMenu->AddCheckBox("Ping Ganks (Local)", "PingEnemyGanks", true);
		EnemyPingType = EnemyMenu->AddComboBox("Ping Type", "EnemyPingType", pings, 1);
		EnemyColor = EnemyMenu->AddColorPicker("Line Color", "EnemyColor", 255, 25, 25, 255);

		AllyMenu = menu->AddSubMenu("Ally Team", "ally");
		AllyGanks = AllyMenu->AddCheckBox("Detect Ally Ganks", "AllyGanks", true);
		PingAllyGanks = AllyMenu->AddCheckBox("Ping Ganks (Local)", "PingAllyGanks", true);
		AllyPingType = AllyMenu->AddComboBox("Ping Type", "AllyPingType", pings, 3);
		AllyColor = AllyMenu->AddColorPicker("Line Color", "AllyColor", 25, 255, 25, 255);

		for (auto& e : g_ObjectManager->GetChampions())
		{
			if (e->IsMe())
				continue;

			auto element = (e->IsEnemy() ? EnemyMenu : AllyMenu)->AddCheckBox("Detect " + e->BaseSkinName() + (IsJungler(e) ? " (Jungler)" : ""),
				e->BaseSkinName(), true);

			element->SetBool(true);
		}

		EventHandler<Events::GameUpdate>::AddEventHandler(OnGameUpdate);
		EventHandler<Events::OnHudDraw>::AddEventHandler(OnHudDraw);
	}

	void Unload()
	{
		EventHandler<Events::GameUpdate>::RemoveEventHandler(OnGameUpdate);
		EventHandler<Events::OnHudDraw>::RemoveEventHandler(OnHudDraw);
	}
}