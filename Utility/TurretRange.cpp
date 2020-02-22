#include "TurretRange.h"

namespace TurretRange
{
	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;
	IMenuElement* DrawAllied = nullptr;
	IMenuElement* AlliedColor = nullptr;
	IMenuElement* DrawEnemy = nullptr;
	IMenuElement* EnemyColor = nullptr;
	
	void DrawRange(IGameObject* turret, bool enemyTurret, float range)
	{
		if (enemyTurret && DrawEnemy->GetBool())
		{
			g_Drawing->AddCircle(turret->Position(), range, EnemyColor->GetColor(), 1.f);
		}
		else if (!enemyTurret && DrawAllied->GetBool())
		{
			g_Drawing->AddCircle(turret->Position(), range, AlliedColor->GetColor(), 1.f);
		}
	}

	void OnHudDraw()
	{
		for (auto const& turret : g_ObjectManager->GetTurrets())
		{
			if (!turret->IsValid() || turret->IsDead() || !turret->IsVisibleOnScreen())
				continue;

			DrawRange(turret, turret->IsEnemy(), 750.f + turret->BoundingRadius());
		}
	}

	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;

		const auto menu = mainMenu->AddSubMenu("Turret Range", "turret_range");
		Enabled = menu->AddCheckBox("Enabled", "enable", true);

		DrawAllied = menu->AddCheckBox("Draw Allied Turrets", "DrawAllied", true);
		AlliedColor = menu->AddColorPicker("Allied Color", "AlliedColor", 0, 255, 50, 250);

		DrawEnemy = menu->AddCheckBox("Draw Enemy Turrets", "DrawEnemy", true);
		EnemyColor = menu->AddColorPicker("Enemy Color", "EnemyColor", 255, 0, 50, 250);

		EventHandler<Events::OnHudDraw>::AddEventHandler(OnHudDraw);
	}

	void Unload()
	{
		EventHandler<Events::OnHudDraw>::RemoveEventHandler(OnHudDraw);
	}
}