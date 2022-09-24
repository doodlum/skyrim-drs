#include "GPUInfo.h"

// Shared

float GPUInfo::GetGPUUsage()
{
	switch (gpuType) {
	case (GPUType::NVIDIA):
		return NVAPI_GetGPUUsage(0) / 100.0f;
		break;
	case (GPUType::AMD):
		return ADL_GetGPUUsage(0);
		break;
	}
	return 100.0f;
}

GPUInfo::GPUType GPUInfo::GetGPUType()
{
	return gpuType;
}

void GPUInfo::Initialize()
{
	if (InitializeNVAPI()) {
		logger::info("NVAPI Initialised");
		if (NVAPI_gpuCount > 0) {
			logger::info("Found {} NVIDIA GPU(s)", NVAPI_gpuCount);
			gpuType = GPUType::NVIDIA;
			logger::info("Using NVAPI");
			return;
		}
	}

	if (InitializeADL()) {
		logger::info("ADL Initialised");
		if (auto gpu_count = ADL_CountActiveGPUs()) {
			logger::info("Found {} AMD GPU(s)", gpu_count);
			gpuType = GPUType::AMD;
			logger::info("Using ADL");
			return;
		}
	}

	logger::error("Did not find an NVIDIA or AMD GPU, frame timing reports will be imprecise");
}

// NVAPI

bool GPUInfo::InitializeNVAPI()
{
	HMODULE hNVAPI_DLL = LoadLibraryA("nvapi64.dll");

	if (!hNVAPI_DLL)
		return false;

	// nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
	NVAPI_QueryInterface = (NVAPI_QueryInterface_t)GetProcAddress(hNVAPI_DLL, "nvapi_QueryInterface");

	// Some useful internal functions that aren't exported by nvapi.dll
	NVAPI_Initialize = (NVAPI_Initialize_t)(*NVAPI_QueryInterface)(0x0150E828);
	NVAPI_EnumPhysicalGPUs = (NVAPI_EnumPhysicalGPUs_t)(*NVAPI_QueryInterface)(0xE5AC921F);
	NVAPI_GPU_GetUsages = (NVAPI_GPU_GetUsages_t)(*NVAPI_QueryInterface)(0x189A1FDF);

	if (NVAPI_Initialize != nullptr &&
		NVAPI_EnumPhysicalGPUs != nullptr &&
		NVAPI_GPU_GetUsages != nullptr) {
		(*NVAPI_Initialize)();
		(*NVAPI_EnumPhysicalGPUs)(NVAPI_gpuHandles, &NVAPI_gpuCount);
		return true;
	}

	return false;
};

float GPUInfo::NVAPI_GetGPUUsage(int GpuIndex)
{
	// gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
	NVAPI_gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;

	(*NVAPI_GPU_GetUsages)(NVAPI_gpuHandles[GpuIndex], NVAPI_gpuUsages);
	int usage = NVAPI_gpuUsages[3];

	return (float)usage;
}

// ADL API

void* __stdcall GPUInfo::ADL_Main_Memory_Alloc(int iSize)
{
	void* lpBuffer = malloc(iSize);
	return lpBuffer;
}

bool GPUInfo::InitializeADL()
{
	for (int i = 0; i < ADL_MAX_ADAPTERS; i++) {
		adl_adapters[i].iSize = sizeof(AdapterInfo);
		adl_active[i].iSize = sizeof(AdapterInfo);
	}

	HMODULE hADL_DLL = LoadLibraryA("atiadlxx.dll");

	if (!hADL_DLL)
		return false;

	ADL_Main_Control_Create =
		(ADL_MAIN_CONTROL_CREATE)GetProcAddress(
			hADL_DLL, "ADL_Main_Control_Create");
	ADL_Main_Control_Destroy =
		(ADL_MAIN_CONTROL_DESTROY)GetProcAddress(
			hADL_DLL, "ADL_Main_Control_Destroy");

	ADL_Adapter_NumberOfAdapters_Get =
		(ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(
			hADL_DLL, "ADL_Adapter_NumberOfAdapters_Get");

	ADL_Adapter_AdapterInfo_Get =
		(ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(
			hADL_DLL, "ADL_Adapter_AdapterInfo_Get");

	ADL_Adapter_Active_Get =
		(ADL_ADAPTER_ACTIVE_GET)GetProcAddress(
			hADL_DLL, "ADL_Adapter_Active_Get");

	ADL_Overdrive5_CurrentActivity_Get =
		(ADL_OVERDRIVE5_CURRENTACTIVITY_GET)GetProcAddress(
			hADL_DLL, "ADL_Overdrive5_CurrentActivity_Get");

	if (ADL_Main_Control_Create != nullptr &&
		ADL_Main_Control_Destroy != nullptr &&
		ADL_Adapter_NumberOfAdapters_Get != nullptr &&
		ADL_Adapter_Active_Get != nullptr &&
		ADL_Adapter_AdapterInfo_Get != nullptr &&
		ADL_Overdrive5_CurrentActivity_Get != nullptr) {
		if (ADL_OK == ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 0)) {
			return true;
		}
	}
	return false;
}

static AdapterInfo adl_adapters[ADL_MAX_ADAPTERS] = {};
static AdapterInfo adl_active[ADL_MAX_ADAPTERS] = {};

int GPUInfo::ADL_CountPhysicalGPUs(void)
{
	int num_gpus = 0;

	if (ADL_Adapter_NumberOfAdapters_Get(&num_gpus) == ADL_OK) {
		return num_gpus;
	}

	return 0;
}

int GPUInfo::ADL_CountActiveGPUs(void)
{
	int active_count = 0;
	int adapter_count = ADL_CountPhysicalGPUs();

	ADL_Adapter_AdapterInfo_Get(
		adl_adapters,
		adapter_count * sizeof(AdapterInfo));

	for (int i = 0; i < adapter_count; i++) {
		int adapter_status = ADL_FALSE;

		ADL_Adapter_Active_Get(adl_adapters[i].iAdapterIndex, &adapter_status);

		if (adapter_status == ADL_TRUE) {
			memcpy(&adl_active[active_count++],
				&adl_adapters[i],
				sizeof(AdapterInfo));
		}
	}

	return active_count;
}

AdapterInfo* GPUInfo::ADL_GetActiveAdapter(int idx)
{
	if (idx < ADL_MAX_ADAPTERS)
		return &adl_active[idx];

	return nullptr;
}

float GPUInfo::ADL_GetGPUUsage(int GpuIndex)
{
	AdapterInfo* pAdapter = ADL_GetActiveAdapter(GpuIndex);

	if (pAdapter->iAdapterIndex >= ADL_MAX_ADAPTERS || pAdapter->iAdapterIndex < 0) {
		return 100.0f;
	}

	ADLPMActivity activity = {};

	activity.iSize = sizeof(ADLPMActivity);

	ADL_Overdrive5_CurrentActivity_Get(pAdapter->iAdapterIndex, &activity);

	return (float)activity.iActivityPercent;
}
