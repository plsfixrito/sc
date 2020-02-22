#include "Summs.h"
#include "Items.h"
#include "Tracker.h"
#include "Misc.h"

PLUGIN_API const char PLUGIN_PRINT_NAME[32] = "kUtility";
PLUGIN_API const char PLUGIN_PRINT_AUTHOR[32] = "NotKappa";
PLUGIN_API ChampionId PLUGIN_TARGET_CHAMP = ChampionId::Unknown;

IMenu* MenuInstance = nullptr;
//IMenuElement* Toggle = nullptr;

PLUGIN_API bool OnLoadSDK(IPluginsSDK* plugin_sdk)
{
    DECLARE_GLOBALS(plugin_sdk);

    MenuInstance = g_Menu->CreateMenu("kUtility", "kappa_utility");
    const auto Toggle = MenuInstance->AddCheckBox("Enabled", "global_toggle", true);

    Tracker::Load(MenuInstance, Toggle);
    Summoners::Load(MenuInstance, Toggle);
    Items::Load(MenuInstance, Toggle);
    Misc::Load(MenuInstance, Toggle);

    return true;
}

PLUGIN_API void OnUnloadSDK()
{
    Tracker::Unload();
    Summoners::Unload();
    Items::Unload();
    Misc::Unload();

    MenuInstance->Remove();
}