#pragma once
#include <amethyst/runtime/ModContext.hpp>
#include <amethyst/runtime/events/InputEvents.hpp>
#include <mc/src/common/network/packet/Packet.hpp>
#include <mc/src/common/network/PacketHandlerDispatcherInstance.hpp>
#include <mc/src/common/network/PacketSender.hpp>
#include <mc/src-client/common/client/player/LocalPlayer.hpp>
#include <mc/src/common/network/ServerNetworkHandler.hpp>
#include <mc/src/common/server/ServerPlayer.hpp>
#include <amethyst/Formatting.hpp>
#include <mc/src-deps/input/MouseDevice.hpp>
#include <mc/src/common/world/entity/components/ActorEquipmentComponent.hpp>
#include <mc/src/common/world/level/ILevel.hpp>
#include <mc/src/common/world/actor/player/Inventory.hpp>
#include <mc/src/common/world/SimpleContainer.hpp>

class SwapOffhandPacket : public Amethyst::CustomPacket {
public:
    virtual std::string getName() const { return "SwapOffhandPacket"; };
    virtual void write(BinaryStream& out) {};
    virtual Bedrock::Result<void, std::error_code> read(ReadOnlyBinaryStream& in) {
        return Bedrock::Result<void, std::error_code>();
    };
};

class SwapOffhandPacketHandler : public Amethyst::CustomPacketHandler {
public:
    virtual void handle(const NetworkIdentifier& networkId, NetEventCallback& netEvent, const Amethyst::CustomPacket& _packet) const override {
		Log::Info("SwapOffhandPacketHandler::handle");

        //SwapOffhandPacket& packet = *(SwapOffhandPacket*)_packet.get();
        ServerNetworkHandler& serverNetwork = (ServerNetworkHandler&)netEvent;

        uintptr_t** address = *(uintptr_t***)&serverNetwork;
		
        ServerPlayer* serverPlayer = serverNetwork._getServerPlayer(networkId, SubClientId::PrimaryClient);

        if (serverPlayer == nullptr) {
            Log::Info("SwapOffhandPacket: ServerPlayer not found?");
            return;
		}

		const PlayerInventory& inventory = serverPlayer->getSupplies();
		const ItemStack& mainhandStack = inventory.getSelectedItem();
		ItemStack mainhandCopy = mainhandStack;

        ActorEquipmentComponent& equipment = *serverPlayer->tryGetComponent<ActorEquipmentComponent>();
        const ItemStack& offhandStack = equipment.mHand->mItems[1];
        ItemStack offhandCopy = offhandStack;

        inventory.mInventory->createTransactionContext(
            [](Container& container, int slot, ItemStack const& from, ItemStack const& to) {
                
            },
            [&inventory, offhandCopy]() {
                inventory.mInventory->setItem(inventory.mSelected, offhandCopy);
            }
        );

        equipment.mHand->createTransactionContext(
            [](Container& container, int slot, ItemStack const& from, ItemStack const& to) {

            },
            [&equipment, mainhandCopy]() {
                equipment.mHand->setItem(1, mainhandCopy);
            }
        );
    }
};

void RegisterKeyListener(RegisterInputsEvent& ev) {
	Amethyst::InputManager& inputManager = ev.inputManager;
	Amethyst::InputAction& swapOffhandKey = inputManager.RegisterNewInput("swap_input", { 'F' }, true, Amethyst::KeybindContext::Gameplay);

    swapOffhandKey.addButtonDownHandler([](FocusImpact focus, ClientInstance& client) {
        LocalPlayer& player = *client.getLocalPlayer();
		ILevel& level = *player.getLevel();
        PacketSender& packetSender = *level.getPacketSender();

        Amethyst::GetNetworkManager().SendToServer(packetSender, std::make_unique<SwapOffhandPacket>());

		// Swap items clientside for instant feedback
        const PlayerInventory& inventory = player.getSupplies();
        const ItemStack& mainhandStack = inventory.getSelectedItem();
		ItemStack mainhandCopy = mainhandStack;

        const ActorEquipmentComponent& equipment = *player.tryGetComponent<ActorEquipmentComponent>();
        const ItemStack& offhandStack = equipment.mHand->mItems[1];
		ItemStack offhandCopy = offhandStack;

        inventory.mInventory->createTransactionContext(
            [](Container& container, int slot, ItemStack const& from, ItemStack const& to) {

            },
            [&inventory, offhandCopy]() {
                inventory.mInventory->setItem(inventory.mSelected, offhandCopy);
            }
        );

        equipment.mHand->createTransactionContext(
            [](Container& container, int slot, ItemStack const& from, ItemStack const& to) {

            },
            [&equipment, mainhandCopy]() {
                equipment.mHand->setItem(1, mainhandCopy);
            }
        );

		return Amethyst::InputPassthrough::Consume;
    });
}

void RegisterSwapOffhandKey() {
    Amethyst::HookManager& hooks = Amethyst::GetHookManager();
	Amethyst::EventBus& events = Amethyst::GetEventBus();
	Amethyst::NetworkManager& networkManager = Amethyst::GetNetworkManager();

	events.AddListener<RegisterInputsEvent>(&RegisterKeyListener);
    networkManager.RegisterPacketType<SwapOffhandPacket>(std::make_unique<SwapOffhandPacketHandler>());
}
