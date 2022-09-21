#pragma once
#include "DirectXMath.h"

#define AutoPtr(Type, Name, OffsetSE, OffsetAE) static Type& Name = (*(Type*)RELOCATION_ID(OffsetSE, OffsetAE).address())
namespace BSGraphics
{
	struct alignas(16) ViewData
	{
		DirectX::XMVECTOR m_ViewUp;
		DirectX::XMVECTOR m_ViewRight;
		DirectX::XMVECTOR m_ViewDir;
		DirectX::XMMATRIX m_ViewMat;
		DirectX::XMMATRIX m_ProjMat;
		DirectX::XMMATRIX m_ViewProjMat;
		DirectX::XMMATRIX m_UnknownMat1;
		DirectX::XMMATRIX m_ViewProjMatrixUnjittered;
		DirectX::XMMATRIX m_PreviousViewProjMatrixUnjittered;
		DirectX::XMMATRIX m_ProjMatrixUnjittered;
		DirectX::XMMATRIX m_UnknownMat2;
		float             m_ViewPort[4];  // NiRect<float> { left = 0, right = 1, top = 1, bottom = 0 }
		RE::NiPoint2      m_ViewDepthRange;
		char              _pad0[0x8];
	};
	static_assert(sizeof(ViewData) == 0x250);

	struct CameraStateData
	{
		RE::NiCamera* pReferenceCamera;
		ViewData      CamViewData;
		RE::NiPoint3  PosAdjust;
		RE::NiPoint3  CurrentPosAdjust;
		RE::NiPoint3  PreviousPosAdjust;
		bool          UseJitter;
		char          _pad0[0x8];
	};
	static_assert(sizeof(CameraStateData) == 0x290);

	//struct State
	//{
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjNoiseMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjDiffuseMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjNormalMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjNormalDetailMap;
	//	char                               _pad0[0x1C];
	//	float                              unknown[0x4];
	////	char                               _pad0[0x3];
	////	char                               _pad0[0x2C];
	//	uint32_t                           uiFrameCount;
	//	bool                               bInsideFrame;
	//	bool                               bLetterbox;
	//	bool                               bUnknown1;
	//	bool                               bCompiledShaderThisFrame;
	//	bool                               bUseEarlyZ;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureBlack;  // "BSShader_DefHeightMap"
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureWhite;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureGrey;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultHeightMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultReflectionCubeMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultFaceDetailMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTexEffectMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureNormalMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureDitherNoiseMap;
	//	RE::BSTArray<CameraStateData>      kCameraDataCacheA;
	//	char                               _pad2[0x4];             // unknown dword
	//	float                              fHaltonSequence[2][8];  // (2, 3) Halton Sequence points
	//	float                              fDynamicResolutionWidthRatio;
	//	float                              fDynamicResolutionHeightRatio;
	//	float                              fDynamicResolutionPreviousWidthRatio;
	//	float                              fDynamicResolutionPreviousHeightRatio;
	//	float							   uiDynamicResolutionUnknown1;
	//	float                              uiDynamicResolutionUnknown2;
	//	uint16_t                           usDynamicResolutionUnknown3;
	//};
	//static_assert(sizeof(State) == 0x118);

	struct State
	{
		char     pad[252];
		float    fDynamicResolutionCurrentWidthScale;
		float    fDynamicResolutionCurrentHeightScale;
		float    fDynamicResolutionPreviousWidthScale;
		float    fDynamicResolutionPreviousHeightScale;
		float    fDynamicResolutionWidthRatio;
		float    fDynamicResolutionHeightRatio;
		uint16_t usDynamicResolutionCounter;
	};

	static_assert(sizeof(State) == 0x118);

	AutoPtr(State, gState, 524998, 411479);

	//AutoPtr(int, gUnknownInt1, 524998, 389029);
	//AutoPtr(float, gUnknownFloat1, 524998, 389023);
	//AutoPtr(float, gUnknownFloat2, 524998, 389020);
	//AutoPtr(float, gUnknownFloat3, 524998, 389017);
	//AutoPtr(float, gUnknownFloat4, 524998, 389026);
	//AutoPtr(float, gUnknownFloat5, 524998, 389035);
	//AutoPtr(float, gUnknownFloat6, 524998, 389032);

}

