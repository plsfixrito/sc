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
    float HealthPred = 0;
    float RecallStart = 0;
    float RecallEnd = 0;
    float Duration = 0;
    float TravelTime = 0;
    int Id = 0;
    bool Shoot = false;
    bool IsActive = false;
    Vector EndPosition;
    std::string BaseSkin;

    bool Ended()
    {
        return IsActive && g_Common->TickCount() > RecallEnd;
    }
};


class BaseUlt
{
public:
    std::shared_ptr<ISpell> R;

    std::vector<TrackedRecall*> TrackedRecalls = std::vector<TrackedRecall*>();

    IMenuElement* Toggle = nullptr;
    IMenuElement* Enabled = nullptr;
    IMenu* Targets = nullptr;
    IMenuElement* DisableKey = nullptr;
    IMenuElement* IncludePing = nullptr;

    IMenuElement* DrawRecalls = nullptr;
    IMenuElement* CanUltColor = nullptr;
    IMenuElement* CantUltColor = nullptr;
    IMenuElement* ShotColor = nullptr;
    IMenuElement* BackgroundColor = nullptr;
    IMenuElement* BlackColor = nullptr;

    const Vector BlueSpawn = Vector(400, 420, 182);
    const Vector RedSpawn = Vector(14290, 14393, 172);

    BaseUlt(IMenu* MenuInstance, IMenuElement* toggle, std::shared_ptr<ISpell> spell)
    {
        R = spell;
        Toggle = toggle;

        const auto BUSubMenu = MenuInstance->AddSubMenu("Base Ult", "base_ult");
        Enabled = BUSubMenu->AddCheckBox("Enabled", "toggle", true);

        DisableKey = BUSubMenu->AddKeybind("Force Disable", "disable", 0x32, false, _KeybindType::KeybindType_Hold);
        Targets = BUSubMenu->AddSubMenu("Targets", "targets");

        for (auto& enemy : g_ObjectManager->GetChampions(false))
            Targets->AddCheckBox(enemy->BaseSkinName(), enemy->BaseSkinName(), true)->SetBool(true);

        IncludePing = BUSubMenu->AddCheckBox("Include Ping In Calculations", "IncludePing", true);

        const auto drawings = BUSubMenu->AddSubMenu("Drawings", "draw2");
        DrawRecalls = drawings->AddCheckBox("Draw Recalls", "DrawRecalls", true);
        CantUltColor = drawings->AddColorPicker("Normal Color", "cantult", 100, 200, 250, 250);
        CanUltColor = drawings->AddColorPicker("Can Ult Color", "canult", 50, 255, 150, 250);
        ShotColor = drawings->AddColorPicker("Shoot Color", "shoot", 255, 125, 40, 250);
        BackgroundColor = drawings->AddColorPicker("Background Color", "back", 245, 245, 245, 250);
        BlackColor = drawings->AddColorPicker("black Color", "back2", 0, 0, 0, 250, false);
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
        return (((pos.Distance(g_LocalPlayer->Position()) / spell->Speed()) * 1000.f) + (spell->Delay() * 1000.f)) + (IncludePing->GetBool() ? -g_Common->Ping() : 0);
    }

    bool IsPossible(TrackedRecall* recall)
    {
        if (recall->Ended())
            return false;

        if (recall->LastHealth > recall->Damage)
            return false;

        auto ticksLeft = recall->RecallEnd - g_Common->TickCount();
        auto travelTime = TravelTime(recall->EndPosition, R);
        return ticksLeft > travelTime;
    }

    bool CanUlt(TrackedRecall* recall)
    {
        if (recall->Ended())
            return false;

        if (recall->LastHealth > recall->Damage)
            return false;

        auto ticksLeft = recall->RecallEnd - g_Common->TickCount();
        recall->TravelTime = TravelTime(recall->EndPosition, R);
        auto castOffset = 50 + g_Common->Ping();
        auto mod = ticksLeft - recall->TravelTime;
        return Targets->GetElement(recall->BaseSkin)->GetBool() && castOffset >= mod && ticksLeft > recall->TravelTime;
    }

    void OnUpdate()
    {
        if (!IsEnabled() || !IsReady())
            return;

        for (auto& recall : TrackedRecalls)
        {
            if (!recall->Shoot && CanUlt(recall))
            {
                recall->Shoot = true;
                R->FastCast(recall->EndPosition);
                break;
            }
        }
    }

    void DrawRectProg(Vector2 start, Vector2 end, uint32_t color, float height, float thickness)
    {
        g_Drawing->AddLineOnScreen(start, end, color, height);
        g_Drawing->AddLineOnScreen(Vector2(end.x, end.y - height * .5f), Vector2(end.x, end.y + height * .5f), BlackColor->GetColor(), thickness);
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

    const float width = g_Renderer->ScreenWidth() * 0.1f;
    const float height = g_Renderer->ScreenHeight() * 0.015f;
    
    void OnEndScene()
    {
        if (!IsEnabled() || !DrawRecalls->GetBool())
            return;

        if (TrackedRecalls.size() == 0)
            return;

        auto pos = Vector2(g_Renderer->ScreenWidth() * 0.0130208333333333f, g_Renderer->ScreenHeight() * 0.45f);
        
        g_Drawing->AddTextOnScreen(pos, BackgroundColor->GetColor(), 18, "Tracked Recalls:");
        for (auto& recall : TrackedRecalls)
        {
            pos.y += height;
            g_Drawing->AddTextOnScreen(pos, BackgroundColor->GetColor(), 16, recall->BaseSkin.c_str());
            pos.y += height * 1.5f;

            DrawRect(pos, Vector2(pos.x + width, pos.y), height, 1);

            auto progress = (recall->RecallEnd - g_Common->TickCount()) / recall->Duration;

            if (recall->Shoot)
            {
                DrawRectProg(pos, Vector2(pos.x + (width * progress), pos.y), ShotColor->GetColor(), height, 1);
                pos.y += height * 1.5f;
                continue;
            }

            DrawRectProg(pos, Vector2(pos.x + (width * progress), pos.y), CantUltColor->GetColor(), height, 1);

            if (IsPossible(recall))
            {
                auto at = recall->TravelTime / recall->Duration;
                DrawRectProg(pos, Vector2(pos.x + (width * at), pos.y), CanUltColor->GetColor(), height, 1);
            }
        }
    }

    void OnTeleport(IGameObject* sender, OnTeleportEventArgs* args)
    {
        if (sender == nullptr || !sender->IsEnemy() || !sender->IsAIHero())
            return;

        if (args->Type == OnTeleportEventArgs::TeleportType::Recall && args->Status == OnTeleportEventArgs::TeleportStatus::Start)
        {
            // pls fix from core
            /*auto spawns = g_ObjectManager->GetByType(EntityType::obj_SpawnPoint);
            spawns.erase(std::remove_if(spawns.begin(), spawns.end(),
                [&sender](IGameObject* t) -> bool
                {
                    return t == nullptr || t->Team() != sender->Team();
                }), spawns.end());
            
            if (spawns.size() <= 0)
            {
                g_Common->ChatPrint("no spawn found");
                return;
            }

            auto spawn = spawns[0];
            if (spawn != nullptr)*/
            {
                TrackedRecalls.erase(std::remove_if(TrackedRecalls.begin(), TrackedRecalls.end(),
                    [&sender](TrackedRecall* recall) -> bool
                    {
                        return recall->Ended() || recall->Id == sender->NetworkId();
                    }), TrackedRecalls.end());

                auto tr = new TrackedRecall();
                tr->BaseSkin = sender->BaseSkinName();
                tr->Damage = R->Damage(sender);
                tr->LastHealth = sender->Health();
                tr->RecallStart = g_Common->TickCount();
                tr->Duration = args->Duration;
                tr->RecallEnd = tr->RecallStart + tr->Duration;
                tr->EndPosition = sender->Team() == GameObjectTeam::Order ? BlueSpawn : RedSpawn;
                tr->Id = sender->NetworkId();
                tr->TravelTime = TravelTime(tr->EndPosition, R);
                tr->IsActive = true;

                TrackedRecalls.push_back(tr);
                //g_Log->PrintToFile("added TrackedRecall");
            }
        }
        else if (args->Status == OnTeleportEventArgs::TeleportStatus::Finish || 
                args->Status == OnTeleportEventArgs::TeleportStatus::Abort)
        {
            TrackedRecalls.erase(std::remove_if(TrackedRecalls.begin(), TrackedRecalls.end(),
                [&sender](TrackedRecall* recall) -> bool
                {
                    return recall->Ended() || recall->Id == sender->NetworkId();
                }), TrackedRecalls.end());
        }
    }
};
