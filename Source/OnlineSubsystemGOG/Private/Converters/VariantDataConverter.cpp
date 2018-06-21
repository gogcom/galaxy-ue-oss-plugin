#include "VariantDataConverter.h"

#include "JsonSerializer.h"
#include "OnlineKeyValuePair.h"
#include "OnlineSubsystem.h"

#include <array>
#include "LogMacros.h"


namespace VariantDataConverter
{

	FLobbyDataEntry ToLobbyDataEntry(const FVariantData& InData)
	{
		if (InData.GetType() == EOnlineKeyValuePairDataType::Empty)
		{
			UE_LOG_ONLINE(VeryVerbose, TEXT("Serializing empty data"));
			return{};
		}

		FLobbyDataEntry convertedData;
		auto jsonWriter = TJsonWriterFactory<>::Create(&convertedData);
		if (!FJsonSerializer::Serialize(InData.ToJson(), jsonWriter))
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to serialize VariantData"));
			return{};
		}

		return convertedData;
	}

	FVariantData FromLobbyDataEntry(const FLobbyDataEntry& InDataEntry)
	{
		if (InDataEntry.IsEmpty())
		{
			UE_LOG_ONLINE(VeryVerbose, TEXT("De-serializing empty data."));
			return{};
		}

		TSharedPtr<FJsonObject> jsonObject;
		auto jsonReader = TJsonReaderFactory<>::Create(InDataEntry);
		FVariantData convertedData;
		if (!FJsonSerializer::Deserialize(jsonReader, jsonObject)
			|| !jsonObject.IsValid()
			|| !convertedData.FromJson(jsonObject.ToSharedRef()))
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to de-serialize data."));
			return{};
		}

		return convertedData;
	}
}
