#include "AntiAfk.h"

namespace AntiAfk
{
	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;

	int LastOrder = 0;

	void OnIssueOrder(IGameObject* sender, OnIssueOrderEventArgs* args)
	{
		if (!sender->IsMe() || !args->Process)
			return;

		LastOrder = g_Common->TickCount();
	}

	void GameUpdate()
	{
		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		if (g_Common->TickCount() - LastOrder < 60000 * 2)
			return;

		g_LocalPlayer->IssueOrder(IssueOrderType::MoveTo, g_LocalPlayer->Position());
		g_Log->Print("kUtility: Moved by Anti Afk.");
	}

	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;
		const auto menu = mainMenu->AddSubMenu("Anti AFK", "antiafk");
		Enabled = menu->AddCheckBox("Enabled", "enabled", true);

		EventHandler<Events::OnIssueOrder>::AddEventHandler(OnIssueOrder);
		EventHandler<Events::GameUpdate>::AddEventHandler(GameUpdate);
	}

	void Unload()
	{
		EventHandler<Events::OnIssueOrder>::RemoveEventHandler(OnIssueOrder);
		EventHandler<Events::GameUpdate>::RemoveEventHandler(GameUpdate);
	}
}