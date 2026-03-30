#include "RDGStarterSceneViewExtension.h"

#include "GlobalShader.h"
#include "HAL/IConsoleManager.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RDGStarterModule.h"
#include "RenderGraphBuilder.h"
#include "SceneRenderTargetParameters.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"

namespace
{
	static constexpr int32 RDGStarterMaxBlurRadius = 16;
	static constexpr int32 RDGStarterMaxGaussianSampleCount = RDGStarterMaxBlurRadius * 2 + 1;
	static constexpr int32 RDGStarterMaxSSAOSampleCount = 16;

	int32 BuildGaussianKernel(
		FVector4f (&OutSampleOffsetsWeights)[RDGStarterMaxGaussianSampleCount],
		const int32 InRadius,
		const float InSigma)
	{
		const int32 Radius = FMath::Clamp(InRadius, 1, RDGStarterMaxBlurRadius);
		const float Sigma = FMath::Max(InSigma, 0.1f);
		const float TwoSigmaSquared = 2.0f * Sigma * Sigma;

		int32 SampleCount = 0;
		float WeightSum = 0.0f;

		for (int32 Offset = -Radius; Offset <= Radius; ++Offset)
		{
			const float OffsetAsFloat = static_cast<float>(Offset);
			const float Weight = FMath::Exp(-(OffsetAsFloat * OffsetAsFloat) / TwoSigmaSquared);

			OutSampleOffsetsWeights[SampleCount] = FVector4f(OffsetAsFloat, Weight, 0.0f, 0.0f);
			WeightSum += Weight;
			++SampleCount;
		}

		const float NormalizationScale = WeightSum > 0.0f ? 1.0f / WeightSum : 0.0f;
		for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
		{
			OutSampleOffsetsWeights[SampleIndex].Y *= NormalizationScale;
		}

		for (int32 SampleIndex = SampleCount; SampleIndex < RDGStarterMaxGaussianSampleCount; ++SampleIndex)
		{
			OutSampleOffsetsWeights[SampleIndex] = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
		}

		return SampleCount;
	}

	int32 BuildSSAOKernel(
		FVector4f (&OutKernel)[RDGStarterMaxSSAOSampleCount],
		const int32 InSampleCount)
	{
		const int32 SampleCount = FMath::Clamp(InSampleCount, 1, RDGStarterMaxSSAOSampleCount);
		static constexpr float GoldenAngle = 2.39996323f;

		for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
		{
			const float T = (static_cast<float>(SampleIndex) + 0.5f) / static_cast<float>(SampleCount);
			const float Angle = GoldenAngle * static_cast<float>(SampleIndex);
			const float DiskRadius = FMath::Sqrt(T);
			const float Scale = FMath::Lerp(0.1f, 1.0f, T * T);

			const float X = FMath::Cos(Angle) * DiskRadius;
			const float Y = FMath::Sin(Angle) * DiskRadius;
			const float Z = FMath::Sqrt(FMath::Max(0.0f, 1.0f - DiskRadius * DiskRadius));

			OutKernel[SampleIndex] = FVector4f(X * Scale, Y * Scale, Z * Scale, 0.0f);
		}

		for (int32 SampleIndex = SampleCount; SampleIndex < RDGStarterMaxSSAOSampleCount; ++SampleIndex)
		{
			OutKernel[SampleIndex] = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
		}

		return SampleCount;
	}
}

// 深度可视化像素着色器。
class FRDGStarterDepthPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FRDGStarterDepthPS);
	SHADER_USE_PARAMETER_STRUCT(FRDGStarterDepthPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// DeferredShadingCommon.ush 里的辅助函数会用到 View。
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		// 深度可视化至少需要 SceneDepth。
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("RDG_STARTER_MAX_GAUSSIAN_SAMPLES"), RDGStarterMaxGaussianSampleCount);
	}
};

// 法线可视化像素着色器。
class FRDGStarterNormalPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FRDGStarterNormalPS);
	SHADER_USE_PARAMETER_STRUCT(FRDGStarterNormalPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("RDG_STARTER_MAX_GAUSSIAN_SAMPLES"), RDGStarterMaxGaussianSampleCount);
	}
};

// 粗糙度可视化像素着色器。
class FRDGStarterRoughnessPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FRDGStarterRoughnessPS);
	SHADER_USE_PARAMETER_STRUCT(FRDGStarterRoughnessPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("RDG_STARTER_MAX_GAUSSIAN_SAMPLES"), RDGStarterMaxGaussianSampleCount);
	}
};

// 高斯模糊像素着色器。
class FRDGStarterGaussianBlurPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FRDGStarterGaussianBlurPS);
	SHADER_USE_PARAMETER_STRUCT(FRDGStarterGaussianBlurPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureInput, Input)
		SHADER_PARAMETER(uint32, SampleCount)
		SHADER_PARAMETER_ARRAY(FVector4f, SampleOffsetsWeights, [RDGStarterMaxGaussianSampleCount])
		SHADER_PARAMETER(FVector2f, BlurStep)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("RDG_STARTER_MAX_GAUSSIAN_SAMPLES"), RDGStarterMaxGaussianSampleCount);
	}
};

// 深度感知双边模糊像素着色器。
class FRDGStarterBilateralBlurPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FRDGStarterBilateralBlurPS);
	SHADER_USE_PARAMETER_STRUCT(FRDGStarterBilateralBlurPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureInput, Input)
		SHADER_PARAMETER(uint32, SampleCount)
		SHADER_PARAMETER_ARRAY(FVector4f, SampleOffsetsWeights, [RDGStarterMaxGaussianSampleCount])
		SHADER_PARAMETER(FVector2f, BlurStep)
		SHADER_PARAMETER(float, DepthSigma)
		SHADER_PARAMETER(float, NormalSigma)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("RDG_STARTER_MAX_GAUSSIAN_SAMPLES"), RDGStarterMaxGaussianSampleCount);
	}
};

// 屏幕空间环境光遮蔽像素着色器。
class FRDGStarterSSAOPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FRDGStarterSSAOPS);
	SHADER_USE_PARAMETER_STRUCT(FRDGStarterSSAOPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
		SHADER_PARAMETER_STRUCT(FScreenPassTextureInput, Input)
		SHADER_PARAMETER(uint32, SSAOSampleCount)
		SHADER_PARAMETER_ARRAY(FVector4f, SSAOKernel, [RDGStarterMaxSSAOSampleCount])
		SHADER_PARAMETER(float, SSAORadius)
		SHADER_PARAMETER(float, SSAOBias)
		SHADER_PARAMETER(float, SSAOIntensity)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("RDG_STARTER_MAX_GAUSSIAN_SAMPLES"), RDGStarterMaxGaussianSampleCount);
		OutEnvironment.SetDefine(TEXT("RDG_STARTER_MAX_SSAO_SAMPLES"), RDGStarterMaxSSAOSampleCount);
	}
};

// AO 合成像素着色器。
class FRDGStarterAOCompositePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FRDGStarterAOCompositePS);
	SHADER_USE_PARAMETER_STRUCT(FRDGStarterAOCompositePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureInput, Input)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, AOTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, AOSampler)
		SHADER_PARAMETER(float, AOCompositeStrength)
		SHADER_PARAMETER(float, AOCompositePower)
		SHADER_PARAMETER(float, AOCompositeMinVisibility)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("RDG_STARTER_MAX_GAUSSIAN_SAMPLES"), RDGStarterMaxGaussianSampleCount);
	}
};

IMPLEMENT_GLOBAL_SHADER(FRDGStarterDepthPS, "/RDGStarter/Private/RDGStarterFullscreen.usf", "MainDepthPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRDGStarterNormalPS, "/RDGStarter/Private/RDGStarterFullscreen.usf", "MainNormalPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRDGStarterRoughnessPS, "/RDGStarter/Private/RDGStarterFullscreen.usf", "MainRoughnessPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRDGStarterGaussianBlurPS, "/RDGStarter/Private/RDGStarterFullscreen.usf", "MainGaussianBlurPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRDGStarterBilateralBlurPS, "/RDGStarter/Private/RDGStarterFullscreen.usf", "MainBilateralBlurPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRDGStarterSSAOPS, "/RDGStarter/Private/RDGStarterFullscreen.usf", "MainSSAOPS", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FRDGStarterAOCompositePS, "/RDGStarter/Private/RDGStarterFullscreen.usf", "MainAOCompositePS", SF_Pixel);

// 统一的调试模式开关。
// 0: 关闭
// 1: 深度可视化
// 2: 法线可视化
// 3: 原始 SceneColor
// 4: Roughness 可视化
// 5: Gaussian Blur
// 6: Bilateral Blur
// 7: SSAO（双边模糊后）
// 8: Raw SSAO
// 9: AOComposite（后处理合成）
static TAutoConsoleVariable<int32> CVarRDGStarterDebugMode(
	TEXT("r.RDGStarter.DebugMode"),
	2,
	TEXT("RDGStarter debug visualization mode.\n")
	TEXT("0: Disabled\n")
	TEXT("1: Depth\n")
	TEXT("2: Normal\n")
	TEXT("3: SceneColor\n")
	TEXT("4: Roughness\n")
	TEXT("5: GaussianBlur\n")
	TEXT("6: BilateralBlur\n")
	TEXT("7: SSAO\n")
	TEXT("8: RawSSAO\n")
	TEXT("9: AOComposite"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarRDGStarterBlurRadius(
	TEXT("r.RDGStarter.BlurRadius"),
	5,
	TEXT("RDGStarter Gaussian blur radius.\n")
	TEXT("Each blur pass samples 2 * Radius + 1 texels.\n")
	TEXT("Valid range: 1 - 16."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarRDGStarterBlurSigma(
	TEXT("r.RDGStarter.BlurSigma"),
	2.0f,
	TEXT("RDGStarter Gaussian blur sigma.\n")
	TEXT("Controls how quickly weights fall off away from the center sample.\n")
	TEXT("Recommended range: 0.5 - 6.0."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarRDGStarterBilateralDepthSigma(
	TEXT("r.RDGStarter.BilateralDepthSigma"),
	0.02f,
	TEXT("RDGStarter bilateral blur relative depth sigma.\n")
	TEXT("Smaller values preserve edges more aggressively.\n")
	TEXT("Recommended range: 0.005 - 0.1."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarRDGStarterBilateralNormalSigma(
	TEXT("r.RDGStarter.BilateralNormalSigma"),
	0.25f,
	TEXT("RDGStarter bilateral blur normal sigma.\n")
	TEXT("Smaller values preserve normal discontinuities more aggressively.\n")
	TEXT("Recommended range: 0.05 - 1.0."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarRDGStarterSSAOSampleCount(
	TEXT("r.RDGStarter.SSAOSampleCount"),
	12,
	TEXT("RDGStarter SSAO sample count.\n")
	TEXT("Valid range: 1 - 16."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarRDGStarterSSAORadius(
	TEXT("r.RDGStarter.SSAORadius"),
	80.0f,
	TEXT("RDGStarter SSAO sample radius in translated world units.\n")
	TEXT("Typical range: 20 - 200."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarRDGStarterSSAOBias(
	TEXT("r.RDGStarter.SSAOBias"),
	4.0f,
	TEXT("RDGStarter SSAO bias used to reduce self-occlusion.\n")
	TEXT("Typical range: 0.5 - 8."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarRDGStarterSSAOIntensity(
	TEXT("r.RDGStarter.SSAOIntensity"),
	1.2f,
	TEXT("RDGStarter SSAO intensity multiplier.\n")
	TEXT("Typical range: 0.5 - 3."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarRDGStarterAOCompositeStrength(
	TEXT("r.RDGStarter.AOCompositeStrength"),
	1.0f,
	TEXT("RDGStarter AO composite strength.\n")
	TEXT("0 keeps the original scene color, 1 applies the AO as-is."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarRDGStarterAOCompositePower(
	TEXT("r.RDGStarter.AOCompositePower"),
	1.0f,
	TEXT("RDGStarter AO composite power curve.\n")
	TEXT("Values above 1 darken occluded areas more aggressively."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarRDGStarterAOCompositeMinVisibility(
	TEXT("r.RDGStarter.AOCompositeMinVisibility"),
	0.0f,
	TEXT("RDGStarter AO composite minimum visibility clamp.\n")
	TEXT("Prevents AO from driving the final color fully black."),
	ECVF_RenderThreadSafe);

FRDGStarterSceneViewExtension::FRDGStarterSceneViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

void FRDGStarterSceneViewExtension::SubscribeToPostProcessingPass(
	EPostProcessingPass Pass,
	const FSceneView& InView,
	FAfterPassCallbackDelegateArray& InOutPassCallbacks,
	bool bIsPassEnabled)
{
	// 继续挂在 Tonemap/FXAA 之后，保持和前一版深度可视化一致。
	if ((Pass == EPostProcessingPass::Tonemap || Pass == EPostProcessingPass::FXAA) && bIsPassEnabled)
	{
		InOutPassCallbacks.Add(
			FAfterPassCallbackDelegate::CreateRaw(this, &FRDGStarterSceneViewExtension::PostProcessPassAfterTonemap_RenderThread));
	}
}

bool FRDGStarterSceneViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	return CVarRDGStarterDebugMode.GetValueOnAnyThread() != 0;
}

FScreenPassTexture FRDGStarterSceneViewExtension::PostProcessPassAfterTonemap_RenderThread(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FPostProcessMaterialInputs& Inputs)
{
	// 后处理输入给到的是一个 slice，这里先转成当前 pass 可直接写入的屏幕纹理。
	const FScreenPassTexture SceneColor =
		FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor), Inputs.OverrideOutput);

	// 模式 3 直接返回当前后处理输入，不额外添加 RDG pass。
	// 这样我们就有了一个显式的“原始 SceneColor”调试模式。
	const uint32 VisualizationMode = static_cast<uint32>(FMath::Clamp(CVarRDGStarterDebugMode.GetValueOnRenderThread(), 0, 9));
	if (VisualizationMode == 3)
	{
		return SceneColor;
	}

	FScreenPassRenderTarget Output(SceneColor, ERenderTargetLoadAction::ENoAction);
	const FScreenPassTextureViewport OutputViewport(Output);
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.FeatureLevel);

	if (VisualizationMode == 1)
	{
		FRDGStarterDepthPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FRDGStarterDepthPS::FParameters>();
		PassParameters->View = View.ViewUniformBuffer;
		PassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
			GraphBuilder,
			View,
			ESceneTextureSetupMode::SceneDepth);
		PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

		TShaderMapRef<FRDGStarterDepthPS> PixelShader(ShaderMap);

		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("RDGStarter.DepthVisualization"),
			FScreenPassViewInfo(View),
			OutputViewport,
			OutputViewport,
			PixelShader,
			PassParameters);
	}
	else if (VisualizationMode == 2)
	{
		FRDGStarterNormalPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FRDGStarterNormalPS::FParameters>();
		PassParameters->View = View.ViewUniformBuffer;
		PassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
			GraphBuilder,
			View,
			ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers);
		PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

		TShaderMapRef<FRDGStarterNormalPS> PixelShader(ShaderMap);

		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("RDGStarter.NormalVisualization"),
			FScreenPassViewInfo(View),
			OutputViewport,
			OutputViewport,
			PixelShader,
			PassParameters);
	}
	else if (VisualizationMode == 4)
	{
		FRDGStarterRoughnessPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FRDGStarterRoughnessPS::FParameters>();
		PassParameters->View = View.ViewUniformBuffer;
		PassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
			GraphBuilder,
			View,
			ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers);
		PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

		TShaderMapRef<FRDGStarterRoughnessPS> PixelShader(ShaderMap);

		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("RDGStarter.RoughnessVisualization"),
			FScreenPassViewInfo(View),
			OutputViewport,
			OutputViewport,
			PixelShader,
			PassParameters);
	}
	else if (VisualizationMode == 5)
	{
		const FScreenPassRenderTarget BlurIntermediate = FScreenPassRenderTarget::CreateFromInput(
			GraphBuilder,
			SceneColor,
			ERenderTargetLoadAction::ENoAction,
			TEXT("RDGStarter.GaussianBlurTemp"));
		const FScreenPassTexture BlurIntermediateTexture(BlurIntermediate.Texture, BlurIntermediate.ViewRect);

		const FScreenPassTextureViewport SceneColorViewport(SceneColor);
		const FScreenPassTextureViewport BlurIntermediateViewport(BlurIntermediate);

		const FVector2f HorizontalBlurStep(1.0f / SceneColor.Texture->Desc.Extent.X, 0.0f);
		const FVector2f VerticalBlurStep(0.0f, 1.0f / BlurIntermediateTexture.Texture->Desc.Extent.Y);
		const int32 BlurRadius = FMath::Clamp(CVarRDGStarterBlurRadius.GetValueOnRenderThread(), 1, RDGStarterMaxBlurRadius);
		const float BlurSigma = FMath::Max(CVarRDGStarterBlurSigma.GetValueOnRenderThread(), 0.1f);

		FVector4f SampleOffsetsWeights[RDGStarterMaxGaussianSampleCount];
		const uint32 SampleCount = static_cast<uint32>(BuildGaussianKernel(SampleOffsetsWeights, BlurRadius, BlurSigma));

		FRHISamplerState* SamplerState = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		TShaderMapRef<FRDGStarterGaussianBlurPS> PixelShader(ShaderMap);

		FRDGStarterGaussianBlurPS::FParameters* HorizontalPassParameters = GraphBuilder.AllocParameters<FRDGStarterGaussianBlurPS::FParameters>();
		HorizontalPassParameters->Input = GetScreenPassTextureInput(SceneColor, SamplerState);
		HorizontalPassParameters->SampleCount = SampleCount;
		HorizontalPassParameters->BlurStep = HorizontalBlurStep;
		HorizontalPassParameters->RenderTargets[0] = BlurIntermediate.GetRenderTargetBinding();
		for (int32 SampleIndex = 0; SampleIndex < RDGStarterMaxGaussianSampleCount; ++SampleIndex)
		{
			HorizontalPassParameters->SampleOffsetsWeights[SampleIndex] = SampleOffsetsWeights[SampleIndex];
		}

		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("RDGStarter.GaussianBlur.Horizontal"),
			FScreenPassViewInfo(View),
			BlurIntermediateViewport,
			SceneColorViewport,
			PixelShader,
			HorizontalPassParameters);

		FRDGStarterGaussianBlurPS::FParameters* VerticalPassParameters = GraphBuilder.AllocParameters<FRDGStarterGaussianBlurPS::FParameters>();
		VerticalPassParameters->Input = GetScreenPassTextureInput(BlurIntermediateTexture, SamplerState);
		VerticalPassParameters->SampleCount = SampleCount;
		VerticalPassParameters->BlurStep = VerticalBlurStep;
		VerticalPassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();
		for (int32 SampleIndex = 0; SampleIndex < RDGStarterMaxGaussianSampleCount; ++SampleIndex)
		{
			VerticalPassParameters->SampleOffsetsWeights[SampleIndex] = SampleOffsetsWeights[SampleIndex];
		}

		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("RDGStarter.GaussianBlur.Vertical"),
			FScreenPassViewInfo(View),
			OutputViewport,
			BlurIntermediateViewport,
			PixelShader,
			VerticalPassParameters);
	}
	else if (VisualizationMode == 6)
	{
		const FScreenPassRenderTarget BlurIntermediate = FScreenPassRenderTarget::CreateFromInput(
			GraphBuilder,
			SceneColor,
			ERenderTargetLoadAction::ENoAction,
			TEXT("RDGStarter.BilateralBlurTemp"));
		const FScreenPassTexture BlurIntermediateTexture(BlurIntermediate.Texture, BlurIntermediate.ViewRect);

		const FScreenPassTextureViewport SceneColorViewport(SceneColor);
		const FScreenPassTextureViewport BlurIntermediateViewport(BlurIntermediate);

		const FVector2f HorizontalBlurStep(1.0f / SceneColor.Texture->Desc.Extent.X, 0.0f);
		const FVector2f VerticalBlurStep(0.0f, 1.0f / BlurIntermediateTexture.Texture->Desc.Extent.Y);
		const int32 BlurRadius = FMath::Clamp(CVarRDGStarterBlurRadius.GetValueOnRenderThread(), 1, RDGStarterMaxBlurRadius);
		const float BlurSigma = FMath::Max(CVarRDGStarterBlurSigma.GetValueOnRenderThread(), 0.1f);
		const float DepthSigma = FMath::Max(CVarRDGStarterBilateralDepthSigma.GetValueOnRenderThread(), 0.0001f);
		const float NormalSigma = FMath::Max(CVarRDGStarterBilateralNormalSigma.GetValueOnRenderThread(), 0.0001f);

		FVector4f SampleOffsetsWeights[RDGStarterMaxGaussianSampleCount];
		const uint32 SampleCount = static_cast<uint32>(BuildGaussianKernel(SampleOffsetsWeights, BlurRadius, BlurSigma));

		TShaderMapRef<FRDGStarterBilateralBlurPS> PixelShader(ShaderMap);
		FRHISamplerState* SamplerState = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

		FRDGStarterBilateralBlurPS::FParameters* HorizontalPassParameters = GraphBuilder.AllocParameters<FRDGStarterBilateralBlurPS::FParameters>();
		HorizontalPassParameters->View = View.ViewUniformBuffer;
		HorizontalPassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
			GraphBuilder,
			View,
			ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers);
		HorizontalPassParameters->Input = GetScreenPassTextureInput(SceneColor, SamplerState);
		HorizontalPassParameters->SampleCount = SampleCount;
		HorizontalPassParameters->BlurStep = HorizontalBlurStep;
		HorizontalPassParameters->DepthSigma = DepthSigma;
		HorizontalPassParameters->NormalSigma = NormalSigma;
		HorizontalPassParameters->RenderTargets[0] = BlurIntermediate.GetRenderTargetBinding();
		for (int32 SampleIndex = 0; SampleIndex < RDGStarterMaxGaussianSampleCount; ++SampleIndex)
		{
			HorizontalPassParameters->SampleOffsetsWeights[SampleIndex] = SampleOffsetsWeights[SampleIndex];
		}

		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("RDGStarter.BilateralBlur.Horizontal"),
			FScreenPassViewInfo(View),
			BlurIntermediateViewport,
			SceneColorViewport,
			PixelShader,
			HorizontalPassParameters);

		FRDGStarterBilateralBlurPS::FParameters* VerticalPassParameters = GraphBuilder.AllocParameters<FRDGStarterBilateralBlurPS::FParameters>();
		VerticalPassParameters->View = View.ViewUniformBuffer;
		VerticalPassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
			GraphBuilder,
			View,
			ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers);
		VerticalPassParameters->Input = GetScreenPassTextureInput(BlurIntermediateTexture, SamplerState);
		VerticalPassParameters->SampleCount = SampleCount;
		VerticalPassParameters->BlurStep = VerticalBlurStep;
		VerticalPassParameters->DepthSigma = DepthSigma;
		VerticalPassParameters->NormalSigma = NormalSigma;
		VerticalPassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();
		for (int32 SampleIndex = 0; SampleIndex < RDGStarterMaxGaussianSampleCount; ++SampleIndex)
		{
			VerticalPassParameters->SampleOffsetsWeights[SampleIndex] = SampleOffsetsWeights[SampleIndex];
		}

		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("RDGStarter.BilateralBlur.Vertical"),
			FScreenPassViewInfo(View),
			OutputViewport,
			BlurIntermediateViewport,
			PixelShader,
			VerticalPassParameters);
	}
	else if (VisualizationMode == 7 || VisualizationMode == 8 || VisualizationMode == 9)
	{
		const int32 SSAOSampleCountValue = FMath::Clamp(CVarRDGStarterSSAOSampleCount.GetValueOnRenderThread(), 1, RDGStarterMaxSSAOSampleCount);
		const float SSAORadius = FMath::Max(CVarRDGStarterSSAORadius.GetValueOnRenderThread(), 1.0f);
		const float SSAOBias = FMath::Max(CVarRDGStarterSSAOBias.GetValueOnRenderThread(), 0.0f);
		const float SSAOIntensity = FMath::Max(CVarRDGStarterSSAOIntensity.GetValueOnRenderThread(), 0.0f);
		const bool bApplySSAOBlur = VisualizationMode == 7 || VisualizationMode == 9;
		const bool bCompositeSSAO = VisualizationMode == 9;
		const int32 BlurRadius = FMath::Clamp(CVarRDGStarterBlurRadius.GetValueOnRenderThread(), 1, RDGStarterMaxBlurRadius);
		const float BlurSigma = FMath::Max(CVarRDGStarterBlurSigma.GetValueOnRenderThread(), 0.1f);
		const float DepthSigma = FMath::Max(CVarRDGStarterBilateralDepthSigma.GetValueOnRenderThread(), 0.0001f);
		const float NormalSigma = FMath::Max(CVarRDGStarterBilateralNormalSigma.GetValueOnRenderThread(), 0.0001f);

		FVector4f SSAOKernel[RDGStarterMaxSSAOSampleCount];
		const uint32 SSAOSampleCount = static_cast<uint32>(BuildSSAOKernel(SSAOKernel, SSAOSampleCountValue));
		FRHISamplerState* SamplerState = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		TShaderMapRef<FRDGStarterSSAOPS> SSAOPixelShader(ShaderMap);

		FRDGStarterSSAOPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FRDGStarterSSAOPS::FParameters>();
		PassParameters->View = View.ViewUniformBuffer;
		PassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
			GraphBuilder,
			View,
			ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers);
		PassParameters->Input = GetScreenPassTextureInput(SceneColor, SamplerState);
		PassParameters->SSAOSampleCount = SSAOSampleCount;
		PassParameters->SSAORadius = SSAORadius;
		PassParameters->SSAOBias = SSAOBias;
		PassParameters->SSAOIntensity = SSAOIntensity;
		PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();
		for (int32 SampleIndex = 0; SampleIndex < RDGStarterMaxSSAOSampleCount; ++SampleIndex)
		{
			PassParameters->SSAOKernel[SampleIndex] = SSAOKernel[SampleIndex];
		}

		if (!bApplySSAOBlur)
		{
			PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

			AddDrawScreenPass(
				GraphBuilder,
				RDG_EVENT_NAME("RDGStarter.SSAO.Raw"),
				FScreenPassViewInfo(View),
				OutputViewport,
				OutputViewport,
				SSAOPixelShader,
				PassParameters);
		}
		else
		{
			FVector4f BlurSampleOffsetsWeights[RDGStarterMaxGaussianSampleCount];
			const uint32 BlurSampleCount = static_cast<uint32>(BuildGaussianKernel(BlurSampleOffsetsWeights, BlurRadius, BlurSigma));
			TShaderMapRef<FRDGStarterBilateralBlurPS> BilateralBlurPixelShader(ShaderMap);

			const FScreenPassRenderTarget SSAORawTarget = FScreenPassRenderTarget::CreateFromInput(
				GraphBuilder,
				SceneColor,
				ERenderTargetLoadAction::ENoAction,
				TEXT("RDGStarter.SSAORaw"));
			const FScreenPassTexture SSAORawTexture(SSAORawTarget.Texture, SSAORawTarget.ViewRect);
			const FScreenPassTextureViewport SSAORawViewport(SSAORawTexture);

			const FScreenPassRenderTarget SSAOBlurIntermediate = FScreenPassRenderTarget::CreateFromInput(
				GraphBuilder,
				SSAORawTexture,
				ERenderTargetLoadAction::ENoAction,
				TEXT("RDGStarter.SSAOBlurTemp"));
			const FScreenPassTexture SSAOBlurIntermediateTexture(SSAOBlurIntermediate.Texture, SSAOBlurIntermediate.ViewRect);
			const FScreenPassTextureViewport SSAOBlurIntermediateViewport(SSAOBlurIntermediate);
			const FScreenPassRenderTarget SSAOBlurFinalTarget = FScreenPassRenderTarget::CreateFromInput(
				GraphBuilder,
				SSAORawTexture,
				ERenderTargetLoadAction::ENoAction,
				TEXT("RDGStarter.SSAOBlurFinal"));
			const FScreenPassTexture SSAOBlurFinalTexture(SSAOBlurFinalTarget.Texture, SSAOBlurFinalTarget.ViewRect);
			const FScreenPassTextureViewport SSAOBlurFinalViewport(SSAOBlurFinalTarget);
			const FVector2f HorizontalBlurStep(1.0f / SSAORawTexture.Texture->Desc.Extent.X, 0.0f);
			const FVector2f VerticalBlurStep(0.0f, 1.0f / SSAOBlurIntermediateTexture.Texture->Desc.Extent.Y);

			PassParameters->RenderTargets[0] = SSAORawTarget.GetRenderTargetBinding();

			AddDrawScreenPass(
				GraphBuilder,
				RDG_EVENT_NAME("RDGStarter.SSAO.Raw"),
				FScreenPassViewInfo(View),
				SSAORawViewport,
				OutputViewport,
				SSAOPixelShader,
				PassParameters);

			FRDGStarterBilateralBlurPS::FParameters* HorizontalBlurPassParameters =
				GraphBuilder.AllocParameters<FRDGStarterBilateralBlurPS::FParameters>();
			HorizontalBlurPassParameters->View = View.ViewUniformBuffer;
			HorizontalBlurPassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
				GraphBuilder,
				View,
				ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers);
			HorizontalBlurPassParameters->Input = GetScreenPassTextureInput(SSAORawTexture, SamplerState);
			HorizontalBlurPassParameters->SampleCount = BlurSampleCount;
			HorizontalBlurPassParameters->BlurStep = HorizontalBlurStep;
			HorizontalBlurPassParameters->DepthSigma = DepthSigma;
			HorizontalBlurPassParameters->NormalSigma = NormalSigma;
			HorizontalBlurPassParameters->RenderTargets[0] = SSAOBlurIntermediate.GetRenderTargetBinding();
			for (int32 SampleIndex = 0; SampleIndex < RDGStarterMaxGaussianSampleCount; ++SampleIndex)
			{
				HorizontalBlurPassParameters->SampleOffsetsWeights[SampleIndex] = BlurSampleOffsetsWeights[SampleIndex];
			}

			AddDrawScreenPass(
				GraphBuilder,
				RDG_EVENT_NAME("RDGStarter.SSAO.BilateralBlur.Horizontal"),
				FScreenPassViewInfo(View),
				SSAOBlurIntermediateViewport,
				SSAORawViewport,
				BilateralBlurPixelShader,
				HorizontalBlurPassParameters);

			FRDGStarterBilateralBlurPS::FParameters* VerticalBlurPassParameters =
				GraphBuilder.AllocParameters<FRDGStarterBilateralBlurPS::FParameters>();
			VerticalBlurPassParameters->View = View.ViewUniformBuffer;
			VerticalBlurPassParameters->SceneTextures = CreateSceneTextureUniformBuffer(
				GraphBuilder,
				View,
				ESceneTextureSetupMode::SceneDepth | ESceneTextureSetupMode::GBuffers);
			VerticalBlurPassParameters->Input = GetScreenPassTextureInput(SSAOBlurIntermediateTexture, SamplerState);
			VerticalBlurPassParameters->SampleCount = BlurSampleCount;
			VerticalBlurPassParameters->BlurStep = VerticalBlurStep;
			VerticalBlurPassParameters->DepthSigma = DepthSigma;
			VerticalBlurPassParameters->NormalSigma = NormalSigma;
			VerticalBlurPassParameters->RenderTargets[0] = bCompositeSSAO
				? SSAOBlurFinalTarget.GetRenderTargetBinding()
				: Output.GetRenderTargetBinding();
			for (int32 SampleIndex = 0; SampleIndex < RDGStarterMaxGaussianSampleCount; ++SampleIndex)
			{
				VerticalBlurPassParameters->SampleOffsetsWeights[SampleIndex] = BlurSampleOffsetsWeights[SampleIndex];
			}

			AddDrawScreenPass(
				GraphBuilder,
				RDG_EVENT_NAME("RDGStarter.SSAO.BilateralBlur.Vertical"),
				FScreenPassViewInfo(View),
				OutputViewport,
				SSAOBlurIntermediateViewport,
				BilateralBlurPixelShader,
				VerticalBlurPassParameters);

			if (bCompositeSSAO)
			{
				TShaderMapRef<FRDGStarterAOCompositePS> CompositePixelShader(ShaderMap);
				FRDGStarterAOCompositePS::FParameters* CompositePassParameters =
					GraphBuilder.AllocParameters<FRDGStarterAOCompositePS::FParameters>();
				CompositePassParameters->Input = GetScreenPassTextureInput(SceneColor, SamplerState);
				CompositePassParameters->AOTexture = SSAOBlurFinalTexture.Texture;
				CompositePassParameters->AOSampler = SamplerState;
				CompositePassParameters->AOCompositeStrength = FMath::Max(CVarRDGStarterAOCompositeStrength.GetValueOnRenderThread(), 0.0f);
				CompositePassParameters->AOCompositePower = FMath::Max(CVarRDGStarterAOCompositePower.GetValueOnRenderThread(), 0.01f);
				CompositePassParameters->AOCompositeMinVisibility =
					FMath::Clamp(CVarRDGStarterAOCompositeMinVisibility.GetValueOnRenderThread(), 0.0f, 1.0f);
				CompositePassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

				AddDrawScreenPass(
					GraphBuilder,
					RDG_EVENT_NAME("RDGStarter.SSAO.Composite"),
					FScreenPassViewInfo(View),
					OutputViewport,
					SSAOBlurFinalViewport,
					CompositePixelShader,
					CompositePassParameters);
			}
		}
	}

	return SceneColor;
}
