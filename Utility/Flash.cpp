#include "Flash.h"

namespace Flash
{
	IMenuElement* Toggle = nullptr;
	IMenuElement* BlockFail = nullptr;
	IMenuElement* ExtendFlash = nullptr;

	std::shared_ptr<ISpell> Flash = nullptr;

	bool Loaded = false;

	// TODO Event broken from core
	void OnCastSpell(IGameObject* sender, OnCastSpellEventArgs* args)
	{
		if (!Toggle->GetBool() || !sender->IsMe() || !args->IsUserCall || args->SpellSlot != Flash->Slot())
		{
			return;
		}

		auto castAgain = false;
		auto endPos = args->CastPointEnd;
		auto startPos = g_LocalPlayer->Position();
		if (ExtendFlash->GetBool())
		{
			if (startPos.Distance(endPos) < 450)
			{
				g_Log->Print("kUtility: Extend Flash");
				endPos = g_LocalPlayer->Position().Extend(endPos, 450);
				args->Process = false;
				castAgain = true;
			}
		}

		if (BlockFail->GetBool())
		{
			if (endPos.IsWall() || endPos.IsBuilding())
			{
				g_Log->Print("kUtility: Block Flash");
				args->Process = false;
				castAgain = false;
			}
		}

		if (castAgain)
		{
			g_Log->Print("kUtility: Recast Flash");
			Flash->FastCast(endPos);
		}
	}

	void Load(IMenu* menu, IMenuElement* toggle, SpellSlot slot)
	{
		if (slot == SpellSlot::Invalid)
			return;

		Toggle = toggle;

		const auto hMenu = menu->AddSubMenu("Flash", "flash");
		hMenu->AddLabel("THIS IS BROKEN FROM CORE CURRENTLY", "note1");
		hMenu->AddLabel("May Interfere With Other Scripts", "note2");
		ExtendFlash = hMenu->AddCheckBox("Extend Flash To Max Range", "ExtendFlash", true);
		BlockFail = hMenu->AddCheckBox("Block Flash On Walls/Buildings", "BlockFail", true);

		Flash = g_Common->AddSpell(slot, 450.f);

		Loaded = true;
		EventHandler<Events::OnCastSpell>::AddEventHandler(OnCastSpell);
	}

	void Unload()
	{
		EventHandler<Events::OnCastSpell>::RemoveEventHandler(OnCastSpell);
	}
}