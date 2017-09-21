#pragma once


#include <conio.h>

#include "msp_cmn.h"
#include "msp_errors.h"
#include "SpeechRecoginzer.h"

/*
* 语音听写(iFly Auto Transform)技术能够实时地将语音转换成对应的文字。
*/
#define FRAME_LEN	640
#define BUFFER_SIZE	4096
// 识别状态类型
enum {
	EVT_START = 0,
	EVT_STOP,
	EVT_QUIT,
	EVT_TOTAL
};

DECLARE_LOG_CATEGORY_EXTERN(XunFeiSpeech, Warning, All);

class FXunFeiSpeech
{

public:
	struct speech_rec iat;

public:

	FXunFeiSpeech();
	FXunFeiSpeech(FString);

	void SetStart();
	void SetStop();
	void SetQuit();
	void speech_mic(const char* session_beging_params);
	const char* get_result() const;

};

static FXunFeiSpeech * xunfeispeech = new FXunFeiSpeech(FString("static xunfeispeech be created !"));
