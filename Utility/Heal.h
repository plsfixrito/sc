#pragma once
#include "Extentions.h"
#include "../SDK/EventHandler.h"
#include "../SDK/EventArgs.h"

namespace Heal
{
	void OnProcessSpellCast(IGameObject* sender, OnProcessSpellEventArgs* args);

	void Load(IMenu* menu, IMenuElement* toggle, SpellSlot slot);

    void Unload();
}
