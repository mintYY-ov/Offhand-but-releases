#include "dllmain.hpp"
#include <functional>
#include <amethyst/runtime/events/RegisterEvents.hpp>
#include <mc/src/common/world/item/registry/ItemRegistry.hpp>
#include "features/SwapOffhandKey.hpp"
#include "features/Offhand.hpp"
#include "features/OffhandHud.hpp"
#include "features/OffhandRendering.hpp"

void OnRegisterItems(RegisterItemsEvent &event)
{
    Log::Info("OnRegisterItems");

    for (auto& pair : event.itemRegistry.mIdToItemMap)
    {
        pair.second.get()->setAllowOffhand(true);
    }
    
}

// Ran when the mod is loaded into the game by AmethystRuntime
ModFunction void Initialize(AmethystContext& ctx, const Amethyst::Mod& mod)
{
    Amethyst::InitializeAmethystMod(ctx, mod);
    Amethyst::GetEventBus().AddListener<RegisterItemsEvent>(&OnRegisterItems);

    //Amethyst::GetContext().mFeatures->enableInputSystem = true;
    Amethyst::EventBus& events = Amethyst::GetEventBus();
    Amethyst::HookManager& hooks = Amethyst::GetHookManager();

    RegisterSwapOffhandKey();
    RegisterOffhandHooks();
    RegisterOffhandHud();
    RegisterOffhandRendering();
}
