#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Delegates/Delegate.h"
#include "Templates/SharedPointer.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRDGStarter, Log, All);

class FRDGStarterSceneViewExtension;
class SWindow;

class RDGSTARTER_API FRDGStarterModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void CreateRuntimeTools();
	void CreateSceneViewExtension();
	void CreateDebugWindow();
	void DestroyDebugWindow();

	FDelegateHandle PostEngineInitHandle;
	TSharedPtr<FRDGStarterSceneViewExtension, ESPMode::ThreadSafe> SceneViewExtension;
	TSharedPtr<SWindow> DebugWindow;
};
