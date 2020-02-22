#include "PlayerRange.h"
using namespace Extentions;

namespace PlayerRange
{
	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;

	IMenu* Enemies = nullptr;
	IMenuElement* DrawEnemies = nullptr;
	IMenuElement* EnemyColor = nullptr;
	IMenuElement* InRangeColor = nullptr;

	IMenu* Allies = nullptr;
	IMenuElement* DrawAllies = nullptr;
	IMenuElement* AllyColor = nullptr;

	void OnHudDraw()
	{
		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		for (auto const& e : g_ObjectManager->GetChampions())
		{
			if (e->IsMe() || e->IsDead() || !e->IsVisibleOnScreen())
				continue;

			if (e->IsEnemy() && !DrawEnemies->GetBool())
				continue;

			if (e->IsAlly() && !DrawAllies->GetBool())
				continue;

			const auto color = e->IsAlly() ? AllyColor : IsInAutoAttackRange(e, g_LocalPlayer) ? InRangeColor : EnemyColor;
			g_Drawing->AddCircle(e->Position(), e->AttackRange(), color->GetColor());
		}
	}

	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;

		const auto menu = mainMenu->AddSubMenu("Players Range", "PlayerRange");
		Enabled = menu->AddCheckBox("Enabled", "enabled", false);

		Enemies = menu->AddSubMenu("Enemies", "enemies");
		DrawEnemies = menu->AddCheckBox("Draw Enemies", "DrawEnemies", true);

		Allies = menu->AddSubMenu("Allies", "allies");
		DrawAllies = menu->AddCheckBox("Draw Allies", "DrawAllies", false);

		for (auto& e : g_ObjectManager->GetChampions())
		{
			if (e->IsMe())
				continue;

			(e->IsEnemy() ? Enemies : Allies)->AddCheckBox("Draw " + e->ChampionName() + " Range", e->ChampionName(), true);
		}

		EnemyColor = Enemies->AddColorPicker("Enemy Range Color", "EnemyColor", 150, 100, 200, 255);
		InRangeColor = Enemies->AddColorPicker("Enemy In Range Color", "InRangeColor", 255, 0, 0, 255);

		AllyColor = Allies->AddColorPicker("Ally Range Color", "AllyColor", 75, 150, 255, 255);

		EventHandler<Events::OnHudDraw>::AddEventHandler(OnHudDraw);
	}

	void Unload()
	{
		EventHandler<Events::OnHudDraw>::RemoveEventHandler(OnHudDraw);
	}
}