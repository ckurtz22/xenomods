#include "main.hpp"

#include <skylaunch/hookng/Hooks.hpp>
#include <xenomods/DebugWrappers.hpp>
#include <xenomods/HidInput.hpp>
#include <xenomods/Logger.hpp>
#include <xenomods/NnFile.hpp>
#include <xenomods/Version.hpp>

#include "State.hpp"
#include "modules/DebugStuff.hpp"
#include "xenomods/engine/fw/Document.hpp"
#include "xenomods/engine/fw/Framework.hpp"
#include "xenomods/engine/ml/Filesystem.hpp"
#include "xenomods/engine/ml/Rand.hpp"
#include "xenomods/stuff/utils/debug_util.hpp"
#include "xenomods/engine/gf/Party.hpp"

namespace {

#if !XENOMODS_CODENAME(bfsw)
	struct FrameworkUpdateHook : skylaunch::hook::Trampoline<FrameworkUpdateHook> {
		static void Hook(fw::Framework* framework) {
			Orig(framework);

			if (framework != nullptr)
				xenomods::update(framework->getUpdateInfo());
		}
	};
#else
	struct FrameworkUpdater_updateStdHook : skylaunch::hook::Trampoline<FrameworkUpdater_updateStdHook> {
		static void Hook(fw::Document* doc, void* FrameworkController) {
			Orig(doc, FrameworkController);

			if(doc != nullptr && doc->applet != nullptr) {
				xenomods::DocumentPtr = doc;
				xenomods::update(&doc->applet->updateInfo);
			}
		}
	};
#endif

#if XENOMODS_CODENAME(bf2)
	struct FixIraOnBF2Mount : skylaunch::hook::Trampoline<FixIraOnBF2Mount> {
		static bool Hook(const char* path) {
			auto view = std::string_view(path);
			if(view.starts_with("menu_ira"))
				return false;
			return Orig(path);
		}
	};
#endif

} // namespace

std::ofstream logfile;

namespace xenomods {

#if XENOMODS_CODENAME(bfsw)
	fw::Document* DocumentPtr = {};
#endif

void fmt_assert_failed(const char* file, int line, const char* message) {
		NN_DIAG_LOG(nn::diag::LogSeverity::Fatal, "fmtlib assert caught @ %s:%d : %s", file, line, message);
	}

	void toastVersion() {
		g_Logger->ToastInfo("xm_version1", "xenomods {}{} [{}]", version::BuildGitVersion(), version::BuildIsDebug ? " (debug)" : "", XENOMODS_CODENAME_STR);
		g_Logger->ToastDebug("xm_version2", "compiled on {}", version::BuildTimestamp());
		g_Logger->ToastDebug("xm_version3", "running {}, version {}", version::RuntimeGame(), version::RuntimeVersion());
		g_Logger->ToastDebug("xm_version4", "exefs {}", version::RuntimeBuildRevision());
	}

	void update(fw::UpdateInfo* updateInfo) {
		// lazy
		using enum xenomods::Keybind;

		HidInput* P1 = GetPlayer(1);
		HidInput* P2 = GetPlayer(2);
		P1->Poll();
		P2->Poll();

		/*std::string p1Buttons = fmt::format("{:#08x} - P1 - {:#08x}", P1->stateCur.Buttons, P1->statePrev.Buttons);
		std::string p2Buttons = fmt::format("{:#08x} - P2 - {:#08x}", P2->stateCur.Buttons, P2->statePrev.Buttons);
		int buttonsP1Width = fw::debug::drawFontGetWidth(p1Buttons.c_str());
		int buttonsP2Width = fw::debug::drawFontGetWidth(p2Buttons.c_str());
		fw::debug::drawFontShadow(1280-buttonsP1Width-5, 5, mm::Col4::white, p1Buttons.c_str());
		fw::debug::drawFontShadow(1280-buttonsP2Width-5, 5+16, mm::Col4::white, p2Buttons.c_str());

		auto testcombo = nn::hid::KEY_A;
		if(P2->InputHeld(testcombo))
			fw::debug::drawFontShadow(1280/2, 0, mm::Col4::cyan, "combo held!");
		if (P2->InputDown(testcombo))
			g_Logger->LogDebug("combo down...");
		if (P2->InputUp(testcombo))
			g_Logger->LogDebug("combo up!");
		if(P2->InputHeldStrict(testcombo))
			fw::debug::drawFontShadow(1280/2, 16, mm::Col4::cyan, "strict combo held!");
		if (P2->InputDownStrict(testcombo))
			g_Logger->LogDebug("strict combo down...");
		if (P2->InputUpStrict(testcombo))
			g_Logger->LogDebug("strict combo up!");*/

		/*
		 * Enforce some things on first update
		 */
		static bool hasUpdated;
		if(!hasUpdated) {
			nn::hid::SetSupportedNpadStyleSet(3);
#if !XENOMODS_CODENAME(bfsw)
			fw::PadManager::enableDebugDraw(true);
#endif
			hasUpdated = true;
		}

		/*
		 * Check buttons
		 */
		static int logging = -1;
		
		if (P1->InputDownStrict(RECORD_JUMPS))
		{
			g_Logger->ToastInfo("record", "start recording jumps");
			logging = 0;
			logfile.open("sd:/skylinetest.log", std::ofstream::out);
		}
		else if (P1->InputDownStrict(SAVE_JUMPS))
		{
			g_Logger->ToastInfo("record", "save recording jumps");
			logging = -1;
			logfile.close();
		}
		{
			gf::GfComTransform* trans = gf::GfGameParty::getLeaderTransform();
			if (trans != nullptr) {
				mm::Vec3* pos = trans->getPosition();
				mm::Quat* rot = trans->getRotation();

				if (pos != nullptr && rot != nullptr) {
					glm::vec3 realPos = *pos;
					glm::quat realRot = *rot;
					auto q = realRot;
					double angle = atan2(q.x, q.z)*2 * 180.0 / M_PI;
					if (angle < 0.0) angle += 360.0;

					double roll = atan2(2 * (q.w * q.x + q.y * q.z), 1 - 2 * (q.x * q.x + q.y * q.y));
					double pitch = -M_PI / 2 + atan2(sqrt(1 + 2 * (q.w * q.y - q.x * q.z)), sqrt(1 - 2 * (q.w * q.y - q.x * q.z)));
					double yaw = atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z));

					glm::vec3 forward = mm::Vec3::unitZ;
					forward = static_cast<glm::quat&>(*rot) * forward;;

					// custom function
					g_Logger->ToastInfo("Testpos", "Pos: ({:.6f}, {:.6f}, {:.6f})  Dir: x: {:.6f}", realPos.x, realPos.y, realPos.z, angle);
					// g_Logger->ToastInfo("Testpos", "Pos: {}  Dir: x: {:.3f}, y: {:.3f}, z: {:.3f}, w: {:.3f}, roll: {:.3f}, pitch: {:.3f}, yaw: {:.3f}", realPos, realRot.x, realRot.y, realRot.z, realRot.w, roll, pitch, yaw);
					fw::debug::drawFontFmtShadow3D(realPos, mm::Col4::white, "{}", realPos);
					fw::debug::drawCompareZ(false);
					fw::debug::drawArrow(realPos, realPos + forward, mm::Col4::red);


					if (P1->InputDown(nn::hid::KEY_B) && logging <= 0)
					{
						logging = 23;
					}
					if (logging > 0)
					{
						logging--;
						logfile << realPos.x << ", " << realPos.y << ", " << realPos.z << "\t\t" << angle << "\n";
						if (logging == 0)
						{
							if (realPos.x < 376.5)
							{
								logfile << "clip";
							}
							else
							{
								logfile << "no clip";
							}
							logfile << std::endl;
						}
					}
					// target.setup(static_cast<mm::Vec3>(point), 1);
					// fw::debug::drawSphere(target, mm::Col4::green, 1);
					fw::PrimSphere target;
					glm::vec3 point = {376.629, 273.984, 101.224};
					point.x = 376.629;
					point.y = 273.984;
					point.z = 101.224;
					// target.setup(point, 0.2);
					// fw::debug::drawSphere(target, mm::Col4::white, 100);
					target.setup(point, 0.05);
					fw::debug::drawSphere(target, mm::Col4::green, 10);
					// fw::debug::drawArrow(realPos, point, mm::Col4::green);
					fw::debug::drawCompareZ(true);

				}
			}
		}


		if(P2->InputDownStrict(DISPLAY_VERSION)) {
			toastVersion();
		} else if(P2->InputDownStrict(RELOAD_CONFIG)) {
			GetState().config.LoadFromFile();
			DebugStuff::PlaySE(gf::GfMenuObjUtil::SEIndex::Sort);
		} else if(P2->InputDownStrict(LOGGER_TEST)) {
			g_Logger->LogDebug("test debug message! {}", ml::mtRand());
			g_Logger->LogInfo("test info message! {}", ml::mtRandf1());
			g_Logger->LogWarning("test warning message! {}", ml::mtRandf2());
			g_Logger->LogError("test error message! {}", ml::mtRandf3());
			g_Logger->LogFatal("test fatal message! {}", nn::os::GetSystemTick());

			int group = ml::mtRand(100, 999);
			g_Logger->ToastWarning(fmt::format("{}", group), "random group ({})", group);
			g_Logger->ToastMessage("logger test", Logger::Severity::Info, "system tick in seconds: {:2f}", nn::os::GetSystemTick() / 19200000.);
		}

		// Update modules
		xenomods::UpdateAllRegisteredModules(updateInfo);

		// draw log messages
		g_Logger->Draw();
	}

	void main() {
		InitializeAllRegisteredModules();

#if XENOMODS_CODENAME(bf2)
		if(GetState().config.mountTornaContent) {
			NnFile file("rom:/ira-xm.arh", nn::fs::OpenMode_Read);

			if(file) {
				file.Close(); // gotta let the game read it lol
				FixIraOnBF2Mount::HookAt(&ml::DevFileTh::checkValidFileBlock);
				ml::DevFileTh::registArchive(ml::MEDIA::Default, "ira-xm.arh", "ira-xm.ard", "aoc1:/");
			}
		}
#endif

		// hook our updater
#if !XENOMODS_CODENAME(bfsw)
		FrameworkUpdateHook::HookAt("_ZN2fw9Framework6updateEv");
#else
		FrameworkUpdater_updateStdHook::HookAt("_ZN2fw16FrameworkUpdater9updateStdERKNS_8DocumentEPNS_19FrameworkControllerE");
#endif

		toastVersion();
	}

} // namespace xenomods
