#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventArgs.h"
#include "../SDK/EventHandler.h"
#include "../SDK/PluginSDK_Enums.h"

class TrackedRecall
{
public:
    float Damage = 0;
    float LastHealth = 0;
    float LastHealthReg = 0;
    float HealthPred = 0;
    float RecallEnd = 0;
    float Duration = 0;
    float TravelTime = 0;
    float InvsTicks = 0;
    IGameObject* Target = nullptr;
    bool ShoutUpdateHealth = false;
    bool Shoot = false;
    bool Skipped = false;
    Vector EndPosition;

    float Health()
    {
        return HealthPred <= 0 ? LastHealth : HealthPred;
    }

    bool Ended()
    {
        return g_Common->TickCount() >= RecallEnd || Target == nullptr || !Target->IsValid() || Target->IsDead();
    }
};

class BaseUlt
{
public:
    std::shared_ptr<ISpell> R;

    std::vector<TrackedRecall> TrackedRecalls = std::vector<TrackedRecall>();
    std::map<int, int> LastVision = std::map<int, int>();

    IMenuElement* Toggle = nullptr;
    IMenuElement* Enabled = nullptr;
    IMenu* Targets = nullptr;
    IMenuElement* DisableKey = nullptr;
    IMenuElement* IncludePing = nullptr;

    IMenuElement* DrawRecalls = nullptr;
    IMenuElement* RecallWidth = nullptr;
    IMenuElement* RecallY = nullptr;
    //IMenuElement* debugProg = nullptr;
    IMenuElement* CanUltColor = nullptr;
    IMenuElement* CantUltColor = nullptr;
    IMenuElement* ShotColor = nullptr;
    IMenuElement* BackgroundColor = nullptr;
    IMenuElement* BlackColor = nullptr;

    const Vector BlueSpawn = Vector(400, 420, 182);
    const Vector RedSpawn = Vector(14290, 14393, 172);
    
    // if visiable recalling, ticks left more than travel time to base and traveltime less than recall spot, ult recall spot
    // if not visiable recalling since 3s or less, predict based on last path position and apply above, prefer baseult
    // normal base ult
    BaseUlt(IMenu* MenuInstance, IMenuElement* toggle, std::shared_ptr<ISpell> spell)
    {
        R = spell;
        Toggle = toggle;

        const auto BUSubMenu = MenuInstance->AddSubMenu("Base Ult", "base_ult");
        Enabled = BUSubMenu->AddCheckBox("Enabled", "toggle", true);

        DisableKey = BUSubMenu->AddKeybind("Force Disable", "disable", VK_SPACE, false, _KeybindType::KeybindType_Hold);
        Targets = BUSubMenu->AddSubMenu("Targets", "targets");

        for (const auto& enemy : g_ObjectManager->GetChampions(false))
            Targets->AddCheckBox(enemy->ChampionName(), enemy->ChampionName(), true)->SetBool(true);

        IncludePing = BUSubMenu->AddCheckBox("Include Ping In Calculations", "IncludePing", true);

        const auto drawings = BUSubMenu->AddSubMenu("Drawings", "draw2");
        DrawRecalls = drawings->AddCheckBox("Draw Recalls", "DrawRecalls", true);

        RecallWidth = drawings->AddSliderF("Drawing Width", "RecallWidth", 0.35f, 0.1f, 0.9f);
        RecallY = drawings->AddSliderF("Drawing Height", "RecallY", -0.545f, -0.95f, 0.95f);
        //debugProg = drawings->AddSliderF("debug", "debug", 0.1f, .1f, 1.f);
        CantUltColor = drawings->AddColorPicker("Normal Color", "cantult", 100, 200, 250, 250);
        CanUltColor = drawings->AddColorPicker("Can Ult Color", "canult", 50, 255, 150, 250);
        ShotColor = drawings->AddColorPicker("Shoot Color", "shoot", 255, 125, 40, 250);
        BackgroundColor = drawings->AddColorPicker("Background Color", "back", 245, 245, 245, 250);
        BlackColor = drawings->AddColorPicker("back Color", "back3", 255, 255, 255, 255, false);
    }

    bool IsEnabled()
    {
        return Toggle->GetBool() && Enabled->GetBool();
    }

    bool IsReady()
    {
        return !g_LocalPlayer->IsDead() && R->IsReady() && !DisableKey->GetBool();
    }
    
    float TravelTime(Vector pos, std::shared_ptr<ISpell> spell)
    {
        return (((pos.Distance(g_LocalPlayer->Position()) / spell->Speed()) * 1000.f) + (spell->Delay() * 1000.f)) + (IncludePing->GetBool() ? g_Common->Ping() : 0);
    }

    bool IsPossible(TrackedRecall recall)
    {
        if (recall.Ended())
            return false;

        //if (recall.LastHealth > recall.Damage)
        if (recall.Health() >= recall.Damage)
            return false;

        //auto ticksLeft = recall.RecallEnd - g_Common->TickCount();
        return recall.Duration > recall.TravelTime && (recall.RecallEnd - g_Common->TickCount()) > recall.TravelTime;
    }

    bool CanUlt(TrackedRecall recall)
    {
        if (recall.Ended())
            return false;

        if (recall.Health() >= recall.Damage)
            return false;

        recall.TravelTime = TravelTime(recall.EndPosition, R);

        if (recall.TravelTime > recall.Duration)
            return false;

        //auto ticksLeft = recall.RecallEnd - g_Common->TickCount();
        //auto castOffset = 50 + g_Common->Ping()/2;
        //auto mod = ticksLeft - recall.TravelTime;

        return Targets->GetElement(recall.Target->ChampionName())->GetBool() && recall.TravelTime >= (recall.RecallEnd - g_Common->TickCount());
            //&& castOffset >= mod && recall.Duration > recall.TravelTime;//&& ticksLeft > recall.TravelTime;
    }

    void OnUpdate()
    {
        if (!IsEnabled())
            return;

        for (const auto& e : g_ObjectManager->GetChampions(false))
        {
            if (e->IsVisible())
            {
                if (LastVision.count(e->NetworkId()) == 0)
                    LastVision.insert({ e->NetworkId(), g_Common->TickCount() });
                else
                    LastVision[e->NetworkId()] = g_Common->TickCount();
            }
        }

        if (TrackedRecalls.empty())
        {
            TrackedRecalls.clear();
            TrackedRecalls.shrink_to_fit();
            return;
        }

        for (auto& recall : TrackedRecalls)
        {
            if (recall.Shoot || recall.Skipped || recall.Ended())
                continue;

            recall.TravelTime = TravelTime(recall.EndPosition, R);

            if (recall.Target->IsVisible())
            {
                recall.InvsTicks = 0;
                if (recall.ShoutUpdateHealth)
                {
                    recall.ShoutUpdateHealth = false;
                    recall.LastHealth = recall.Target->RealHealth(false, false);
                }

                recall.LastHealthReg = recall.Target->HPRegenRate();
                recall.HealthPred = recall.LastHealth + (recall.LastHealthReg * (recall.Duration / 1000.f));
            }
            else
            {
                if (LastVision.count(recall.Target->NetworkId()) > 0)
                {
                    if (recall.InvsTicks == 0)
                        recall.InvsTicks = g_Common->TickCount() - LastVision[recall.Target->NetworkId()];
                    recall.HealthPred = recall.LastHealth + (recall.LastHealthReg * ((recall.InvsTicks / 1000.f) + (recall.Duration / 1000.f)));
                }
                else
                {
                    recall.HealthPred = recall.LastHealth + (recall.LastHealthReg * (recall.Duration / 1000.f));
                }
            }

            if (recall.HealthPred >= recall.Target->MaxHealth())
                recall.HealthPred = recall.Target->MaxHealth();
            else if (recall.HealthPred == 0)
                recall.HealthPred = recall.LastHealth;

            if (CanUlt(recall))
            {
                if (IsReady())
                {
                    recall.Shoot = true;
                    R->FastCast(recall.EndPosition);
                }
                else
                {
                    recall.Skipped = true;
                }
                break;
            }
        }
    }

    void DrawRectProg(Vector2 start, Vector2 end, uint32_t color, float height, float thickness)
    {
        g_Drawing->AddLineOnScreen(start, end, color, height);
        g_Drawing->AddLineOnScreen(Vector2(end.x, end.y - height), Vector2(end.x, end.y + height * .5f), BlackColor->GetColor(), thickness);
    }

    void DrawRect(Vector2 start, Vector2 end, float height, float thickness)
    {
        auto x1 = start.x;
        auto y1 = start.y - height * .5f;
        auto x2 = end.x;
        auto y2 = end.y - height * .5f;

        g_Drawing->AddLineOnScreen(Vector2(x1, y1), Vector2(x2, y2), BackgroundColor->GetColor(), thickness);

        x1 = x2;
        y1 = y2;
        x2 = end.x;
        y2 = end.y + height * .5f;

        g_Drawing->AddLineOnScreen(Vector2(x1, y1), Vector2(x2, y2), BackgroundColor->GetColor(), thickness);

        x1 = start.x;
        y1 = start.y + height * .5f;

        g_Drawing->AddLineOnScreen(Vector2(x1, y1), Vector2(x2, y2), BackgroundColor->GetColor(), thickness);

        x2 = start.x;
        y2 = start.y - height * .5f;

        g_Drawing->AddLineOnScreen(Vector2(x1, y1), Vector2(x2, y2), BackgroundColor->GetColor(), thickness);
    }

    //float width = g_Renderer->ScreenWidth() * 0.25f;
    const float height = g_Renderer->ScreenHeight() * 0.01f;
    const Vector2 center = Vector2(g_Renderer->ScreenWidth() / 2, g_Renderer->ScreenHeight() / 2);
    
    void OnEndScene()
    {
        if (!IsEnabled() || !DrawRecalls->GetBool())
            return;

        if (TrackedRecalls.empty())
            return;

        auto start = Vector2(center.x, center.y);
        start.x = start.x - (start.x * RecallWidth->GetFloat());
        start.y = start.y - (start.y * RecallY->GetFloat());

        auto end = Vector2(center.x, start.y);
        end.x = end.x + (end.x * RecallWidth->GetFloat());
        auto width = end.x - start.x;

        DrawRect(start, end, height, 1);

        auto pos = Vector2(start.x , end.y);
        auto i = 0;
        for (auto& recall : TrackedRecalls)
        {
            if (recall.Ended())
                continue;

            const auto progress = (recall.RecallEnd - g_Common->TickCount()) / recall.Duration;
            const auto endx = pos.x + (width * progress);

            if (recall.Shoot)
            {
                DrawRectProg(pos, Vector2(endx, pos.y), ShotColor->GetColor(), height, 1);
                //pos.y += height * 1.5f;
            }
            else if (IsPossible(recall))
            {
                DrawRectProg(pos, Vector2(endx, pos.y), CantUltColor->GetColor(), height, 1);
                const auto at = recall.TravelTime / recall.Duration;
                DrawRectProg(pos, Vector2(pos.x + (width * at), pos.y), CanUltColor->GetColor(), height, 1);
            }
            else
            {
                DrawRectProg(pos, Vector2(endx, pos.y), CantUltColor->GetColor(), height, 1);
            }

            i++;
            const std::string s = recall.Target->ChampionName() + " (" + std::to_string((int)recall.LastHealth) + " | " + std::to_string((int)recall.HealthPred) + ")";
            g_Drawing->AddTextOnScreen(Vector2(endx + 10, pos.y - ((height * 2.f) * i)), BackgroundColor->GetColor(), 16, s.c_str());
        }
    }

    void OnTeleport(IGameObject* sender, OnTeleportEventArgs* args)
    {
        if (sender == nullptr || !sender->IsEnemy() || !sender->IsAIHero())
            return;

        if (args->Type == OnTeleportEventArgs::TeleportType::Recall && args->Status == OnTeleportEventArgs::TeleportStatus::Start)
        {
            TrackedRecalls.erase(std::remove_if(TrackedRecalls.begin(), TrackedRecalls.end(),
                [&sender](TrackedRecall recall) -> bool
                {
                    return recall.Ended() || recall.Target->NetworkId() == sender->NetworkId();
                }), TrackedRecalls.end());

            auto tr = TrackedRecall();
            tr.Target = sender;
            tr.Damage = R->Damage(sender);
            tr.LastHealth = sender->RealHealth(false, false);
            tr.LastHealthReg = sender->HPRegenRate();
            tr.Duration = sender->HasBuff("exaltedwithbaronnashor") ? args->Duration / 2 : args->Duration;
            tr.RecallEnd = g_Common->TickCount() + tr.Duration;
            tr.EndPosition = sender->Team() == GameObjectTeam::Order ? BlueSpawn : RedSpawn;
            tr.TravelTime = TravelTime(tr.EndPosition, R);
            tr.Skipped = tr.TravelTime > tr.Duration;
            tr.ShoutUpdateHealth = !sender->IsVisible();
            if (!sender->IsVisible() && LastVision.count(sender->NetworkId()) > 0)
                tr.InvsTicks = g_Common->TickCount() - LastVision[sender->NetworkId()];

            tr.HealthPred = tr.LastHealth + (tr.LastHealthReg * (tr.InvsTicks / 1000.f)) + (tr.LastHealthReg * (tr.Duration / 1000.f));

            TrackedRecalls.push_back(tr);
        }
        else if (args->Status == OnTeleportEventArgs::TeleportStatus::Finish || 
                args->Status == OnTeleportEventArgs::TeleportStatus::Abort)
        {
            TrackedRecalls.erase(std::remove_if(TrackedRecalls.begin(), TrackedRecalls.end(),
                [&sender](TrackedRecall recall) -> bool
                {
                    return recall.Ended() || recall.Target->NetworkId() == sender->NetworkId();
                }), TrackedRecalls.end());
        }
    }

    void Unload()
    {
        if (!TrackedRecalls.empty())
        {
            TrackedRecalls.clear();
            TrackedRecalls.shrink_to_fit();
        }

        if (!LastVision.empty())
        {
            LastVision.clear();
        }
    }
};
