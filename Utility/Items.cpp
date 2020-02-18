#include "Items.h"

namespace Items
{
	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;

	IMenuElement* UseCutlass = nullptr;
	IMenuElement* UseCutlassHP = nullptr;
	IMenuElement* UseBotrk = nullptr;
	IMenuElement* UseBotrkHP = nullptr;
	IMenuElement* UseGunblade = nullptr;
	IMenuElement* UseGunbladeHP = nullptr;

	void OnBeforeAttackOrbwalker(BeforeAttackOrbwalkerArgs* args)
	{
		if (args->Target == nullptr || !args->Target->IsValid())
			return;

		if (g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo))
		{
			if (args->Target->IsAIHero() && args->Target->Distance(g_LocalPlayer) < 650)
			{
				if (UseCutlass->GetBool() && args->Target->HealthPercent() <= UseCutlassHP->GetInt() && g_LocalPlayer->CanUseItem(ItemId::Bilgewater_Cutlass))
				{
					g_LocalPlayer->CastItem(ItemId::Bilgewater_Cutlass, args->Target);
					return;
				}

				if (UseBotrk->GetBool() && args->Target->HealthPercent() <= UseBotrkHP->GetInt() && g_LocalPlayer->CanUseItem(ItemId::Blade_of_the_Ruined_King))
				{
					g_LocalPlayer->CastItem(ItemId::Blade_of_the_Ruined_King, args->Target);
					return;
				}

				if (UseGunblade->GetBool() && args->Target->HealthPercent() <= UseGunbladeHP->GetInt() && g_LocalPlayer->CanUseItem(ItemId::Hextech_Gunblade))
				{
					g_LocalPlayer->CastItem(ItemId::Hextech_Gunblade, args->Target);
					return;
				}
			}
		}
	}

	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;

		const auto menu = mainMenu->AddSubMenu("Items", "items");
		Enabled = menu->AddCheckBox("Enabled", "enable", true);

		const auto cutlass = menu->AddSubMenu("Bilgewater Cutlass", "cutlass");
		UseCutlass = cutlass->AddCheckBox("Use Bilgewater Cutlass", "UseCutlass", true);
		UseCutlassHP = cutlass->AddSlider("Target HP%", "UseCutlassHP", 65, 1, 100);

		const auto botrk = menu->AddSubMenu("Blade of The Ruined King", "botrk");
		UseBotrk = botrk->AddCheckBox("Use Blade of The Ruined King", "UseBotrk", true);
		UseBotrkHP = botrk->AddSlider("Target HP%", "UseBotrkHP", 65, 1, 100);

		const auto gunb = menu->AddSubMenu("Hextech Gunblade", "gunb");
		UseGunblade = gunb->AddCheckBox("Use Hextech Gunblade", "UseGunblade", true);
		UseGunbladeHP = gunb->AddSlider("Target HP%", "UseGunbladeHP", 65, 1, 100);

		EventHandler<Events::OnBeforeAttackOrbwalker>::AddEventHandler(OnBeforeAttackOrbwalker);
	}

	void Unload()
	{
		EventHandler<Events::OnBeforeAttackOrbwalker>::RemoveEventHandler(OnBeforeAttackOrbwalker);
	}
}