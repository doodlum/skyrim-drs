
#pragma comment(lib, "d3d11.lib")
#include <d3d11.h>
#include <dxgi.h>

#include <Detours.h>

#include "DRS.h"
#include "Nukem/GpuTimer.h"

ID3D11Device*        g_Device;
ID3D11DeviceContext* g_DeviceContext;
IDXGISwapChain*      g_SwapChain;

decltype(&ID3D11DeviceContext::ClearState) ptrClearState;
decltype(&IDXGISwapChain::Present)         ptrPresent;
decltype(&D3D11CreateDeviceAndSwapChain)   ptrD3D11CreateDeviceAndSwapChain;

void WINAPI hk_ClearState(ID3D11DeviceContext* This)
{
	g_GPUTimers.BeginFrame(g_DeviceContext);
	g_GPUTimers.StartTimer(g_DeviceContext, 0);
	(This->*ptrClearState)();
}

HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	g_GPUTimers.StopTimer(g_DeviceContext, 0);
	g_GPUTimers.EndFrame(g_DeviceContext);
	auto hr = (This->*ptrPresent)(SyncInterval, Flags);
	DRS::GetSingleton()->Update();
	return hr;
}

HRESULT WINAPI hk_D3D11CreateDeviceAndSwapChain(
	IDXGIAdapter*               pAdapter,
	D3D_DRIVER_TYPE             DriverType,
	HMODULE                     Software,
	UINT                        Flags,
	const D3D_FEATURE_LEVEL*    pFeatureLevels,
	UINT                        FeatureLevels,
	UINT                        SDKVersion,
	const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	IDXGISwapChain**            ppSwapChain,
	ID3D11Device**              ppDevice,
	D3D_FEATURE_LEVEL*          pFeatureLevel,
	ID3D11DeviceContext**       ppImmediateContext)
{
	logger::info("Calling original D3D11CreateDeviceAndSwapChain");
	HRESULT hr = (*ptrD3D11CreateDeviceAndSwapChain)(pAdapter,
		DriverType,
		Software,
		Flags,
		pFeatureLevels,
		FeatureLevels,
		SDKVersion,
		pSwapChainDesc,
		ppSwapChain,
		ppDevice,
		pFeatureLevel,
		ppImmediateContext);

	logger::info("Storing render device information");
	g_Device = *ppDevice;
	g_DeviceContext = *ppImmediateContext;
	g_SwapChain = *ppSwapChain;

	return hr;
}

struct Hooks
{
	struct BSGraphics_Renderer_Init_InitD3D
	{
		static void thunk()
		{
			logger::info("Calling original Init3D");
			func();
			logger::info("Accessing render device information");
			auto manager = RE::BSRenderManager::GetSingleton();
			g_Device = manager->GetRuntimeData().forwarder;
			g_DeviceContext = manager->GetRuntimeData().context;
			g_SwapChain = manager->GetRuntimeData().swapChain;
			logger::info("Detouring virtual function tables");
			*(uintptr_t*)&ptrPresent = Detours::X64::DetourClassVTable(*(uintptr_t*)g_SwapChain, &hk_IDXGISwapChain_Present, 8);
			*(uintptr_t*)&ptrClearState = Detours::X64::DetourClassVTable(*(uintptr_t*)g_DeviceContext, &hk_ClearState, 110);
			logger::info("Creating GPU timer");
			g_GPUTimers.Create(g_Device, 10);
			logger::info("Initializing GPU APIs");
			GPUInfo::GetSingleton()->Initialize();
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	static void Install()
	{
		stl::write_thunk_call<BSGraphics_Renderer_Init_InitD3D>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x50, 0x2BC));
		logger::info("Installed render startup hook");
	}
};

void PatchD3D11()
{
	Hooks::Install();
}
