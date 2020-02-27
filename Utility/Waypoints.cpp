#include "Waypoints.h"

namespace Waypoints
{
	IMenuElement* Toggle = nullptr;
	IMenuElement* Enabled = nullptr;

	IMenuElement* Ally = nullptr;
	IMenuElement* Enemy = nullptr;

	IMenuElement* FontSize = nullptr;
	IMenuElement* TextColor = nullptr;
	IMenuElement* AllyColor = nullptr;
	IMenuElement* EnemyColor = nullptr;

	IMenu* Allies = nullptr;
	IMenu* Enemies = nullptr;

	std::map<int, int> LastVision;
	std::vector<TrackedWaypoints> Paths;

	float RealDistance(std::vector<Vector> path)
	{
		if (path.empty())
			return -1;
		auto result = 0.f;
		const auto size = path.size() - 1;
		for (auto i = 0; i < size; i++)
			result += path[i].Distance(path[i + 1]);
		return result;
	}

	float TravelTime(IGameObject* sender, float dis)
	{
		return dis / sender->MoveSpeed();
	}	

	float TravelTime(IGameObject* sender, std::vector<Vector> path)
	{
		return TravelTime(sender, RealDistance(path));
	}

	void OnNewPath(IGameObject* sender, OnNewPathEventArgs* args)
	{
		if (sender == nullptr || args == nullptr || !sender->IsAIHero())
			return;

		Paths.erase(std::remove_if(Paths.begin(), Paths.end(), [&sender](TrackedWaypoints x)
			{
				return x.sender == nullptr || x.Remove || sender->NetworkId() == x.sender->NetworkId() || g_Common->Time() > x.ArriveTime;
			}), Paths.end());

		auto path = sender->RealPath();
		Paths.push_back(TrackedWaypoints(sender, path, TravelTime(sender, path)));
	}

	void DrawPath(TrackedWaypoints wayPoint)
	{
		const auto sender = wayPoint.sender;
		if (sender->IsDead())
			return;

		if (sender->IsAlly() && (!Ally->GetBool() || !Allies->GetElement(sender->IsMe() ? "me" : sender->ChampionName())->GetBool()))
			return;

		if (sender->IsEnemy() && (!Enemy->GetBool() || !Enemies->GetElement(sender->ChampionName())->GetBool()))
			return;

		const auto path = wayPoint.path;

		if (path.empty())
			return;

		const auto color = (sender->IsEnemy() ? EnemyColor : AllyColor)->GetColor();

		const auto size = path.size() - 1;
		for (auto i = 0; i < size; i++)
		{
			const auto start = path[i].WorldToScreen();
			const auto end = path[i + 1].WorldToScreen();

			g_Drawing->AddLineOnScreen(start, end, color, 1.5f);
			if (i == size - 1) // last point
			{
				g_Drawing->AddCircleOnScreen(end, 30, color, 1);
				const auto num_text = std::to_string(wayPoint.ArriveTime - g_Common->Time());
				const auto str = num_text.substr(0, num_text.find(".") + 3);
				g_Drawing->AddTextOnScreen(end, TextColor->GetColor(), FontSize->GetInt(), (sender->ChampionName() + " - " + str).c_str());
			}
		}
	}

	void OnHudDraw()
	{
		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		if (Paths.empty())
			return;

		for (auto const& path : Paths)
			DrawPath(path);
	}

	void GameUpdate()
	{
		if (!Toggle->GetBool() || !Enabled->GetBool())
			return;

		for (auto const& e : g_ObjectManager->GetChampions(false))
		{
			if (e->IsVisible())
			{
				if (LastVision.count(e->NetworkId()) == 0)
					LastVision.insert({ e->NetworkId(), g_Common->TickCount() });
				else
					LastVision[e->NetworkId()] = g_Common->TickCount();
			}
		}
		
		Paths.erase(std::remove_if(Paths.begin(), Paths.end(), [](TrackedWaypoints x)
			{
				return x.sender == nullptr || x.Remove || !x.sender->IsValid() || x.sender->IsDead() || g_Common->Time() > x.ArriveTime;
			}), Paths.end());

		//path[0].Distance(g_LocalPlayer->Position())
		for (auto& p : Paths)
		{
			if (p.sender->IsAlly() && Ally->GetBool() && Allies->GetElement(p.sender->IsMe() ? "me" : p.sender->ChampionName())->GetBool())
			{
				p.path = Geometry::Geometry::CutPath(p.path, p.path[0].Distance(p.sender->Position()));
				p.TravelTime = TravelTime(p.sender, p.path);
				p.ArriveTime = g_Common->Time() + p.TravelTime;
			}
			else if (p.sender->IsEnemy() && Enemy->GetBool() && Enemies->GetElement(p.sender->ChampionName())->GetBool())
			{
				if (p.sender->IsVisible())
				{
					p.path = Geometry::Geometry::CutPath(p.path, p.path[0].Distance(p.sender->Position()));
					p.TravelTime = TravelTime(p.sender, p.path);
					p.ArriveTime = g_Common->Time() + p.TravelTime;
					p.DistanceRemoved = 0;
				}
				else
				{
					if (p.sender->IsRecalling() || p.sender->IsRecallingFow())
					{
						p.Remove = true;
						continue;
					}
					const auto timeInvs = (g_Common->TickCount() - LastVision[p.sender->NetworkId()]) / 1000.f;
					const auto dtr = (p.sender->MoveSpeed() * timeInvs) - p.DistanceRemoved;
					const auto tdis = RealDistance(p.path);
					if (dtr >= tdis)
					{
						p.Remove = true;
						continue;
					}
					p.DistanceRemoved += dtr;
					const auto tt = TravelTime(p.sender, tdis);
					p.path = Geometry::Geometry::CutPath(p.path, dtr);
					p.TravelTime = TravelTime(p.sender, p.path);
					p.ArriveTime = g_Common->Time() + p.TravelTime;
				}
			}
		}
	}

	void Load(IMenu* mainMenu, IMenuElement* toggle)
	{
		Toggle = toggle;

		const auto menu = mainMenu->AddSubMenu("Waypoints", "waypoints");
		Enabled = menu->AddCheckBox("Enabled", "enable", false);

		Allies = menu->AddSubMenu("Allies", "allies");
		Ally = Allies->AddCheckBox("Track Allies", "Ally", false);

		for (auto const& e : g_ObjectManager->GetChampions(true))
			Allies->AddCheckBox("Track " + e->ChampionName() + (e->IsMe() ? " (Me)" : ""), e->IsMe() ? "me" : e->ChampionName(), true);

		AllyColor = Allies->AddColorPicker("Ally Color", "AllyColor", 50, 100, 255, 255);

		Enemies = menu->AddSubMenu("Enemies", "enemies");
		Enemy = Enemies->AddCheckBox("Track Enemies", "Enemy", true);
		for (auto const& e : g_ObjectManager->GetChampions(false))
			Enemies->AddCheckBox("Track " + e->ChampionName(), e->ChampionName(), true);

		EnemyColor = Enemies->AddColorPicker("Enemy Color", "EnemyColor", 255, 50, 69, 255);

		FontSize = menu->AddSlider("Font Size", "font_size", 16, 1, 32);
		TextColor = menu->AddColorPicker("Text Color", "TextColor", 255, 255, 255, 255);
		EventHandler<Events::GameUpdate>::AddEventHandler(GameUpdate);
		EventHandler<Events::OnHudDraw>::AddEventHandler(OnHudDraw);
		EventHandler<Events::OnNewPath>::AddEventHandler(OnNewPath);
	}

	void Unload()
	{
		EventHandler<Events::GameUpdate>::RemoveEventHandler(GameUpdate);
		EventHandler<Events::OnHudDraw>::RemoveEventHandler(OnHudDraw);
		EventHandler<Events::OnNewPath>::RemoveEventHandler(OnNewPath);
	}
}
