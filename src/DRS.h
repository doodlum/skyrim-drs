#pragma once

#include <shared_mutex>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <Detours.h>

#include "GPUInfo.h"
#include "RE/BSGraphics.h"

extern ID3D11DeviceContext* g_DeviceContext;
extern ID3D11Device*        g_Device;
extern IDXGISwapChain*      g_SwapChain;

static float& g_deltaTimeRealTime = (*(float*)RELOCATION_ID(523661, 410200).address());  // 2F6B94C, 30064CC

class MenuOpenCloseEventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource);
	static bool                      Register();
};

class DRS
{
public:
	static DRS* GetSingleton()
	{
		static DRS handler;
		return &handler;
	}

	static void InstallHooks()
	{
		Hooks::Install();
	}

	// Settings

	RE::Setting* bEnableAutoDynamicResolution;
	void         GetGameSettings();

	int   iTargetFPS = 60;
	float fHighestScaleFactor = 1.0f;
	float fLowestScaleFactor = 0.7f;
	bool  bEnableWithENB = false;
	void  LoadINI();

	// Constants

	const float ScaleRaiseCounterLimit = 360.0f * (1.0f / 60.0f);

	const float ScaleRaiseCounterSmallIncrement = 3;
	const float ScaleRaiseCounterBigIncrement = 10;

	const float HeadroomThreshold = 0.03f;
	const float DeltaThreshold = 0.035f;

	const float ScaleIncreaseBasis = (HeadroomThreshold < DeltaThreshold) ? (float)HeadroomThreshold : (float)DeltaThreshold;
	const float ScaleIncreaseSmallFactor = 0.25f;
	const float ScaleIncreaseBigFactor = 1.0f;
	const float ScaleHeadroomClampMin = 0.1f;
	const float ScaleHeadroomClampMax = 0.5f;

	// Variables

	float currentScaleFactor = 1.0f;
	float prevGPUFrameTime = 0;
	float scaleRaiseCounter = 0;
	bool  reset = false;

	void Update();
	void ControlResolution();
	void ResetScale();

	void SetDRS(BSGraphics::State* a_state);

	void MessageHandler(SKSE::MessagingInterface::Message* a_msg);

	// Performance Queries

	float lastCPUFrameTime;

	static inline long long PerformanceCounter() noexcept
	{
		LARGE_INTEGER li;
		::QueryPerformanceCounter(&li);
		return li.QuadPart;
	}

	static inline long long PerformanceFrequency() noexcept
	{
		LARGE_INTEGER li;
		::QueryPerformanceFrequency(&li);
		return li.QuadPart;
	}

	long long frameStart = PerformanceCounter();

	void UpdateCPUFrameTime();

protected:
	struct Hooks
	{
		struct Main_SetDRS
		{
			static void thunk([[maybe_unused]] BSGraphics::State* a_state)
			{
				GetSingleton()->SetDRS(a_state);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct Main_Update_Start
		{
			static void thunk(INT64 a_unk)
			{
				GetSingleton()->frameStart = PerformanceCounter();
				func(a_unk);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct Main_Update_Render
		{
			static void thunk(RE::Main* a_main)
			{
				GetSingleton()->UpdateCPUFrameTime();
				func(a_main);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		static void Install()
		{
			stl::write_thunk_call<Main_SetDRS>(REL::RelocationID(35556, 36555).address() + REL::Relocate(0x2D, 0x2D));
			stl::write_thunk_call<Main_Update_Start>(REL::RelocationID(35565, 36564).address() + REL::Relocate(0x1E, 0x3E, 0x33));
			stl::write_thunk_call<Main_Update_Render>(REL::RelocationID(35565, 36564).address() + REL::Relocate(0x5D2, 0xA92, 0x678));
		}
	};

private:
	DRS(){};

	DRS(const DRS&) = delete;
	DRS(DRS&&) = delete;

	~DRS() = default;

	DRS& operator=(const DRS&) = delete;
	DRS& operator=(DRS&&) = delete;
};
