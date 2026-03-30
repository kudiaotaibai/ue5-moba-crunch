// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include"AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "ValueGauge.generated.h"

/**
 * 
 */
UCLASS()
class CRUNCH_API UValueGauge : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	 void SetValue(float NewValue,float NewMaxValue);
	void SetAndBoundToGameplayAttribute(class UAbilitySystemComponent* AbilitySystemComponent,const FGameplayAttribute& Attribute,const FGameplayAttribute& MaxAttribute);
	
private:

	void ValueChanged(const FOnAttributeChangeData& ChangedData);
	void MaxValueChanged(const FOnAttributeChangeData& ChangedData);
	
	UPROPERTY(EditAnywhere,Category="Value")
	FLinearColor BarColor;

	UPROPERTY(EditAnywhere,Category="Value")
	FSlateFontInfo ValueTextFont;

	UPROPERTY(EditAnywhere,Category="Value")
	bool bValueTextVisible=true;

	UPROPERTY(EditAnywhere,Category="Value")
	bool bProgressBarVisible=true;
	
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	class UProgressBar* ProgressBar;

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	class UTextBlock* ValueText;

	float CachedValue;
	float CachedMaxValue;
};
