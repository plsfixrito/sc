#include "Extentions.h"

namespace Extentions
{
    // TODO add Sviri W and other spell shields
    bool IsKillable(IGameObject* target)
    {
		if (target->IsZombie() || target->IsUnkillable())
			return false;

		for (const auto& buff : target->GetBuffList())
		{
			if (StringEquals(buff.Name.c_str(), "kindredrnodeathbuff", true) && target->HealthPercent() < 15.f)
				return false;

			if (StringEquals(buff.Name.c_str(), "UndyingRage", true) ||
				StringEquals(buff.Name.c_str(), "ChronoShift", true) ||
				StringEquals(buff.Name.c_str(), "bansheesveil", true) ||
				StringEquals(buff.Name.c_str(), "KayleR", true) ||
				StringEquals(buff.Name.c_str(), "itemmagekillerevil", true))
				return false;

			if (buff.Type == BuffType::Invulnerability ||
				//buff.Type == BuffType::SpellShield ||
				buff.Type == BuffType::Counter ||
				buff.Type == BuffType::PhysicalImmunity ||
				buff.Type == BuffType::SpellImmunity)
				return false;
		}

		return true;
    }

	float GetAutoAttackRange(IGameObject* source, IGameObject* target)
	{
		auto result = source->AttackRange() + source->BoundingRadius() +
			(target != nullptr ? target->BoundingRadius() - 5 : 0);

		if (source->IsAIHero() && target != nullptr && source->Team() != target->Team())
		{
			switch (source->ChampionId())
			{
			case ChampionId::Caitlyn:
				if (target->IsAIBase() && target->HasBuff("caitlynyordletrapinternal"))
				{
					result += 650.f;
				}
				break;
			}
		}
		else if (source->IsAITurret())
		{
			return 750.f + source->BoundingRadius();
		}

		return result;
	}

	bool IsInRange(Vector source, Vector target, float range)
	{
		return source.Distance(target) < range;
	}

	bool IsInRange(IGameObject* source, IGameObject* target, float range)
	{
		return IsInRange(source->Position(), target->Position(), range);
	}

	bool IsInAutoAttackRange(IGameObject* source, IGameObject* target)
	{
		const auto aarange = GetAutoAttackRange(source, target);
		const auto inrange1 = IsInRange(source, target, aarange);
		const auto inrange2 = IsInRange(source->ServerPosition(), target->ServerPosition(), aarange);

		return inrange1 && inrange2;
	}

	bool OnScreen(Vector2 pos)
	{
		if (pos.x <= 0 || pos.y <= 0)
			return false;

		if (pos.x > g_Renderer->ScreenWidth() || pos.y > g_Renderer->ScreenHeight())
			return false;

		return true;
	}

	void DrawPoints(std::vector<Vector> points, uint32_t color)
	{
		const auto size = points.size();
		for (size_t i = 0; i < size; ++i)
		{
			size_t const next_index = size - 1 == i ? 0 : i + 1;

			auto const start = points[i];
			auto const end = points[next_index];
			if (OnScreen(start.WorldToScreen()) && OnScreen(end.WorldToScreen()))
				g_Drawing->AddLine(start, end, color);
		}
	}

}