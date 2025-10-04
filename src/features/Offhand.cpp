#include "features/Offhand.hpp"
#include <amethyst/runtime/events/RegisterEvents.hpp>
#include <mc/src/common/world/item/registry/ItemRegistry.hpp>
#include <mc/src/common/world/actor/player/Player.hpp>
#include <mc/src/common/world/actor/player/PlayerInventory.hpp>
#include <mc/src/common/world/actor/player/Inventory.hpp>
#include <mc/src/common/world/level/BlockPos.hpp>
#include <mc/src/common/world/level/ILevel.hpp>
#include <mc/src/common/world/level/BlockSource.hpp>
#include <mc/src/common/world/entity/components/ActorEquipmentComponent.hpp>
#include <mc/src/common/world/entity/components/StateVectorComponent.hpp>
#include <mc/src/common/world/gamemode/GameMode.hpp>
#include <mc/src/common/world/item/ItemDescriptorCount.hpp>
#include <mc/src/common/world/inventory/transaction/ItemUseInventoryTransaction.hpp>
#include <mc/src/common/world/Facing.hpp>
#include <mc/src/common/world/gamemode/GameModeMessenger.hpp>
#include <mc/src/common/world/inventory/network/ItemStackNetManagerBase.hpp>
#include <mc/src/common/world/events/PlayerEventListener.hpp>
#include <mc/src/common/world/inventory/transaction/ComplexInventoryTransaction.hpp>
#include <mc/src/common/world/inventory/transaction/InventoryTransactionManager.hpp>
#include <mc/src/common/world/level/BlockSource.hpp>
#include <mc/src/common/world/level/Level.hpp>
#include <mc/src/common/world/level/BlockPalette.hpp>
#include <mc/src/common/world/events/BlockEventCoordinator.hpp>
#include <mc/src-client/common/client/renderer/game/ItemInHandRenderer.hpp>
#include <mc/src-deps/renderer/MatrixStack.hpp>
#include <amethyst/runtime/events/InputEvents.hpp>
#include <amethyst/runtime/HookManager.hpp>
#include <amethyst/runtime/ModContext.hpp>
#include "features/OffhandSwingComponent.hpp"
#include <mc/src/common/world/SimpleContainer.hpp>

extern "C" void *ItemUseInventoryTransaction_ctor = nullptr;
extern "C" void *NetworkItemStackDescriptor_ctor = nullptr;

class PlayerEventCoordinator
{
public:
    void todoMoveToProperClass_processEvent(std::function<EventResult(class PlayerEventListener &)> &ev)
    {
        using function = decltype(&PlayerEventCoordinator::todoMoveToProperClass_processEvent);
        static auto func = std::bit_cast<function>(SigScan("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 4C 8B EA 48 8B F1"));
        (this->*func)(ev);
    }
};

SafetyHookInline _GameMode_buildBlock;

SafetyHookInline _GameMode_useItemOn;

//InteractionResult GameMode_useItemOn(GameMode* self, ItemStack& item, const BlockPos& at, FacingID face, const Vec3& hit, const Block* targetBlock)
// Returning an InteractionResult directly is for some reason returning in a different register than the one used by the game
// force it lmao
InteractionResult* GameMode_useItemOn(GameMode* self, InteractionResult* result, ItemStack& item, const BlockPos& at, FacingID face, const Vec3& hit, const Block* targetBlock)
{
    // todo: seems to set self+0x40 to 0
    bool isSneakDown = false; // todo: get this

    result->mResult = 0;

    // idk what this does but it doesn't seem to have anything to do with networking / decreasing item stack
    if ((self->_sendUseItemOnEvents(item, at, face, hit).mResult & (int)InteractionResult::Result::SUCCESS) == 0) return result;

    // todo: check that the player has use ability
    bool hasPermissions = true; 
    if (!hasPermissions) return result;

    BlockSource& region = self->mPlayer.getDimensionBlockSource();
	const Block& block = region.getBlock(at);
	const Block& liquidBlock = region.getLiquidBlock(at);
	const BlockLegacy& blockLegacy = *block.mLegacyBlock.get();

    // todo: game checks if its invisible bedrock

    if (targetBlock) {
		uint64_t blockSerializationHash = targetBlock->mSerializationIdHash;

        if (block.mSerializationIdHash != blockSerializationHash && liquidBlock.mSerializationIdHash != blockSerializationHash)
			return result;
    }

	const BlockSource& constRegion = self->mPlayer.getDimensionBlockSourceConst();

    AABB blockCollision = AABB::BLOCK_SHAPE;
    bool isBlockSolid = block.getCollisionShape(blockCollision, constRegion, at, nullptr);
    bool isItemBucket = item.isInstance("minecraft:bucket", false);

	const HashedString& blockFullName = blockLegacy.mNameInfo.mFullName;
	bool isPowderedSnow = blockFullName == "minecraft:powder_snow";
    bool canContainLiquid = blockLegacy.canContainLiquid();

	// Handles clicking with buckets on non-powdered snow blocks, or blocks that cannot contain liquids
	if (isBlockSolid && isItemBucket && !isPowderedSnow && !canContainLiquid) return result;

	BlockLegacy* blockToUse = nullptr;

    bool itemCanInteractWithBlock = item.hasTag("minecraft:is_axe")
        || item.hasTag("minecraft:is_shovel")
        || item.hasTag("minecraft:is_hoe")
        || item.isInstance("minecraft:honeycomb", false);

    if (!isSneakDown || itemCanInteractWithBlock || !item) {
		blockToUse = block.mLegacyBlock;
        bool isAir = blockToUse->isAir();

        if (!isAir && block.use(self->mPlayer, at, face, hit)) {
			self->mBuildContext.mLastBuildBlockWasInteractive = block.isInteractiveBlock();

            bool wasSnappable = false;

            if (!self->mBuildContext.mLastBuildBlockWasInteractive && item.isBlock()) {
                wasSnappable = blockToUse->isSnappableBlock();
            }

            // This is probably just for scripting so dont care
			// ItemEventCoordinator::sendEvent(v42, &v67); <- ItemUsedOnEvent
            self->_sendPlayerInteractWithBlockAfterEvent(self->mPlayer, at, face, hit);

			self->mBuildContext.mLastBuildBlockWasSnappable = wasSnappable;
			result->mResult = (int)InteractionResult::Result::SUCCESS | (int)InteractionResult::Result::SWING;

            return result;
        }
    }
     
    if (!item) {
        Log::Info("Todo handle !item scenario");
		return result;
    }

    if (!item.isBlock()) {
        self->mBuildContext.mLastBuildBlockWasSnappable = blockLegacy.isSnappableBlock();

        bool isCreative = false;
		InteractionResult useResult;

        if (isCreative) {
            Log::Info("todo handle creative");
        }
        else {
            useResult.mResult = item.useOn(self->mPlayer, at.x, at.y, at.z, face, hit).mResult;
        }

        if ((useResult.mResult & (int)InteractionResult::Result::SUCCESS) != 0) {
            
            result->mResult = useResult.mResult; 
            return result;
		}

        if (item) {
            Log::Info("Todo: Player::setSelectedItem()");
        }
        else {
            Log::Info("Todo: PlayerInventory::clearSlot()");
        }

        Log::Info("Todo: GameMode::_sendPlayerInteractWithBlockAfterEvent");

        return result;
    }

    blockToUse = item.getLegacyBlock();
    const Block& renderBlock = blockToUse->getRenderBlock();

    if (self->_canUseBlock(renderBlock)) {
        bool isInCreative = false;

        if (isInCreative) {
			Log::Info("Todo handle Using block in creative mode");
            return result;
        }
        else {
            result->mResult = item.useOn(self->mPlayer, at.x, at.y, at.z, face, hit).mResult;
        }
    }

    return result;
}

bool tryUseItem(GameMode &self, const ItemStack &stack, const Player &player, Container &container, int slot, ContainerID containerId, const BlockPos &pos, FacingID face, bool isSimTick, bool isOffhand)
{
    const Item *item = stack.getItem();
    // I don't know what the fuck this is, or what it filters out xD.
    bool realMayUse = !stack.mValid || stack.mItem == nullptr || stack.isNull() || stack.mCount == 0 || !isSimTick || !item || item->canUseOnSimTick();

    if (!realMayUse)
    {
        Log::Info("May not use item: {}", stack);
        return false;
    }

    self.mPlayerLastPosition = *player.getPosition();
    std::unique_ptr<ItemUseInventoryTransaction> transaction = std::make_unique<ItemUseInventoryTransaction>();

    bool useLegacyTransaction = player.isClientSide() && player.mItemStackNetManager != nullptr;
    auto onScopeClosed = useLegacyTransaction
                             ? player.mItemStackNetManager->_tryBeginClientLegacyTransactionRequest()
                             : gsl::final_action<std::function<void()>>([]() {});

    InteractionResult result;

    container.createTransactionContext(
        [&transaction, &self, containerId](Container &container, unsigned int slot, const ItemStack &oldStack, const ItemStack &newStack)
        {
            InventorySource source(InventorySourceType::ContainerInventory, containerId, InventorySource::InventorySourceFlags::NoFlag);
            InventoryAction action(source, slot, oldStack, newStack);

            self.mPlayer.mTransactionManager.addExpectedAction(action);
            transaction->mTransaction.addAction(action);
        },
        [&transaction, &self, pos, face, &result, &stack, slot, isOffhand]()
        {
            Player &player = self.mPlayer;
            ILevel &level = *player.mLevel;

            ItemStack stackCopy = stack;
            ItemInstance itemInstance(stackCopy);
            NetworkItemStackDescriptor networkDescriptor(stackCopy);

            transaction->mItem = networkDescriptor;
            transaction->mSlot = slot;

            HitResult hitResult = level.getHitResult();
            HitResult liquidHit = level.getLiquidHitResult();

            const BlockSource &region = player.getDimensionBlockSourceConst();
            const Block &block = region.getBlock(pos);

            FacingID faceDir = face;
            BlockPos calculatedPlacePos = self._calculatePlacePos(stackCopy, pos, faceDir);

            if (hitResult.mType == HitResultType::TILE || liquidHit.mLiquid.y == 0 || self.mBuildContext.mHasBuildDirection)
            {
                transaction->mPos = pos;
                transaction->setTargetBlock(block);
                transaction->mFace = faceDir;

                Vec3 clickPos;

                if (hitResult.mType == HitResultType::NO_HIT)
                {
                    clickPos = Vec3::ZERO;
                }
                else
                {
                    clickPos = hitResult.mPos - Vec3(pos);
                }

                transaction->mClickPos = clickPos;
                transaction->mFromPos = *player.getPosition();
                transaction->mActionType = face == FacingID::MAX ? ItemUseInventoryTransaction::ActionType::Use : ItemUseInventoryTransaction::ActionType::Place;

                // if (block.isFenceBlock() && LeadItem::canBindPlayerMobs) {
                if (block.isFenceBlock() && false)
                {
                    result.mResult = (int)InteractionResult::Result::SUCCESS | (int)InteractionResult::Result::SWING;
                }
                else
                {
					GameMode_useItemOn(&self, &result, stackCopy, pos, face, hitResult.mPos, nullptr);
                }
            }

            // This section handles liquid clipped items, i.e. lillypads on water
            bool isLiquidClippingItem = (result.mResult & (int)InteractionResult::Result::SUCCESS) == 0 && !stackCopy.isNull() && stackCopy.isLiquidClipItem() && !stackCopy.shouldInteractionWithBlockBypassLiquid(block);

            if (isLiquidClippingItem)
            {
                Log::Info("Todo: Handle liquid clipping items");
            }

            // Block placement timings
            if ((result.mResult & (int)InteractionResult::Result::SUCCESS) != 0)
            {
                self.mBuildContext.mLastBuiltBlockPosition = calculatedPlacePos;

                // Seems to get reset often enogu to be false each item use.
                if (!self.mBuildContext.mHasLastBuiltPosition)
                {
                    //self.mMessenger->startSendItemUseOn(pos, self.mBuildContext.mLastBuiltBlockPosition, (int)face);
                    self.mBuildContext.mHasLastBuiltPosition = true;
                }

                self.mLastBuildTime = std::chrono::steady_clock::now();
                if ((result.mResult & (int)InteractionResult::Result::SWING) != 0)
                {
                    if (!isOffhand) player.swing();

                    else {
						OffhandSwingComponent* _swingComp = player.tryGetComponent<OffhandSwingComponent>();
                        if (!_swingComp) {
							player.addComponent<OffhandSwingComponent>();
                        }

						OffhandSwingComponent* swingComp = player.tryGetComponent<OffhandSwingComponent>();
                        
                        // player.swing() implemented using OffhandSwingComponent instead off mainhand.
                        int currentSwingDuration = player.getCurrentSwingDuration();
                        int time = swingComp->mOffhandSwingTime;

                        if (!swingComp->mOffhandSwinging || time >= currentSwingDuration / 2 || time < 0) {
							swingComp->mOffhandSwinging = true;
							swingComp->mOffhandSwingTime = -1;
                        }
                    }
                }
            }
        });

    if (player.isClientSide())
    {
        player.sendComplexInventoryTransaction(std::move(transaction));
    }

    if (result.mResult & (int)InteractionResult::Result::SUCCESS)
    {
        return true;
	}

    return false;
}

// Modded version of GameMode::buildBlock that supports offhand items
// note: the game only seems to call this on the client?
// I wasn't able to see it called on the server when hooking it and logging the caller
bool GameMode_buildBlock(GameMode *self, BlockPos *pos, FacingID face, bool isSimTick)
{
    
    Player &player = self->mPlayer;
    ILevel &level = *player.mLevel;
    PlayerInventory &playerInv = *player.playerInventory;
    Inventory &inv = *playerInv.mInventory.get();

    const ItemStack &mainHandItem = playerInv.getSelectedItem();
    ActorEquipmentComponent *equipment = player.tryGetComponent<ActorEquipmentComponent>();
    const ItemStack &offHandItem = equipment->mHand->mItems[1];

    bool usedMainhand = tryUseItem(*self, mainHandItem, player, inv, playerInv.mSelected, ContainerID::CONTAINER_ID_INVENTORY, *pos, face, isSimTick, false);
    if (usedMainhand) return true;
    
    return tryUseItem(*self, offHandItem, player, *equipment->mHand, 1, ContainerID::CONTAINER_ID_OFFHAND, *pos, face, isSimTick, true);
}

SafetyHookInline _ItemUseInventoryTransaction_handle;

InventoryTransactionError ItemUseInventoryTransaction_handle(ItemUseInventoryTransaction* self, Player& player, bool isSenderAuthority) {
    if (!player.isAlive()) {
		Log::Warning("InventoryTransactionError::StateMismatch - Player is not alive");
		return InventoryTransactionError::StateMismatch;
    }

	Level& level = *player.getLevel()->asLevel();
	bool isClientSide = level.isClientSide;
    BlockPalette& blockPalette = level.getBlockPalette();

    ItemStack stack = ItemStack::fromDescriptor(self->mItem, blockPalette, isClientSide);

    PlayerInventory& playerInv = *player.playerInventory;
    Inventory& inv = *playerInv.mInventory.get();
    const ItemStack& mainHandItem = playerInv.getSelectedItem();
    ActorEquipmentComponent* equipment = player.tryGetComponent<ActorEquipmentComponent>();
    const ItemStack& offHandItem = equipment->mHand->mItems[1];

    // This could probably be more verbose in checking, but the logic is if the two stacks are the same, it was probably the mainhand?
    // There probably is some weird edgecase around this
    bool usedMainhand = stack.mItem == mainHandItem.mItem;
    const ItemStack& stackToUse = usedMainhand ? mainHandItem : offHandItem;
    Container* containerToUse = usedMainhand ? &inv : (Container*)equipment->mHand.get();

    bool itemsMatch = true; // InventoryTransaction::checkTransactionItemsMatch
	bool slotsMatch = true; // self->mSlot == playerInv.mSelected; <- to check properly probs need to switch between offhand and inv slots

    bool areItemsAndSlotsValid = isSenderAuthority || itemsMatch && slotsMatch;

    if (!areItemsAndSlotsValid) {
		Log::Warning("InventoryTransactionError::ProbablyError - Items or slots do not match");
        return InventoryTransactionError::ProbablyError;
    }

    GameMode& gameMode = player.getGameMode();
    float maxPickRange = gameMode.getMaxPickRange() + 0.5f;
    bool distanceCheck = player.distanceTo(self->mPos) <= maxPickRange;

    bool isValidAction = self->mActionType == ItemUseInventoryTransaction::ActionType::Use || distanceCheck || isSenderAuthority;

    if (!isValidAction) {
        return InventoryTransactionError::ProbablyError;
    }

    ContainerID containerId = usedMainhand ? ContainerID::CONTAINER_ID_INVENTORY : ContainerID::CONTAINER_ID_OFFHAND;

	InventorySource source(
        InventorySourceType::ContainerInventory, 
        containerId,
        InventorySource::InventorySourceFlags::NoFlag
    );

	const std::vector<InventoryAction>& actions = self->mTransaction.getActions(source);
    
    for (auto& action : actions) {
        Log::Info("Todo: action not handled!!");
        // todo: game does something, there are no actions sent for placing blocks it seems?
    }

    playerInv.createTransactionContext(
        [&self, &player, containerId](Container& container, unsigned int slot, const ItemStack& oldStack, const ItemStack newStack) {
            InventorySource source(InventorySourceType::ContainerInventory, containerId, InventorySource::InventorySourceFlags::NoFlag);
            InventoryAction action(source, slot, oldStack, newStack);
            player.mTransactionManager.addExpectedAction(action);
        },
        [&self, &gameMode, &stackToUse, &level, &containerToUse]() {
            if (self->mActionType == ItemUseInventoryTransaction::ActionType::Use) {
				ItemStack stackCopy = ItemStack(stackToUse);

                // Seems to return false for things the server needs to update the client on
                // i.e. stone block returns true
                // but torch item returns false
                if (!gameMode.baseUseItem(stackCopy)) {
					self->resendPlayerState(gameMode.mPlayer);
                }
            }
            else if (self->mActionType == ItemUseInventoryTransaction::ActionType::Destroy) {
				Log::Info("todo impl self->mActionType == ItemUseInventoryTransaction::ActionType::Destroy");
            }
            else if (self->mActionType == ItemUseInventoryTransaction::ActionType::Place) {
				bool isClientSide = level.isClientSide;
                BlockPalette& blockPalette = level.getBlockPalette();
                ItemStack stackCopy = ItemStack(stackToUse);

                // Idk why this exists or what the fuck it does... 
                //if (!Actor::isSneaking(*(a1 + 8)))
                //{
                //    v8 = Player::getSelectedItem(*(a1 + 8));
                //    ItemStack::operator=(&itemStack, v8);
                //}

				const Block& blockToPlaceOn = blockPalette.getBlock(self->mTargetBlockId);

                Block* v18 = nullptr;
                //Actor::setPos(*(a1 + 8), (*a1 + 232i64));

                /*while (1)
                {
                    v18 = *v17;
                    if ((*v17)->mSerializationIdHash == v14->mSerializationIdHash)
                        break;
                    mLegacyBlock = v18->mLegacyBlock;
                    if (!mLegacyBlock || (v16 = v14->mLegacyBlock) == 0i64)
                        gsl::details::terminate(v16);
                    if (mLegacyBlock == v16 && Block::allowStateMismatchOnPlacement(*v17, v14))
                        break;
                    if (++v17 == &v42)
                        goto LABEL_17;
                }*/

                InteractionResult result = gameMode.useItemOn(stackCopy, self->mPos, self->mFace, self->mClickPos, v18);
                // Actor::setPos(*(a1 + 8), v39);

                if ((result.mResult & (int)InteractionResult::Result::SUCCESS) != 0)
                {
                    bool itemsMatch = false; // todo: ItemStackBase::operator!=(&itemStack, v25)

                    if (!itemsMatch) {
                        containerToUse->setItem(self->mSlot, stackCopy);
                    }   

                    if (gameMode.isLastBuildBlockInteractive()) {
                        // I'm pretty sure this is just for scripting and so i dont really care to impl it.
                         BlockEventCoordinator& blockEvents = level.getBlockEventCoordinator();
                         blockEvents.sendBlockInteractedWith(gameMode.mPlayer, self->mPos);
                    }
                }
                else {
					self->resendBlocksAroundArea(gameMode.mPlayer, self->mPos, self->mFace);
					self->resendPlayerState(gameMode.mPlayer);
                }
            }
        }
    );

    // What on earth does this mean
    // ComplexInventoryTransaction::_setDepenetrationOverride(this, (player + 8));

    return InventoryTransactionError::ProbablyError; // todo: return correct error code
}

void RegisterOffhandHooks()
{
    Amethyst::HookManager &hooks = Amethyst::GetHookManager();

    ItemUseInventoryTransaction_ctor = (void*)SigScan("48 89 5C 24 ? 57 48 83 EC ? 48 8D 59 ? C7 41 ? ? ? ? ? 48 8D 05 ? ? ? ? 48 89 5C 24 ? 48 89 01 48 8B F9 48 8B CB E8 ? ? ? ? 33 C9 48 8D 05 ? ? ? ? 48 89 4B ? 0F 57 C0 48 89 4B ? 48 89 4B ? 48 8B 5C 24 ? 48 89 07 48 8D 05 ? ? ? ? 48 89 4F ? 48 89 4F");

	VHOOK(GameMode, buildBlock, this);
    VHOOK(GameMode, useItemOn, this);
	HOOK(ItemUseInventoryTransaction, handle);
}
