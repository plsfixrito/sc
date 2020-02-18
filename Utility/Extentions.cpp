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
}