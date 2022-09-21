#pragma once

#pragma comment(lib, "d3d11.lib")
#include <dxgi.h>
#include <d3d11.h>

#pragma warning(push)
#pragma warning(disable: 4828)
#include <NVAPI/nvapi.h>
#pragma warning(pop)

#include <adl/adl_defines.h>
#include <adl/adl_sdk.h>
#include <adl/adl_structures.h>

class GPUInfo
{
public:
	static GPUInfo* GetSingleton()
	{
		static GPUInfo handler;
		return &handler;
	}

	void  Initialize();
	float GetGPUUsage();

	enum class GPUType
	{
		NVIDIA,
		AMD,
		INTEL,
		UNKNOWN
	};

	GPUType GetGPUType();

private:
	GPUInfo(){};

	GPUInfo(const GPUInfo&) = delete;
	GPUInfo(GPUInfo&&) = delete;

	~GPUInfo() = default;

	GPUInfo& operator=(const GPUInfo&) = delete;
	GPUInfo& operator=(GPUInfo&&) = delete;

	// NVAPI

#define NVAPI_MAX_PHYSICAL_GPUS 64
#define NVAPI_MAX_USAGES_PER_GPU 34

	typedef int* (*NVAPI_QueryInterface_t)(unsigned int offset);
	typedef int (*NVAPI_Initialize_t)();
	typedef int (*NVAPI_EnumPhysicalGPUs_t)(int** handles, int* count);
	typedef int (*NVAPI_GPU_GetUsages_t)(int* handle, unsigned int* usages);

	NVAPI_QueryInterface_t   NVAPI_QueryInterface = NULL;
	NVAPI_Initialize_t       NVAPI_Initialize = NULL;
	NVAPI_EnumPhysicalGPUs_t NVAPI_EnumPhysicalGPUs = NULL;
	NVAPI_GPU_GetUsages_t    NVAPI_GPU_GetUsages = NULL;

	int          NVAPI_gpuCount = 0;
	int*         NVAPI_gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
	unsigned int NVAPI_gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };

	bool  InitializeNVAPI();
	float NVAPI_GetGPUUsage(int GpuIndex);

	// ADL API

	typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
	typedef int (*ADL_MAIN_CONTROL_DESTROY)();

	typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int*);
	typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo, int);
	typedef int (*ADL_ADAPTER_ACTIVE_GET)(int, int*);

	typedef int (*ADL_OVERDRIVE5_CURRENTACTIVITY_GET)(int iAdapterIndex, ADLPMActivity* lpActivity);

	ADL_MAIN_CONTROL_CREATE  ADL_Main_Control_Create = nullptr;
	ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy = nullptr;

	ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get = nullptr;
	ADL_ADAPTER_ADAPTERINFO_GET      ADL_Adapter_AdapterInfo_Get = nullptr;
	ADL_ADAPTER_ACTIVE_GET           ADL_Adapter_Active_Get = nullptr;

	ADL_OVERDRIVE5_CURRENTACTIVITY_GET ADL_Overdrive5_CurrentActivity_Get = nullptr;

	ADL_CONTEXT_HANDLE context = NULL;

	AdapterInfo adl_adapters[ADL_MAX_ADAPTERS] = {};
	AdapterInfo adl_active[ADL_MAX_ADAPTERS] = {};

	static void* __stdcall ADL_Main_Memory_Alloc(int iSize);
	bool         InitializeADL();
	int          ADL_CountPhysicalGPUs(void);
	int          ADL_CountActiveGPUs(void);
	AdapterInfo* ADL_GetActiveAdapter(int idx);
	float        ADL_GetGPUUsage(int GpuIndex);

	// Shared

	GPUType gpuType = GPUType::UNKNOWN;
};
