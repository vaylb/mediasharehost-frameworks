

#ifndef ANDROID_STREAMOUT_H
#define ANDROID_STREAMOUT_H

#include <cutils/sched_policy.h>
#include <media/AudioSystem.h>
#include <media/IStreamOut.h>
#include <utils/threads.h>

//vaylb added for CblkMemory
#include "CblkMemory.h"

#include <hardware/audio.h>
#include <media/IAudioFlinger.h>
#include <binder/MemoryDealer.h>




namespace android {

class StreamOut : public RefBase
{
public:

                        StreamOut();

protected:
                        virtual ~StreamOut();


};

}; // namespace android

#endif // ANDROID_STREAMOUT_H