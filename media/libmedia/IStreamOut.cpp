

#define LOG_TAG "IStreamOut"
//#define LOG_NDEBUG 0
#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <media/IStreamOut.h>

//added for CblkMemory
#include "CblkMemory.h"

#include <hardware/audio.h>



namespace android {

enum {
	STREAMOUT_OPEN = IBinder::FIRST_CALL_TRANSACTION,
	SET_STARTFLAG,
	WRITE_I,
	WRITE_I_OFFLOAD,
	CHECK_STANDBY,
	CHECK_EXIT,
	STANDBY_I,
	TIME_DELAY_HOST,
	NEED_CHECK_WRITED, //add pzhao
};


class BpStreamOut : public BpInterface<IStreamOut>
{
public:
	BpStreamOut(const sp<IBinder>& impl)
        : BpInterface<IStreamOut>(impl)
    {
    }

	virtual status_t open(int* count, sp<IMemory>* buffer1, sp<IMemory>* buffer2)
    {
    	ALOGE("audio_test-->BpStreamOut open()");
        Parcel data, reply;
        data.writeInterfaceToken(IStreamOut::getInterfaceDescriptor());
        status_t status = remote()->transact(STREAMOUT_OPEN, data, &reply); 
        if (status == NO_ERROR) {
			*count = reply.readInt32();
            *buffer1 = interface_cast<IMemory>(reply.readStrongBinder());
			CblkMemory* cblk = static_cast<CblkMemory*>((*buffer1)->pointer());
			*buffer2 = interface_cast<IMemory>(reply.readStrongBinder());
        }
        return status;
    }

	virtual void setstartflag(int flag) {
    	ALOGE("audio_test-->BpStreamOut setstartflag()");
        Parcel data, reply;
        data.writeInterfaceToken(IStreamOut::getInterfaceDescriptor());
        data.writeInt32(flag);
        remote()->transact(SET_STARTFLAG, data, &reply);
    }

	virtual void write_I() {
		Parcel data, reply;
		size_t size;
		data.writeInterfaceToken(IStreamOut::getInterfaceDescriptor());
		remote()->transact(WRITE_I, data, &reply);
 	}

	#if 0
	virtual ssize_t write_I_offload() {
		Parcel data, reply;
		size_t size;
		data.writeInterfaceToken(IStreamOut::getInterfaceDescriptor());
		remote()->transact(WRITE_I_OFFLOAD, data, &reply);
		return reply.readInt32();
 	}
	#endif


	virtual bool checkstandby() {
		Parcel data, reply;
		size_t size;
		data.writeInterfaceToken(IStreamOut::getInterfaceDescriptor());
		remote()->transact(CHECK_STANDBY, data, &reply);
		return reply.readInt32();
 	}

	//add pzhao
	virtual bool needcheckwrited(){
		Parcel data, reply;
		size_t size;
		data.writeInterfaceToken(IStreamOut::getInterfaceDescriptor());
		remote()->transact(NEED_CHECK_WRITED, data, &reply);
		return reply.readInt32();
		}
	
	virtual bool checkexit() {
		Parcel data, reply;
		size_t size;
		data.writeInterfaceToken(IStreamOut::getInterfaceDescriptor());
		remote()->transact(CHECK_EXIT, data, &reply);
		return reply.readInt32();
 	}

	virtual void standby_I() {
		Parcel data, reply;
		data.writeInterfaceToken(IStreamOut::getInterfaceDescriptor());
		remote()->transact(STANDBY_I, data, &reply);
 	}

	virtual void time_delay_host(long time) {
        Parcel data, reply;
        data.writeInterfaceToken(IStreamOut::getInterfaceDescriptor());
        data.writeInt32(time);
        remote()->transact(TIME_DELAY_HOST, data, &reply);
    }

};

IMPLEMENT_META_INTERFACE(StreamOut, "android.media.IStreamOut");

// ----------------------------------------------------------------------

status_t BnStreamOut::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
		case STREAMOUT_OPEN: {
			ALOGE("audio_test-->BnStreamOut onTransact case STREAMOUT_OPEN");
			int count;
			sp<IMemory> mem1,mem2;
            CHECK_INTERFACE(IStreamOut, data, reply);
            open(&count,&mem1,&mem2);
			reply->writeInt32(count);
            reply->writeStrongBinder(mem1->asBinder());
            reply->writeStrongBinder(mem2->asBinder());
            return NO_ERROR;
        } break;
		case SET_STARTFLAG: {
			ALOGE("audio_test-->BnStreamOut onTransact case SET_STARTFLAG");
            CHECK_INTERFACE(IStreamOut, data, reply);
            setstartflag(data.readInt32());
            return NO_ERROR;
        } break;
		case WRITE_I: {
            CHECK_INTERFACE(IStreamOut, data, reply);
            write_I();
            return NO_ERROR;
        } break;
		#if 0
		case WRITE_I_OFFLOAD: {
            CHECK_INTERFACE(IStreamOut, data, reply);
             reply->writeInt32(write_I_offload());
            return NO_ERROR;
        } break;
		#endif
		case CHECK_STANDBY: {
            CHECK_INTERFACE(IStreamOut, data, reply);
            reply->writeInt32(checkstandby());
            return NO_ERROR;
        } break;
		case CHECK_EXIT: {
            CHECK_INTERFACE(IStreamOut, data, reply);
            reply->writeInt32(checkexit());
            return NO_ERROR;
        } break;
		case STANDBY_I: {
            CHECK_INTERFACE(IStreamOut, data, reply);
            standby_I();
            return NO_ERROR;
        } break;
		case TIME_DELAY_HOST: {
            CHECK_INTERFACE(IStreamOut, data, reply);
            time_delay_host(data.readInt32());
            return NO_ERROR;
        } break;
		case NEED_CHECK_WRITED:{
		CHECK_INTERFACE(IStreamOut, data, reply);
		reply->writeInt32(needcheckwrited());
            return NO_ERROR;
		}break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

}; // namespace android

