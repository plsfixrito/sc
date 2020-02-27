#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventHandler.h"

namespace Waypoints
{
	class TrackedWaypoints
	{
	public:
		IGameObject* sender = nullptr;
		std::vector<Vector> path;
		Vector CurrentPosition;
		Vector LastDirection;
		float TravelTime = 0.f;
		float ArriveTime = 0.f;
		float DistanceRemoved = 0.f;
		bool ChangedPath = false;

		TrackedWaypoints(IGameObject* s, std::vector<Vector> a, float tt)
		{
			sender = s;
			path = a;
			TravelTime = tt;
			ArriveTime = g_Common->Time() + TravelTime;
		}
	};

	IMenuElement* Toggle = nullptr;

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
		if (sender == nullptr || args == nullptr || !sender->IsAIHero() || sender->IsAlly())
			return;

		const auto path = sender->RealPath();
		const auto tt = TravelTime(sender, path);
		for (auto& x : Paths)
		{
			if (sender->NetworkId() == x.sender->NetworkId())
			{
				x.path = path;
				x.TravelTime = tt;
				x.ArriveTime = g_Common->Time() + tt;
				x.LastDirection = (x.path[1] - x.path[0]).Normalized();
				x.CurrentPosition = x.path[0];
				return;
			}
		}

		Paths.push_back(TrackedWaypoints(sender, path, tt));
	}

	Vector GetPredictedPosition(IGameObject* obj)
	{
		for (auto const& p : Paths)
			if (p.sender->NetworkId() == obj->NetworkId() && !p.ChangedPath)
				return p.CurrentPosition;

		return Vector(0, 0);
	}

	void GameUpdate()
	{
		if (!Toggle->GetBool())
			return;

		for (auto const& e : g_ObjectManager->GetChampions(false))
		{
			if (e->IsVisible())
				LastVision[e->NetworkId()] = g_Common->TickCount();
		}
		
		for (auto& p : Paths)
		{
			if (p.path.empty())
				continue;
			if (p.sender->IsVisible())
			{
				if (p.sender->IsMoving())
				{
					p.path = Geometry::Geometry::CutPath(p.path, p.path[0].Distance(p.sender->Position()));
					p.TravelTime = TravelTime(p.sender, p.path);
					p.ArriveTime = g_Common->Time() + p.TravelTime;
					p.DistanceRemoved = 0;
					p.CurrentPosition = p.sender->Position();
					p.ChangedPath = false;
					p.LastDirection = (p.path[1] - p.path[0]).Normalized();
				}
			}
			else
			{
				if (p.sender->IsRecalling() || p.sender->IsRecallingFow())
				{
					p.CurrentPosition = p.path[0];
					continue;
				}

				const auto timeInvs = (g_Common->TickCount() - LastVision[p.sender->NetworkId()]) / 1000.f;
				const auto dtr = (p.sender->MoveSpeed() * timeInvs) - p.DistanceRemoved;
				const auto tdis = RealDistance(p.path);

				if (dtr >= tdis)
				{
					p.ChangedPath = true;
					continue;
				}

				p.DistanceRemoved += dtr;
				const auto tt = TravelTime(p.sender, tdis);
				p.path = Geometry::Geometry::CutPath(p.path, dtr);
				p.TravelTime = TravelTime(p.sender, p.path);
				p.ArriveTime = g_Common->Time() + p.TravelTime;
				p.CurrentPosition = p.path[0];
				p.ChangedPath = false;
				p.LastDirection = (p.path[1] - p.path[0]).Normalized();
			}
		}
	}

	void Load(IMenuElement* toggle)
	{
		Toggle = toggle;
		EventHandler<Events::GameUpdate>::AddEventHandler(GameUpdate);
		EventHandler<Events::OnNewPath>::AddEventHandler(OnNewPath);
	}

	void Unload()
	{
		EventHandler<Events::GameUpdate>::RemoveEventHandler(GameUpdate);
		EventHandler<Events::OnNewPath>::RemoveEventHandler(OnNewPath);
	}
}
