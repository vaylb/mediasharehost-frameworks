#include "audio_hw.h"

class StreamOutHandle:public android::BnStreamOut {
public:
	StreamOutHandle(const sp<PlaybackThread>& thread, audio_stream_out_t* streamout);

	virtual 			~StreamOutHandle();

	virtual status_t open(int* count, sp<IMemory>* buffer1, sp<IMemory>* buffer2); //init the two IMemory and some otherthings,return mFrameCount and two IMemory
	virtual void	setstartflag(int flag);
	//virtual ssize_t	write_I_offload();
	virtual void	write_I();
	virtual void	standby_I();
	virtual bool	checkstandby();
	virtual bool	checkexit();
	virtual void	time_delay_host(long time);
	//add pzhao
	virtual bool  	needcheckwrited();
	
	virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags);

private:
	sp<PlaybackThread> mThread;
	stream_out* mStream;
	sp<MemoryDealer> mDealer; //use MemoryDealer to allocate two Imemory.
	sp<IMemory> 		mIM1,mIM2;
};


