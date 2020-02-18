#pragma once
#include "../SDK/PluginSDK.h"
#include "../SDK/EventHandler.h"

namespace SkinHack
{
    void OnGameUpdate();

    void Load(IMenu* mainMenu, IMenuElement* toggle);

    void Unload();
}