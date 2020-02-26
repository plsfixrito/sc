#include "Misc.h"
#include "SkinHack.h"
#include "PlayerRange.h"
#include "TurretRange.h"
#include "Orbfix.h"
#include "AntiAfk.h"

namespace Misc
{
	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		const auto menu = mainMenu->AddSubMenu("Misc", "misc");

		SkinHack::Load(menu, toggle);
		PlayerRange::Load(menu, toggle);
		TurretRange::Load(menu, toggle);
		Orbfix::Load(menu, toggle);
		AntiAfk::Load(menu, toggle);
	}

	void Unload()
	{
		SkinHack::Unload();
		PlayerRange::Unload();
		TurretRange::Unload();
		Orbfix::Unload();
		AntiAfk::Unload();
	}
}
