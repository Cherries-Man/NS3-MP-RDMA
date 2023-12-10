#include <iostream>

#include "mp-qbb-remote-channel.h"
#include "mp-qbb-net-device.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/mpi-interface.h"

using namespace std;

NS_LOG_COMPONENT_DEFINE("MpQbbRemoteChannel");

namespace ns3
{

    NS_OBJECT_ENSURE_REGISTERED(MpQbbRemoteChannel);

    TypeId
    MpQbbRemoteChannel::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::MpQbbRemoteChannel")
                                .SetParent<MpQbbChannel>()
                                .AddConstructor<MpQbbRemoteChannel>();
        return tid;
    }

    MpQbbRemoteChannel::MpQbbRemoteChannel()
    {
    }

    MpQbbRemoteChannel::~MpQbbRemoteChannel()
    {
    }

    bool
    MpQbbRemoteChannel::TransmitStart(
        Ptr<Packet> p,
        Ptr<MpQbbNetDevice> src,
        Time txTime)
    {
        NS_LOG_FUNCTION(this << p << src);
        NS_LOG_LOGIC("UID is " << p->GetUid() << ")");

        IsInitialized();

        uint32_t wire = src == GetSource(0) ? 0 : 1;
        Ptr<MpQbbNetDevice> dst = GetDestination(wire);

#ifdef NS3_MPI
        // Calculate the rxTime (absolute)
        Time rxTime = Simulator::Now() + txTime + GetDelay();
        MpiInterface::SendPacket(p, rxTime, dst->GetNode()->GetId(), dst->GetIfIndex());
#else
        NS_FATAL_ERROR("Can't use distributed simulator without MPI compiled in");
#endif
        return true;
    }

} // namespace ns3
