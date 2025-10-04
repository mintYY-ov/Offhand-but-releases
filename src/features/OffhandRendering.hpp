#pragma once
#include <amethyst/runtime/ModContext.hpp>
#include <mc/src-client/common/client/renderer/game/ItemInHandRenderer.hpp>
#include <mc/src-client/common/client/renderer/BaseActorRenderContext.hpp>
#include <mc/src-client/common/client/world/item/ItemIconManager.hpp>
#include <mc/src/common/util/Timer.hpp>
#include <mc/src/common/Minecraft.hpp>
#include <mc/src-deps/core/math/Math.hpp>
#include "features/OffhandSwingComponent.hpp"

SafetyHookInline _ItemInHandRenderer__transformOffhandItem;

float getOffhandAttackAnim(Player& player, float a) {
	OffhandSwingComponent* offhandSwing = player.tryGetComponent<OffhandSwingComponent>();
	if (!offhandSwing) return 0.0f;

	float delta = offhandSwing->mOffhandAttackAnim - offhandSwing->mOffhandOAttackAnim;
	if (delta < 0.0f) delta += 1.0f;
	return (delta * a) + offhandSwing->mOffhandOAttackAnim;
}

void ItemInHandRenderer__transformOffhandItem(ItemInHandRenderer* self, MatrixStack::MatrixStackRef& matrixStack) {
	ClientInstance& client = *Amethyst::GetContext().mClientInstance;
	LocalPlayer* player = client.getLocalPlayer();
	if (!player) return;

	ActorEquipmentComponent* equipment = player->tryGetComponent<ActorEquipmentComponent>();
	if (!equipment || equipment->mHand->mItems.size() <= 1) return;
	const ItemStack& offhand = equipment->mHand->mItems[1];

	if (offhand.isNull() || !offhand.isBlock() || offhand.getItem()->isBlockPlanterItem() || !self->_canTessellateAsBlockItem(offhand)) {
		_ItemInHandRenderer__transformOffhandItem.call<void, ItemInHandRenderer*, MatrixStack::MatrixStackRef&>(self, matrixStack);
		return;
	}

	float interpolatedTick = Amethyst::GetContext().mClientMinecraft->mSimTimer.mAlpha;

	float depth = -1.8f;   // depth
	float side = -1.5f;   // left
	float height = -1.3f;  // down

	float v12 = ((self->mHeight - self->mOldHeight) * interpolatedTick) + self->mOldHeight;

	//float attackAnim = player->getAttackAnim(interpolatedTick);
	float attackAnim = getOffhandAttackAnim(*player, interpolatedTick);

	float alsoAttackAnim = attackAnim;
	float attackAnimSqrt = mce::Math::sqrt(attackAnim);
	float rotSin = mce::Math::sin((alsoAttackAnim * alsoAttackAnim) * 3.1415927);
	float posSin = mce::Math::sin(alsoAttackAnim * 3.1415927);
	float sinAnimSqrtTimesPi = mce::Math::sin(attackAnimSqrt * 3.1415927);

	attackAnim = mce::Math::sqrt(alsoAttackAnim);
	attackAnim = mce::Math::sin((*&attackAnim * 3.1415927) + (*&attackAnim * 3.1415927));

	// original - mainhand
	/*matrixStack->translate(sinAnimSqrtTimesPi * -0.30000001, attackAnim * 0.40000001, posSin * -0.40000001);

	float v275 = (1.0 - v12) * 0.60000002;
	Matrix::translate(v274, 0.64000005, -0.60000002 - v275, -0.71999997);


	matrixStack->rotate(45.0f, 0.0f, 1.0f, 0.0f);

	matrixStack->rotate(sinAnimSqrtTimesPi * 70.0f, 0.0f, 1.0f, 0.0f);
	matrixStack->rotate(rotSin * -20.0f, 1.0f, 0.0f, 0.0f);*/

	matrixStack->translate(sinAnimSqrtTimesPi * 0.30000001, attackAnim * 0.40000001, posSin * -0.40000001);

	float v275 = (v12) * 0.60000002;
	matrixStack->translate(side, height - v275, depth);

	matrixStack->rotate(45.0f, 0.0f, 1.0f, 0.0f);

	matrixStack->rotate(sinAnimSqrtTimesPi * 70.0f, 0.0f, -1.0f, 0.0f);
	matrixStack->rotate(rotSin * -20.0f, -1.0f, 0.0f, 0.0f);
}

SafetyHookInline _Player_aiStep;

void Player_aiStep(Player* self) {
	_Player_aiStep.call<void, Player*>(self);

	OffhandSwingComponent* offhandSwing = self->tryGetComponent<OffhandSwingComponent>();
	if (!offhandSwing) return;

	// pretty much just reimplements Mob::updateAttackAnim but for offhand
	int currentSwingDuration = self->getCurrentSwingDuration();
	int nextSwingTick = 0;

	if (offhandSwing->mOffhandSwinging) {
		nextSwingTick = offhandSwing->mOffhandSwingTime + 1;
		offhandSwing->mOffhandSwingTime = nextSwingTick;

		if (nextSwingTick < currentSwingDuration) {
			goto LABEL6;
		}

		offhandSwing->mOffhandSwinging = false;
	}

	offhandSwing->mOffhandSwingTime = 0;

LABEL6:
	if (currentSwingDuration != 0) {
		offhandSwing->mOffhandAttackAnim = (float)offhandSwing->mOffhandSwingTime / (float)currentSwingDuration;
	}
	else {
		offhandSwing->mOffhandAttackAnim = 0.0f;
	}
}

SafetyHookInline _Mob_baseTick;

void Mob_baseTick(Mob* self) {
	_Mob_baseTick.call<void, Mob*>(self);

	OffhandSwingComponent* offhandSwing = self->tryGetComponent<OffhandSwingComponent>();
	if (!offhandSwing) return;

	offhandSwing->mOffhandOAttackAnim = offhandSwing->mOffhandAttackAnim;
}

void RegisterOffhandRendering() {
	Amethyst::HookManager& hooks = Amethyst::GetHookManager();

	//hooks.RegisterFunction<&_transformOffhandItem>("40 53 48 83 EC ? 48 8B 02 0F 57 DB");
	//hooks.CreateHook<&_transformOffhandItem>(_ItemInHandRenderer_transformOffhandItem, &_transformOffhandItem);

	//hooks.RegisterFunction<&Player_aiStep>("48 8B C4 48 89 58 ? 48 89 70 ? 57 48 81 EC ? ? ? ? 0F 29 70 ? 0F 29 78 ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 48 8B F9");
	//hooks.CreateHook<&Player_aiStep>(_Player_aiStep, &Player_aiStep);

	//hooks.RegisterFunction<&Mob_baseTick>("48 89 5C 24 ? 56 48 83 EC ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 8B 81");
	//hooks.CreateHook<&Mob_baseTick>(_Mob_baseTick, &Mob_baseTick);

	VHOOK(Player, aiStep, this);
	VHOOK(Mob, baseTick, this);
	HOOK(ItemInHandRenderer, _transformOffhandItem);
}
