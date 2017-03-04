#include "AudioFlinger.h"

namespace android {

//------------------------------------------------------
//vaylb add
//methods defined in StreamOutHandle.h
//------------------------------------------------------
AudioFlinger::StreamOutHandle::StreamOutHandle(const sp<PlaybackThread>& thread, audio_stream_out_t* streamout)
    : BnStreamOut(),
    mThread(thread),
    mStream((stream_out*)streamout)
{
	ALOGE("vaylb_test-->StreamOutHandle::construct, StreamOutHandle.cpp");
	mDealer = NULL;
	mIM1 = NULL;
	mIM2 = NULL;
}

AudioFlinger::StreamOutHandle::~StreamOutHandle() {
  ALOGE("vaylb_test-->StreamOutHandle destruct!!");
  mStream->startflag = false;
  mStream->standbyflag = false;
  mStream->exitflag = false;
  mStream->mem1->mFront = 0;
  mStream->mem1->mRear = 0;
  mStream->mem1 = NULL;
  mStream->mem2 = NULL;
  mThread.clear();
  mDealer.clear();
  mIM1.clear();
  mIM2.clear();
  mIM1 = NULL;
  mIM2 = NULL;
  mStream = NULL;
}

status_t AudioFlinger::StreamOutHandle::open(int* count, sp<IMemory>* buffer1, sp<IMemory>* buffer2)
{
    ALOGE("vaylb_test-->StreamOutHandle open()");
	size_t framecount;
	//vaylb change for offload
	#ifdef OFFLOAD_TASK
		framecount = (mThread->mFrameCount)>>2;
	#else
		framecount = mThread->mFrameCount;
	#endif
	
	ALOGE("vaylb_test-->StreamOutHandle framecount = %d",framecount);
	*count = framecount;
	mStream->mCount = framecount; //set HAL mCount

	mDealer = new MemoryDealer(1024*1024, "StreamOutHandle");
	CblkMemory* cblkmem;
	
	size_t size = sizeof(CblkMemory);
	size_t buffersize = framecount*8; //framecount*4*2=240*8
	size += buffersize;

	if(mDealer != NULL && mIM1== NULL){
		ALOGE("vaylb_test_memory-->StreamOutHandle alloc mIM1 size = %d",size);
		mIM1 = mDealer->allocate(size);
		if(mIM1 != 0){
            cblkmem= static_cast<CblkMemory*>(mIM1->pointer());
		}
	}
	if (cblkmem != NULL) {
		new(cblkmem) CblkMemory();
		cblkmem->mFront = 0;
		cblkmem->mRear = 0;
		cblkmem->mMaxFrames = framecount*2;
		cblkmem->mBuffer = (char*)cblkmem + sizeof(CblkMemory);
        memset(cblkmem->mBuffer,0, buffersize);
	}

	mStream->mem1 = cblkmem;
	*buffer1 = mIM1;
	ALOGE("vaylb_test-->mIM1 address = %p",cblkmem->mBuffer);

	//mMonoMemory do not need CblkMemory, because it is not a loop-buffer.
	size = framecount * 4;   //==240*4
	if(mDealer != NULL&&mIM2== NULL){
		ALOGE("vaylb_test_memory-->StreamOutHandle alloc mIM2 size = %d",size);
		mIM2 = mDealer->allocate(size);
		if(mIM2 != 0){
			memset(mIM2->pointer(), 0, size);
		}
	}
	void * buffers = mIM2->pointer();
	*buffer2 = mIM2;
	mStream->mem2 = buffers;
	ALOGE("vaylb_test-->mIM2 address = %p",buffers);
	return NO_ERROR;
}

void AudioFlinger::StreamOutHandle::setstartflag(int flag)
{
	mStream->startflag = flag;
	ALOGE("vaylb_test_msm8974-->StreamOutHandle set the flag = %d ------------------------------------------",mStream->startflag);
}

void AudioFlinger::StreamOutHandle::write_I()
{
	//ALOGE("vaylb_test_offload-->StreamOutHandle write_I");
	size_t bytes = mThread->mFrameCount*4; //240*4
	mStream->write_I(&mStream->stream,mIM2->pointer(),bytes);
}

#if 0
ssize_t AudioFlinger::StreamOutHandle::write_I_offload()
{
	ALOGE("vaylb_test_offload-->StreamOutHandle write_I_offload");
	size_t bytes = mThread->mFrameCount;
	mStream->offload_direct = true;
	return mStream->write_I_offload(&mStream->stream,mIM2->pointer(),bytes);
}
#endif

void AudioFlinger::StreamOutHandle::standby_I()
{
	mStream->standby_I(&mStream->stream.common);
}

bool AudioFlinger::StreamOutHandle::needcheckwrited()
{
	//ALOGE("pzhao->call needcheckwrited success!");
	bool tmp=mThread->needcheckwrited;
	if(tmp){
		ALOGE("pzhao->needcheckwrited is true");
		mThread->needcheckwrited=false;
		}
	return tmp;
}

bool AudioFlinger::StreamOutHandle::checkstandby()
{
	return mStream->standbyflag;
}

bool AudioFlinger::StreamOutHandle::checkexit()
{
	return mStream->exitflag;
}

void AudioFlinger::StreamOutHandle::time_delay_host(long time)
{
	mStream->time_delay_host = time;
	mStream->time_delay_host_flag = true;
}

status_t AudioFlinger::StreamOutHandle::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    return BnStreamOut::onTransact(code, data, reply, flags);
}
}; // namespace android

