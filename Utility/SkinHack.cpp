#include "SkinHack.h"

namespace SkinHack
{
    IMenuElement* Toggle = nullptr;
    IMenuElement* Enabled = nullptr;
    IMenuElement* SelectedSkin = nullptr;
    bool SkinRefresh = true;

    void OnGameUpdate()
    {
        if (!Toggle->GetBool() || !Enabled->GetBool())
            return;

        if (g_LocalPlayer->IsDead())
        {
            SkinRefresh = true;
            return;
        }

        if (SkinRefresh || g_LocalPlayer->GetSkinId() != SelectedSkin->GetInt())
        {
            SkinRefresh = false;
            g_LocalPlayer->SetSkin(SelectedSkin->GetInt(),
                g_ChampionManager->GetDatabase().find(g_LocalPlayer->ChampionId())->second[SelectedSkin->GetInt()].Model);
        }
    }

    void Load(IMenu* mainMenu, IMenuElement* toggle)
    {
        Toggle = toggle;
        const auto SkinMenu = mainMenu->AddSubMenu("Skin Hack", "skin");
        Enabled = SkinMenu->AddCheckBox("Enabled", "SkinHack_" + g_LocalPlayer->ChampionName(), false);

        std::vector<std::string> skins = std::vector<std::string>();
        for (const auto& data : g_ChampionManager->GetDatabase())
            if (data.first == g_LocalPlayer->ChampionId())
            {
                for (const auto& skin : data.second)
                    skins.push_back(skin.SkinName);
                break;
            }

        SelectedSkin = SkinMenu->AddComboBox("", "skin_" + g_LocalPlayer->ChampionName(), skins, 0);

        EventHandler<Events::GameUpdate>::AddEventHandler(OnGameUpdate);
    }

    void Unload()
    {
        EventHandler<Events::GameUpdate>::RemoveEventHandler(OnGameUpdate);
    }
}