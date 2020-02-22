#include "Items.h"

namespace Items
{
	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;

	IMenuElement* UseQss = nullptr;
	IMenuElement* UseQssHP = nullptr;
	IMenuElement* QssMinDelay = nullptr;
	IMenuElement* QssMaxDelay = nullptr;
	IMenu* QssMenu = nullptr;
	IMenuElement* UseCutlass = nullptr;
	IMenuElement* UseCutlassHP = nullptr;
	IMenuElement* UseBotrk = nullptr;
	IMenuElement* UseBotrkHP = nullptr;
	IMenuElement* UseGunblade = nullptr;
	IMenuElement* UseGunbladeHP = nullptr;

	std::vector<ItemId> QssItems = std::vector<ItemId>
	{
		ItemId::Quicksilver_Sash,
		ItemId::Mercurial_Scimitar,
		ItemId::Dervish_Blade,
		ItemId::Mikaels_Crucible,
	};

	std::map<BuffType, std::string> Debuffs = std::map<BuffType, std::string>
	{
		{ BuffType::Stun, "Stun" },
		{ BuffType::Charm, "Charm" },
		{ BuffType::Fear, "Fear" },
		{ BuffType::Knockback, "Knockback" },
		{ BuffType::Knockup, "Knockup" },
		{ BuffType::NearSight, "NearSight" },
		{ BuffType::Poison, "Poison" },
		{ BuffType::Polymorph, "Polymorph" },
		{ BuffType::Shred, "Shred" },
		{ BuffType::Silence, "Silence" },
		{ BuffType::Slow, "Slow" },
		{ BuffType::Snare, "Snare" },
		{ BuffType::Blind, "Blind" },
		{ BuffType::Suppression, "Suppression" },
		{ BuffType::Taunt, "Taunt" },
	};

	std::map<BuffType, bool> Defaults = std::map<BuffType, bool>
	{
		{ BuffType::Stun, true },
		{ BuffType::Charm, true },
		{ BuffType::Fear, true },
		{ BuffType::Knockback, true },
		{ BuffType::Knockup, true },
		{ BuffType::NearSight, false },
		{ BuffType::Poison, false },
		{ BuffType::Polymorph, false },
		{ BuffType::Shred, false },
		{ BuffType::Silence, true },
		{ BuffType::Slow, false },
		{ BuffType::Snare, true },
		{ BuffType::Blind, true },
		{ BuffType::Suppression, true },
		{ BuffType::Taunt, true },
	};

	void OnBeforeAttackOrbwalker(BeforeAttackOrbwalkerArgs* args)
	{
		if (args->Target == nullptr || !args->Target->IsValid())
			return;

		if (!Toggle->GetBool() || !Enabled->GetBool())
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

	void UseItem(ItemId itemId)
	{
		if (g_LocalPlayer->CanUseItem(itemId))
		{
			g_LocalPlayer->CastItem(itemId);
		}
	}

	void OnBuff(IGameObject* sender, OnBuffEventArgs* args)
	{
		if (!sender->IsMe() || !args->IsBuffGain)
			return;

		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		if (Debuffs.count(args->Buff.Type) == 0)
			return;

		if (UseQssHP->GetInt() > g_LocalPlayer->HealthPercent())
			return;

		if (!QssMenu->GetElement(Debuffs[args->Buff.Type])->GetBool())
			return;

		for (auto& itemId : QssItems)
		{
			if (g_LocalPlayer->CanUseItem(itemId))
			{
				g_Common->DelayAction(rand()%(QssMaxDelay->GetInt()- QssMinDelay->GetInt() + 1) + QssMinDelay->GetInt(), [&]() { UseItem(itemId); });
				return;
			}
		}
	}

	void GameUpdate()
	{
		QssMinDelay->SetInt(min(QssMinDelay->GetInt(), QssMaxDelay->GetInt()));
		QssMaxDelay->SetInt(max(QssMinDelay->GetInt(), QssMaxDelay->GetInt()));
	}

	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;

		const auto menu = mainMenu->AddSubMenu("Items", "items");

		Enabled = menu->AddCheckBox("Enabled", "enable", true);

		const auto qss = menu->AddSubMenu("Quicksilver Sash", "qss");
		UseQss = qss->AddCheckBox("Use Quicksilver Sash", "UseQss", true);
		UseQssHP = qss->AddSlider("My HP% Under", "UseQssHP", 75, 1, 100);
		QssMinDelay = qss->AddSlider("Min Delay In ms", "QssMinDelay", 100, 0, 500);
		QssMaxDelay = qss->AddSlider("Max Delay In ms", "QssMaxDelay", 300, 0, 500);
		QssMenu = qss->AddSubMenu("Debuffs", "debuffs");
		for (auto& debuff : Debuffs)
			QssMenu->AddCheckBox(debuff.second, debuff.second, Defaults[debuff.first]);

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
		EventHandler<Events::OnBuff>::AddEventHandler(OnBuff);
		EventHandler<Events::GameUpdate>::AddEventHandler(GameUpdate);
	}

	void Unload()
	{
		EventHandler<Events::OnBeforeAttackOrbwalker>::RemoveEventHandler(OnBeforeAttackOrbwalker);
		EventHandler<Events::OnBuff>::RemoveEventHandler(OnBuff);
		EventHandler<Events::GameUpdate>::RemoveEventHandler(GameUpdate);
	}
}