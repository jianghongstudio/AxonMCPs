#include "AxonLlmWorkerProfileCustomization.h"
#include "AxonLlmClient.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "AxonLlmWorkerProfileCustomization"

TSharedRef<IPropertyTypeCustomization> FAxonLlmWorkerProfileCustomization::MakeInstance()
{
	return MakeShareable(new FAxonLlmWorkerProfileCustomization);
}

void FAxonLlmWorkerProfileCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*StructCustomizationUtils*/)
{
	HeaderRow.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()];
	HeaderRow.ValueContent()[StructPropertyHandle->CreatePropertyValueWidget()];
}

void FAxonLlmWorkerProfileCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& /*StructCustomizationUtils*/)
{
	BaseUrlHandle = StructPropertyHandle->GetChildHandle(TEXT("BaseUrl"));
	ModelHandle = StructPropertyHandle->GetChildHandle(TEXT("Model"));

	if (BaseUrlHandle.IsValid())
	{
		BaseUrlHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FAxonLlmWorkerProfileCustomization::RefreshModelsFromServer));
	}

	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);
	for (uint32 i = 0; i < NumChildren; ++i)
	{
		TSharedPtr<IPropertyHandle> Child = StructPropertyHandle->GetChildHandle(i);
		if (!Child.IsValid())
		{
			continue;
		}
		const FName PropName = Child->GetProperty() ? Child->GetProperty()->GetFName() : NAME_None;
		if (PropName == TEXT("Model"))
		{
			StructBuilder.AddCustomRow(LOCTEXT("ModelRow", "Model"))
				.NameContent()
				[
					Child->CreatePropertyNameWidget()
				]
				.ValueContent()
				.MinDesiredWidth(360.f)
				[
					BuildModelRow()
				];
		}
		else
		{
			StructBuilder.AddProperty(Child.ToSharedRef());
		}
	}

	RefreshModelsFromServer();
}

FString FAxonLlmWorkerProfileCustomization::GetBaseUrl() const
{
	FString Url;
	if (BaseUrlHandle.IsValid())
	{
		BaseUrlHandle->GetValue(Url);
	}
	return Url.TrimStartAndEnd();
}

FString FAxonLlmWorkerProfileCustomization::GetModel() const
{
	FString Model;
	if (ModelHandle.IsValid())
	{
		ModelHandle->GetValue(Model);
	}
	return Model;
}

void FAxonLlmWorkerProfileCustomization::SetModel(const FString& NewModel)
{
	if (ModelHandle.IsValid())
	{
		ModelHandle->SetValue(NewModel);
	}
}

void FAxonLlmWorkerProfileCustomization::NotifyComboOptionsChanged()
{
	if (TSharedPtr<SComboBox<TSharedPtr<FString>>> Combo = ModelComboWeak.Pin())
	{
		Combo->RefreshOptions();
		Combo->SetSelectedItem(CurrentModelItem);
	}
}

void FAxonLlmWorkerProfileCustomization::RefreshModelsFromServer()
{
	ModelOptions.Reset();
	LastFetchError.Reset();

	const FString Url = GetBaseUrl();
	const FString Current = GetModel();
	if (Url.IsEmpty())
	{
		LastFetchError = TEXT("请先填写 Base Url");
		if (!Current.IsEmpty())
		{
			ModelOptions.Add(MakeShared<FString>(Current));
			CurrentModelItem = ModelOptions[0];
		}
		NotifyComboOptionsChanged();
		return;
	}

	const FAxonLlmTagsResult Tags = FAxonLlmClient::GetTags(Url, 5);
	if (!Tags.bOk)
	{
		LastFetchError = Tags.Error;
		if (!Current.IsEmpty())
		{
			ModelOptions.Add(MakeShared<FString>(Current));
			CurrentModelItem = ModelOptions[0];
		}
		NotifyComboOptionsChanged();
		return;
	}

	for (const FString& Name : Tags.Models)
	{
		ModelOptions.Add(MakeShared<FString>(Name));
	}
	if (!Current.IsEmpty() && !Tags.Models.Contains(Current))
	{
		ModelOptions.Insert(MakeShared<FString>(Current), 0);
	}

	CurrentModelItem.Reset();
	for (const TSharedPtr<FString>& Opt : ModelOptions)
	{
		if (Opt.IsValid() && *Opt == Current)
		{
			CurrentModelItem = Opt;
			break;
		}
	}
	if (!CurrentModelItem.IsValid() && ModelOptions.Num() > 0)
	{
		CurrentModelItem = ModelOptions[0];
		SetModel(*CurrentModelItem);
	}

	NotifyComboOptionsChanged();
}

TSharedRef<SWidget> FAxonLlmWorkerProfileCustomization::BuildModelRow()
{
	TSharedRef<SComboBox<TSharedPtr<FString>>> Combo =
		SNew(SComboBox<TSharedPtr<FString>>)
		.OptionsSource(&ModelOptions)
		.InitiallySelectedItem(CurrentModelItem)
		.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
		{
			return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
		})
		.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
		{
			if (Item.IsValid())
			{
				CurrentModelItem = Item;
				SetModel(*Item);
			}
		})
		.Content()
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				return FText::FromString(GetModel());
			})
			.ToolTipText_Lambda([this]()
			{
				return LastFetchError.IsEmpty()
					? FText::FromString(TEXT("从 Base Url 的 Ollama /api/tags 选择模型"))
					: FText::FromString(LastFetchError);
			})
		];

	ModelComboWeak = Combo;

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		[
			Combo
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(6.f, 0.f, 0.f, 0.f)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(LOCTEXT("RefreshModels", "刷新模型"))
			.ToolTipText(LOCTEXT("RefreshModelsTip", "从当前 Base Url 重新拉取模型列表"))
			.OnClicked_Lambda([this]()
			{
				RefreshModelsFromServer();
				return FReply::Handled();
			})
		];
}

#undef LOCTEXT_NAMESPACE
