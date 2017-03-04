

#ifndef ANDROID_HOSTPLAY_H
#define ANDROID_HOSTPLAY_H

#include <cutils/sched_policy.h>
#include <media/AudioSystem.h>
#include <media/IStreamOut.h>
#include <utils/threads.h>

//added for CblkMemory
#include "CblkMemory.h"

#include <hardware/audio.h>
#include <media/IAudioFlinger.h>
#include <binder/MemoryDealer.h>
#include <utils/Debug.h>
#include <utils/Thread.h>
#include <binder/IMemory.h>
#include <pthread.h>

namespace android {

class HostPlay : public RefBase
{
public:

            	 HostPlay();

protected:
          virtual ~HostPlay();
public:
			void start_threads();
			void stop_threads();
			int setstartflag(int flag);
			void clearBuffer(CblkMemory* cblk);
			void setBufferTemp(void* buffer,long length);
			int create(int receivebuffer, int sendbuffer);
			void exit();
			bool checkstandby();
			bool checkexit();
			void write_I();
			void standby_I();

			//add pzhao
			bool needcheckwrited();
			void checkstandbyflag();
			void changestandby(bool);
			void setcallback(void (*fun)(int),void(*getJni)(bool*),void(*detachJni)());
			void  (*callbackfun)(int);
			void (*getJniEnv)(bool*);
			void (*detachJniEnv)();
			void checkCanWrite();
			void singalToWrite();
			bool checkCanRead();
			void checkPlayFlag();
			void changePlayFlag(bool);
			void setHostMute();

//private:

	int16_t*			mSendBuffer;
	long 			mSendBufferLength;

	//9.21 edit
	volatile bool		playflag;
	volatile  bool      	standbyflag;
	bool				readflag;
	bool				exitflag1;
	bool				exitflag2;
	sp<IStreamOut>   mHandle;
	size_t			mCount;
	sp<IMemory>		mMemory1;
	sp<IMemory>		mMemory2;
	CblkMemory*     mBuffer1_cblk; //first IMemory cblk
	void*			mBuffer1; //first IMemory address 
	void*			mBuffer2;  //second IMemory address
	void*			mBuffer3;  //HostBuffer adress
	CblkMemory*     mBuffer3_cblk; //HostBuffer cblk
	sp<IAudioFlinger> mAudioFlinger;
	bool				time_delay_flag;

	//add pzhao
	volatile int	haswrite;
	volatile bool    	readaheadflag;
	volatile int 	readaheadcount;
	volatile int 	sendReadpos;
	volatile int 	sendWritepos;
	Mutex	   lock;
	Condition    isStandby;
	Condition    isFull;
	Condition   isStart;

	//vaylb added at 2016-0531 for support host mute in with middlewere project
	volatile bool		hostmuteflag;

public:
	class HostProcessThread : public Thread {

	public:
			HostProcessThread(HostPlay* host);
    		virtual ~HostProcessThread();

		// Thread virtuals
    	virtual     bool        threadLoop();
		virtual     void        threadLoop_separate(size_t stereo_offset, size_t host_offset, size_t sendoffset,int count);
		void        threadLoop_exit();
		void        threadLoop_run();
		size_t		availableToRead(CblkMemory* cblk);
		size_t		availableToWrite(CblkMemory* cblk);

	private:
		sp<HostPlay>		mHostPlay;
		CblkMemory*     mHostBuffer;
		CblkMemory*     mBuffer;
		bool			lockflag;
		bool			needdetach;
		bool 			prossesflag;
	};  // class HostProcessThread

	class HostPlayThread : public Thread {

	public:
			HostPlayThread(HostPlay* host);
    		virtual ~HostPlayThread();
		    virtual     bool        threadLoop();
			void        threadLoop_exit();
			void        threadLoop_run();
			size_t		availableToRead(CblkMemory* cblk);
			size_t		availableToWrite(CblkMemory* cblk);

	private:
		sp<HostPlay>		mHostPlay;
		CblkMemory*     mHostBuffer;
		void*     	mBuffer;
		bool			lockflag;
	};  // class HostPlayThread
	
private:
	sp<HostProcessThread>		mProcessThread;
	sp<HostPlayThread>		mPlayThread;
};



}; // namespace android

#endif // ANDROID_HOSTPLAY_H