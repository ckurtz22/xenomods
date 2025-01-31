// Created by block on 3/19/23.

#include "FrameratePatch.hpp"

#include <skylaunch/hookng/Hooks.hpp>
#include <xenomods/DebugWrappers.hpp>
#include <xenomods/HidInput.hpp>
#include <xenomods/Logger.hpp>
#include <xenomods/Utils.hpp>

#include "xenomods/engine/ml/DevGraph.hpp"

namespace xenomods {

	void VSyncUpdate() {
		if (GetState().config.enable60FPS)
			ml::DevGraph::setVSync(1);
		else
			ml::DevGraph::setVSync(2);
	}

	void FrameratePatch::Initialize() {
		UpdatableModule::Initialize();
		g_Logger->LogDebug("Setting up framerate patch...");

		VSyncUpdate();

		// this only works well in DE right now as it properly has frame skipping
		// 2/Torna do not have it, so the game becomes a slow mess when it can't hit 60 (as is usually the case)

		// steps to add frame skipping will go here
	}

	void FrameratePatch::OnConfigUpdate() {
		VSyncUpdate();
	}

#if XENOMODS_CODENAME(bfsw)
	XENOMODS_REGISTER_MODULE(FrameratePatch);
#endif

} // namespace xenomods