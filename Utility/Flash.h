#pragma once
#include "Extentions.h"
#include "../SDK/EventHandler.h"
#include "../SDK/EventArgs.h"

namespace Flash
{
	void OnCastSpell(IGameObject* sender, OnCastSpellEventArgs* args);

	void Load(IMenu* menu, IMenuElement* toggle, SpellSlot slot);

	void Unload();
}
