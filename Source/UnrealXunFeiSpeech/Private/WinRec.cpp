#pragma once
#pragma comment(lib, "winmm.lib")

#include "WinRec.h"

/* Do not change the sequence */
enum {
	RECORD_STATE_CREATED,	/* Init		*/
	RECORD_STATE_READY,		/* Opened	*/
	RECORD_STATE_STOPPING,	/* During Stop	*/
	RECORD_STATE_RECORDING,	/* Started	*/
};

#define SAMPLE_RATE  16000
#define SAMPLE_BIT_SIZE 16
#define FRAME_CNT   10
#define BUF_COUNT   4

static HANDLE msgqueue_ready_evt = NULL; /* signaled: the message queque has been created in the thread */
/* the recording callback thread procedure */
static unsigned int  __stdcall record_thread_proc(void * para);

DEFINE_LOG_CATEGORY(WinRec);

FWinRec::FWinRec(FString Log)
{
	UE_LOG(WinRec, Warning, TEXT("%s"), *Log);
}

FWinRec::~FWinRec()
{
}

// public functions
int FWinRec::get_default_input_dev()
{
	return WAVE_MAPPER;
}

unsigned int FWinRec::get_input_dev_num()
{
	return waveInGetNumDevs();
}

int FWinRec::create_recorder(struct recorder ** out_rec, void(*on_data_ind)(char *data, unsigned long len, void *user_para), void* user_cb_para)
{
	struct recorder * myrec;
	myrec = (struct recorder *)malloc(sizeof(struct recorder));
	if (!myrec)
		return -RECORD_ERR_MEMFAIL;

	memset(myrec, 0, sizeof(struct recorder));
	myrec->on_data_ind = on_data_ind;
	myrec->user_cb_para = user_cb_para;
	myrec->state = RECORD_STATE_CREATED;

	*out_rec = myrec;
	return 0;
}

void FWinRec::destroy_recorder(struct recorder *rec)
{
	if (!rec)
		return;

	free(rec);
}

int FWinRec::open_recorder(struct recorder * rec, unsigned int dev, WAVEFORMATEX * fmt)
{
	int ret = 0;
	if (!rec)
		return -RECORD_ERR_INVAL;
	if (rec->state >= RECORD_STATE_READY)
		return 0;

	ret = open_recorder_internal(rec, dev, fmt);
	if (ret == 0)
		rec->state = RECORD_STATE_READY;
	return 0;
}

void FWinRec::close_recorder(struct recorder *rec)
{
	if (rec == NULL || rec->state < RECORD_STATE_READY)
		return;
	if (rec->state == RECORD_STATE_RECORDING)
		stop_record(rec);

	close_recorder_internal(rec);

	rec->state = RECORD_STATE_CREATED;
}

int FWinRec::start_record(struct recorder * rec)
{
	int ret;
	if (rec == NULL)
		return -RECORD_ERR_INVAL;
	if (rec->state < RECORD_STATE_READY)
		return -RECORD_ERR_NOT_READY;
	if (rec->state == RECORD_STATE_RECORDING)
		return 0;

	ret = start_record_internal((HWAVEIN)rec->wavein_hdl, (WAVEHDR*)rec->bufheader, rec->bufcount);
	if (ret == 0)
		rec->state = RECORD_STATE_RECORDING;
	return ret;
}

int FWinRec::stop_record(struct recorder * rec)
{
	int ret;
	if (rec == NULL)
		return -RECORD_ERR_INVAL;
	if (rec->state < RECORD_STATE_RECORDING)
		return 0;

	rec->state = RECORD_STATE_STOPPING;
	ret = stop_record_internal((HWAVEIN)rec->wavein_hdl);
	if (ret == 0) {
		rec->state = RECORD_STATE_READY;
	}
	return ret;
}

int FWinRec::is_record_stopped(struct recorder *rec)
{
	if (rec->state == RECORD_STATE_RECORDING)
		return 0;

	return is_stopped_internal(rec);
}

// private functions
void FWinRec::dbg_wave_header(WAVEHDR * buf)
{
	UE_LOG(SnowingWarning, Warning, TEXT("-----\n"));
	UE_LOG(SnowingWarning, Warning, TEXT("Buf %x: User= %d, Len=%d, Rec = %d, Flag = %x\n"), buf,
		buf->dwUser, buf->dwBufferLength, buf->dwBytesRecorded,
		buf->dwFlags);
	UE_LOG(SnowingWarning, Warning, TEXT("-----\n"));
}
/*
* If format is NULL, the default 16K sample rate, 16 bit will be used
* bufheader
*/
int FWinRec::create_callback_thread(void *thread_proc_para, HANDLE *thread_hdl_out)
{
	int ret = 0;

	HANDLE rec_thread_hdl = 0;
	unsigned int rec_thread_id;

	/* For indicating the thread's life stage.
	. signaled after the call back thread started and create its message
	queue.
	. will be close after callback thread exit.
	. must be manual reset. once signaled, keep in signaled state */
	msgqueue_ready_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (msgqueue_ready_evt == NULL)
		return -1;

	rec_thread_hdl = (HANDLE)_beginthreadex(NULL, 0, record_thread_proc, thread_proc_para, 0, &rec_thread_id);
	if (rec_thread_hdl == 0) {
		/* close the event handle */
		CloseHandle(msgqueue_ready_evt);
		msgqueue_ready_evt = NULL;

		return -1;
	}


	*thread_hdl_out = rec_thread_hdl;

	/* wait the message queue of the new thread has been created */
	WaitForSingleObject(msgqueue_ready_evt, INFINITE);

	return 0;
}

void FWinRec::close_callback_thread(HANDLE thread)
{
	if (thread == NULL)
		return;

	if (msgqueue_ready_evt) {
		/* if quit before the thread ready */
		WaitForSingleObject(msgqueue_ready_evt, INFINITE);
		CloseHandle(msgqueue_ready_evt);
		msgqueue_ready_evt = NULL;

		PostThreadMessage(GetThreadId(thread), WM_QUIT, 0, 0);
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
	}
}

int FWinRec::open_rec_device(int dev, WAVEFORMATEX *format, HANDLE thread, HWAVEIN *wave_hdl_out)
{
	MMRESULT res;
	HWAVEIN wi = NULL;
	WAVEFORMATEX fmt;
	WAVEFORMATEX *final_fmt;

	if (thread == NULL)
		return -RECORD_ERR_INVAL;

	if (format == NULL) {
		fmt.wFormatTag = WAVE_FORMAT_PCM;
		fmt.nChannels = 1;
		fmt.nSamplesPerSec = SAMPLE_RATE;
		fmt.nAvgBytesPerSec = SAMPLE_RATE * (SAMPLE_BIT_SIZE >> 3);
		fmt.nBlockAlign = 2;
		fmt.wBitsPerSample = SAMPLE_BIT_SIZE;
		fmt.cbSize = sizeof(WAVEFORMATEX);
		final_fmt = &fmt;
	}
	else {
		final_fmt = format;
	}
	res = waveInOpen((LPHWAVEIN)&wi, dev, final_fmt, GetThreadId(thread), (DWORD_PTR)0, CALLBACK_THREAD);
	if (res != MMSYSERR_NOERROR) {
		return 0 - res;
	}

	*wave_hdl_out = wi;
	return 0;
}

int FWinRec::prepare_rec_buffer(HWAVEIN wi, WAVEHDR ** bufheader_out, unsigned int headercount, unsigned int bufsize)
{
	int ret = 0;
	unsigned int i = 0;
	char clearout = 0;
	WAVEHDR *header;
	MMRESULT res;

	/* at least doubel buffering */
	if (headercount < 2 || bufheader_out == NULL)
		return -RECORD_ERR_INVAL;

	header = (WAVEHDR *)malloc(sizeof(WAVEHDR) * headercount);
	if (!header)
		return -RECORD_ERR_MEMFAIL;
	memset(header, 0, sizeof(WAVEHDR) * headercount);

	for (i = 0; i < headercount; ++i) {
		(header + i)->lpData = (LPSTR)malloc(bufsize);
		if ((header + i)->lpData == NULL) {
			clearout = 1;
			goto exit;
		}
		(header + i)->dwBufferLength = bufsize;
		(header + i)->dwFlags = 0;
		(header + i)->dwUser = i + 1; /* my usage: if 0, indicate it's not used */

		res = waveInPrepareHeader(wi, header + i, sizeof(WAVEHDR));
		if (res != MMSYSERR_NOERROR) {
			clearout = 1;
			goto exit;
		}
	}

	*bufheader_out = header;

exit:
	if (clearout) {
		free_rec_buffer(wi, header, headercount);
	}
	return ret;
}

void FWinRec::free_rec_buffer(HWAVEIN wi, WAVEHDR *first_header, unsigned headercount)
{
	unsigned int i;
	WAVEHDR *header;
	if (first_header == NULL || headercount == 0)
		return;

	header = first_header;
	for (i = 0; i < headercount; ++i) {
		if (header->lpData) {
			if (WHDR_PREPARED & header->dwFlags)
				waveInUnprepareHeader(wi, header, sizeof(WAVEHDR));
			free(header->lpData);
		}
		header++;
	}
	free(first_header);
}

void FWinRec::close_rec_device(HWAVEIN wi)
{
	if (wi != NULL) {
		waveInClose(wi);
	}
}

int FWinRec::start_record_internal(HWAVEIN wi, WAVEHDR *header, unsigned int bufcount)
{
	MMRESULT res;
	unsigned int i;

	if (bufcount < 2)
		return -1;

	/* must put at least one buffer into the driver first.
	and this buffer must has been allocated and prepared. */
	for (i = 0; i < bufcount; ++i) {
		if ((header->dwFlags & WHDR_INQUEUE) == 0) {
			header->dwUser = i + 1;
			res = waveInAddBuffer(wi, header, sizeof(WAVEHDR));
			if (res != MMSYSERR_NOERROR) {
				waveInReset(wi);
				return 0 - res;
			}
		}
		header++;
	}
	res = waveInStart(wi);
	if (MMSYSERR_NOERROR != res) {
		waveInReset(wi);
		return 0 - res;
	}

	return 0;
}

int FWinRec::stop_record_internal(HWAVEIN wi)
{
	MMRESULT res;
	//res = waveInStop(wi); /* some buffer may be still in the driver's queue */
	res = waveInReset(wi);

	if (MMSYSERR_NOERROR != res) {
		return 0 - res;
	}

	return 0;
}

void FWinRec::data_proc(struct recorder *rec, MSG *msg)
{
	HWAVEIN whdl;
	WAVEHDR *buf;

	whdl = (HWAVEIN)msg->wParam;
	buf = (WAVEHDR *)msg->lParam;

	UE_LOG(SnowingWarning, Warning, TEXT("data....\n"));
	dbg_wave_header(buf);

	/* dwUser should be index + 1 */
	if (buf->dwUser > rec->bufcount) {
		UE_LOG(SnowingWarning, Warning, TEXT("data_proc: something wrong. maybe buffer is reset.\n"));
		return;
	}

	rec->on_data_ind(buf->lpData, buf->dwBytesRecorded, rec->user_cb_para);

	switch (rec->state) {
	case RECORD_STATE_RECORDING:
		// after copied, put it into the queue of driver again.
		waveInAddBuffer(whdl, buf, sizeof(WAVEHDR));
		break;
	case RECORD_STATE_STOPPING:
	default:
		/* from this flag, can check if the whole data is processed after stopping */
		buf->dwUser = 0;
		break;
	}
}

int FWinRec::is_stopped_internal(struct recorder *rec)
{
	unsigned int i;
	WAVEHDR *header;

	header = (WAVEHDR*)(rec->bufheader);
	/* after close, already free */
	if (header == NULL || rec->bufcount == 0 /*|| rec->using_flags == NULL*/)
		return 1;

	for (i = 0; i < rec->bufcount; ++i) {
		if ((header)->dwFlags & WHDR_INQUEUE)
			return 0;
		/* after stop, we called the waveInReset to return all buffers */
		/* dwUser, see data_proc; */
		if (header->dwUser != 0)
			return 0;

		header++;
	}

	return 1;
}

int FWinRec::open_recorder_internal(struct recorder * rec, unsigned int dev, WAVEFORMATEX * fmt)
{
	unsigned int buf_size;
	int ret = 0;

	rec->bufcount = BUF_COUNT;
	rec->wavein_hdl = NULL;
	rec->rec_thread_hdl = NULL;
	ret = create_callback_thread((void *)rec, &rec->rec_thread_hdl);
	if (ret != 0)
		goto fail;

	ret = open_rec_device(dev, fmt, (HANDLE)rec->rec_thread_hdl, (HWAVEIN *)&rec->wavein_hdl);
	if (ret != 0) {
		goto fail;
	}

	if (fmt)
		buf_size = fmt->nBlockAlign *(fmt->nSamplesPerSec / 50) * FRAME_CNT; // 200ms
	else
		buf_size = FRAME_CNT * 20 * 16 * 2;  // 16khz, 16bit, 200ms;

	ret = prepare_rec_buffer((HWAVEIN)rec->wavein_hdl, (WAVEHDR **)&rec->bufheader, rec->bufcount, buf_size);
	if (ret != 0) {
		goto fail;
	}

	return 0;

fail:

	if (rec->bufheader) {
		free_rec_buffer((HWAVEIN)rec->wavein_hdl, (WAVEHDR *)rec->bufheader, rec->bufcount);
		rec->bufheader = NULL;
		rec->bufcount = 0;
	}
	if (rec->wavein_hdl) {
		close_rec_device((HWAVEIN)rec->wavein_hdl);
		rec->wavein_hdl = NULL;
	}
	if (rec->rec_thread_hdl) {
		close_callback_thread(rec->rec_thread_hdl);
		rec->rec_thread_hdl = NULL;
	}

	return ret;
}

void FWinRec::close_recorder_internal(struct recorder *rec)
{
	if (rec->wavein_hdl) {
		close_rec_device((HWAVEIN)rec->wavein_hdl);

		if (rec->rec_thread_hdl) {
			close_callback_thread((HANDLE)rec->rec_thread_hdl);
			rec->rec_thread_hdl = NULL;
		}
		if (rec->bufheader) {
			free_rec_buffer((HWAVEIN)rec->wavein_hdl, (WAVEHDR*)rec->bufheader, rec->bufcount);
			rec->bufheader = NULL;
			rec->bufcount = 0;
		}
		rec->wavein_hdl = NULL;
	}
}

/* the recording callback thread procedure */
unsigned int  __stdcall record_thread_proc(void * para)
{
	MSG msg;
	BOOL bRet;
	struct recorder *rec = (struct recorder *)para;

	/* trigger the message queue generator */
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(msgqueue_ready_evt);

	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (bRet == -1) {
			continue;
		}

		switch (msg.message) {
		case MM_WIM_OPEN:
			UE_LOG(SnowingWarning, Warning, TEXT("opened....\n"));
			break;
		case MM_WIM_CLOSE:
			UE_LOG(SnowingWarning, Warning, TEXT("closed....\n"));
			PostQuitMessage(0);
			break;
		case MM_WIM_DATA:
			winrec->data_proc(rec, &msg);
			break;
		default:
			break;
		}
	}

	return 0;
}