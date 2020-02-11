#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventArgs.h"
#include "../SDK/EventHandler.h"
#include "../SDK/PluginSDK_Enums.h"

class TrackedMob
{
public:
    float DamageOnMob = 0;
    float DPS = 0;
    float LastHealth = 0;
    float HealthPred = 0;
    float TravelTime = 0;
    int LastCheck = 0;
    int Id = 0;
    bool Shoot = false;
    std::string BaseSkin;
};

class JungleSteal
{
public:
    std::shared_ptr<ISpell> R;

    std::map<int, TrackedMob*> TrackedMobs = std::map<int, TrackedMob*>();

    IMenuElement* Toggle = nullptr;
    IMenuElement* Enabled = nullptr;
    IMenuElement* StealBaron = nullptr;
    IMenuElement* StealDragon = nullptr;
    IMenuElement* StealRed = nullptr;
    IMenuElement* StealBlue = nullptr;
    IMenuElement* NoAlly = nullptr;
    IMenuElement* NoVision = nullptr;
    IMenuElement* IncludePing = nullptr;
    IMenuElement* DrawDebug = nullptr;
    IMenuElement* TextColor = nullptr;

	JungleSteal(IMenu* MenuInstance, IMenuElement* toggle, std::shared_ptr<ISpell> spell)
	{
		R = spell;
        Toggle = toggle;

        const auto JSSubMenu = MenuInstance->AddSubMenu("Jungle Steal", "jungle_steal");
        Enabled = JSSubMenu->AddCheckBox("Enabled", "toggle", true);

        const auto TargetsMenu = MenuInstance->AddSubMenu("Targets", "js_targets");
        StealBaron = TargetsMenu->AddCheckBox("Steal Baron", "StealBaron", true);
        StealDragon = TargetsMenu->AddCheckBox("Steal Dragon", "StealDragon", true);
        StealRed = TargetsMenu->AddCheckBox("Steal Red", "StealRed", true);
        StealBlue = TargetsMenu->AddCheckBox("Steal Blue", "StealBlue", true);

        NoAlly = JSSubMenu->AddCheckBox("Dont Steal If Allies Are Near", "NoAlly", true);
        NoVision = JSSubMenu->AddCheckBox("Try To Steal With No Vision", "NoVision", true);
        IncludePing = JSSubMenu->AddCheckBox("Include Ping In Calculations", "IncludePing", true);
        DrawDebug = JSSubMenu->AddCheckBox("Enable Debug Drawings", "DrawDebug", false);
        TextColor = JSSubMenu->AddColorPicker("Text Color", "color_text", 0, 255, 250, 250);
	}
    
    bool IsEnabled()
    {
        return Toggle->GetBool() && Enabled->GetBool();
    }

    bool IsReady()
    {
        return !g_LocalPlayer->IsDead() && R->IsReady();
    }

    float TravelTime(IGameObject* target, std::shared_ptr<ISpell> spell)
    {
        return (((target->Distance(g_LocalPlayer) / spell->Speed()) * 1000.f) + (spell->Delay() * 1000)) + (IncludePing->GetBool() ? g_Common->Ping() : 0);
    }

    bool IsDragon(IGameObject* target)
    {
        return target->BaseSkinName().find("sru_dragon_") != std::string::npos;
    }

    bool IsBaron(IGameObject* target)
    {
        return target->BaseSkinName() == "SRU_Baron";
    }

    bool IsRed(IGameObject* target)
    {
        return target->BaseSkinName().find("SRU_Red") != std::string::npos;
    }

    bool IsBlue(IGameObject* target)
    {
        return target->BaseSkinName().find("SRU_Blue") != std::string::npos;
    }

    bool MobEnabled(IGameObject* target)
    {
        return (IsDragon(target) && StealDragon->GetBool()) ||
               (IsBaron(target) && StealBaron->GetBool()) ||
               (IsRed(target) && StealRed->GetBool()) ||
               (IsBlue(target) && StealBlue->GetBool());
    }

    int CountAllies(IGameObject* target, float range = 1000)
    {
        return target->CountMyAlliesInRange(range);
    }

    bool UpdateMob(IGameObject* mob)
    {
        if (!mob->IsValid() || mob->IsDead())
        {
            if (mob->IsDead())
                if (TrackedMobs.count(mob->NetworkId()) > 0)
                    TrackedMobs.erase(mob->NetworkId());

            return false;
        }

        TrackedMob* trackedMob;

        if (TrackedMobs.count(mob->NetworkId()) == 0)
        {
            trackedMob = new TrackedMob();
            trackedMob->BaseSkin = mob->BaseSkinName();
            TrackedMobs[mob->NetworkId()] = trackedMob;
        }
        else
        {
            trackedMob = TrackedMobs[mob->NetworkId()];
        }

        if (mob->IsVisible())
        {
            // check every second
            if (g_Common->TickCount() - trackedMob->LastCheck > 1000)
            {
                // new mob
                if (trackedMob->LastHealth == 0 || trackedMob->Id != mob->NetworkId())
                {
                    trackedMob->Id = mob->NetworkId();
                    trackedMob->LastHealth = mob->Health();
                    trackedMob->LastCheck = g_Common->TickCount();
                    trackedMob->Shoot = false;
                    TrackedMobs[mob->NetworkId()] = trackedMob;
                    return false;
                }

                auto secondsPassed = max(1.f, (g_Common->TickCount() - trackedMob->LastCheck) / 1000.f);
                trackedMob->DPS = (trackedMob->LastHealth - mob->Health()) / secondsPassed;
                trackedMob->LastHealth = mob->Health();
                trackedMob->LastCheck = g_Common->TickCount();
                trackedMob->DamageOnMob = R->Damage(mob);
            }
        }

        if (trackedMob->Id == mob->NetworkId())
        {
            if (g_Common->TickCount() - trackedMob->LastCheck > 5000)
                return false;

            trackedMob->TravelTime = TravelTime(mob, R);
            auto invsTime = mob->IsVisible() ? 0.f : (g_Common->TickCount() - trackedMob->LastCheck) / 1000.f;

            trackedMob->HealthPred = (!mob->IsVisible() ? trackedMob->LastHealth : mob->Health()) - (trackedMob->DPS * invsTime) - (trackedMob->DPS * (trackedMob->TravelTime / 1000.f));

            // TODO IMPROVE THIS WTF
            if (trackedMob->DamageOnMob > trackedMob->HealthPred && trackedMob->HealthPred - trackedMob->DamageOnMob > -trackedMob->DamageOnMob)
            {
                if ((!NoVision->GetBool() && !mob->IsVisible()) ||
                    (NoAlly->GetBool() && CountAllies(mob) > 0))
                {
                    trackedMob->Shoot = false;
                    TrackedMobs[mob->NetworkId()] = trackedMob;
                    return false;
                }

                trackedMob->Shoot = true;
                TrackedMobs[mob->NetworkId()] = trackedMob;
                return true;
            }
        }

        trackedMob->Shoot = false;
        TrackedMobs[mob->NetworkId()] = trackedMob;
        return false;
    }

    void OnGameUpdate()
    {
        if (!Enabled->GetBool())
            return;
        if (TrackedMobs.size() > 0)
        {
            for (auto it = TrackedMobs.begin(); it != TrackedMobs.end();)
            {
                auto mob = g_ObjectManager->GetEntityByNetworkID(it->first);
                (!mob->IsValid() || mob->IsDead()) ? TrackedMobs.erase(it++) : (++it);
            }
        }

        bool skipdrag = false;
        bool skipbaron = false;

        for (auto& tracked : TrackedMobs)
        {
            if (tracked.first != 0)
            {
                auto mob = g_ObjectManager->GetEntityByNetworkID(tracked.first);

                if (IsBaron(mob))
                    skipbaron = true;
                else if (IsDragon(mob))
                    skipdrag = true;
                else
                    continue;

                if (UpdateMob(mob) && IsReady() &&
                    ((IsBaron(mob) && StealBaron->GetBool()) || 
                    (IsDragon(mob) && StealDragon->GetBool())) && IsEnabled()) {

                    R->FastCast(mob->ServerPosition());
                    break;
                }
            }
        }

        if (skipbaron && skipdrag)
            return;

        auto jungleMobs = g_ObjectManager->GetJungleMobs();
        for (auto& mob : jungleMobs)
        {
            if (((!skipdrag && IsDragon(mob) && StealDragon->GetBool()) ||
                (!skipbaron && IsBaron(mob) && StealBaron->GetBool())) &&
                UpdateMob(mob) && IsReady() && IsEnabled()) {

                R->FastCast(mob->ServerPosition());
                break;
            }
        }
    }

    const Vector2 DrawPos = Vector2(g_Renderer->ScreenWidth() * 0.0130208333333333f, g_Renderer->ScreenHeight() * 0.5f);

    void OnHudDraw()
    {
        if (!IsEnabled() || !DrawDebug->GetBool())
            return;

        std::string s = "";
        for (auto& tracked : TrackedMobs)
        {
            s += tracked.second->BaseSkin + " HPred: " + std::to_string(tracked.second->HealthPred) + " | " + std::to_string(tracked.second->DamageOnMob) +
                "\n- Shoot: " + (tracked.second->Shoot ? "true" : "false") + "\n";
        }

        if (s != "")
            g_Drawing->AddTextOnScreen(DrawPos, TextColor->GetColor(), 16, s.c_str());
    }
};
