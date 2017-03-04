
#define LOG_TAG "HostPlay"

#include <sys/resource.h>
#include <audio_utils/primitives.h>
#include <binder/IPCThreadState.h>
#include <media/HostPlay.h>
#include <utils/Log.h>
#include <cutils/atomic.h>
#include <time.h>
#include <utils/Trace.h>
#include <system/audio.h>


namespace android {

HostPlay::HostPlay()
{
	ALOGE("vaylb_test-->HostPlay construct.");
	playflag = false;
	time_delay_flag = true;
	sendReadpos=0;
	sendWritepos=0;
}

HostPlay::~HostPlay()
{
	ALOGE("vaylb_test-->HostPlay destruct.");
	mBuffer1 = NULL;
	mBuffer2 = NULL;
	mBuffer3 = NULL;
	mBuffer1_cblk = NULL;
	mBuffer3_cblk = NULL;

	playflag = true;
	standbyflag = false;
	readflag = false;
	haswrite=0;
	mAudioFlinger->closehandle();
	mHandle.clear();
	mProcessThread.clear();
	mPlayThread.clear();
    
}

int HostPlay::create(int receivebuffer, int sendbuffer)
{
	ALOGE("vaylb_test-->HostPlay:create.");
	
	standbyflag = true;
	playflag = false;
	readflag = false;
	exitflag1 = false;
	exitflag2 = false;
	haswrite=0;
	readaheadflag=false;
	readaheadcount=0;

	hostmuteflag = false;
	
	mAudioFlinger = AudioSystem::get_audio_flinger();
	if (mAudioFlinger == 0) {
        ALOGE("HostPlay can not get audioflinger.");
        return -1;
    }
	mHandle = mAudioFlinger->gethandle();
	int count;
	sp<IMemory> buffer1,buffer2;
	status_t status = mHandle->open(&count, &buffer1,&buffer2);
	if(status != NO_ERROR){
		ALOGE("HostPlay can not open the StreamOutHandle.");
		return status;
	}
	mCount = count;   //240
	mMemory1 = buffer1;
	//mMemory2 = buffer2;
	mBuffer1_cblk = static_cast<CblkMemory*>(buffer1->pointer());
	mBuffer1 = (char*)mBuffer1_cblk +sizeof(CblkMemory);
	mBuffer2 = buffer2->pointer();
	size_t mHostBufferFrame;
	
	//BufferFrame need to be calculated.
	mHostBufferFrame = count*128 + receivebuffer/2 + sendbuffer/2;	//count=240
	
	mHostBufferFrame = (int)(mHostBufferFrame/count)*count;
	ALOGE("audio_test-->HostPlay create hostbufferframe size = %d",mHostBufferFrame);

	mBuffer3 = malloc(mHostBufferFrame*4 + sizeof(CblkMemory));
	mBuffer3_cblk = static_cast<CblkMemory*>(mBuffer3);
	if (mBuffer3_cblk != NULL) {
		new(mBuffer3_cblk) CblkMemory();
		mBuffer3_cblk->mFront = 0;
		mBuffer3_cblk->mRear = 0;
		mBuffer3_cblk->mMaxFrames = mHostBufferFrame;
		mBuffer3_cblk->mBuffer = (char*)mBuffer3 + sizeof(CblkMemory);
        memset(mBuffer3_cblk->mBuffer, 0, mHostBufferFrame*4);
	}
	
	mProcessThread = new HostProcessThread(this);
	mPlayThread = new HostPlayThread(this);
	return mCount;
}

int HostPlay::setstartflag(int flag){
	ALOGE("pzhao-->HostPlay set startflag = %d",flag);
	if(flag == 1) start_threads();
	else stop_threads();
	mHandle->setstartflag(flag);
	return flag;
}

bool HostPlay::checkstandby(){
	return mHandle->checkstandby();
}

bool HostPlay::checkexit(){
	return mHandle->checkexit();
}

void HostPlay::standby_I(){

	mHandle->standby_I();
}

void HostPlay::clearBuffer(CblkMemory* cblk){
	cblk->mFront = 0;
	cblk->mRear = 0;
}

void HostPlay::start_threads()
{
	if(mProcessThread != NULL)  mProcessThread->run("HostProcessThread", ANDROID_PRIORITY_URGENT_AUDIO);
	if(mPlayThread != NULL) mPlayThread->run("HostPlayThread", ANDROID_PRIORITY_URGENT_AUDIO);
}

void HostPlay::stop_threads()
{
	ALOGE("vaylb_test-->HostPlay::stop thread");
	
	if(mPlayThread != NULL) mPlayThread->requestExit();
	if(mProcessThread != NULL) mProcessThread->requestExit();
	if(mBuffer1_cblk != NULL) clearBuffer(mBuffer1_cblk);
	if(mBuffer3_cblk != NULL) clearBuffer(mBuffer3_cblk);
}

void HostPlay::setBufferTemp(void* buffer,long length){
	ALOGE("pzhao-->Hostplay:setBufferTemp_l: mSendBuffer address = %p, mSendBufferLength = %ld",buffer,length);
	mSendBuffer = (int16_t*)buffer;
	mSendBufferLength = length;
}

void HostPlay::exit(){
	ALOGE("vaylb_test-->HostPlay exit.");
	mAudioFlinger->closehandle();
}

//add pzhao
bool HostPlay::needcheckwrited(){
	bool needchecktest=mHandle->needcheckwrited();
	//ALOGE("pzhao->need check write %d",needchecktest);
	return needchecktest;
}
//add pzhao
void HostPlay::setcallback(void (*fun)(int),void(*getJni)(bool*),void(*detachJni)()){
	callbackfun=fun;
	getJniEnv=getJni;
	detachJniEnv=detachJni;
}
void HostPlay::checkstandbyflag(){
	if(!standbyflag)
		return;
	Mutex::Autolock _l(lock);
	while(standbyflag){
		ALOGE("pzhao->wait standby");
		isStandby.wait(lock);
		}
}
void HostPlay::changestandby(bool standby){
	Mutex::Autolock _l(lock);
	standbyflag=standby;
	if(!standby)
		isStandby.signal();
}
void HostPlay::checkCanWrite(){
	if(!(sendWritepos-sendReadpos)>=mSendBufferLength)
		return;
	Mutex::Autolock _l(lock);
	while((sendWritepos-sendReadpos)>=mSendBufferLength){
		//ALOGE("pzhao->checkCanWrite can't write at %d",sendWritepos);
		callbackfun(3);	//singal java tcp to read
		isFull.wait(lock);
		}
}
bool HostPlay::checkCanRead(){
	bool re= sendWritepos>sendReadpos;
//	ALOGE("pzhao->checkCanRead %d",re);
	return re;
}
void HostPlay::singalToWrite(){
	Mutex::Autolock _l(lock);
	isFull.signal();
}
void HostPlay::checkPlayFlag(){
	if(playflag)
		return;
	Mutex::Autolock _l(lock);
	while(!playflag){
		ALOGE("pzhao->wait playFlag");
		haswrite=0;
		isStart.wait(lock);
		}
}
void HostPlay::changePlayFlag(bool flag){
	Mutex::Autolock _l(lock);
	playflag=flag;
	if(playflag){
		ALOGE("pzhao->signal to play");
		isStart.signal();
		}
		
}

void HostPlay::setHostMute(){
	hostmuteflag = true;		
}
//----------------------------
// HostProcessThread
//----------------------------

HostPlay::HostProcessThread::HostProcessThread(HostPlay* host)
	:Thread(false /*canCallJava*/)
{
	ALOGE("vaylb_test-->HostProcessThread construct.");
	ALOGE("pzhao->for test");
	mHostPlay = host;
	mHostBuffer = mHostPlay->mBuffer3_cblk;
	mBuffer = mHostPlay->mBuffer1_cblk;
	lockflag = false;
	needdetach=false;
	prossesflag=true;
}


HostPlay::HostProcessThread::~HostProcessThread()
{
	mHostPlay.clear();
	mHostPlay = NULL;
	mHostBuffer = NULL;
	mBuffer = NULL;
	lockflag = false;
	prossesflag=true;
}

bool HostPlay::HostProcessThread::threadLoop()
{
	ALOGE("vaylb_test-->HostPlay::HostProcessThread::threadLoop.");
	size_t written;
	long sleepNs;
	mHostPlay->getJniEnv(&needdetach);
	while (!exitPending())
    {	
    	if(prossesflag){
			mHostPlay->checkCanWrite();
			if((availableToRead(mBuffer)>=mHostPlay->mCount)&&(availableToWrite(mHostBuffer)>=mHostPlay->mCount)){
				written = mHostPlay->mCount;  //240
				size_t front = mBuffer->mFront % mBuffer->mMaxFrames;
    			size_t part1 = mBuffer->mMaxFrames - front;
    			if (part1 > written) {
        			part1 = written;
    			}
				size_t hostrear = mHostBuffer->mRear % mHostBuffer->mMaxFrames;
				int sendwrite=mHostPlay->sendWritepos%(mHostPlay->mSendBufferLength);
				//vaylb separate data.
				threadLoop_separate(front,hostrear,sendwrite,(int)part1);
				if (CC_UNLIKELY(front + part1 == mBuffer->mMaxFrames)) {
					size_t part2 = written - part1;
					if (CC_LIKELY(part2 > 0)) {
						if(hostrear + part1 == mHostBuffer->mMaxFrames) threadLoop_separate(0,0,sendwrite,(int)part2);
						else threadLoop_separate(0,hostrear + part1,sendwrite,(int)part2);
					}
				}
				mHostPlay->sendWritepos+=mHostPlay->mCount;
		//		ALOGE("pzhao->sendBuffer writePos  %d,readPos %d",mHostPlay->sendWritepos,mHostPlay->sendReadpos);
				android_atomic_release_store(written + mBuffer->mFront, &mBuffer->mFront);
				android_atomic_release_store(written + mHostBuffer->mRear, &mHostBuffer->mRear);
			}
    	}
	
	//ALOGE("pzhao->check  writepos:%d,readpos:%d ",mHostPlay->sendWritepos,mHostPlay->sendReadpos);
		if((!lockflag )&&!(mHostPlay->checkstandby())){
			mHostPlay->callbackfun(1);
			mHostPlay->changestandby(false);
			lockflag = true;
			prossesflag=true;
		}
		if((mHostPlay->checkstandby() )&&(lockflag )){
			mHostPlay->changePlayFlag( false);
			mBuffer->mRear = 0;
			mBuffer->mFront = 0;
			mHostBuffer->mRear = 0;
			mHostBuffer->mFront = 0;
			prossesflag=false;
			mHostPlay->sendWritepos=0;
			mHostPlay->sendReadpos=0;
			mHostPlay->standby_I();
			mHostPlay->callbackfun(2); 
			mHostPlay->changestandby(true);
			lockflag = false;
		}
		
		sleepNs = 1000000; //1ms
		const struct timespec req = {0, sleepNs};
        nanosleep(&req, NULL);

    }
	if(needdetach)
		mHostPlay->detachJniEnv();
	ALOGE("vaylb_test-->HostProcessThread:: threadLoop end.");
    return false;
}

void HostPlay::HostProcessThread::threadLoop_separate(size_t stereo_offset, size_t host_offset,size_t sendoffset, int count)
{
	int16_t* stereobit = static_cast<int16_t*>(stereo_offset > 0? (void*)((char*)mBuffer + sizeof(CblkMemory)  + (stereo_offset<<2)) : (void*)((char*)mBuffer + sizeof(CblkMemory)));
	int16_t* hostbit = static_cast<int16_t*>(host_offset > 0? (void*)((char*)mHostBuffer->mBuffer + (host_offset<<2)) : mHostBuffer->mBuffer);
	count = count<<1;
	int i;
	for(i = 0; i < count; i=i+2){
		*(hostbit + i) = *(stereobit + i);
		*(hostbit + i + 1) = *(stereobit + i);
		*(mHostPlay->mSendBuffer +sendoffset+ (i>>1)) = *(stereobit + i + 1);
	}
}

size_t HostPlay::HostProcessThread::availableToRead(CblkMemory * cblk){
	return android_atomic_acquire_load(&cblk->mRear) - cblk->mFront; 
}


size_t HostPlay::HostProcessThread::availableToWrite(CblkMemory * cblk){
	return cblk->mMaxFrames - (cblk->mRear - android_atomic_acquire_load(&cblk->mFront));
}


void HostPlay::HostProcessThread::threadLoop_run(){
	ALOGE("vaylb_test-->HostProcessThread::threadLoop_run.");
	run("HostProcessThread", ANDROID_PRIORITY_URGENT_AUDIO);
}

void HostPlay::HostProcessThread::threadLoop_exit(){
	ALOGE("vaylb_test-->close HostProcessThread.");
	this->requestExit();
	this->requestExitAndWait();
}

//----------------------------
// HostPlayThread
//----------------------------

HostPlay::HostPlayThread::HostPlayThread(HostPlay* host)
	:Thread(false /*canCallJava*/)
{
	ALOGE("vaylb_test-->HostPlayThread construct.");
	mHostPlay = host;
	mHostBuffer = mHostPlay->mBuffer3_cblk;
	mBuffer = mHostPlay->mBuffer2;
	lockflag = false;
}

HostPlay::HostPlayThread::~HostPlayThread()
{
	ALOGE("vaylb_test-->HostPlayThread destruct.");
	mHostPlay.clear();
	mHostPlay = NULL;
	mHostBuffer = NULL;
	mBuffer = NULL;
	lockflag = false;
}

bool HostPlay::HostPlayThread::threadLoop()
{
	ALOGE("vaylb_test-->HostPlay::HostPlayThread::threadLoop.");
	bool initflag = true;
	size_t written;
	//struct timespec req;
	long sleepNs;
	while(!exitPending()){
			mHostPlay->checkPlayFlag();
		    	mHostPlay->checkstandbyflag();
			if(mHostPlay->time_delay_flag){
				struct timeval tv;	  
				gettimeofday(&tv,NULL);    
				long time_hostplay = tv.tv_sec * 1000 + tv.tv_usec / 1000;
				ALOGE("vaylb_test-->[time_delay]HostPlayThread:  first playflag is true time = %ld",time_hostplay);
			}
			if(initflag==true){
				//vaylb modify for test offload
				#ifndef OFFLOAD_TASK
				//	mHostPlay->standby_I();
				#endif
				initflag = false;
			}
			
			if(mHostPlay->time_delay_flag)ALOGE("vaylb_test-->[time_delay]HostPlayThread: hostbuffer availableToRead = %d",availableToRead(mHostBuffer));
			if(availableToRead(mHostBuffer)>=mHostPlay->mCount){
				written = mHostPlay->mCount;
				size_t front = mHostBuffer->mFront % mHostBuffer->mMaxFrames;
				size_t part1 = mHostBuffer->mMaxFrames - front;
				if (part1 > written) {
					part1 = written;
				}
				if (CC_LIKELY(part1 > 0)) {
					memcpy(mBuffer, (char *) mHostBuffer->mBuffer + (front << 2), part1 << 2);
					if (CC_UNLIKELY(front + part1 == mHostBuffer->mMaxFrames)) {
						size_t part2 = written - part1;
						if (CC_UNLIKELY(part2 > 0)) {
							memcpy((char *) mBuffer+ (part1 << 2), (char*)mHostBuffer->mBuffer, part2 << 2);
						}
					}
					
				}
				size_t newFront=written + mHostBuffer->mFront;
				if(mHostPlay->readaheadflag){
					if(mHostPlay->readaheadcount>=0){
					size_t readcount;
					size_t needtoread=mHostPlay->readaheadcount*mHostPlay->mCount+written;
					size_t availableread=availableToRead(mHostBuffer);
					if(needtoread>availableread){
						mHostPlay->readaheadcount=(needtoread-availableread)/mHostPlay->mCount-1;
						readcount=availableread;
						mHostPlay->haswrite+=(readcount/mHostPlay->mCount-1);
						ALOGE("pzhao->needtoread>availableread");
						}
					else{
						mHostPlay->haswrite+=mHostPlay->readaheadcount;
						mHostPlay->readaheadcount=0;
						mHostPlay->readaheadflag=false;
						readcount=needtoread;
						ALOGE("pzhao->read ahead success!");
						}
					newFront=readcount+mHostBuffer->mFront;
				//	android_atomic_release_store(newFront, &mHostBuffer->mFront);
					}
					else{	
						mHostPlay->readaheadcount++;
						mHostPlay->haswrite--;
						newFront= mHostBuffer->mFront;
					//s	ALOGE("pzhao->read back and haswrite-1");							
						}
					}
			
					android_atomic_release_store(newFront, &mHostBuffer->mFront);
					
					if(mHostPlay->time_delay_flag){
						mHostPlay->time_delay_flag = false;
						struct timeval tv;	  
						gettimeofday(&tv,NULL);    
						long time_hostplay = tv.tv_sec * 1000 + tv.tv_usec / 1000;
						ALOGE("vaylb_test-->[time_delay]HostPlayThread:  first write time = %ld",time_hostplay);
					}
					if(!mHostPlay->hostmuteflag)mHostPlay->mHandle->write_I();
					mHostPlay->haswrite++;
				//	ALOGE("pzhao_test->mHostBuffer mFront %d mRear %d haswrite %d",mHostBuffer->mFront,mHostBuffer->mRear,mHostPlay->haswrite);
				

			}
			if(availableToRead(mHostBuffer)<mHostPlay->mCount){
				mHostPlay->exitflag1 = true;
			//	mHostPlay->setstartflag(0);
				ALOGE("pzhao->mHostPlay->exitflag1 = true");
			}
		

		if(mHostPlay->checkexit() == true){
			mHostPlay->exitflag2 = true;
			ALOGE("pzhao->mHostPlay->exitflag2 = true");
		}

//		sleepNs = 1000000; //1ms
	//	const struct timespec req = {0, sleepNs};
    // 	        nanosleep(&req, NULL);
	}
	ALOGE("vaylb_test-->HostPlayThread:: threadLoop end.");
    return false;
}

size_t HostPlay::HostPlayThread::availableToRead(CblkMemory * cblk){
	return android_atomic_acquire_load(&cblk->mRear) - cblk->mFront; 
}

size_t HostPlay::HostPlayThread::availableToWrite(CblkMemory * cblk){
	return cblk->mMaxFrames - (cblk->mRear - android_atomic_acquire_load(&cblk->mFront));
}

void HostPlay::HostPlayThread::threadLoop_run(){
	ALOGE("vaylb_test-->HostPlayThread::threadLoop_run.");
	run("HostPlayThread", ANDROID_PRIORITY_URGENT_AUDIO);
}

void HostPlay::HostPlayThread::threadLoop_exit(){
	ALOGE("vaylb_test-->close HostPlayThread.");
	this->requestExit();
	this->requestExitAndWait();
}

}; // namespace android
