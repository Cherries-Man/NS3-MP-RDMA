#include "mp-qbb-channel.h"
#include "mp-qbb-net-device.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <iostream>
#include <fstream>

NS_LOG_COMPONENT_DEFINE("MpQbbChannel");

namespace ns3
{

    NS_OBJECT_ENSURE_REGISTERED(MpQbbChannel);

    TypeId
    MpQbbChannel::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::QbbChannel")
                                .SetParent<Channel>()
                                .AddConstructor<MpQbbChannel>()
                                .AddAttribute("Delay", "Transmission delay through the channel",
                                              TimeValue(Seconds(0)),
                                              MakeTimeAccessor(&MpQbbChannel::m_delay),
                                              MakeTimeChecker())
                                .AddTraceSource("TxRxQbb",
                                                "Trace source indicating transmission of packet from the QbbChannel, used by the Animation interface.",
                                                MakeTraceSourceAccessor(&MpQbbChannel::m_txrxQbb));
        return tid;
    }

    //
    // By default, you get a channel that
    // has an "infitely" fast transmission speed and zero delay.
    MpQbbChannel::MpQbbChannel()
        : PointToPointChannel()
    {
        NS_LOG_FUNCTION_NOARGS();
        m_nDevices = 0;
    }

    void
    MpQbbChannel::Attach(Ptr<MpQbbNetDevice> device)
    {

        // std::cout << m_nDevices << " " << N_DEVICES << "\n";
        // fflush(stdout);
        NS_LOG_FUNCTION(this << device);
        NS_ASSERT_MSG(m_nDevices < N_DEVICES, "Only two devices permitted");
        NS_ASSERT(device != 0);

        m_link[m_nDevices++].m_src = device;
        //
        // If we have both devices connected to the channel, then finish introducing
        // the two halves and set the links to IDLE.
        //
        if (m_nDevices == N_DEVICES)
        {
            m_link[0].m_dst = m_link[1].m_src;
            m_link[1].m_dst = m_link[0].m_src;
            m_link[0].m_state = IDLE;
            m_link[1].m_state = IDLE;
        }
    }

    bool
    MpQbbChannel::TransmitStart(
        Ptr<Packet> p,
        Ptr<MpQbbNetDevice> src,
        Time txTime)
    {
        NS_LOG_FUNCTION(this << p << src);
        NS_LOG_LOGIC("UID is " << p->GetUid() << ")");

        NS_ASSERT(m_link[0].m_state != INITIALIZING);
        NS_ASSERT(m_link[1].m_state != INITIALIZING);

        uint32_t wire = src == m_link[0].m_src ? 0 : 1;

        Simulator::ScheduleWithContext(m_link[wire].m_dst->GetNode()->GetId(),
                                       txTime + m_delay, &MpQbbNetDevice::Receive,
                                       m_link[wire].m_dst, p);

        // Call the tx anim callback on the net device
        m_txrxQbb(p, src, m_link[wire].m_dst, txTime, txTime + m_delay);
        return true;
    }

    uint32_t
    MpQbbChannel::GetNDevices(void) const
    {
        NS_LOG_FUNCTION_NOARGS();

        // std::cout<<m_nDevices<<"\n";
        // std::cout.flush();

        return m_nDevices;
    }

    Ptr<MpQbbNetDevice>
    MpQbbChannel::GetQbbDevice(uint32_t i) const
    {
        NS_LOG_FUNCTION_NOARGS();
        NS_ASSERT(i < 2);
        return m_link[i].m_src;
    }

    Ptr<NetDevice>
    MpQbbChannel::GetDevice(uint32_t i) const
    {
        NS_LOG_FUNCTION_NOARGS();
        return GetQbbDevice(i);
    }

    Time
    MpQbbChannel::GetDelay(void) const
    {
        return m_delay;
    }

    Ptr<MpQbbNetDevice>
    MpQbbChannel::GetSource(uint32_t i) const
    {
        return m_link[i].m_src;
    }

    Ptr<MpQbbNetDevice>
    MpQbbChannel::GetDestination(uint32_t i) const
    {
        return m_link[i].m_dst;
    }

    bool
    MpQbbChannel::IsInitialized(void) const
    {
        NS_ASSERT(m_link[0].m_state != INITIALIZING);
        NS_ASSERT(m_link[1].m_state != INITIALIZING);
        return true;
    }

} // namespace ns3
