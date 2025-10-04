#include "OffhandHud.hpp"
#include <amethyst/runtime/ModContext.hpp>
#include <mc/src-client/common/client/gui/UIResolvedDef.hpp>
#include <mc/src-client/common/client/gui/gui/UIControl.hpp>
#include <mc/src-client/common/client/gui/controls/CustomRenderComponent.hpp>
#include <mc/src-client/common/client/gui/controls/renderers/MinecraftUICustomRenderer.hpp>
#include <mc/src-deps/core/math/Color.hpp>
#include <amethyst/Formatting.hpp>
#include <mc/src-client/common/client/player/LocalPlayer.hpp>
#include <mc/src/common/world/entity/components/ActorEquipmentComponent.hpp>
#include <mc/src-client/common/client/renderer/BaseActorRenderContext.hpp>
#include <mc/src-client/common/client/renderer/screen/ScreenContext.hpp>
#include <mc/src-client/common/client/renderer/actor/ItemRenderer.hpp>
#include <mc/src-client/common/client/game/MinecraftGame.hpp>
#include <mc/src-deps/input/InputMappingFactory.hpp>
#include <mc/src-deps/input/InputMapping.hpp>
#include <mc/src/common/world/SimpleContainer.hpp>
#include <mc/src-client/common/client/gui/controls/UIControlFactory.hpp>

class UIResolvedDef;
class UIControl;
class UIControlFactory;

SafetyHookInline _UIControlFactory__populateCustomRenderComponent;

static HashedString flushString(0xA99285D21E94FC80, "ui_flush");

class TestRenderer : public MinecraftUICustomRenderer {
public:
	TestRenderer() : MinecraftUICustomRenderer() {};

	virtual std::shared_ptr<UICustomRenderer> clone() const override {
		return std::make_shared<TestRenderer>();
	}

	virtual void render(MinecraftUIRenderContext& ctx, IClientInstance& _client, UIControl& owner, int32_t pass, RectangleArea& renderAABB) override {
		ClientInstance& client = _client.asInstance();
		LocalPlayer* player = client.getLocalPlayer();
		if (!player || mPropagatedAlpha < 0.5) return;

		ActorEquipmentComponent* equipment = player->tryGetComponent<ActorEquipmentComponent>();
		if (!equipment || equipment->mHand->mItems.size() <= 1) return;

		const ItemStack& offhand = equipment->mHand->mItems[1];
		if (offhand.isNull()) return;

		// This disables the item visual from bobbing whenever the offhand stack changes in content
		ItemStack offhandCopy = offhand;
		offhandCopy.mShowPickup = false;

		glm::tvec2<float> pos = owner.getPosition();

		BaseActorRenderContext renderCtx(*ctx.mScreenContext, client, *client.mMinecraftGame);
		int yOffset = offhand.getItem()->getIconYOffset();

		renderCtx.itemRenderer->renderGuiItemNew(&renderCtx, &offhandCopy, 0, pos.x + 3.0f, pos.y - yOffset + 3.0f, false, 1.0f, mPropagatedAlpha, 1.0f);
		mce::Color color(1.0f, 1.0f, 1.0f, mPropagatedAlpha);
		ctx.flushImages(color, 1.0f, flushString);

		if (offhand.mCount == 1) return;

		std::string text = std::format("{}", offhand.mCount);
		float lineLength = ctx.getLineLength(*client.mMinecraftGame->mFontHandle.mDefaultFont, text, 1.0f, false);

		renderAABB._x0 = pos.x + (20.0f - lineLength);
		renderAABB._x1 = pos.x + 22.0f;
		renderAABB._y0 = pos.y + 11.0f;
		renderAABB._y1 = pos.y + 22.0f;

		TextMeasureData textData;
		memset(&textData, 0, sizeof(TextMeasureData));
		textData.fontSize = 1.0f;
		textData.renderShadow = true;

		CaretMeasureData caretData;
		memset(&caretData, 1, sizeof(CaretMeasureData));

		ctx.drawDebugText(&renderAABB, &text, &mce::Color::WHITE, 1.0f, ui::Right, &textData, &caretData);
		ctx.flushText(0.0f);
	}
};

void UIControlFactory__populateCustomRenderComponent(UIControlFactory* self, const UIResolvedDef& resolved, UIControl& control) {
	std::string rendererType = resolved.getAsString("renderer");

	if (rendererType == "offhand_hud_renderer") {
		Log::Info("Making offhand_hud_renderer!");

		control.setComponent<CustomRenderComponent>(
			std::make_unique<CustomRenderComponent>(control)
		);

		CustomRenderComponent* component = control.getComponent<CustomRenderComponent>();
		component->setRenderer(std::make_shared<TestRenderer>());

		return;
	}

	_UIControlFactory__populateCustomRenderComponent.call<void, UIControlFactory*, const UIResolvedDef&, UIControl&>(self, resolved, control);
}

void RegisterOffhandHud()
{
    Amethyst::HookManager& hooks = Amethyst::GetHookManager();
	HOOK(UIControlFactory, _populateCustomRenderComponent);
} 
