#include "Extentions.h"

namespace Extentions
{
    // TODO add Sviri W and other spell shields
    bool IsKillable(IGameObject* target)
    {
        const auto kindred = target->HasBuff("kindredrnodeathbuff") && target->HealthPercent() < 15.f;

        return !kindred && !target->HasBuff("UndyingRage") &&
            !target->HasBuff("ChronoShift") && !target->HasBuff("bansheesveil") &&
            !target->IsZombie() && !target->HasBuffOfType(BuffType::Invulnerability) &&
            !target->HasBuffOfType(BuffType::SpellShield) && !target->HasBuffOfType(BuffType::Counter) &&
            !target->HasBuffOfType(BuffType::PhysicalImmunity) && !target->HasBuffOfType(BuffType::SpellImmunity);
    }

	float GetAutoAttackRange(IGameObject* source, IGameObject* target)
	{
		auto result = source->AttackRange() + source->BoundingRadius() +
			(target != nullptr ? (target->BoundingRadius() - 15) : 0);

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

}