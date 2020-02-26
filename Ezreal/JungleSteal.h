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
    float AvgDPS = 0;
    float LastHealth = 0;
    float HealthPred = 0;
    float TravelTime = 0;
    int LastCheck = 0;
    bool Shoot = false;
    IGameObject* Mob = nullptr;
    std::string Status = "";
};

class JungleSteal
{
public:
    std::shared_ptr<ISpell> R;

    std::vector<TrackedMob> TrackedMobs = std::vector<TrackedMob>();

    IMenuElement* Toggle = nullptr;
    IMenuElement* Enabled = nullptr;
    IMenuElement* DisableKey = nullptr;

    IMenuElement* StealBaron = nullptr;
    IMenuElement* StealDragon = nullptr;
    IMenuElement* StealRed = nullptr;
    IMenuElement* StealBlue = nullptr;
    IMenuElement* StealMode = nullptr;
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
        DisableKey = JSSubMenu->AddKeybind("Force Disable", "disable", VK_SPACE, false, _KeybindType::KeybindType_Hold);

        const auto TargetsMenu = JSSubMenu->AddSubMenu("Targets", "js_targets");
        StealBaron = TargetsMenu->AddCheckBox("Steal Baron", "StealBaron", true);
        StealDragon = TargetsMenu->AddCheckBox("Steal Dragon", "StealDragon", true);
        StealRed = TargetsMenu->AddCheckBox("Steal Red", "StealRed", true);
        StealBlue = TargetsMenu->AddCheckBox("Steal Blue", "StealBlue", true);

        StealMode = JSSubMenu->AddComboBox("Mode", "stealmode", { "Very Fast (InAccurate)", "Fast", "Moderate (Recommended)", "Accurate (Slow)" }, 2);
        NoAlly = JSSubMenu->AddCheckBox("Dont Steal If Allies Are Near", "NoAlly", true);
        NoVision = JSSubMenu->AddCheckBox("Try To Steal With No Vision", "NoVision", true);
        IncludePing = JSSubMenu->AddCheckBox("Include Ping In Calculations", "IncludePing", true);
        DrawDebug = JSSubMenu->AddCheckBox("Enable Debug Drawings", "DrawDebug", false);
        TextColor = JSSubMenu->AddColorPicker("Text Color", "color_text", 0, 255, 250, 250);
	}
    
    bool IsEnabled()
    {
        return Toggle->GetBool() && Enabled->GetBool() && !DisableKey->GetBool();
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
        return target->Name().find("SRU_Red") != std::string::npos;
    }

    bool IsBlue(IGameObject* target)
    {
        return target->Name().find("SRU_Blue") != std::string::npos;
    }

    bool MobEnabled(IGameObject* target)
    {
        return (IsDragon(target) && StealDragon->GetBool()) ||
               (IsBaron(target) && StealBaron->GetBool()) ||
               (IsRed(target) && StealRed->GetBool()) ||
               (IsBlue(target) && StealBlue->GetBool());
    }

    bool MobEnabled(TrackedMob target)
    {
        return MobEnabled(target.Mob);
    }

    int CountAllies(IGameObject* target, float range = 1000)
    {
        return target->CountMyAlliesInRange(range);
    }

    int CountEnemies(IGameObject* target, float range = 1000)
    {
        return target->CountMyEnemiesInRange(range);
    }

    TrackedMob UpdateMob(TrackedMob trackedMob)
    {
        if (!trackedMob.Mob->IsValid() || trackedMob.Mob->IsDead())
        {
            return trackedMob;
        }
        
        const auto mob = trackedMob.Mob;

        if (mob->IsVisible())
        {
            // check every second
            if (g_Common->TickCount() - trackedMob.LastCheck > 1000)
            {
                // new mob
                if (trackedMob.LastHealth == 0)
                {
                    trackedMob.LastHealth = mob->Health();
                    trackedMob.LastCheck = g_Common->TickCount();
                    trackedMob.Shoot = false;
                    return trackedMob;
                }

                const auto secondsPassed = max(1.f, (g_Common->TickCount() - trackedMob.LastCheck) / 1000.f);
                trackedMob.DPS = (trackedMob.LastHealth - mob->Health()) / secondsPassed;
                if (trackedMob.DPS > 0)
                {
                    if (trackedMob.AvgDPS == 0)
                        trackedMob.AvgDPS += trackedMob.DPS;
                    else 
                    {
                        trackedMob.AvgDPS += trackedMob.DPS;
                        trackedMob.AvgDPS /= 2;
                    }
                }
                else
                {
                    trackedMob.AvgDPS = 0;
                }
                trackedMob.LastHealth = mob->Health();
                trackedMob.LastCheck = g_Common->TickCount();
                trackedMob.DamageOnMob = R->Damage(mob);
            }
        }

        if (g_Common->TickCount() - trackedMob.LastCheck > 5000)
            return trackedMob;

        trackedMob.TravelTime = TravelTime(mob, R);
        const auto invsTime = mob->IsVisible() ? 0.f : (g_Common->TickCount() - trackedMob.LastCheck) / 1000.f;

        const auto dps = trackedMob.DPS;//trackedMob.AvgDPS > 0 ? trackedMob.AvgDPS : trackedMob.DPS;
        trackedMob.HealthPred = (!mob->IsVisible() ? trackedMob.LastHealth : mob->Health()) - (dps * invsTime) - (dps * (trackedMob.TravelTime / 1000.f));

        // TODO IMPROVE THIS WTF
        auto gotDamage = false;

        switch (StealMode->GetInt())
        {
            // very fast
        case 0:
            gotDamage = trackedMob.DamageOnMob * 1.5f >= trackedMob.HealthPred && trackedMob.HealthPred - trackedMob.DamageOnMob > -(trackedMob.DamageOnMob * 1.25f);
            break;
            // fast
        case 1:
            gotDamage = trackedMob.DamageOnMob * 1.25f >= trackedMob.HealthPred && trackedMob.HealthPred - trackedMob.DamageOnMob > -(trackedMob.DamageOnMob * 1.15f);
            break;
            // moderate
        case 2:
            gotDamage = trackedMob.DamageOnMob >= trackedMob.HealthPred && trackedMob.HealthPred - trackedMob.DamageOnMob > -trackedMob.DamageOnMob;
            break;
            // slow
        case 3:
            gotDamage = trackedMob.DamageOnMob >= trackedMob.HealthPred && trackedMob.HealthPred - trackedMob.DamageOnMob > 0;
            break;
        }

        if (gotDamage)
        {
            const auto vision = !mob->IsVisible() && !NoVision->GetBool();
            const auto allies = NoAlly->GetBool() && CountAllies(mob) > 0 && CountEnemies(mob) == 0;
            const auto distance = g_LocalPlayer->Position().Distance(mob->Position()) < 1250.f;
            if (vision || allies || distance)
            {
                trackedMob.Status = vision ? "No Vision" : allies ? "Allies Near" : "In Range";
                trackedMob.Shoot = false;
                return trackedMob;
            }

            trackedMob.Status = "Can Shoot";
            trackedMob.Shoot = true;
            return trackedMob;
        }

        trackedMob.Status = "No Dmg/Time";
        trackedMob.Shoot = false;
        return trackedMob;
    }

    void AddMob(IGameObject* mob)
    {
        if (MobEnabled(mob))
        {
            for (const auto& tracked : TrackedMobs)
                if (tracked.Mob->NetworkId() == mob->NetworkId())
                    return;

            auto tm = TrackedMob();
            tm.Mob = mob;
            TrackedMobs.push_back(tm);
        }
    }

    void OnGameUpdate()
    {
        if (!Enabled->GetBool())
            return;
        
        for (const auto& mob : g_ObjectManager->GetJungleMobs())
            AddMob(mob);

        if (TrackedMobs.empty())
            return;

        TrackedMobs.erase(std::remove_if(TrackedMobs.begin(), TrackedMobs.end(),
            [](TrackedMob mob) -> bool
            {
                return mob.Mob == nullptr || !mob.Mob->IsValid() || mob.Mob->IsDead();
            }), TrackedMobs.end());

        for (auto& tracked : TrackedMobs)
        {
            tracked = UpdateMob(tracked);
            if (tracked.Shoot && IsReady() && IsEnabled())
            {
                R->FastCast(tracked.Mob->ServerPosition());
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
        for (const auto& tracked : TrackedMobs)
        {
            s += tracked.Mob->BaseSkinName() + " HPred: " + std::to_string((int)tracked.HealthPred) + " | " + std::to_string((int)tracked.DamageOnMob) +
                "\n- Status: " + tracked.Status + "\n";
        }

        if (s != "")
            g_Drawing->AddTextOnScreen(DrawPos, TextColor->GetColor(), 16, s.c_str());
    }

    void Unload()
    {
        if (!TrackedMobs.empty())
        {
            TrackedMobs.clear();
        }
    }
};
