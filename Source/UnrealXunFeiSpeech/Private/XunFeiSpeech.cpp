#pragma once

#include "XunFeiSpeech.h"

static char *g_result = NULL;
static unsigned int g_buffersize = BUFFER_SIZE;
static HANDLE events[EVT_TOTAL] = { NULL,NULL,NULL };	// 识别状态定义

static void on_result(const char *result, char is_last);
static void on_speech_begin();
static void on_speech_end(int reason);

DEFINE_LOG_CATEGORY(XunFeiSpeech);

// static functions
void on_result(const char *result, char is_last)
{
	if (result) {
		size_t left = g_buffersize - 1 - strlen(g_result);
		size_t size = strlen(result);
		if (left < size) {
			g_result = (char*)realloc(g_result, g_buffersize + BUFFER_SIZE);
			if (g_result)
				g_buffersize += BUFFER_SIZE;
			else {
				printf("mem alloc failed\n");
				return;
			}
		}
		strncat(g_result, result, size);
		//UE_LOG(SnowingWarning, Warning, TEXT("%s"), *FString(g_result));
		//show_result(g_result, is_last);
	}
}

void on_speech_begin()
{
	if (g_result)
	{
		free(g_result);
	}
	g_result = (char*)malloc(BUFFER_SIZE);
	g_buffersize = BUFFER_SIZE;
	memset(g_result, 0, g_buffersize);

	UE_LOG(SnowingWarning, Warning, TEXT("Start Listening...\n"));
}

void on_speech_end(int reason)
{
	if (reason == END_REASON_VAD_DETECT)
	{
		UE_LOG(SnowingWarning, Warning, TEXT("\nSpeaking done \n"));
	}
	else
	{
		UE_LOG(SnowingError, Error, TEXT("\nRecognizer error %d\n"), reason);
	}
}


// public functions

FXunFeiSpeech::FXunFeiSpeech()
{

}

FXunFeiSpeech::FXunFeiSpeech(FString Log)
{
	UE_LOG(XunFeiSpeech, Warning, TEXT("%s"), *Log);
}

void FXunFeiSpeech::SetStart()
{
	SetEvent(events[EVT_START]);

	return;
}

void FXunFeiSpeech::SetStop()
{
	SetEvent(events[EVT_STOP]);

	return;
}

void FXunFeiSpeech::SetQuit()
{
	SetEvent(events[EVT_QUIT]);

	return;
}

void FXunFeiSpeech::speech_mic(const char* session_beging_params)
{
	int errcode;
	int i = 0;

	//struct speech_rec iat;
	DWORD waiters;
	char isquit = 0;

	struct speech_rec_notifier recnotifier = {
		on_result,
		on_speech_begin,
		on_speech_end
	};

	errcode = speechrecoginzer->sr_init(&iat, session_beging_params, SR_MIC, DEFAULT_INPUT_DEVID, &recnotifier);	// 初始化识别参数
	if (errcode)
	{
		UE_LOG(SnowingError, Error, TEXT("speech recognizer init failed\n"));
		return;
	}
	// 创建识别事件状态
	for (i = 0; i < EVT_TOTAL; ++i) {
		events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	// 监听识别状态
	while (1) {
		waiters = WaitForMultipleObjects(EVT_TOTAL, events, FALSE, INFINITE);
		switch (waiters) {
		case WAIT_FAILED:
		case WAIT_TIMEOUT:
			UE_LOG(SnowingWarning, Warning, TEXT("Why it happened !?\n"));
			break;
		case WAIT_OBJECT_0 + EVT_START:
			errcode = speechrecoginzer->sr_start_listening(&iat);
			if (errcode) {
				UE_LOG(SnowingError, Error, TEXT("start listen failed %d\n"), errcode);
				isquit = 1;
			}
			break;
		case WAIT_OBJECT_0 + EVT_STOP:
			errcode = speechrecoginzer->sr_stop_listening(&iat);
			if (errcode) {
				UE_LOG(SnowingError, Error, TEXT("stop listening failed %d\n"), errcode);
				isquit = 1;
			}
			break;
		case WAIT_OBJECT_0 + EVT_QUIT:
			speechrecoginzer->sr_stop_listening(&iat);
			isquit = 1;
			break;
		default:
			break;
		}
		if (isquit)
			break;
	}

	for (i = 0; i < EVT_TOTAL; ++i) {
		if (events[i])
			CloseHandle(events[i]);
	}

	speechrecoginzer->sr_uninit(&iat);
}

const char* FXunFeiSpeech::get_result() const
{
	return g_result;
}
