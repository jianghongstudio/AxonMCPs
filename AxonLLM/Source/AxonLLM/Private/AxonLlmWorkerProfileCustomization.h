#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Widgets/Input/SComboBox.h"

class IPropertyHandle;
class SWidget;

class FAxonLlmWorkerProfileCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	TSharedPtr<IPropertyHandle> BaseUrlHandle;
	TSharedPtr<IPropertyHandle> ModelHandle;

	TArray<TSharedPtr<FString>> ModelOptions;
	TSharedPtr<FString> CurrentModelItem;
	FString LastFetchError;

	TWeakPtr<SComboBox<TSharedPtr<FString>>> ModelComboWeak;

	void RefreshModelsFromServer();
	void NotifyComboOptionsChanged();
	FString GetBaseUrl() const;
	FString GetModel() const;
	void SetModel(const FString& NewModel);

	TSharedRef<SWidget> BuildModelRow();
};
