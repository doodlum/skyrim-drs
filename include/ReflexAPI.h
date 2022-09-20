#pragma once

#pragma warning(push)
#pragma warning(disable: 4828)
#include <NVAPI/nvapi.h>
#pragma warning(pop)

// Reflex modder interface
class ReflexAPI
{
public:

	ReflexAPI(HMODULE a_enbmodule)
	{
		this->reflexmodule = a_enbmodule;
	}

	typedef bool (*_GetReflexEnabled)();
	typedef bool (*_GetLatencyReport)(NV_LATENCY_RESULT_PARAMS* pGetLatencyParams);
	typedef bool (*_SetFPSLimit)(float a_limit);

	bool GetReflexEnabled()
	{
		logger::debug("GetReflexEnabled");
		return reinterpret_cast<_GetReflexEnabled>(GetProcAddress(reflexmodule, "GetReflexEnabled"))();
	}

	bool GetLatencyReport(NV_LATENCY_RESULT_PARAMS* pGetLatencyParams)
	{
		logger::debug("GetLatencyReport");
		return reinterpret_cast<_GetLatencyReport>(GetProcAddress(reflexmodule, "GetLatencyReport"))(pGetLatencyParams);
	}

	bool SetFPSLimit(float a_limit)
	{
		logger::debug("SetFPSLimit");
		return reinterpret_cast<_SetFPSLimit>(GetProcAddress(reflexmodule, "SetFPSLimit"))(a_limit);
	}

protected:
	HMODULE reflexmodule = NULL;
};


/// <summary>
/// Request the Reflex interface.
/// Recommended: Send your request during or after SKSEMessagingInterface::kMessage_PostLoad to make sure the dll has already been loaded
/// </summary>
/// <returns>The pointer to the API singleton, or nullptr if request failed</returns>
[[nodiscard]] static inline void* RequestReflexAPI()
{
	auto reflexmodule = GetModuleHandleA("NVIDIA_Reflex.dll");

	if (!reflexmodule)
		return nullptr;

	return new ReflexAPI(reflexmodule);
}
