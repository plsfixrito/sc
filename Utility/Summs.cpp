#include "Summs.h"

namespace Summoners
{
	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		const auto Menu = mainMenu->AddSubMenu("Summoner Spells", "summs");
		Heal::Load(Menu, toggle, g_LocalPlayer->GetSpellbook()->GetSpellSlotFromName("SummonerHeal"));
		Barrier::Load(Menu, toggle, g_LocalPlayer->GetSpellbook()->GetSpellSlotFromName("SummonerBarrier"));
		Flash::Load(Menu, toggle, g_LocalPlayer->GetSpellbook()->GetSpellSlotFromName("SummonerFlash"));
	}

	void Unload()
	{
		Heal::Unload();
		Barrier::Unload();
		Flash::Unload();
	}
}
