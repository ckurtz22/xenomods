#include "PlayerMovement.hpp"

#include <bf2mods/DebugWrappers.hpp>
#include <bf2mods/HidInput.hpp>
#include <bf2mods/Logger.hpp>
#include <bf2mods/Utils.hpp>
#include <skylaunch/hookng/Hooks.hpp>

#include "../State.hpp"
#include "../main.hpp"
#include "DebugStuff.hpp"
#include "bf2mods/engine/fw/UpdateInfo.hpp"
#include "bf2mods/engine/gf/PlayerController.hpp"
#include "bf2mods/stuff/utils/debug_util.hpp"
#include "bf2mods/stuff/utils/util.hpp"

namespace {

	struct ApplyVelocityChanges : skylaunch::hook::Trampoline<ApplyVelocityChanges> {
		static void Hook(gf::GfComBehaviorPc* this_pointer, fw::UpdateInfo* updateInfo, gf::GfComPropertyPc* pcProperty) {
			//using enum gf::GfComPropertyPc::Flags;
			//bool flagInAir = (pcProperty->flags & static_cast<std::uint32_t>(InAir)) != 0;
			//bool flagOnWall = (pcProperty->flags & static_cast<std::uint32_t>(OnWall)) != 0;

			if(bf2mods::PlayerMovement::moonJump)
				static_cast<glm::vec3&>(pcProperty->velocityActual).y = std::max(10.f, 10.f * (bf2mods::PlayerMovement::movementSpeedMult / 64.f));

			auto& wish = static_cast<glm::vec3&>(pcProperty->velocityWish);

			wish *= bf2mods::PlayerMovement::movementSpeedMult;

			//fw::debug::drawFontFmtShadow(0, 200, mm::Col4::white, "velAct {:2} (len {:.2f})", static_cast<const glm::vec3&>(pcProperty->velocityActual), pcProperty->velocityActual.XZLength());
			//fw::debug::drawFontFmtShadow(0, 216, mm::Col4::white, "velWsh {:2} (len {:.2f})", static_cast<const glm::vec3&>(pcProperty->velocityWish), pcProperty->velocityWish.XZLength());
			//fw::debug::drawFontFmtShadow(0, 232, mm::Col4::white, "who knows {:2} {:2}", static_cast<const glm::vec2&>(pcProperty->inputReal), static_cast<const glm::vec2&>(pcProperty->inputDupe));
			//fw::debug::drawFontFmtShadow(0, 248, mm::Col4::white, "flags: {:032b}", static_cast<std::uint32_t>(pcProperty->flags));
			//fw::debug::drawFontFmtShadow(0, 264, mm::Col4::white, "in air? {} on wall? {}", flagInAir, flagOnWall);

			Orig(this_pointer, updateInfo, pcProperty);

			wish /= bf2mods::PlayerMovement::movementSpeedMult;
		}
	};

	struct DisableFallDamagePlugin : skylaunch::hook::Trampoline<DisableFallDamagePlugin> {
		static float Hook(gf::pc::FallDamagePlugin* this_pointer, mm::Vec3* vec) {
			return bf2mods::PlayerMovement::disableFallDamage ? 0.f : Orig(this_pointer, vec);
		}
	};

	struct DisableStateUtilFallDamage : skylaunch::hook::Trampoline<DisableStateUtilFallDamage> {
		static void Hook(gf::GfComBehaviorPc* GfComBehaviorPc, bool param_2) {
			//bf2mods::g_Logger->LogInfo("StateUtil::setFallDamageDisable(GfComBehaviorPc: {:p}, bool: {})", GfComBehaviorPc, param_2);
			Orig(GfComBehaviorPc, bf2mods::PlayerMovement::disableFallDamage || param_2);
		}
	};

	struct CorrectCameraTarget : skylaunch::hook::Trampoline<CorrectCameraTarget> {
		static void Hook(gf::PlayerCameraTarget* this_pointer) {
			Orig(this_pointer);
			if(bf2mods::PlayerMovement::moonJump) {
				// makes the game always take the on ground path in gf::PlayerCamera::updateTracking
				this_pointer->inAir = false;
				// should stop the camera from suddenly jerking back to the maximum height moonjumped to
				this_pointer->surfaceHeight = static_cast<glm::vec3&>(this_pointer->moverPos).y;
				this_pointer->aboveWalkableGround = true;
			}
		}
	};

}

namespace bf2mods {

	bool PlayerMovement::moonJump = false;
	bool PlayerMovement::disableFallDamage = true;
	float PlayerMovement::movementSpeedMult = 1.f;

	void PlayerMovement::Initialize() {
		g_Logger->LogDebug("Setting up player movement hooks...");

		ApplyVelocityChanges::HookAt("_ZN2gf15GfComBehaviorPc19integrateMoveNormalERKN2fw10UpdateInfoERNS_15GfComPropertyPcE");

		DisableFallDamagePlugin::HookAt("_ZNK2gf2pc18FallDistancePlugin12calcDistanceERKN2mm4Vec3E");
		DisableStateUtilFallDamage::HookAt("_ZN2gf2pc9StateUtil20setFallDamageDisableERNS_15GfComBehaviorPcEb");

		CorrectCameraTarget::HookAt("_ZN2gf18PlayerCameraTarget15writeTargetInfoEv");
	}

	void PlayerMovement::Update() {
		moonJump = GetPlayer(1)->InputHeld(Keybind::MOONJUMP);

		if(GetPlayer(2)->InputDownStrict(Keybind::MOVEMENT_SPEED_UP)) {
			movementSpeedMult *= 2.0f;
			g_Logger->LogInfo("Movement speed multiplier set to {:.2f}", movementSpeedMult);
		} else if(GetPlayer(2)->InputDownStrict(Keybind::MOVEMENT_SPEED_DOWN)) {
			movementSpeedMult /= 2.0f;
			g_Logger->LogInfo("Movement speed multiplier set to {:.2f}", movementSpeedMult);
		} else if(GetPlayer(2)->InputDownStrict(Keybind::DISABLE_FALL_DAMAGE)) {
			disableFallDamage = !disableFallDamage;
			g_Logger->LogInfo("Disable fall damage: {}", disableFallDamage);
		}
	}

#if BF2MODS_CODENAME(bf2) || BF2MODS_CODENAME(ira)
	BF2MODS_REGISTER_MODULE(PlayerMovement);
#endif

} // namespace bf2mods
