#include "Tracker.h"
#include "GankAlert.h"

namespace Tracker
{
	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		const auto menu = mainMenu->AddSubMenu("Tracker", "tracker");

		GankAlert::Load(menu, toggle);
	}

	void Unload()
	{
		GankAlert::Unload();
	}
}