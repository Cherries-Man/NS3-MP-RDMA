#ifndef MP_QBB_REMOTE_CHANNEL_H
#define MP_QBB_REMOTE_CHANNEL_H

#include "mp-qbb-channel.h"

namespace ns3
{

    /**
     * \ingroup qbb
     */
    class MpQbbRemoteChannel : public MpQbbChannel
    {
    public:
        static TypeId GetTypeId(void);
        MpQbbRemoteChannel();
        ~MpQbbRemoteChannel();
        virtual bool TransmitStart(Ptr<Packet> p, Ptr<MpQbbNetDevice> src, Time txTime);
    };
}

#endif
