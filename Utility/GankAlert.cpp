#include "GankAlert.h"

namespace GankAlert
{
	class GankInfo
	{
	public:
		std::string SkinName;
		Vector LastPosition;
		bool IsEnemy;
		bool IsJungler;
		int LastPing;
		int LastVisible;
		int StartTick;
		int Id;
	};

	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;

	IMenuElement* FoW = nullptr;
	IMenuElement* JunglerOnly = nullptr;
	IMenuElement* PingDelay = nullptr;
	IMenuElement* PreviewDetectRange = nullptr;
	IMenuElement* DetectRange = nullptr;
	IMenuElement* FontSize = nullptr;

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

	std::map<int, GankInfo*> Ganks = std::map<int, GankInfo*>();

	int LastDetectRange = 0;
	int LastPreviewStart = 0;

	bool IsEnabled(GankInfo* gank)
	{
		if (JunglerOnly->GetBool() && !gank->IsJungler)
		{
			return false;
		}

		if (gank->IsEnemy && !EnemyGanks->GetBool())
			return false;

		if (!gank->IsEnemy && !AllyGanks->GetBool())
			return false;

		return (gank->IsEnemy ? EnemyMenu : AllyMenu)->GetElement(gank->SkinName)->GetBool();
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

	void DrawLine(GankInfo* gank)
	{

	}

	void Ping(Vector location, bool isEnemy)
	{
		g_Common->CastLocalPing(location, GetPingType(isEnemy ? EnemyPingType->GetInt() : AllyPingType->GetInt()), true);
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
			if (!IsEnabled(gank.second))
				continue;

			DrawLine(gank.second);
		}
	}

	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;

		const auto menu = mainMenu->AddSubMenu("Gank Alert", "ganks");
		Enabled = menu->AddCheckBox("Enabled", "enable", true);
		FoW = menu->AddCheckBox("Detect Ganks From FoW", "fow", true);
		JunglerOnly = menu->AddCheckBox("Detect Ganks From Junglers Only", "JunglerOnly", true);
		PingDelay = menu->AddSlider("Ping Delay in Seconds", "PingDelay", 10, 1, 30);
		PreviewDetectRange = menu->AddCheckBox("Preview Detect Range", "PreviewDetectRange", true);
		DetectRange = menu->AddSlider("Detect Range", "DetectRange", 5000, 1000, 25000);
		FontSize = menu->AddSlider("Font Size", "FontSize", 14, 10, 24);

		std::vector<std::string> pings = std::vector<std::string>{ "Normal", "Danger", "Enemy Missing", "On My Way", "Assist Me", "Vision Here" };
		EnemyMenu = menu->AddSubMenu("Enemy Team", "enemy");
		EnemyGanks = EnemyMenu->AddCheckBox("Detect Enemy Ganks", "EnemyGanks", true);
		PingEnemyGanks = EnemyMenu->AddCheckBox("Ping Ganks (Local)", "PingEnemyGanks", true);
		EnemyPingType = EnemyMenu->AddComboBox("Ping Type", "EnemyPingType", pings, 1);
		EnemyColor = EnemyMenu->AddColorPicker("Line Color", "EnemyColor", 255, 25, 25, 255);

		AllyMenu = menu->AddSubMenu("Ally Team", "ally");
		AllyGanks = AllyMenu->AddCheckBox("Detect Enemy Ganks", "EnemyGanks", true);
		PingAllyGanks = AllyMenu->AddCheckBox("Ping Ganks (Local)", "PingEnemyGanks", true);
		AllyPingType = AllyMenu->AddComboBox("Ping Type", "EnemyPingType", pings, 3);
		AllyColor = AllyMenu->AddColorPicker("Line Color", "EnemyColor", 25, 255, 25, 255);

		for (auto& e : g_ObjectManager->GetChampions())
		{
			if (e->IsMe())
				continue;

			(e->IsEnemy() ? EnemyMenu : AllyMenu)->AddCheckBox("Detect " + e->BaseSkinName() + (IsJungler(e) ? " (Jungler)" : ""),
				e->BaseSkinName() + e->Name(), true);
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