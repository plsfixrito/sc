#include "Orbfix.h"

using namespace Extentions;

namespace Orbfix
{
	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;

	IMenuElement* FixMaxRange = nullptr;
	IMenuElement* FixCancel = nullptr;
	IMenuElement* TextColor = nullptr;

	IGameObject* LastTarget;
	int AAEnd = 0;
	int MaxRangeFixes = 0;
	int CancelFixes = 0;

	int GetAutoAttackSlot(IGameObject* obj) 
	{
		return static_cast<int>(obj->GetSpellbook()->GetSpellSlotFromName(obj->GetAutoAttack().SpellName));
	}

	float AttackCastDelay()
	{
		switch (g_LocalPlayer->ChampionId())
		{
		case ChampionId::TwistedFate:
			if (g_LocalPlayer->HasBuff("BlueCardPreAttack") || g_LocalPlayer->HasBuff("RedCardPreAttack") || g_LocalPlayer->HasBuff("GoldCardPreAttack"))
			{
				return 130.f;
			}
			break;
		}
		return g_LocalPlayer->AttackCastDelay(GetAutoAttackSlot(g_LocalPlayer)) * 1000.f;
	}

	void OnIssueOrder(IGameObject* sender, OnIssueOrderEventArgs* args)
	{
		if (!sender->IsMe() || args->IsUserCall || g_Orbwalker->GetOrbwalkingMode() == 0)
			return;

		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		if (FixMaxRange->GetBool())
		{
			if (args->IssueOrderType == IssueOrderType::AttackUnit && args->Target != nullptr)
			{
				if (!IsInAutoAttackRange(g_LocalPlayer, args->Target))
				{
					MaxRangeFixes++;
					args->Process = false;
					g_LocalPlayer->IssueOrder(IssueOrderType::MoveTo, g_Common->CursorPosition());
					return;
				}
			}
		}

		if (FixCancel->GetBool())
		{
			if (args->IssueOrderType != IssueOrderType::AttackUnit)
			{
				if (AAEnd > g_Common->TickCount())
				{
					CancelFixes++;
					args->Process = false;
					return;
				}
			}

			if (args->IssueOrderType == IssueOrderType::AttackUnit)
			{
				if (AAEnd > g_Common->TickCount() &&
					LastTarget != nullptr &&
					args->Target != nullptr &&
					LastTarget->IsValid() &&
					!LastTarget->IsDead() &&
					LastTarget->NetworkId() != args->Target->NetworkId())
				{
					CancelFixes++;
					args->Process = false;
					return;
				}
			}
		}

		if (args->IssueOrderType == IssueOrderType::AttackUnit && args->Process)
		{
			AAEnd = g_Common->TickCount() + AttackCastDelay() + (g_Common->Ping() / 4);
			LastTarget = args->Target;
		}
	}

	void OnHudDraw()
	{
		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		std::string s = "";

		if (FixMaxRange->GetBool())
		{
			s += "FixMaxRange " + std::to_string(MaxRangeFixes);
		}

		if (FixCancel->GetBool())
		{
			s += (s != "" ? "\n" : "") + ("FixCancel " + std::to_string(CancelFixes));
		}

		if (s != "")
			g_Drawing->AddText(g_LocalPlayer->Position(), TextColor->GetColor(), 16, s.c_str());
	}

	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;

		const auto menu = mainMenu->AddSubMenu("Orbwalker Extra", "orbfix");

		Enabled = menu->AddCheckBox("Enabled", "enabled", false);
		FixMaxRange = menu->AddCheckBox("Fix Max Range Stutter", "FixMaxRange", true);
		FixCancel = menu->AddCheckBox("Fix AA Cancel", "FixCancel", true);
		TextColor = menu->AddColorPicker("text color", "TextColor", 255, 255, 255, 255, false);

		EventHandler<Events::OnIssueOrder>::AddEventHandler(OnIssueOrder);
		EventHandler<Events::OnHudDraw>::AddEventHandler(OnHudDraw);
	}

	void Unload()
	{
		EventHandler<Events::OnIssueOrder>::RemoveEventHandler(OnIssueOrder);
		EventHandler<Events::OnHudDraw>::RemoveEventHandler(OnHudDraw);
	}
}
