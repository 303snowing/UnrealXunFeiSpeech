// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "SpeechActor.h"

#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"



// Sets default values
ASpeechActor::ASpeechActor() :
	Result{}
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ASpeechActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ASpeechActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASpeechActor::SpeechInit()
{
	FAutoDeleteAsyncTask<FSpeechTask>* SpeechTask = new FAutoDeleteAsyncTask<FSpeechTask>();

	if (SpeechTask)
	{
		SpeechTask->StartBackgroundTask();
	}
	else
	{
		UE_LOG(SnowingError, Error, TEXT("XunFei task object could not be create !"));
		return;
	}

	UE_LOG(SnowingWarning, Warning, TEXT("XunFei Task Stopped !"));
	return;
}

void ASpeechActor::SpeechOpen()
{
	xunfeispeech->SetStart();
	return;
}

void ASpeechActor::SpeechStop()
{
	xunfeispeech->SetStop();
	return;
}

FString ASpeechActor::SpeechResult()
{	
	Result = FString(UTF8_TO_TCHAR(xunfeispeech->get_result()));
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef< TJsonReader<TCHAR> > Reader = TJsonReaderFactory<TCHAR>::Create(Result);
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		Result.Reset();
		TArray< TSharedPtr<FJsonValue> > TempArray = JsonObject->GetArrayField("ws");
		for (auto rs : TempArray)
		{
			Result.Append((rs->AsObject()->GetArrayField("cw"))[0]->AsObject()->GetStringField("w"));
		}
	}
	UE_LOG(SnowingError, Error, TEXT("%s"), *Result);
	
	return Result;
	
}

