// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "SpeechActor.h"


// Sets default values
ASpeechActor::ASpeechActor()
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

FString ASpeechActor::SpeechResult()
{
	xunfeispeech->SetStop();
	Sleep(300);
	UE_LOG(SnowingError, Error, TEXT("%s"), *FString(UTF8_TO_TCHAR(xunfeispeech->get_result())));
	return TEXT("speech result !");
}

