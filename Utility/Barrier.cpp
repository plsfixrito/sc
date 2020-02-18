#include "Barrier.h"

namespace Barrier
{
    IMenuElement* Toggle = nullptr;
    IMenuElement* UseBarrier = nullptr;
    IMenuElement* UseHP = nullptr;
    IMenuElement* BarrierPercent = nullptr;

    std::shared_ptr<ISpell> Barrier = nullptr;

    bool Loaded = false;

    void OnProcessSpellCast(IGameObject* sender, OnProcessSpellEventArgs* args)
    {
        if (sender == nullptr || !sender->IsEnemy() || args->Target == nullptr || !args->Target->IsMe())
            return;

        try
        {
            if (!Toggle->GetBool() || !UseBarrier->GetBool() || !Barrier->IsReady())
                return;

            if (!args->IsAutoAttack && args->SpellSlot == SpellSlot::Invalid)
                return;

            if (!Extentions::IsKillable(args->Target))
                return;

            auto incdmg = 0;
            if (!sender->IsAIHero())
            {
                incdmg = sender->AutoAttackDamage(args->Target, true);
            }
            else
            {
                incdmg = args->IsAutoAttack ? sender->AutoAttackDamage(args->Target, true) : g_Common->GetSpellDamage(sender, args->Target, args->SpellSlot, false);
            }

            const auto hp = UseHP->GetBool() ? g_HealthPrediction->GetHealthPrediction(args->Target, 100 + g_Common->Ping()) : args->Target->RealHealth(true, true);

            const auto healthAfterDmg = ((hp - incdmg) / args->Target->MaxHealth()) * 100.f;

            if (healthAfterDmg < BarrierPercent->GetInt())
            {
                if (sender->IsAIHero() || healthAfterDmg <= 0)
                {
                    std::string xd = "USE Barrier";
                    g_Common->ChatPrint(xd.c_str());
                    /*
                    std::string inc = " HAD:" + std::to_string(healthAfterDmg);
                    std::string incp = inc + " INCD: " + std::to_string(incdmg);
                    std::string s = sender->BaseSkinName() + incp;
                    g_Common->ChatPrint(s.c_str());
                    */
                    Barrier->Cast();
                }
            }
        }
        catch (const std::exception & e)
        {
            g_Log->PrintToFile(e.what());
        }
    }

    void Load(IMenu* menu, IMenuElement* toggle, SpellSlot slot)
    {
        if (slot == SpellSlot::Invalid)
            return;

        Loaded = true;

        Toggle = toggle;

        const auto hMenu = menu->AddSubMenu("Barrier", "barrier");
        UseBarrier = hMenu->AddCheckBox("Use Barrier", "UseBarrier", true);
        UseHP = hMenu->AddCheckBox("Use Health Prediction", "UseHP", true);
        BarrierPercent = hMenu->AddSlider("Barrier Under HP%", "BarrierPercent", 10, 0, 100);
        Barrier = g_Common->AddSpell(slot, 800.f);

        EventHandler<Events::OnProcessSpellCast>::AddEventHandler(OnProcessSpellCast);
    }

    void Unload()
    {
        if (!Loaded)
            return;

        EventHandler<Events::OnProcessSpellCast>::RemoveEventHandler(OnProcessSpellCast);
    }
}
