#include "DRS.h"

#include <ReflexAPI.h>
extern ReflexAPI* g_Reflex;

#include "Nukem/GpuTimer.h"

#include <ENB/ENBSeriesAPI.h>
#include <ENB/AntTweakBar.h>


void DRS::LoadSettings()
{
	//std::ifstream i(L"Data\\SKSE\\Plugins\\DynamicResolutionScaling.json");
	//i >> JSONSettings;

	//AutoDynamicResolutionEnabled->data.b = JSONSettings["DynamicResolutionEnabled"];
	//HighestScaleFactor = JSONSettings["HighestScaleFactor"];
	//LowestScaleFactor = JSONSettings["LowestScaleFactor"];
	//TargetFPS = JSONSettings["TargetFPS"];

	ResetScale();
}

void DRS::SaveSettings()
{
	//std::ofstream o(L"Data\\SKSE\\Plugins\\DynamicResolutionScaling.json");

	//JSONSettings["DynamicResolutionEnabled"] = AutoDynamicResolutionEnabled->GetBool();
	//JSONSettings["HighestScaleFactor"] = HighestScaleFactor;
	//JSONSettings["LowestScaleFactor"] = LowestScaleFactor;
	//JSONSettings["TargetFPS"] = TargetFPS;

	//o << JSONSettings.dump(1);
}

void DRS::GetGameSettings()
{
	auto ini = RE::INISettingCollection::GetSingleton();
	AutoDynamicResolutionEnabled = ini->GetSetting("bEnableAutoDynamicResolution:Display");
}

void DRS::Update()
{
	float gpuFrameInnerWorkTime;
	float gpuUsagePercent;
	bool  enabled = HaveUpdatedAveragedData(gpuFrameInnerWorkTime, gpuUsagePercent);
	if (reset || !enabled)
		ResetScale();
	else if (enabled)
		ControlResolution(gpuFrameInnerWorkTime, gpuUsagePercent);
}

void DRS::ControlResolution(float a_gpuFrameInnerWorkTime, float a_gpuUsagePercent)
{
	float desiredFrameTime = 1000 / fTargetFPS;
	float assumedGPUtime;
	if (g_Reflex && g_Reflex->GetReflexEnabled()){
		assumedGPUtime = a_gpuFrameInnerWorkTime;
	} else {
		float estGPUTime = std::clamp(a_gpuFrameInnerWorkTime * 1000, desiredFrameTime, FLT_MAX);
		float unboundedGPUTime = estGPUTime * a_gpuUsagePercent;
		if ((estGPUTime - desiredFrameTime) <= UnboundedHeadroomThreshold) {
			auto scale = (UnboundedHeadroomThreshold - (estGPUTime - desiredFrameTime)) * (1 / UnboundedHeadroomThreshold);
			assumedGPUtime = std::lerp(estGPUTime, unboundedGPUTime, (desiredFrameTime / estGPUTime) * scale);
		} else {
			assumedGPUtime = estGPUTime;
		}
		logger::debug("Estimated GPU Time: {} Unbounded GPU Time: {} Assumed Real GPU Time: {}", estGPUTime, unboundedGPUTime, assumedGPUtime);
	}

	if (prevGPUFrameTime != 0) {
		float headroom = desiredFrameTime - assumedGPUtime;
		float GPUTimeDelta = assumedGPUtime - prevGPUFrameTime;
		logger::debug("Headroom: {} GPU Time Delta: {}", headroom, GPUTimeDelta);

		// If headroom is negative, we've exceeded target and need to scale down.
		if (headroom < 0.0) {
			ScaleRaiseCounter = 0;

			// Since headroom is guaranteed to be negative here, we can add rather than negate and subtract.
			float scaleDecreaseFactor = headroom / desiredFrameTime;
			currentScaleFactor = std::clamp(currentScaleFactor + scaleDecreaseFactor, 0.0f, 1.0f);
		} else {
			// If delta is greater than headroom, we expect to exceed target and need to scale down.
			if (GPUTimeDelta > headroom) {
				ScaleRaiseCounter = 0;

				float scaleDecreaseFactor = GPUTimeDelta / desiredFrameTime;
				currentScaleFactor = std::clamp(currentScaleFactor - scaleDecreaseFactor, 0.0f, 1.0f);
			} else {
				// If delta is negative, then perf is moving in a good direction and we can increment to scale up faster.
				if (GPUTimeDelta < 0.0) {
					ScaleRaiseCounter += ScaleRaiseCounterBigIncrement;
				} else {
					float headroomThreshold = assumedGPUtime * HeadroomThreshold;
					float deltaThreshold = assumedGPUtime * DeltaThreshold;

					// If we're too close to target or the delta is too large, do nothing out of concern that we could scale up and exceed target.
					// Otherwise, slow increment towards a scale up.
					if ((headroom > headroomThreshold) && (GPUTimeDelta < deltaThreshold)) {
						ScaleRaiseCounter += ScaleRaiseCounterSmallIncrement;
					}
				}

				if (ScaleRaiseCounter >= 30) {
					ScaleRaiseCounter = 0;

					// Headroom as percent of target is unlikely to use the full 0-1 range, so clamp on user settings and then remap to 0-1.
					float headroomPercent = headroom / desiredFrameTime;
					float clampedHeadroom = std::clamp(headroomPercent, ScaleHeadroomClampMin, ScaleHeadroomClampMax);
					float remappedHeadroom = (clampedHeadroom - ScaleHeadroomClampMin) / (ScaleHeadroomClampMax - ScaleHeadroomClampMin);
					float scaleIncreaseFactor = ScaleIncreaseBasis * std::lerp(ScaleIncreaseSmallFactor, ScaleIncreaseBigFactor, remappedHeadroom);
					currentScaleFactor = std::clamp(currentScaleFactor + scaleIncreaseFactor, 0.0f, 1.0f);
				}
			}
		}
		currentScaleFactor = std::clamp(currentScaleFactor, LowestScaleFactor, HighestScaleFactor);
	}
	prevGPUFrameTime = assumedGPUtime;
	logger::debug("Current scale factor {}", currentScaleFactor);
}

bool DRS::HaveUpdatedAveragedData(float& a_avgFrameTime, float& a_avgUsageTiming)
{
	bool badframe = false;
	a_avgUsageTiming = 0.0f;
	for (int i = keepNumFrames - 1; i > 0; i--) {
		if (!usagePerFrame[i])
			badframe = true;
		usagePerFrame[i] = usagePerFrame[i - 1];
		a_avgUsageTiming += usagePerFrame[i];
	}
	usagePerFrame[0] = GPUInfo::GetSingleton()->GetGPUUsage() / 100.0f;
	if (badframe)
		return false;
	a_avgUsageTiming += usagePerFrame[0];
	a_avgUsageTiming /= keepNumFrames;

	badframe = false;
	a_avgFrameTime = 0.0f;
	for (int i = keepNumFrames - 1; i > 0; i--) {
		if (!frameTimes[i])
			badframe = true;
		frameTimes[i] = frameTimes[i - 1];
		a_avgFrameTime += frameTimes[i];
	}
	if (g_Reflex && g_Reflex->GetReflexEnabled()) {
		NV_LATENCY_RESULT_PARAMS
		latencyResults = {};
		latencyResults.version = NV_LATENCY_RESULT_PARAMS_VER;
		auto valid = g_Reflex->GetLatencyReport(&latencyResults);
		if (valid) {
			frameTimes[0] = (float)(latencyResults.frameReport->gpuActiveRenderTimeUs / 1000);
			logger::debug("Active Render Time {} GPU Render Time {}", frameTimes[0], latencyResults.frameReport->gpuFrameTimeUs / 1000);
		} else {
			frameTimes[0] = 0;
		}
	} else {
		frameTimes[0] = g_GPUTimers.GetGPUTimeInMS(0) / 1000.0f;
	}
	if (badframe || !frameTimes[0])
		return false;
	a_avgFrameTime += frameTimes[0];
	a_avgFrameTime /= keepNumFrames;
	return true;
}

void DRS::ResetScale()
{
	currentScaleFactor = 1.0f;
	ScaleRaiseCounter = 0;
	for (int i = 0; i < keepNumFrames; i++)
		frameTimes[i] = 0;
	for (int i = 0; i < keepNumFrames; i++)
		usagePerFrame[i] = 0;
	prevGPUFrameTime = 0;
}

void DRS::SetDRS([[maybe_unused]] BSGraphics::State* a_state)
{
	if (AutoDynamicResolutionEnabled && AutoDynamicResolutionEnabled->GetBool()) {
		a_state->fDynamicResolutionPreviousHeightScale = a_state->fDynamicResolutionCurrentHeightScale;
		a_state->fDynamicResolutionPreviousWidthScale = a_state->fDynamicResolutionCurrentWidthScale;
		a_state->fDynamicResolutionCurrentHeightScale = currentScaleFactor;
		a_state->fDynamicResolutionCurrentWidthScale = currentScaleFactor;
	} else {
		a_state->fDynamicResolutionCurrentHeightScale = 1;
		a_state->fDynamicResolutionCurrentWidthScale = 1;
		a_state->fDynamicResolutionPreviousHeightScale = a_state->fDynamicResolutionCurrentHeightScale;
		a_state->fDynamicResolutionPreviousWidthScale = a_state->fDynamicResolutionCurrentWidthScale;
	}
}

void DRS::MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		GetGameSettings();
	//	LoadSettings();
		break;

	case SKSE::MessagingInterface::kNewGame:
		//LoadSettings();
		break;

	case SKSE::MessagingInterface::kPreLoadGame:
	//	LoadSettings();
		break;
	}
}

extern ENB_API::ENBSDKALT1001* g_ENB;

void DRS::UpdateUI()
{
	auto bar = g_ENB->TwGetBarByEnum(ENB_API::ENBWindowType::EditorBarEffects);
	g_ENB->TwAddVarRW(bar, "Target FPS", ETwType::TW_TYPE_FLOAT, &fTargetFPS, "group='Dynamic Resolution Scaling' min=1.00 max=1000.0 step=1.00 precision=0.01");
}


// Fader Menu
// Mist Menu
// Loading Menu
// LoadWaitSpinner

RE::BSEventNotifyControl MenuOpenCloseEventHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
	if (a_event->menuName == "Mist Menu") {
		if (a_event->opening)
			DRS::GetSingleton()->reset = true;
		else
			DRS::GetSingleton()->reset = false;
	}

	return RE::BSEventNotifyControl::kContinue;
}

bool MenuOpenCloseEventHandler::Register()
{
	static MenuOpenCloseEventHandler singleton;
	auto                             ui = RE::UI::GetSingleton();

	if (!ui) {
		logger::error("UI event source not found");
		return false;
	}

	ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(&singleton);

	logger::info("Registered {}", typeid(singleton).name());

	return true;
}
