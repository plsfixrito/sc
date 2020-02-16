#include "../SDK/PluginSDK.h"
#include "../SDK/EventArgs.h"
#include "../SDK/EventHandler.h"
#include "../SDK/Geometry.h"
#include "../SDK/PluginSDK_Enums.h"
#include <fstream>

#include "JungleSteal.h"
#include "BaseUlt.h"

class PreCalculatedDamage
{
public:
    int UnitId;
    int LastCheck;
    float Damage;
    SpellSlot Slot;
};

PLUGIN_API const char PLUGIN_PRINT_NAME[32] = "Ezreal";
PLUGIN_API const char PLUGIN_PRINT_AUTHOR[32] = "NotKappa";
PLUGIN_API ChampionId PLUGIN_TARGET_CHAMP = ChampionId::Ezreal;

namespace Menu
{
    IMenu* MenuInstance = nullptr;
    IMenuElement* Toggle = nullptr;

    namespace Combo
    {
        IMenuElement* Q = nullptr;
        IMenuElement* W = nullptr;
        IMenuElement* E = nullptr;
        IMenuElement* EQ = nullptr;
        IMenuElement* EAA = nullptr;
        IMenuElement* RExecute = nullptr;
        IMenuElement* RAoE = nullptr;
        IMenuElement* RAoEHits = nullptr;
        IMenuElement* RRange = nullptr;
        IMenuElement* RDistance = nullptr;
    }

    namespace Harass
    {
        IMenuElement* Q = nullptr;
        IMenuElement* W = nullptr;
        IMenuElement* ManaLimit = nullptr;
    }

    namespace LaneClear
    {
        IMenuElement* Q = nullptr;
        IMenuElement* QOnlyLastHit = nullptr;
        IMenuElement* MinionHP = nullptr;
        IMenuElement* WTurret = nullptr;
        IMenuElement* ManaLimit = nullptr;
    }

    namespace LastHit
    {
        IMenuElement* Q = nullptr;
        IMenuElement* MinionHP = nullptr;
        IMenuElement* ManaLimit = nullptr;
    }

    namespace JungleClear
    {
        IMenuElement* Q = nullptr;
        IMenuElement* W = nullptr;
        IMenuElement* ManaLimit = nullptr;
    }

    namespace Misc
    {
        IMenuElement* SemiManualR = nullptr;
        IMenuElement* QKillSteal = nullptr;
        IMenuElement* QStack = nullptr;
        IMenuElement* QInSpawn = nullptr;
        IMenuElement* QAwayFromEnemy = nullptr;
        IMenuElement* StackManaLimit = nullptr;
        IMenuElement* UseBotrk = nullptr;
    }

    namespace Drawings
    {
        IMenuElement* Toggle = nullptr;
        IMenuElement* Q = nullptr;
        IMenuElement* W = nullptr;
        IMenuElement* E = nullptr;
        IMenuElement* R = nullptr;
        IMenuElement* RDistance = nullptr;
    }

    namespace Colors
    {
        IMenuElement* Q = nullptr;
        IMenuElement* W = nullptr;
        IMenuElement* E = nullptr;
        IMenuElement* R = nullptr;
        IMenuElement* RDistance = nullptr;
    }
}

namespace Spells
{
    std::shared_ptr<ISpell> Q = nullptr;
    std::shared_ptr<ISpell> W = nullptr;
    std::shared_ptr<ISpell> E = nullptr;
    std::shared_ptr<ISpell> R = nullptr;
}

IPredictionOutput RPred;
bool TryEQ = false;
const char* EzrealWBuff = "ezrealwattach";

BaseUlt* BUInstance;
JungleSteal* JSInstance;
std::map<int, IGameObject*> TurretLastTarget = std::map<int, IGameObject*>();
std::vector<PreCalculatedDamage*> DamageCache = std::vector<PreCalculatedDamage*>();
//std::vector<PreCalculatedHealth*> HealthCache = std::vector<PreCalculatedHealth*>();
//std::vector<PreCalculatedPrediction*> PredCache = std::vector<PreCalculatedPrediction*>();

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

Vector PredPos(IGameObject* target, float time)
{
    return g_Common->GetPrediction(target, time).UnitPosition;
}

IPredictionOutput GetPred(IGameObject* target, std::shared_ptr<ISpell> spell, Vector souce)
{
    return g_Common->GetPrediction(target, spell->Range(), spell->Delay(), spell->Radius(),
        spell->Speed(), spell->CollisionFlags(), souce);
}

IPredictionOutput GetRPred(IGameObject* target)
{
    return g_Common->GetPrediction(target, Menu::Combo::RRange->GetInt(), Spells::R->Delay(), Spells::R->Radius(),
        Spells::R->Speed(), Spells::R->CollisionFlags(), g_LocalPlayer->Position());
}

bool TurretTargetingHero(IGameObject* turret)
{
    return turret != nullptr && turret->IsValidTarget() &&
        turret->CountAlliesInRange(turret->AttackRange() + turret->BoundingRadius()) + (g_LocalPlayer->IsUnderEnemyTurret() ? -1 : 0) > 0 &&
        TurretLastTarget.count(turret->NetworkId()) > 0 && TurretLastTarget[turret->NetworkId()] != nullptr &&
        TurretLastTarget[turret->NetworkId()]->IsAIHero() && TurretLastTarget[turret->NetworkId()]->NetworkId() != g_LocalPlayer->NetworkId();
}

float WDamage(IGameObject* target)
{
    const auto wDmg = (25.f + (55.f * Spells::W->Level()));
    const auto bonusAD = 0.6f * g_LocalPlayer->PercentBonusPhysicalDamageMod();
    const auto bonusAP = (0.65f + (Spells::W->Level() * 0.05f)) * g_LocalPlayer->TotalAbilityPower();
    return g_Common->CalculateDamageOnUnit(g_LocalPlayer, target, DamageType::Magical, wDmg + bonusAD + bonusAP);
}

float QDamage(IGameObject* target)
{
    auto dmg = 0.f;

    if (target->HasBuff(EzrealWBuff))
        dmg += WDamage(target);

    const auto ad = 5.f + (Spells::Q->Level() * 25.f) + (1.2f * g_LocalPlayer->TotalAttackDamage());
    const auto ap = 0.15f * g_LocalPlayer->TotalAbilityPower();
    return dmg + g_Common->CalculateDamageOnUnit(g_LocalPlayer, target, DamageType::Physical, ad + ap);
}

float HealthPrediction(IGameObject* target, std::shared_ptr<ISpell> spell, Vector castPos)
{
    const auto travelTime = (((castPos.Distance(g_LocalPlayer->Position()) / spell->Speed()) * 1000.f) + (spell->Delay() * 1000.f)) / 1000.f;
    return g_HealthPrediction->GetHealthPrediction(target, travelTime);
}

float HealthPrediction(IGameObject* target, std::shared_ptr<ISpell> spell)
{
    return HealthPrediction(target, spell, GetPred(target, spell, g_LocalPlayer->Position()).CastPosition);
}

float RHealthPrediction(IGameObject* target)
{
    RPred = GetRPred(target);
    const auto travelTime = (((RPred.CastPosition.Distance(g_LocalPlayer->Position()) / Spells::R->Speed()) * 1000.f) + (Spells::R->Delay() * 1000.f)) / 1000.f;
    return g_HealthPrediction->GetHealthPrediction(target, travelTime);
}

float GetDamage(IGameObject* target, std::shared_ptr<ISpell> spell)
{
    DamageCache.erase(std::remove_if(DamageCache.begin(), DamageCache.end(),
        [](PreCalculatedDamage* t) -> bool
        {
            return g_Common->TickCount() - t->LastCheck > 100;
        }), DamageCache.end());

    const bool isaa = spell == nullptr;
    for (const auto& dc : DamageCache)
    {
        if (dc->UnitId == target->NetworkId() && (isaa ? dc->Slot == SpellSlot::Invalid : spell->Slot() == dc->Slot))
            return dc->Damage;
    }

    auto ndc = new PreCalculatedDamage();
    ndc->UnitId = target->NetworkId();
    ndc->LastCheck = g_Common->TickCount();
    ndc->Slot = isaa ? SpellSlot::Invalid : spell->Slot();
    ndc->Damage = isaa ? g_LocalPlayer->AutoAttackDamage(target, true) :
        spell->Slot() == SpellSlot::Q ? QDamage(target) : 
        spell->Slot() == SpellSlot::W ? WDamage(target) : 
        spell->Damage(target);

    DamageCache.push_back(ndc);

    return ndc->Damage;
}

bool compareHealth(IGameObject* t1, IGameObject* t2) {
    return t1->Health() < t2->Health();
}

bool compareMaxHealth(IGameObject* t1, IGameObject* t2) {
    return t1->MaxHealth() < t2->MaxHealth();
}

std::vector<IGameObject*> SortByHealth(std::vector<IGameObject*> targets) {
    sort(targets.begin(), targets.end(), compareHealth);
    return targets;
}

std::vector<IGameObject*> SortByMaxHealth(std::vector<IGameObject*> targets) {
    sort(targets.begin(), targets.end(), [](IGameObject* a, IGameObject* b) {return a->MaxHealth() > b->MaxHealth(); });
    return targets;
}

IPredictionOutput* GetLastHitMinion(std::vector<IGameObject*> minions, bool useHP = true)
{
    //minions = SortByHealth(minions);
    for (auto& t : minions)
    {
        if (t == nullptr ||
            !t->IsValidTarget(Spells::Q->Range()) ||
            (g_Orbwalker->GetLastTarget() != nullptr && g_Orbwalker->GetLastTarget()->NetworkId() == t->NetworkId()))
            continue;

        auto healthpred = HealthPrediction(t, Spells::Q);
        // will die before Q reach or unkillable
        if (healthpred <= 0 || GetDamage(t, Spells::Q) < (useHP ? healthpred : t->Health()))
            continue;

        // orbwalker should take care of this minion
        if (g_LocalPlayer->IsInAutoAttackRange(t) && g_Orbwalker->CanAttack())
            continue;

        auto pred = GetPred(t, Spells::Q, g_LocalPlayer->Position());

        // unhittable
        if (pred.Hitchance < HitChance::Low)
            continue;

        return &pred;
    }

    return nullptr;
}

IPredictionOutput* GetLaneClearMinion(std::vector<IGameObject*> minions)
{
    //minions = SortByHealth(minions);
    for (auto& t : minions)
    {
        if (t == nullptr ||
            !t->IsValidTarget(Spells::Q->Range()) ||
            (g_Orbwalker->GetLastTarget() != nullptr && g_Orbwalker->GetLastTarget()->NetworkId() == t->NetworkId()))
            continue;

        auto healthpred = HealthPrediction(t, Spells::Q);
        // will die before Q reach
        if (healthpred <= 0)
            continue;

        // orbwalker should take care of this minion
        if (g_LocalPlayer->IsInAutoAttackRange(t) && g_Orbwalker->CanAttack())
            continue;

        auto pred = GetPred(t, Spells::Q, g_LocalPlayer->Position());

        // unhittable
        if (pred.Hitchance < HitChance::Medium)
            continue;

        return &pred;
    }

    return nullptr;
}

IPredictionOutput* GetJungleClearMinion()
{
    for (auto& t : SortByMaxHealth(g_ObjectManager->GetJungleMobs()))
    {
        if (t == nullptr || !t->IsValidTarget(Spells::Q->Range()))
            continue;

        auto healthpred = HealthPrediction(t, Spells::Q);
        // will die before Q reach
        if (healthpred <= 0)
            continue;

        // orbwalker should take care of this minion
        if (g_LocalPlayer->IsInAutoAttackRange(t) && g_Orbwalker->CanAttack())
            continue;

        auto pred = GetPred(t, Spells::Q, g_LocalPlayer->Position());

        // unhittable
        if (pred.Hitchance < HitChance::Medium)
            continue;

        return &pred;
    }

    return nullptr;
}

IPredictionOutput* GetKSTarget()
{
    for (const auto& t : g_ObjectManager->GetChampions(false))
    {
        if (t == nullptr || !t->IsValidTarget(Spells::Q->Range()) || !IsKillable(t))
            continue;

        auto healthpred = HealthPrediction(t, Spells::Q);
        // will die before Q reach or unkillable
        if (healthpred <= 0 || GetDamage(t, Spells::Q) < healthpred)
            continue;

        // orbwalker should take care of this target
        if (g_LocalPlayer->IsInAutoAttackRange(t) && g_Orbwalker->CanAttack())
            continue;

        auto pred = GetPred(t, Spells::Q, g_LocalPlayer->Position());

        // unhittable
        if (pred.Hitchance < HitChance::High)
            continue;

        return &pred;
    }

    return nullptr;
}

std::string ECase = "";
bool CastE(IGameObject* target)
{
    if (Spells::Q->ManaCost() > g_LocalPlayer->Mana() - Spells::E->ManaCost()) {
        ECase = "Q Mana";
        return false;
    }

    const auto extended = g_LocalPlayer->Position().Extend(g_Common->CursorPosition(), Spells::E->Range());

    if (extended.IsUnderEnemyTurret())
    {
        std::vector<IGameObject*> turrets = g_ObjectManager->GetTurrets();

        turrets.erase(std::remove_if(turrets.begin(), turrets.end(),
            [&extended](IGameObject* t) -> bool
            {
                return !t->IsInAutoAttackRange(extended);
            }), turrets.end());

        if (turrets.size() > 0)
        {
            if (!TurretTargetingHero(turrets[0])) {

                ECase = "TurretTargetingHero";
                return false;
            }
        }
    }

    const auto epred = PredPos(target, Spells::E->Delay());
    if (g_LocalPlayer->IsUnderEnemyTurret())
    {
        if (!extended.IsUnderEnemyTurret() && extended.Distance(epred) < g_LocalPlayer->AttackRange())
        {
            return Spells::E->Cast(extended);
        }
    }

    if (extended.CountEnemiesInRange(500) > extended.CountAlliesInRange(500)) {

        ECase = "CountEnemiesInRange";
        return false;
    }

    /*auto canBeCucked = false;
    for (const auto& enemy : g_ObjectManager->GetChampions(false))
    {
        if (enemy->IsInAutoAttackRange(extended) && enemy->AutoAttackDamage(g_LocalPlayer, true) > g_LocalPlayer->RealHealth(true))
        {
            canBeCucked = true;
            break;
        }
    }*/

    
    const auto aadmg = GetDamage(target, nullptr);

    if (!Menu::Combo::EAA->GetBool() && aadmg > target->RealHealth(true, false))
    {
        if (extended.Distance(epred) < g_LocalPlayer->AttackRange())
        {
            return Spells::E->Cast(extended);
        }
    }

    const auto qpred = GetPred(target, Spells::Q, extended);
    if (Menu::Combo::Q->GetBool() && Spells::Q->IsReady() && Menu::Combo::EQ->GetBool())
    {
        if (qpred.Hitchance >= HitChance::High &&
            extended.Distance(qpred.CastPosition) < Spells::Q->Range() &&
            GetDamage(target, Spells::Q) > HealthPrediction(target, Spells::Q, qpred.CastPosition))
        {
            if (Spells::E->Cast(extended)) {
                TryEQ = true;
                return true;
            }
        }
    }

    const auto tdis = epred.Distance(extended);
    if (tdis < Spells::E->Radius())
    {
        auto comboDmg = GetDamage(target, Spells::E);
        auto mana = Spells::E->ManaCost();

        if (tdis < g_LocalPlayer->AttackRange())
            comboDmg += aadmg;

        if (qpred.Hitchance >= HitChance::High)
        {
            if (Menu::Combo::Q->GetBool() && Spells::Q->IsReady())
            {
                comboDmg += GetDamage(target, Spells::Q);
                mana += Spells::Q->ManaCost();
            }

            if (Menu::Combo::W->GetBool() && Spells::W->IsReady())
            {
                comboDmg += GetDamage(target, Spells::W);
                mana += Spells::W->ManaCost();
            }
        }

        if (comboDmg > target->RealHealth(true, true) && g_LocalPlayer->Mana() - mana > 0)
            return Spells::E->Cast(extended);


        if (target->HasBuff(EzrealWBuff) && g_LocalPlayer->Mana() - mana > 0)
            return Spells::E->Cast(extended);
    }

    ECase = "No Options";
    return false;
}

bool RAoE(int hits)
{
    auto targets = g_ObjectManager->GetChampions(false);
    std::vector<Vector> positions = std::vector<Vector>();

    for (const auto& target : targets)
    {
        if (target == nullptr || !target->IsValidTarget(Menu::Combo::RRange->GetInt() + 150) || !IsKillable(target))
            continue;

        auto pred = GetRPred(target);
        if (pred.Hitchance >= HitChance::Low)
            positions.push_back(pred.CastPosition);
    }

    Vector biggestPos;
    int biggestHits = 0;

    for (auto& castPos : positions)
    {
        auto rect = new Geometry::Rectangle(g_LocalPlayer->Position(),
                    g_LocalPlayer->Position().Extend(castPos, Menu::Combo::RRange->GetInt()), 
                    Spells::R->Radius());

        auto counter = 0;
        auto poly = rect->ToPolygon();
        for (const auto& castPos2 : positions)
            if (poly.IsInside(castPos2))
                counter++;

        if (counter > biggestHits)
        {
            biggestPos = castPos;
            biggestHits = counter;
        }
    }
    
    if (biggestHits >= hits)
        return Spells::R->FastCast(biggestPos);

    return false;
}

bool HandleCombo()
{
    if (Spells::R->IsReady() && g_LocalPlayer->CountEnemiesInRange(Menu::Combo::RDistance->GetInt()) == 0)
    {
        if (Menu::Combo::RExecute->GetBool())
        {
            auto rExecute = g_Common->GetTarget(Menu::Combo::RRange->GetInt(), DamageType::Magical);

            if (rExecute != nullptr && 
                rExecute->IsValidTarget() &&
                IsKillable(rExecute) && 
                rExecute->CountMyAlliesInRange(669) == 0)
            {
                const auto rhp = RHealthPrediction(rExecute);

                if (rhp > 0 && GetDamage(rExecute, Spells::R) > rhp && RPred.Hitchance >= HitChance::Low) {
                    Spells::R->FastCast(RPred.CastPosition);
                    return true;
                }
            }
        }

        if (Menu::Combo::RAoE->GetBool())
        {
            if (RAoE(Menu::Combo::RAoEHits->GetInt()))
                return true;
        }
    }

    const auto qTarget = g_Common->GetTarget(Spells::Q->Range() + 50, DamageType::Physical);
    if (qTarget == nullptr || !qTarget->IsValidTarget() || !IsKillable(qTarget))
        return false;

    if (!TryEQ && (g_LocalPlayer->IsInAutoAttackRange(qTarget) || g_LocalPlayer->GetSpellbook()->IsAutoAttacking()))
        return false;

    if (Menu::Combo::E->GetBool() && Spells::E->IsReady() && CastE(qTarget))
        return false;

    const auto qPred = GetPred(qTarget, Spells::Q, g_LocalPlayer->Position());
    if (!TryEQ)
    {
        if (Menu::Combo::W->GetBool() && Spells::W->IsReady() && Spells::Q->IsReady() &&
            g_LocalPlayer->Mana() > Spells::Q->ManaCost() + Spells::W->ManaCost() &&
            qPred.Hitchance >= HitChance::Medium)
        {
            if (Spells::W->Cast(qTarget, HitChance::Medium))
                return true;
        }
    }

    if (Menu::Combo::Q->GetBool() && Spells::Q->IsReady())
    {
        if (qPred.Hitchance >= ((TryEQ || qTarget->HasBuff(EzrealWBuff)) ? HitChance::Low : HitChance::Medium))
        {
            Spells::Q->Cast(qPred.CastPosition);
            TryEQ = false;
            return true;
        }
    }
    else {
        TryEQ = false;
    }

    return false;
}

bool HandleHarass()
{
    if (Menu::Harass::ManaLimit->GetInt() >= g_LocalPlayer->ManaPercent())
        return false;

    auto qTarget = g_Common->GetTarget(Spells::Q->Range(), DamageType::Physical);
    if (qTarget == nullptr || !qTarget->IsValidTarget() || !IsKillable(qTarget))
        return false;

    if (Menu::Harass::W->GetBool() && Spells::W->IsReady() && g_LocalPlayer->IsInAutoAttackRange(qTarget))
    {
        if (Spells::W->Cast(qTarget, HitChance::Medium))
            return true;
    }
    if (Menu::Harass::Q->GetBool() && Spells::Q->IsReady())
    {
        if (Spells::Q->Cast(qTarget, HitChance::Low))
            return true;
    }
}

bool HandleLaneClear()
{
    if (Menu::LaneClear::ManaLimit->GetInt() >= g_LocalPlayer->ManaPercent())
        return false;

    if (g_LocalPlayer->GetSpellbook()->IsAutoAttacking())
        return false;

    if (!Menu::LaneClear::Q->GetBool() || !Spells::Q->IsReady())
        return false;

    auto minions = g_ObjectManager->GetMinionsEnemy();

    if (minions.size() == 0)
        return false;

    auto lastHit = GetLastHitMinion(minions, Menu::LaneClear::MinionHP->GetBool());
    if (lastHit != nullptr)
    {
        Spells::Q->Cast(lastHit->CastPosition);
        return true;
    }

    if (Menu::LaneClear::QOnlyLastHit->GetBool())
        return false;

    auto laneClear = GetLaneClearMinion(minions);
    if (laneClear != nullptr)
    {
        Spells::Q->Cast(laneClear->CastPosition);
        return true;
    }
}

bool HandleJungleClear()
{
    if (Menu::JungleClear::ManaLimit->GetInt() >= g_LocalPlayer->ManaPercent())
        return false;

    if (g_LocalPlayer->GetSpellbook()->IsAutoAttacking())
        return false;

    if (Menu::JungleClear::Q->GetBool() && Spells::Q->IsReady())
    {
        auto target = GetJungleClearMinion();
        if (target != nullptr)
        {
            Spells::Q->Cast(target->CastPosition);
            return true;
        }
    }

    return false;
}

bool HandleLasthit()
{
    if (Menu::LastHit::ManaLimit->GetInt() >= g_LocalPlayer->ManaPercent())
        return false;

    if (g_LocalPlayer->GetSpellbook()->IsAutoAttacking())
        return false;

    if (!Menu::LastHit::Q->GetBool() || !Spells::Q->IsReady())
        return false;

    auto lastHit = GetLastHitMinion(g_ObjectManager->GetMinionsEnemy(), Menu::LastHit::MinionHP->GetBool());
    if (lastHit != nullptr)
    {
        Spells::Q->Cast(lastHit->CastPosition);
        return true;
    }

    return false;
}

void HandleKS()
{
    if (!Menu::Misc::QKillSteal->GetBool() || !Spells::Q->IsReady())
        return;

    auto target = GetKSTarget();
    if (target == nullptr)
        return;

    Spells::Q->Cast(target->CastPosition);
}

bool CanTear()
{
    if (g_LocalPlayer->HasItem(ItemId::Muramana) ||
        g_LocalPlayer->HasItem(ItemId::Muramana_1) ||
        g_LocalPlayer->HasItem(ItemId::Seraphs_Embrace) ||
        g_LocalPlayer->HasItem(ItemId::Seraphs_Embrace_1))
        return false;

    return g_LocalPlayer->HasItem(ItemId::Tear_of_the_Goddess) ||
        g_LocalPlayer->HasItem(ItemId::Tear_of_the_Goddess_Quick_Charge) ||
        g_LocalPlayer->HasItem(ItemId::Manamune) ||
        g_LocalPlayer->HasItem(ItemId::Manamune_Quick_Charge) ||
        g_LocalPlayer->HasItem(ItemId::Archangels_Staff) ||
        g_LocalPlayer->HasItem(ItemId::Archangels_Staff_Quick_Charge);
}

void StackTear()
{
    if (!Menu::Misc::QStack->GetBool() || !Spells::Q->IsReady() || g_LocalPlayer->IsRecalling())
        return;

    if (!CanTear())
        return;

    auto inSpawn = g_LocalPlayer->InFountain();

    if (!inSpawn && Menu::Misc::StackManaLimit->GetInt() > g_LocalPlayer->ManaPercent())
        return;

    if (Menu::Misc::QInSpawn->GetBool() && inSpawn)
    {
        Spells::Q->Cast(g_LocalPlayer->Position().Extend(g_Common->CursorPosition(), 127));
        return;
    }

    auto awayFromEnemies = g_LocalPlayer->CountMyEnemiesInRange(1750) <= 0;

    if (Menu::Misc::QAwayFromEnemy->GetBool() && awayFromEnemies)
    {
        Spells::Q->Cast(g_LocalPlayer->Position().Extend(g_Common->CursorPosition(), 174));
        return;
    }

    if (inSpawn || awayFromEnemies)
        Spells::Q->Cast(g_LocalPlayer->Position().Extend(g_Common->CursorPosition(), 132));
}

bool SemiManualR()
{
    if (!Menu::Misc::SemiManualR->GetBool() || !Spells::R->IsReady())
        return false;

    if (RAoE(2))
        return true;

    auto rTarget = g_Common->GetTarget(2000, DamageType::Magical);
    if (rTarget != nullptr && rTarget->IsValidTarget() && IsKillable(rTarget))
        if (!Spells::R->Cast(rTarget, HitChance::Low))
            if (Spells::R->FastCast(rTarget->ServerPosition()))
                return true;

    return false;
}

bool IsStructure(IGameObject* target)
{
    return target->IsAITurret() || target->IsInhibitor() || target->IsNexus();
}

void OnGameUpdate()
{
    try
    {
        JSInstance->OnGameUpdate();
        BUInstance->OnUpdate();

        if (Menu::Combo::RDistance->GetInt() + 100 >= Menu::Combo::RRange->GetInt())
            Menu::Combo::RRange->SetInt(Menu::Combo::RDistance->GetInt() + 100);

        if (!Menu::Toggle->GetBool() || g_LocalPlayer->IsDead())
        {
            return;
        }

        // avoid useless code to run while casting stuff
        if (g_LocalPlayer->GetSpellbook()->IsCastingSpell() || g_LocalPlayer->GetSpellbook()->IsChanneling())
        {
            return;
        }

        HandleKS();

        if (SemiManualR())
        {
            return;
        }

        if (g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo))
        {
            if (HandleCombo())
            {
                return;
            }
        }

        if (g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeHarass))
        {
            if (HandleHarass())
            {
                return;
            }
        }

        if (g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeLaneClear))
        {
            if (HandleLaneClear() || HandleJungleClear())
            {
                return;
            }
        }

        if (g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeFarm))
        {
            if (HandleLasthit())
            {
                return;
            }
        }

        if (g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeMixed))
        {
            if (HandleLasthit() || HandleHarass())
            {
                return;
            }
        }

        if (g_Orbwalker->GetOrbwalkingMode() == 0)
        {
            StackTear();
        }
    }
    catch (const std::exception & e)
    {
        // ignore
    }
}

const auto CirclesWidth = 1.f;

void OnHudDraw()
{
    try
    {
        JSInstance->OnHudDraw();

        if (!Menu::Toggle->GetBool() || !Menu::Drawings::Toggle->GetBool())
        {
            return;
        }

        const auto PlayerPosition = g_LocalPlayer->Position();

        if (Menu::Drawings::Q->GetBool())
            g_Drawing->AddCircle(PlayerPosition, Spells::Q->Range(), Menu::Colors::Q->GetColor(), CirclesWidth);

        if (Menu::Drawings::W->GetBool())
            g_Drawing->AddCircle(PlayerPosition, Spells::W->Range(), Menu::Colors::W->GetColor(), CirclesWidth);

        if (Menu::Drawings::E->GetBool())
            g_Drawing->AddCircle(PlayerPosition, Spells::E->Range(), Menu::Colors::E->GetColor(), CirclesWidth);

        if (Menu::Drawings::R->GetBool())
            g_Drawing->AddCircle(PlayerPosition, Menu::Combo::RRange->GetInt(), Menu::Colors::R->GetColor(), CirclesWidth);

        if (Menu::Drawings::RDistance->GetBool())
            g_Drawing->AddCircle(PlayerPosition, Menu::Combo::RDistance->GetInt(), Menu::Colors::RDistance->GetColor(), CirclesWidth);

    }
    catch (const std::exception & e)
    {
        // ignore
    }
    //g_Drawing->AddTextOnScreen(Vector2(g_Renderer->ScreenWidth() * 0.0130208333333333f, g_Renderer->ScreenHeight() * 0.45f), Menu::Colors::Q->GetColor(), 16, ECase.c_str());
}

void OnProcessSpellCast(IGameObject* sender, OnProcessSpellEventArgs* args)
{
    if (sender == nullptr || args->Target == nullptr)
        return;

    try
    {
        if (!sender->IsAITurret())
        {
            return;
        }

        TurretLastTarget[sender->NetworkId()] = args->Target;
    }
    catch (const std::exception & e)
    {
        // ignore
    }
}

void OnBeforeAttackOrbwalker(BeforeAttackOrbwalkerArgs* args)
{
    if (args->Target == nullptr || !args->Target->IsValid())
        return;

    try
    {
        if (g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo))
        {
            if (args->Target->IsAIHero())
            {
                if (Menu::Misc::UseBotrk->GetBool() && args->Target->Distance(g_LocalPlayer) < 650)
                {
                    if (g_LocalPlayer->CanUseItem(ItemId::Blade_of_the_Ruined_King))
                    {
                        g_LocalPlayer->CastItem(ItemId::Blade_of_the_Ruined_King, args->Target);
                        return;
                    }

                    if (g_LocalPlayer->CanUseItem(ItemId::Bilgewater_Cutlass))
                    {
                        g_LocalPlayer->CastItem(ItemId::Bilgewater_Cutlass, args->Target);
                        return;
                    }

                    if (g_LocalPlayer->CanUseItem(ItemId::Hextech_Gunblade))
                    {
                        g_LocalPlayer->CastItem(ItemId::Hextech_Gunblade, args->Target);
                        return;
                    }
                }

                if (Menu::Combo::W->GetBool() && Spells::W->IsReady())
                {
                    args->Process = !Spells::W->Cast(args->Target, HitChance::Low);
                    return;
                }
                if (Menu::Combo::Q->GetBool() && Spells::Q->IsReady())
                {
                    args->Process = !Spells::Q->Cast(args->Target, HitChance::Low);
                    return;
                }
            }
        }

        if (g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeLaneClear))
        {
            if (IsStructure(args->Target))
            {
                if (Menu::LaneClear::ManaLimit->GetInt() >= g_LocalPlayer->ManaPercent())
                {
                    return;
                }

                if (Menu::LaneClear::WTurret->GetBool() && Spells::W->IsReady())
                {
                    args->Process = !Spells::W->Cast(args->Target->Position());
                    return;
                }
            }

            if (args->Target->IsEpicMonster())
            {
                if (Menu::JungleClear::ManaLimit->GetInt() >= g_LocalPlayer->ManaPercent())
                {
                    return;
                }

                if (Menu::JungleClear::W->GetBool() && Spells::W->IsReady())
                {
                    args->Process = !Spells::W->Cast(args->Target, HitChance::Low);
                    return;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        // ignore
    }
}

void OnAfterAttackOrbwalker(IGameObject* target)
{
    if (target == nullptr || !target->IsValid())
        return;

    try
    {
        if (g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo))
        {
            if (target->IsAIHero())
            {
                if (Menu::Combo::Q->GetBool() && Spells::Q->IsReady())
                {
                    Spells::Q->Cast(target, HitChance::Low);
                }
            }
        }

    }
    catch (const std::exception & e)
    {
        // ignore
    }
}

void OnTeleport(IGameObject* sender, OnTeleportEventArgs* args)
{
    if (sender == nullptr || !sender->IsValid())
        return;

    try
    {
        BUInstance->OnTeleport(sender, args);
    }
    catch (const std::exception & e)
    {
        // ignore
    }
}

void OnEndScene()
{
    try
    {
        BUInstance->OnEndScene();
    }
    catch (const std::exception & e)
    {
        // ignore
    }
}

PLUGIN_API bool OnLoadSDK(IPluginsSDK* plugin_sdk) 
{
    DECLARE_GLOBALS(plugin_sdk);
    
    Spells::Q = g_Common->AddSpell(SpellSlot::Q, 1150.f);
    Spells::Q->SetSkillshot(.25f, 60.f, 2000.f, kCollidesWithYasuoWall | kCollidesWithHeroes | kCollidesWithMinions, eSkillshotType::kSkillshotLine);

    Spells::W = g_Common->AddSpell(SpellSlot::W, 1150.f);
    Spells::W->SetSkillshot(.25f, 65.f, 1500.f, kCollidesWithNothing, eSkillshotType::kSkillshotLine);

    Spells::E = g_Common->AddSpell(SpellSlot::E, 475.f);
    Spells::E->SetSkillshot(.25f, 750.f, 999999.f, kCollidesWithNothing, eSkillshotType::kSkillshotCircle);

    Spells::R = g_Common->AddSpell(SpellSlot::R, 250000.f);
    Spells::R->SetSkillshot(1.f, 320.f, 2000.f, kCollidesWithYasuoWall, eSkillshotType::kSkillshotLine);

    using namespace Menu;
    MenuInstance = g_Menu->CreateMenu("kEzreal", "kappa_ezreal");
    Toggle = Menu::MenuInstance->AddCheckBox("Enabled", "global_toggle", true);

    const auto ComboMenu = MenuInstance->AddSubMenu("Combo", "combo");
    Combo::Q = ComboMenu->AddCheckBox("Use Q", "q", true);
    Combo::W = ComboMenu->AddCheckBox("Use W", "w", true);
    Combo::E = ComboMenu->AddCheckBox("Use E", "e", true);
    Combo::EQ = ComboMenu->AddCheckBox("Try E > Q Finisher", "eq", true);
    Combo::EAA = ComboMenu->AddCheckBox("Try E > AA Finisher", "eaa", true);
    Combo::RExecute = ComboMenu->AddCheckBox("Use R Execute", "rexecute", true);
    Combo::RAoE = ComboMenu->AddCheckBox("Use R AoE", "raoe", true);
    Combo::RAoEHits = ComboMenu->AddSlider("AoE R Hits", "raoehits", 3, 2, 5);
    Combo::RRange = ComboMenu->AddSlider("Custom R Range", "RRange", 1850, 0, 3000);
    Combo::RRange->SetTooltip("Has To Be Bigger Than R Distance");
    Combo::RDistance = ComboMenu->AddSlider("R Distance From Enemies", "rdistance", 650, 0, 2500);
    Combo::RDistance->SetTooltip("Has To Be Smaller Than R Range");

    const auto HarassMenu = MenuInstance->AddSubMenu("Harass", "harass");
    Harass::Q = HarassMenu->AddCheckBox("Use Q", "q", true);
    Harass::W = HarassMenu->AddCheckBox("Use W", "w", true);
    Harass::ManaLimit = HarassMenu->AddSlider("Mana Limit", "manalimit", 50, 0, 99);

    const auto LaneClearMenu = MenuInstance->AddSubMenu("Lane Clear", "laneclear");
    LaneClear::Q = LaneClearMenu->AddCheckBox("Use Q", "q", true);
    LaneClear::QOnlyLastHit = LaneClearMenu->AddCheckBox("Q Only Last Hit", "QOnlyLastHit", true);
    LaneClear::MinionHP = LaneClearMenu->AddCheckBox("Use Health Prediction", "MinionHP", true);
    LaneClear::WTurret = LaneClearMenu->AddCheckBox("Use W On Turrets", "WTurret", true);
    LaneClear::ManaLimit = LaneClearMenu->AddSlider("Mana Limit", "manalimit", 50, 0, 99);

    const auto LastHitMenu = MenuInstance->AddSubMenu("Last Hit", "lasthit");
    LastHit::Q = LastHitMenu->AddCheckBox("Use Q", "q", true);
    LastHit::MinionHP = LastHitMenu->AddCheckBox("Use Health Prediction", "MinionHP", true);
    LastHit::ManaLimit = LastHitMenu->AddSlider("Mana Limit", "manalimit", 50, 0, 99);

    const auto JungleClearMenu = MenuInstance->AddSubMenu("Jungle Clear", "jungleclear");
    JungleClear::Q = JungleClearMenu->AddCheckBox("Use Q", "q", true);
    JungleClear::W = JungleClearMenu->AddCheckBox("Use W", "w", true);
    JungleClear::ManaLimit = JungleClearMenu->AddSlider("Mana Limit", "manalimit", 50, 0, 99);

    const auto MiscMenu = MenuInstance->AddSubMenu("Misc", "misc");

    Misc::SemiManualR = MiscMenu->AddKeybind("Semi Manual R", "r", 0x53, false, _KeybindType::KeybindType_Hold);
    Misc::QKillSteal = MiscMenu->AddCheckBox("Q Kill Steal", "QKillSteal", true);
    Misc::QStack = MiscMenu->AddCheckBox("Stack Tear Q", "QStack", true);
    Misc::QInSpawn = MiscMenu->AddCheckBox("Stack Tear In Spawn", "QInSpawn", true);
    Misc::QAwayFromEnemy = MiscMenu->AddCheckBox("Stack Tear If No Enemies Are Near", "QAwayFromEnemy", true);
    Misc::StackManaLimit = MiscMenu->AddSlider("Stacking Mana Limit", "manalimit", 75, 0, 99);
    Misc::UseBotrk = MiscMenu->AddCheckBox("Use BOTRK/Cutlass/Gunblade", "botrk", true);

    const auto DrawingMenu = MenuInstance->AddSubMenu("Drawings", "draw");
    Drawings::Toggle = DrawingMenu->AddCheckBox("Enabled", "toggle", true);
    Drawings::Q = DrawingMenu->AddCheckBox("Draw Q Range", "q", true);
    Drawings::W = DrawingMenu->AddCheckBox("Draw W Range", "w", true);
    Drawings::E = DrawingMenu->AddCheckBox("Draw E Range", "e", true);
    Drawings::R = DrawingMenu->AddCheckBox("Draw R Range", "r", true);
    Drawings::RDistance = DrawingMenu->AddCheckBox("Draw R Distance From Enemies", "rdis", false);

    const auto ColorsMenu = MenuInstance->AddSubMenu("Colors", "colors");
    Colors::Q = ColorsMenu->AddColorPicker("Draw Q Color", "q", 0, 255, 250, 250);
    Colors::W = ColorsMenu->AddColorPicker("Draw W Color", "w", 0, 250, 255, 250);
    Colors::E = ColorsMenu->AddColorPicker("Draw E Color", "e", 250, 200, 225, 250);
    Colors::R = ColorsMenu->AddColorPicker("Draw R Color", "r", 255, 225, 200, 250);
    Colors::RDistance = ColorsMenu->AddColorPicker("Draw R Distance From Enemies Color", "rdis", 150, 225, 200, 250);

    BUInstance = new BaseUlt(Menu::MenuInstance, Menu::Toggle, Spells::R);
    JSInstance = new JungleSteal(Menu::MenuInstance, Menu::Toggle, Spells::R);

    EventHandler<Events::GameUpdate>::AddEventHandler(OnGameUpdate);
    EventHandler<Events::OnHudDraw>::AddEventHandler(OnHudDraw);
    EventHandler<Events::OnProcessSpellCast>::AddEventHandler(OnProcessSpellCast);
    EventHandler<Events::OnBeforeAttackOrbwalker>::AddEventHandler(OnBeforeAttackOrbwalker);
    EventHandler<Events::OnAfterAttackOrbwalker>::AddEventHandler(OnAfterAttackOrbwalker);
    EventHandler<Events::OnTeleport>::AddEventHandler(OnTeleport);
    EventHandler<Events::OnEndScene>::AddEventHandler(OnEndScene);

    return true;
}

PLUGIN_API void OnUnloadSDK()
{
    EventHandler<Events::GameUpdate>::RemoveEventHandler(OnGameUpdate);
    EventHandler<Events::OnHudDraw>::RemoveEventHandler(OnHudDraw);
    EventHandler<Events::OnProcessSpellCast>::RemoveEventHandler(OnProcessSpellCast);
    EventHandler<Events::OnBeforeAttackOrbwalker>::RemoveEventHandler(OnBeforeAttackOrbwalker);
    EventHandler<Events::OnAfterAttackOrbwalker>::RemoveEventHandler(OnAfterAttackOrbwalker);
    EventHandler<Events::OnTeleport>::RemoveEventHandler(OnTeleport);
    EventHandler<Events::OnEndScene>::RemoveEventHandler(OnEndScene);

    Menu::MenuInstance->Remove();
}
