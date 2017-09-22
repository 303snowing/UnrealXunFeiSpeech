// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SpeechTask.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpeechActor.generated.h"

UCLASS()
class UNREALXUNFEISPEECH_API ASpeechActor : public AActor
{
	GENERATED_BODY()
private:
	FString Result;

public:	
	// Sets default values for this actor's properties
	ASpeechActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "XunFei", meta = (DisplayName = "SpeechInit", Keywords = "Speech Recognition Initialization"))
		void SpeechInit();

	UFUNCTION(BlueprintCallable, Category = "XunFei", meta = (DisplayName = "SpeechOpen", Keywords = "Speech Recognition Open"))
		void SpeechOpen();

	UFUNCTION(BlueprintCallable, Category = "XunFei", meta = (DisplayName = "SpeechStop", Keywords = "Speech Recognition Stop"))
		void SpeechStop();

	UFUNCTION(BlueprintCallable, Category = "XunFei", meta = (DisplayName = "SpeechQuit", Keywords = "Speech Recognition Quit"))
		void SpeechQuit();

	UFUNCTION(BlueprintCallable, Category = "XunFei", meta = (DisplayName = "SpeechResult", Keywords = "Speech Recognition GetResult"))
		FString SpeechResult();
};
