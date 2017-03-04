
#ifndef ANDROID_ISTREAMOUT_H
#define ANDROID_ISTREAMOUT_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/IMemory.h>
#include <utils/LinearTransform.h>
#include <utils/String8.h> 

namespace android {

// ----------------------------------------------------------------------------

class IStreamOut : public IInterface
{
public:
    DECLARE_META_INTERFACE(StreamOut);
	virtual status_t		open(int* count, sp<IMemory>* buffer1, sp<IMemory>* buffer2) = 0;
	virtual void        setstartflag(int flag) = 0;
	virtual void        write_I() = 0;
	//vaylb added  for offload
	//virtual ssize_t        write_I_offload() = 0;
	virtual bool        checkstandby() = 0;
	virtual bool        checkexit() = 0;
	virtual void        standby_I() = 0;
	virtual void        time_delay_host(long time) = 0;

	//add pzhao for check track become ready
	virtual bool     	needcheckwrited()=0;
};

// ----------------------------------------------------------------------------

class BnStreamOut : public BnInterface<IStreamOut>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_ISTREAMOUT_H