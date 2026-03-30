#include "RDGStarterModule.h"

#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "RDGStarterSceneViewExtension.h"
#include "SceneViewExtension.h"
#include "ShaderCore.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY(LogRDGStarter);

namespace
{
	static constexpr TCHAR DebugModeCVarName[] = TEXT("r.RDGStarter.DebugMode");
	static constexpr TCHAR BlurRadiusCVarName[] = TEXT("r.RDGStarter.BlurRadius");
	static constexpr TCHAR BlurSigmaCVarName[] = TEXT("r.RDGStarter.BlurSigma");
	static constexpr TCHAR BilateralDepthSigmaCVarName[] = TEXT("r.RDGStarter.BilateralDepthSigma");
	static constexpr TCHAR BilateralNormalSigmaCVarName[] = TEXT("r.RDGStarter.BilateralNormalSigma");
	static constexpr TCHAR SSAOSampleCountCVarName[] = TEXT("r.RDGStarter.SSAOSampleCount");
	static constexpr TCHAR SSAORadiusCVarName[] = TEXT("r.RDGStarter.SSAORadius");
	static constexpr TCHAR SSAOBiasCVarName[] = TEXT("r.RDGStarter.SSAOBias");
	static constexpr TCHAR SSAOIntensityCVarName[] = TEXT("r.RDGStarter.SSAOIntensity");
	static constexpr TCHAR AOCompositeStrengthCVarName[] = TEXT("r.RDGStarter.AOCompositeStrength");
	static constexpr TCHAR AOCompositePowerCVarName[] = TEXT("r.RDGStarter.AOCompositePower");
	static constexpr TCHAR AOCompositeMinVisibilityCVarName[] = TEXT("r.RDGStarter.AOCompositeMinVisibility");
	static constexpr int32 RDGStarterBlurRadiusMin = 1;
	static constexpr int32 RDGStarterBlurRadiusMax = 16;
	static constexpr float RDGStarterBlurSigmaMin = 0.25f;
	static constexpr float RDGStarterBlurSigmaMax = 10.0f;
	static constexpr float RDGStarterBlurSigmaStep = 0.25f;
	static constexpr float RDGStarterBilateralDepthSigmaMin = 0.005f;
	static constexpr float RDGStarterBilateralDepthSigmaMax = 0.1f;
	static constexpr float RDGStarterBilateralDepthSigmaStep = 0.005f;
	static constexpr float RDGStarterBilateralNormalSigmaMin = 0.05f;
	static constexpr float RDGStarterBilateralNormalSigmaMax = 1.0f;
	static constexpr float RDGStarterBilateralNormalSigmaStep = 0.05f;
	static constexpr int32 RDGStarterSSAOSampleCountMin = 1;
	static constexpr int32 RDGStarterSSAOSampleCountMax = 16;
	static constexpr int32 RDGStarterSSAORadiusMin = 1;
	static constexpr int32 RDGStarterSSAORadiusMax = 300;
	static constexpr float RDGStarterSSAORadiusStep = 10.0f;
	static constexpr float RDGStarterSSAOBiasMin = 0.0f;
	static constexpr float RDGStarterSSAOBiasMax = 16.0f;
	static constexpr float RDGStarterSSAOBiasStep = 0.5f;
	static constexpr float RDGStarterSSAOIntensityMin = 0.0f;
	static constexpr float RDGStarterSSAOIntensityMax = 5.0f;
	static constexpr float RDGStarterSSAOIntensityStep = 0.1f;
	static constexpr float RDGStarterAOCompositeStrengthMin = 0.0f;
	static constexpr float RDGStarterAOCompositeStrengthMax = 2.0f;
	static constexpr float RDGStarterAOCompositeStrengthStep = 0.1f;
	static constexpr float RDGStarterAOCompositePowerMin = 0.25f;
	static constexpr float RDGStarterAOCompositePowerMax = 4.0f;
	static constexpr float RDGStarterAOCompositePowerStep = 0.1f;
	static constexpr float RDGStarterAOCompositeMinVisibilityMin = 0.0f;
	static constexpr float RDGStarterAOCompositeMinVisibilityMax = 1.0f;
	static constexpr float RDGStarterAOCompositeMinVisibilityStep = 0.05f;

	int32 GetRDGStarterDebugMode()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(DebugModeCVarName))
		{
			return CVar->GetInt();
		}

		return 0;
	}

	int32 GetRDGStarterBlurRadius()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(BlurRadiusCVarName))
		{
			return CVar->GetInt();
		}

		return 5;
	}

	float GetRDGStarterBlurSigma()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(BlurSigmaCVarName))
		{
			return CVar->GetFloat();
		}

		return 2.0f;
	}

	float GetRDGStarterBilateralDepthSigma()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(BilateralDepthSigmaCVarName))
		{
			return CVar->GetFloat();
		}

		return 0.02f;
	}

	float GetRDGStarterBilateralNormalSigma()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(BilateralNormalSigmaCVarName))
		{
			return CVar->GetFloat();
		}

		return 0.25f;
	}

	int32 GetRDGStarterSSAOSampleCount()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(SSAOSampleCountCVarName))
		{
			return CVar->GetInt();
		}

		return 12;
	}

	float GetRDGStarterSSAORadius()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(SSAORadiusCVarName))
		{
			return CVar->GetFloat();
		}

		return 80.0f;
	}

	float GetRDGStarterSSAOBias()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(SSAOBiasCVarName))
		{
			return CVar->GetFloat();
		}

		return 4.0f;
	}

	float GetRDGStarterSSAOIntensity()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(SSAOIntensityCVarName))
		{
			return CVar->GetFloat();
		}

		return 1.2f;
	}

	float GetRDGStarterAOCompositeStrength()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(AOCompositeStrengthCVarName))
		{
			return CVar->GetFloat();
		}

		return 1.0f;
	}

	float GetRDGStarterAOCompositePower()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(AOCompositePowerCVarName))
		{
			return CVar->GetFloat();
		}

		return 1.0f;
	}

	float GetRDGStarterAOCompositeMinVisibility()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(AOCompositeMinVisibilityCVarName))
		{
			return CVar->GetFloat();
		}

		return 0.0f;
	}

	void SetRDGStarterDebugMode(const int32 NewMode)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(DebugModeCVarName))
		{
			CVar->Set(NewMode, ECVF_SetByCode);
		}
	}

	void SetRDGStarterBlurRadius(const int32 NewRadius)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(BlurRadiusCVarName))
		{
			CVar->Set(FMath::Clamp(NewRadius, RDGStarterBlurRadiusMin, RDGStarterBlurRadiusMax), ECVF_SetByCode);
		}
	}

	void SetRDGStarterBlurSigma(const float NewSigma)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(BlurSigmaCVarName))
		{
			CVar->Set(FMath::Clamp(NewSigma, RDGStarterBlurSigmaMin, RDGStarterBlurSigmaMax), ECVF_SetByCode);
		}
	}

	void SetRDGStarterBilateralDepthSigma(const float NewSigma)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(BilateralDepthSigmaCVarName))
		{
			CVar->Set(
				FMath::Clamp(NewSigma, RDGStarterBilateralDepthSigmaMin, RDGStarterBilateralDepthSigmaMax),
				ECVF_SetByCode);
		}
	}

	void SetRDGStarterBilateralNormalSigma(const float NewSigma)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(BilateralNormalSigmaCVarName))
		{
			CVar->Set(
				FMath::Clamp(NewSigma, RDGStarterBilateralNormalSigmaMin, RDGStarterBilateralNormalSigmaMax),
				ECVF_SetByCode);
		}
	}

	void SetRDGStarterSSAOSampleCount(const int32 NewSampleCount)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(SSAOSampleCountCVarName))
		{
			CVar->Set(FMath::Clamp(NewSampleCount, RDGStarterSSAOSampleCountMin, RDGStarterSSAOSampleCountMax), ECVF_SetByCode);
		}
	}

	void SetRDGStarterSSAORadius(const float NewRadius)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(SSAORadiusCVarName))
		{
			CVar->Set(FMath::Clamp(NewRadius, static_cast<float>(RDGStarterSSAORadiusMin), static_cast<float>(RDGStarterSSAORadiusMax)), ECVF_SetByCode);
		}
	}

	void SetRDGStarterSSAOBias(const float NewBias)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(SSAOBiasCVarName))
		{
			CVar->Set(FMath::Clamp(NewBias, RDGStarterSSAOBiasMin, RDGStarterSSAOBiasMax), ECVF_SetByCode);
		}
	}

	void SetRDGStarterSSAOIntensity(const float NewIntensity)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(SSAOIntensityCVarName))
		{
			CVar->Set(FMath::Clamp(NewIntensity, RDGStarterSSAOIntensityMin, RDGStarterSSAOIntensityMax), ECVF_SetByCode);
		}
	}

	void SetRDGStarterAOCompositeStrength(const float NewStrength)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(AOCompositeStrengthCVarName))
		{
			CVar->Set(FMath::Clamp(NewStrength, RDGStarterAOCompositeStrengthMin, RDGStarterAOCompositeStrengthMax), ECVF_SetByCode);
		}
	}

	void SetRDGStarterAOCompositePower(const float NewPower)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(AOCompositePowerCVarName))
		{
			CVar->Set(FMath::Clamp(NewPower, RDGStarterAOCompositePowerMin, RDGStarterAOCompositePowerMax), ECVF_SetByCode);
		}
	}

	void SetRDGStarterAOCompositeMinVisibility(const float NewMinVisibility)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(AOCompositeMinVisibilityCVarName))
		{
			CVar->Set(
				FMath::Clamp(NewMinVisibility, RDGStarterAOCompositeMinVisibilityMin, RDGStarterAOCompositeMinVisibilityMax),
				ECVF_SetByCode);
		}
	}

	FText GetRDGStarterDebugModeText()
	{
		switch (GetRDGStarterDebugMode())
		{
		case 1:
			return FText::FromString(TEXT("Current Mode: Depth"));
		case 2:
			return FText::FromString(TEXT("Current Mode: Normal"));
		case 3:
			return FText::FromString(TEXT("Current Mode: SceneColor"));
		case 4:
			return FText::FromString(TEXT("Current Mode: Roughness"));
		case 5:
			return FText::FromString(TEXT("Current Mode: GaussianBlur"));
		case 6:
			return FText::FromString(TEXT("Current Mode: BilateralBlur"));
		case 7:
			return FText::FromString(TEXT("Current Mode: SSAO"));
		case 8:
			return FText::FromString(TEXT("Current Mode: RawSSAO"));
		case 9:
			return FText::FromString(TEXT("Current Mode: AOComposite"));
		default:
			return FText::FromString(TEXT("Current Mode: Disabled"));
		}
	}

	FText GetRDGStarterBlurSettingsText()
	{
		return FText::FromString(FString::Printf(
			TEXT("Radius: %d | Sigma: %.2f | Depth Sigma: %.3f | Normal Sigma: %.2f"),
			GetRDGStarterBlurRadius(),
			GetRDGStarterBlurSigma(),
			GetRDGStarterBilateralDepthSigma(),
			GetRDGStarterBilateralNormalSigma()));
	}

	FText GetRDGStarterSSAOSettingsText()
	{
		return FText::FromString(FString::Printf(
			TEXT("SSAO Samples: %d | Radius: %.0f | Bias: %.2f | Intensity: %.2f"),
			GetRDGStarterSSAOSampleCount(),
			GetRDGStarterSSAORadius(),
			GetRDGStarterSSAOBias(),
			GetRDGStarterSSAOIntensity()));
	}

	FText GetRDGStarterAOCompositeSettingsText()
	{
		return FText::FromString(FString::Printf(
			TEXT("AO Strength: %.2f | AO Power: %.2f | Min Visibility: %.2f"),
			GetRDGStarterAOCompositeStrength(),
			GetRDGStarterAOCompositePower(),
			GetRDGStarterAOCompositeMinVisibility()));
	}
}

void FRDGStarterModule::CreateSceneViewExtension()
{
	if (SceneViewExtension.IsValid())
	{
		return;
	}

	if (!GEngine || !GEngine->ViewExtensions)
	{
		UE_LOG(LogRDGStarter, Warning, TEXT("GEngine or ViewExtensions is not ready yet, skipping extension creation."));
		return;
	}

	// Keep the view extension alive for the module lifetime so UE can call it every frame.
	SceneViewExtension = FSceneViewExtensions::NewExtension<FRDGStarterSceneViewExtension>();
}

void FRDGStarterModule::CreateDebugWindow()
{
	if (DebugWindow.IsValid() || !GIsEditor || !FSlateApplication::IsInitialized())
	{
		return;
	}

	const auto MakeModeButton = [](const TCHAR* Label, const int32 Mode, const TCHAR* Tooltip)
	{
		return SNew(SButton)
			.ToolTipText(FText::FromString(Tooltip))
			.OnClicked_Lambda([Mode]()
			{
				SetRDGStarterDebugMode(Mode);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
			];
	};

	const auto MakeActionButton = [](const TCHAR* Label, const TCHAR* Tooltip, TFunction<void()> Action)
	{
		return SNew(SButton)
			.ToolTipText(FText::FromString(Tooltip))
			.OnClicked_Lambda([Action = MoveTemp(Action)]() mutable
			{
				Action();
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
			];
	};

	DebugWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("RDGStarter Debug")))
		.ClientSize(FVector2D(380.0f, 760.0f))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(true);

	DebugWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>&)
	{
		DebugWindow.Reset();
	}));

	DebugWindow->SetContent(
		SNew(SBorder)
		.Padding(12.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("RDGStarter Debug View Switcher")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("\u628A\u9F20\u6807\u505C\u5728\u6309\u94AE\u6216\u53C2\u6570\u4E0A\u53EF\u4EE5\u770B\u89E3\u91CA")))
				.ToolTipText(FText::FromString(TEXT(
					"\u6BCF\u4E2A mode \u548C\u53C2\u6570\u90FD\u6709 tooltip\u3002"
					"\u4F18\u5148\u770B\u201C\u8C03\u5927/\u8C03\u5C0F\u4F1A\u53D1\u751F\u4EC0\u4E48\u753B\u9762\u53D8\u5316\u201D\u3002")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([]()
				{
					return GetRDGStarterDebugModeText();
				})
				.ToolTipText(FText::FromString(TEXT(
					"DebugMode \u51B3\u5B9A\u8FD9\u4E00\u5E27 pass \u5230\u5E95\u8F93\u51FA\u4EC0\u4E48\u3002"
					" 0 \u662F\u5B8C\u5168\u5173\u95ED\uff0c7/8/9 \u662F AO \u94FE\u8DEF\u3002")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([]()
				{
					return GetRDGStarterBlurSettingsText();
				})
				.ToolTipText(FText::FromString(TEXT(
					"BlurRadius \u51B3\u5B9A\u770B\u591A\u8FDC\uff0cBlurSigma \u51B3\u5B9A\u8FDC\u5904\u6837\u672C\u8870\u51CF\u591A\u5FEB\u3002"
					" DepthSigma \u548C NormalSigma \u51B3\u5B9A\u8DE8\u8FB9\u754C\u65F6\u8FD8\u613F\u4E0D\u613F\u610F\u7EE7\u7EED\u6DF7\u8272\u3002")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([]()
				{
					return GetRDGStarterSSAOSettingsText();
				})
				.ToolTipText(FText::FromString(TEXT(
					"Samples \u51B3\u5B9A\u566A\u70B9\u591A\u5C11\uff0cRadius \u51B3\u5B9A\u906E\u853D\u8303\u56F4\uff0c"
					"Bias \u7528\u6765\u9632\u6B62\u81EA\u906E\u6321\uff0cIntensity \u51B3\u5B9A AO \u6700\u540E\u6709\u591A\u9ED1\u3002")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([]()
				{
					return GetRDGStarterAOCompositeSettingsText();
				})
				.ToolTipText(FText::FromString(TEXT(
					"Strength \u51B3\u5B9A AO \u53C2\u4E0E\u591A\u5C11\uff0cPower \u6539\u53D8 AO \u66F2\u7EBF\u5F62\u72B6\uff0c"
					"MinVisibility \u7ED9\u6700\u7EC8\u753B\u9762\u7559\u4E00\u4E2A\u6700\u4F4E\u4EAE\u5EA6\u5E95\u7EBF\u3002")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeModeButton(
					TEXT("0 - Disabled"),
					0,
					TEXT("\u5173\u95ED\u6240\u6709 RDGStarter \u53EF\u89C6\u5316 pass\u3002\u753B\u9762\u6062\u590D\u6B63\u5E38\uff0C\u9002\u5408\u548C\u5176\u4ED6 mode \u505A A/B \u5BF9\u6BD4\u3002"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeModeButton(
					TEXT("1 - Depth"),
					1,
					TEXT("\u628A\u6DF1\u5EA6\u5F53\u7070\u5EA6\u56FE\u770B\u3002\u9002\u5408\u786E\u8BA4\u54EA\u91CC\u8FD1\u3001\u54EA\u91CC\u8FDC\uff0C\u4EE5\u53CA\u6DF1\u5EA6\u91CD\u5EFA\u662F\u5426\u6B63\u5E38\u3002"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeModeButton(
					TEXT("2 - Normal"),
					2,
					TEXT("\u628A\u6CD5\u7EBF xyz \u6620\u5C04\u5230 RGB\u3002\u8FD9\u4E0D\u662F\u660E\u6697\uff0C\u800C\u662F\u8868\u9762\u671D\u5411\u7684\u53EF\u89C6\u5316\u3002"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeModeButton(
					TEXT("3 - SceneColor"),
					3,
					TEXT("\u76F4\u63A5\u900F\u4F20\u539F\u59CB SceneColor\u3002\u7528\u6765\u786E\u8BA4\u63D2\u4EF6\u94FE\u8DEF\u8FD8\u5728\uff0C\u4F46\u4E0D\u6539\u753B\u9762\u3002"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeModeButton(
					TEXT("4 - Roughness"),
					4,
					TEXT("\u628A roughness \u5F53\u7070\u5EA6\u56FE\u770B\u3002\u9ED1\u66F4\u504F\u5149\u6ED1\uff0C\u767D\u66F4\u504F\u7C97\u7CD9\u3002"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeModeButton(
					TEXT("5 - GaussianBlur"),
					5,
					TEXT("\u666E\u901A\u9AD8\u65AF\u6A21\u7CCA\uff0C\u53EA\u770B\u8DDD\u79BB\u4E0D\u770B\u51E0\u4F55\u8FB9\u754C\u3002"
						" \u9002\u5408\u770B\u201C\u6A21\u7CCA\u672C\u8EAB\u201D\uff0C\u4F46\u5BB9\u6613\u628A\u524D\u666F\u80CC\u666F\u4E00\u8D77\u7CCA\u6389\u3002"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeModeButton(
					TEXT("6 - BilateralBlur"),
					6,
					TEXT("\u53CC\u8FB9\u6A21\u7CCA\uff0C\u4F1A\u53C2\u8003\u6DF1\u5EA6\u548C\u6CD5\u7EBF\u3002"
						" \u5C3D\u91CF\u53EA\u5728\u540C\u4E00\u8868\u9762\u5185\u5E73\u6ED1\uff0C\u51CF\u5C11\u628A\u8F6E\u5ED3\u7CCA\u7A7F\u3002"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeModeButton(
					TEXT("7 - SSAO"),
					7,
					TEXT("\u5148\u7B97 raw AO\uff0C\u518D\u505A normal-aware bilateral blur\u3002"
						" \u8F93\u51FA\u7684\u662F\u964D\u566A\u540E\u7684 AO \u7070\u5EA6\u56FE\u3002"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeModeButton(
					TEXT("8 - RawSSAO"),
					8,
					TEXT("\u53EA\u770B\u539F\u59CB SSAO\uff0C\u4E0D\u505A\u4EFB\u4F55\u6A21\u7CCA\u3002\u566A\u70B9\u4F1A\u6BD4\u8F83\u91CD\uff0C\u4F46\u9002\u5408\u770B\u91C7\u6837\u539F\u59CB\u6548\u679C\u3002"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				MakeModeButton(
					TEXT("9 - AOComposite"),
					9,
					TEXT("\u628A\u6A21\u7CCA\u540E\u7684 AO \u4E58\u56DE SceneColor\u3002"
						" \u8FD9\u662F\u540E\u5904\u7406\u5F0F AO \u5408\u6210\uff0C\u4E0D\u662F\u5F15\u64CE lighting \u9636\u6BB5\u7684\u7269\u7406\u6B63\u786E\u96C6\u6210\u3002"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 12.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("Radius -"), TEXT(
						"\u6A21\u7CCA\u91C7\u6837\u534A\u5F84\u3002\u8C03\u5230 1 \u51E0\u4E4E\u770B\u4E0D\u51FA\u6A21\u7CCA\uff0C"
						" \u8C03\u5F97\u5F88\u5927\u4F1A\u53D8\u6210\u6574\u7247\u62B9\u5F00\uFF0C\u8FD8\u4F1A\u66F4\u6162\u3002"
						" \u5E38\u7528 3~8\u3002"), []()
					{
						SetRDGStarterBlurRadius(GetRDGStarterBlurRadius() - 1);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("Radius +"), TEXT(
						"\u6A21\u7CCA\u91C7\u6837\u534A\u5F84\u3002\u8C03\u5230 1 \u51E0\u4E4E\u770B\u4E0D\u51FA\u6A21\u7CCA\uff0C"
						" \u8C03\u5F97\u5F88\u5927\u4F1A\u53D8\u6210\u6574\u7247\u62B9\u5F00\uFF0C\u8FD8\u4F1A\u66F4\u6162\u3002"
						" \u5E38\u7528 3~8\u3002"), []()
					{
						SetRDGStarterBlurRadius(GetRDGStarterBlurRadius() + 1);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("Sigma -"), TEXT(
						"\u7A7A\u95F4\u9AD8\u65AF\u5BBD\u5EA6\u3002\u592A\u5C0F\u50CF\u53EA\u63C9\u4E2D\u5FC3\u51E0\u4E2A\u50CF\u7D20\uff0C"
						" \u592A\u5927\u50CF\u628A\u753B\u9762\u62B9\u5976\u6CB9\u3002\u5E38\u7528 1~3\u3002"), []()
					{
						SetRDGStarterBlurSigma(GetRDGStarterBlurSigma() - RDGStarterBlurSigmaStep);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("Sigma +"), TEXT(
						"\u7A7A\u95F4\u9AD8\u65AF\u5BBD\u5EA6\u3002\u592A\u5C0F\u50CF\u53EA\u63C9\u4E2D\u5FC3\u51E0\u4E2A\u50CF\u7D20\uff0C"
						" \u592A\u5927\u50CF\u628A\u753B\u9762\u62B9\u5976\u6CB9\u3002\u5E38\u7528 1~3\u3002"), []()
					{
						SetRDGStarterBlurSigma(GetRDGStarterBlurSigma() + RDGStarterBlurSigmaStep);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("DepthSigma -"), TEXT(
						"\u63A7\u5236\u8DE8\u6DF1\u5EA6\u65AD\u5C42\u65F6\u8FD8\u8981\u4E0D\u8981\u7EE7\u7EED\u5E73\u5747\u3002"
						" \u592A\u5C0F\u4F1A\u5F88\u4FDD\u8FB9\u4F46\u53EF\u80FD\u9897\u7C92\uFF0C\u592A\u5927\u4F1A\u628A\u524D\u540E\u7269\u4F53\u7CCA\u5728\u4E00\u8D77\u3002"
						" \u5E38\u7528 0.01~0.03\u3002"), []()
					{
						SetRDGStarterBilateralDepthSigma(
							GetRDGStarterBilateralDepthSigma() - RDGStarterBilateralDepthSigmaStep);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("DepthSigma +"), TEXT(
						"\u63A7\u5236\u8DE8\u6DF1\u5EA6\u65AD\u5C42\u65F6\u8FD8\u8981\u4E0D\u8981\u7EE7\u7EED\u5E73\u5747\u3002"
						" \u592A\u5C0F\u4F1A\u5F88\u4FDD\u8FB9\u4F46\u53EF\u80FD\u9897\u7C92\uFF0C\u592A\u5927\u4F1A\u628A\u524D\u540E\u7269\u4F53\u7CCA\u5728\u4E00\u8D77\u3002"
						" \u5E38\u7528 0.01~0.03\u3002"), []()
					{
						SetRDGStarterBilateralDepthSigma(
							GetRDGStarterBilateralDepthSigma() + RDGStarterBilateralDepthSigmaStep);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("NormalSigma -"), TEXT(
						"\u63A7\u5236\u6CD5\u7EBF\u5DEE\u591A\u5927\u8FD8\u7B97\u540C\u4E00\u8868\u9762\u3002"
						" \u592A\u5C0F\u6298\u89D2\u5F88\u786C\u4F46\u53EF\u80FD\u4E0D\u591F\u5E73\u6ED1\uff0C\u592A\u5927\u8F6C\u89D2\u4F1A\u88AB\u7CCA\u6389\u3002"
						" \u5E38\u7528 0.15~0.35\u3002"), []()
					{
						SetRDGStarterBilateralNormalSigma(
							GetRDGStarterBilateralNormalSigma() - RDGStarterBilateralNormalSigmaStep);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("NormalSigma +"), TEXT(
						"\u63A7\u5236\u6CD5\u7EBF\u5DEE\u591A\u5927\u8FD8\u7B97\u540C\u4E00\u8868\u9762\u3002"
						" \u592A\u5C0F\u6298\u89D2\u5F88\u786C\u4F46\u53EF\u80FD\u4E0D\u591F\u5E73\u6ED1\uff0C\u592A\u5927\u8F6C\u89D2\u4F1A\u88AB\u7CCA\u6389\u3002"
						" \u5E38\u7528 0.15~0.35\u3002"), []()
					{
						SetRDGStarterBilateralNormalSigma(
							GetRDGStarterBilateralNormalSigma() + RDGStarterBilateralNormalSigmaStep);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 12.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("Samples -"), TEXT(
						"\u6BCF\u4E2A\u50CF\u7D20\u53D1\u51FA\u591A\u5C11\u6761 AO \u63A2\u9488\u3002"
						" \u592A\u5C11\u4F1A\u50CF\u6492\u4E86\u9897\u7C92\uff0C\u592A\u591A\u4F1A\u66F4\u7A33\u4F46\u66F4\u6162\u3002\u5E38\u7528 8~16\u3002"), []()
					{
						SetRDGStarterSSAOSampleCount(GetRDGStarterSSAOSampleCount() - 1);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("Samples +"), TEXT(
						"\u6BCF\u4E2A\u50CF\u7D20\u53D1\u51FA\u591A\u5C11\u6761 AO \u63A2\u9488\u3002"
						" \u592A\u5C11\u4F1A\u50CF\u6492\u4E86\u9897\u7C92\uff0C\u592A\u591A\u4F1A\u66F4\u7A33\u4F46\u66F4\u6162\u3002\u5E38\u7528 8~16\u3002"), []()
					{
						SetRDGStarterSSAOSampleCount(GetRDGStarterSSAOSampleCount() + 1);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("AO Radius -"), TEXT(
						"\u63A7\u5236 AO \u63A2\u6D4B\u591A\u8FDC\u3002"
						" \u592A\u5C0F\u53EA\u6709\u7F1D\u9689\u548C\u63A5\u89E6\u8FB9\u53D1\u9ED1\uff0C\u592A\u5927\u4F1A\u50CF\u7070\u96FE\u8499\u5728\u7269\u4F53\u4E0A\u3002"
						" \u5E38\u7528 40~120\u3002"), []()
					{
						SetRDGStarterSSAORadius(GetRDGStarterSSAORadius() - RDGStarterSSAORadiusStep);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("AO Radius +"), TEXT(
						"\u63A7\u5236 AO \u63A2\u6D4B\u591A\u8FDC\u3002"
						" \u592A\u5C0F\u53EA\u6709\u7F1D\u9689\u548C\u63A5\u89E6\u8FB9\u53D1\u9ED1\uff0C\u592A\u5927\u4F1A\u50CF\u7070\u96FE\u8499\u5728\u7269\u4F53\u4E0A\u3002"
						" \u5E38\u7528 40~120\u3002"), []()
					{
						SetRDGStarterSSAORadius(GetRDGStarterSSAORadius() + RDGStarterSSAORadiusStep);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("AO Bias -"), TEXT(
						"\u7528\u6765\u9632\u6B62\u201C\u81EA\u5DF1\u6321\u81EA\u5DF1\u201D\u3002"
						" \u8C03\u5230 0 \u9644\u8FD1\u5F88\u5BB9\u6613\u6EE1\u8868\u9762\u53D1\u810F\uff0C\u592A\u5927\u53C8\u4F1A\u628A\u771F\u6B63\u7684\u63A5\u89E6\u9634\u5F71\u6F0F\u6389\u3002"
						" \u5E38\u7528 2~5\u3002"), []()
					{
						SetRDGStarterSSAOBias(GetRDGStarterSSAOBias() - RDGStarterSSAOBiasStep);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("AO Bias +"), TEXT(
						"\u7528\u6765\u9632\u6B62\u201C\u81EA\u5DF1\u6321\u81EA\u5DF1\u201D\u3002"
						" \u8C03\u5230 0 \u9644\u8FD1\u5F88\u5BB9\u6613\u6EE1\u8868\u9762\u53D1\u810F\uff0C\u592A\u5927\u53C8\u4F1A\u628A\u771F\u6B63\u7684\u63A5\u89E6\u9634\u5F71\u6F0F\u6389\u3002"
						" \u5E38\u7528 2~5\u3002"), []()
					{
						SetRDGStarterSSAOBias(GetRDGStarterSSAOBias() + RDGStarterSSAOBiasStep);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("AO Intensity -"), TEXT(
						"\u539F\u59CB AO \u5F3A\u5EA6\u30020 \u57FA\u672C\u770B\u4E0D\u89C1\uff0C\u62C9\u5F97\u8FC7\u5927\u6574\u5F20\u56FE\u4F1A\u88AB\u538B\u9ED1\u3002"
						" \u5E38\u7528 1.0~1.5\u3002"), []()
					{
						SetRDGStarterSSAOIntensity(GetRDGStarterSSAOIntensity() - RDGStarterSSAOIntensityStep);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("AO Intensity +"), TEXT(
						"\u539F\u59CB AO \u5F3A\u5EA6\u30020 \u57FA\u672C\u770B\u4E0D\u89C1\uff0C\u62C9\u5F97\u8FC7\u5927\u6574\u5F20\u56FE\u4F1A\u88AB\u538B\u9ED1\u3002"
						" \u5E38\u7528 1.0~1.5\u3002"), []()
					{
						SetRDGStarterSSAOIntensity(GetRDGStarterSSAOIntensity() + RDGStarterSSAOIntensityStep);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 12.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("AO Strength -"), TEXT(
						"\u5408\u6210\u65F6\u8BA9 AO \u53C2\u4E0E\u591A\u5C11\u30020 \u7B49\u4E8E\u4E0D\u5408\u6210\uff0C2 \u9644\u8FD1\u4F1A\u5F88\u91CD\u3002"
						" \u5E38\u7528 0.6~1.2\u3002"), []()
					{
						SetRDGStarterAOCompositeStrength(
							GetRDGStarterAOCompositeStrength() - RDGStarterAOCompositeStrengthStep);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("AO Strength +"), TEXT(
						"\u5408\u6210\u65F6\u8BA9 AO \u53C2\u4E0E\u591A\u5C11\u30020 \u7B49\u4E8E\u4E0D\u5408\u6210\uff0C2 \u9644\u8FD1\u4F1A\u5F88\u91CD\u3002"
						" \u5E38\u7528 0.6~1.2\u3002"), []()
					{
						SetRDGStarterAOCompositeStrength(
							GetRDGStarterAOCompositeStrength() + RDGStarterAOCompositeStrengthStep);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("AO Power -"), TEXT(
						"\u6539\u53D8 AO \u66F2\u7EBF\u3002\u5C0F\u4E8E 1 \u4F1A\u66F4\u67D4\uff0C\u5927\u4E8E 1 \u4F1A\u8BA9\u6697\u5904\u66F4\u91CD\u3002"
						" \u592A\u5927\u5BB9\u6613\u6B7B\u9ED1\u3002\u5E38\u7528 0.8~1.4\u3002"), []()
					{
						SetRDGStarterAOCompositePower(
							GetRDGStarterAOCompositePower() - RDGStarterAOCompositePowerStep);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("AO Power +"), TEXT(
						"\u6539\u53D8 AO \u66F2\u7EBF\u3002\u5C0F\u4E8E 1 \u4F1A\u66F4\u67D4\uff0C\u5927\u4E8E 1 \u4F1A\u8BA9\u6697\u5904\u66F4\u91CD\u3002"
						" \u592A\u5927\u5BB9\u6613\u6B7B\u9ED1\u3002\u5E38\u7528 0.8~1.4\u3002"), []()
					{
						SetRDGStarterAOCompositePower(
							GetRDGStarterAOCompositePower() + RDGStarterAOCompositePowerStep);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					MakeActionButton(TEXT("MinVisibility -"), TEXT(
						"\u7ED9\u6700\u7EC8\u753B\u9762\u8BBE\u4E00\u4E2A\u6700\u4F4E\u4EAE\u5EA6\u5E95\u7EBF\u3002"
						" 0 \u662F\u4E0D\u8BBE\u4FDD\u62A4\uff0C1 \u7B49\u4E8E\u5B8C\u5168\u770B\u4E0D\u89C1 AO\u3002"
						" \u5E38\u7528 0.05~0.25\u3002"), []()
					{
						SetRDGStarterAOCompositeMinVisibility(
							GetRDGStarterAOCompositeMinVisibility() - RDGStarterAOCompositeMinVisibilityStep);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeActionButton(TEXT("MinVisibility +"), TEXT(
						"\u7ED9\u6700\u7EC8\u753B\u9762\u8BBE\u4E00\u4E2A\u6700\u4F4E\u4EAE\u5EA6\u5E95\u7EBF\u3002"
						" 0 \u662F\u4E0D\u8BBE\u4FDD\u62A4\uff0C1 \u7B49\u4E8E\u5B8C\u5168\u770B\u4E0D\u89C1 AO\u3002"
						" \u5E38\u7528 0.05~0.25\u3002"), []()
					{
						SetRDGStarterAOCompositeMinVisibility(
							GetRDGStarterAOCompositeMinVisibility() + RDGStarterAOCompositeMinVisibilityStep);
					})
				]
			]
		]);

	FSlateApplication::Get().AddWindow(DebugWindow.ToSharedRef());
}

void FRDGStarterModule::DestroyDebugWindow()
{
	if (DebugWindow.IsValid() && FSlateApplication::IsInitialized())
	{
		DebugWindow->RequestDestroyWindow();
	}

	DebugWindow.Reset();
}

void FRDGStarterModule::CreateRuntimeTools()
{
	CreateSceneViewExtension();
	CreateDebugWindow();
}

void FRDGStarterModule::StartupModule()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("RDGStarter"));
	if (!Plugin.IsValid())
	{
		UE_LOG(LogRDGStarter, Error, TEXT("RDGStarter plugin descriptor was not found."));
		return;
	}

	// Register the plugin shader directory so IMPLEMENT_GLOBAL_SHADER can find the .usf files.
	const FString PluginShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/RDGStarter"), PluginShaderDir);

	if (GEngine && GEngine->ViewExtensions)
	{
		CreateRuntimeTools();
	}
	else
	{
		PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FRDGStarterModule::CreateRuntimeTools);
	}
}

void FRDGStarterModule::ShutdownModule()
{
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}

	SceneViewExtension.Reset();
	DestroyDebugWindow();
}

IMPLEMENT_MODULE(FRDGStarterModule, RDGStarter);
