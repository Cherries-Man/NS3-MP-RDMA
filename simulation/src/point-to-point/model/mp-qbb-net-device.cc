#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include <stdio.h>
#include "mp-qbb-net-device.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/object-vector.h"
#include "ns3/pause-header.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/assert.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-channel.h"
#include "mp-qbb-channel.h"
#include "ns3/random-variable.h"
#include "ns3/flow-id-tag.h"
#include "ns3/qbb-header.h"
#include "ns3/error-model.h"
#include "ns3/cn-header.h"
#include "ns3/ppp-header.h"
#include "ns3/udp-header.h"
#include "ns3/seq-ts-header.h"
#include "ns3/pointer.h"
#include "ns3/custom-header.h"

#include <iostream>

NS_LOG_COMPONENT_DEFINE("QbbNetDevice");

namespace ns3
{

    uint32_t MpRdmaEgressQueue::ack_q_idx = 3;
    // RdmaEgressQueue
    TypeId MpRdmaEgressQueue::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::RdmaEgressQueue")
                                .SetParent<Object>()
                                .AddTraceSource("RdmaEnqueue", "Enqueue a packet in the RdmaEgressQueue.",
                                                MakeTraceSourceAccessor(&MpRdmaEgressQueue::m_traceRdmaEnqueue))
                                .AddTraceSource("RdmaDequeue", "Dequeue a packet in the RdmaEgressQueue.",
                                                MakeTraceSourceAccessor(&MpRdmaEgressQueue::m_traceRdmaDequeue));
        return tid;
    }

    MpRdmaEgressQueue::MpRdmaEgressQueue()
    {
        m_rrlast = 0;
        m_qlast = 0;
        m_ackQ = CreateObject<DropTailQueue>();
        m_ackQ->SetAttribute("MaxBytes", UintegerValue(0xffffffff)); // queue limit is on a higher level, not here
    }

    Ptr<Packet> MpRdmaEgressQueue::DequeueQindex(int qIndex)
    {
        if (qIndex == -1)
        { // high prio
            Ptr<Packet> p = m_ackQ->Dequeue();
            m_qlast = -1;
            m_traceRdmaDequeue(p, 0);
            return p;
        }
        if (qIndex >= 0)
        { // qp
            Ptr<Packet> p = m_rdmaGetNxtPkt(m_qpGrp->Get(qIndex));
            m_rrlast = qIndex;
            m_qlast = qIndex;
            m_traceRdmaDequeue(p, m_qpGrp->Get(qIndex)->m_pg);
            return p;
        }
        return 0;
    }
    int MpRdmaEgressQueue::GetNextQindex(bool paused[])
    {
        bool found = false;
        uint32_t qIndex;
        if (!paused[ack_q_idx] && m_ackQ->GetNPackets() > 0)
            return -1;

        // no pkt in highest priority queue, do rr for each qp
        int res = -1024;
        uint32_t fcount = m_qpGrp->GetN();
        uint32_t min_finish_id = 0xffffffff;
        for (qIndex = 1; qIndex <= fcount; qIndex++)
        {
            uint32_t idx = (qIndex + m_rrlast) % fcount;
            Ptr<MpRdmaQueuePair> qp = m_qpGrp->Get(idx);
            if (!paused[qp->m_pg] && qp->GetBytesLeft() > 0 && !qp->IsWinBound())
            {
                if (m_qpGrp->Get(idx)->m_nextAvail.GetTimeStep() > Simulator::Now().GetTimeStep()) // not available now
                    continue;
                res = idx;
                break;
            }
            else if (qp->IsFinished())
            {
                min_finish_id = idx < min_finish_id ? idx : min_finish_id;
            }
        }

        // clear the finished qp
        if (min_finish_id < 0xffffffff)
        {
            int nxt = min_finish_id;
            auto &qps = m_qpGrp->m_qps;
            for (int i = min_finish_id + 1; i < fcount; i++)
                if (!qps[i]->IsFinished())
                {
                    if (i == res) // update res to the idx after removing finished qp
                        res = nxt;
                    qps[nxt] = qps[i];
                    nxt++;
                }
            qps.resize(nxt);
        }
        return res;
    }

    int MpRdmaEgressQueue::GetLastQueue()
    {
        return m_qlast;
    }

    uint32_t MpRdmaEgressQueue::GetNBytes(uint32_t qIndex)
    {
        NS_ASSERT_MSG(qIndex < m_qpGrp->GetN(), "RdmaEgressQueue::GetNBytes: qIndex >= m_qpGrp->GetN()");
        return m_qpGrp->Get(qIndex)->GetBytesLeft();
    }

    uint32_t MpRdmaEgressQueue::GetFlowCount(void)
    {
        return m_qpGrp->GetN();
    }

    Ptr<MpRdmaQueuePair> MpRdmaEgressQueue::GetQp(uint32_t i)
    {
        return m_qpGrp->Get(i);
    }

    void MpRdmaEgressQueue::RecoverQueue(uint32_t i)
    {
        NS_ASSERT_MSG(i < m_qpGrp->GetN(), "RdmaEgressQueue::RecoverQueue: qIndex >= m_qpGrp->GetN()");
        m_qpGrp->Get(i)->snd_nxt = m_qpGrp->Get(i)->snd_una;
    }

    void MpRdmaEgressQueue::EnqueueHighPrioQ(Ptr<Packet> p)
    {
        m_traceRdmaEnqueue(p, 0);
        m_ackQ->Enqueue(p);
    }

    void MpRdmaEgressQueue::CleanHighPrio(TracedCallback<Ptr<const Packet>, uint32_t> dropCb)
    {
        while (m_ackQ->GetNPackets() > 0)
        {
            Ptr<Packet> p = m_ackQ->Dequeue();
            dropCb(p, 0);
        }
    }

    /******************
     * QbbNetDevice
     *****************/
    NS_OBJECT_ENSURE_REGISTERED(MpQbbNetDevice);

    TypeId
    MpQbbNetDevice::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::QbbNetDevice")
                                .SetParent<PointToPointNetDevice>()
                                .AddConstructor<MpQbbNetDevice>()
                                .AddAttribute("QbbEnabled",
                                              "Enable the generation of PAUSE packet.",
                                              BooleanValue(true),
                                              MakeBooleanAccessor(&MpQbbNetDevice::m_qbbEnabled),
                                              MakeBooleanChecker())
                                .AddAttribute("QcnEnabled",
                                              "Enable the generation of PAUSE packet.",
                                              BooleanValue(false),
                                              MakeBooleanAccessor(&MpQbbNetDevice::m_qcnEnabled),
                                              MakeBooleanChecker())
                                .AddAttribute("DynamicThreshold",
                                              "Enable dynamic threshold.",
                                              BooleanValue(false),
                                              MakeBooleanAccessor(&MpQbbNetDevice::m_dynamicth),
                                              MakeBooleanChecker())
                                .AddAttribute("PauseTime",
                                              "Number of microseconds to pause upon congestion",
                                              UintegerValue(5),
                                              MakeUintegerAccessor(&MpQbbNetDevice::m_pausetime),
                                              MakeUintegerChecker<uint32_t>())
                                .AddAttribute("TxBeQueue",
                                              "A queue to use as the transmit queue in the device.",
                                              PointerValue(),
                                              MakePointerAccessor(&MpQbbNetDevice::m_queue),
                                              MakePointerChecker<Queue>())
                                .AddAttribute("RdmaEgressQueue",
                                              "A queue to use as the transmit queue in the device.",
                                              PointerValue(),
                                              MakePointerAccessor(&MpQbbNetDevice::m_rdmaEQ),
                                              MakePointerChecker<Object>())
                                .AddTraceSource("QbbEnqueue", "Enqueue a packet in the QbbNetDevice.",
                                                MakeTraceSourceAccessor(&MpQbbNetDevice::m_traceEnqueue))
                                .AddTraceSource("QbbDequeue", "Dequeue a packet in the QbbNetDevice.",
                                                MakeTraceSourceAccessor(&MpQbbNetDevice::m_traceDequeue))
                                .AddTraceSource("QbbDrop", "Drop a packet in the QbbNetDevice.",
                                                MakeTraceSourceAccessor(&MpQbbNetDevice::m_traceDrop))
                                .AddTraceSource("RdmaQpDequeue", "A qp dequeue a packet.",
                                                MakeTraceSourceAccessor(&MpQbbNetDevice::m_traceQpDequeue))
                                .AddTraceSource("QbbPfc", "get a PFC packet. 0: resume, 1: pause",
                                                MakeTraceSourceAccessor(&MpQbbNetDevice::m_tracePfc));

        return tid;
    }

    MpQbbNetDevice::MpQbbNetDevice()
    {
        NS_LOG_FUNCTION(this);
        m_ecn_source = new std::vector<ECNAccount>;
        for (uint32_t i = 0; i < qCnt; i++)
        {
            m_paused[i] = false;
        }

        m_rdmaEQ = CreateObject<MpRdmaEgressQueue>();
    }

    MpQbbNetDevice::~MpQbbNetDevice()
    {
        NS_LOG_FUNCTION(this);
    }

    void MpQbbNetDevice::DoDispose()
    {
        NS_LOG_FUNCTION(this);

        PointToPointNetDevice::DoDispose();
    }

    void MpQbbNetDevice::TransmitComplete(void)
    {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
        m_txMachineState = READY;
        NS_ASSERT_MSG(m_currentPkt != 0, "QbbNetDevice::TransmitComplete(): m_currentPkt zero");
        m_phyTxEndTrace(m_currentPkt);
        m_currentPkt = 0;
        DequeueAndTransmit();
    }

    void MpQbbNetDevice::DequeueAndTransmit(void)
    {
        NS_LOG_FUNCTION(this);
        if (!m_linkUp)
            return; // if link is down, return
        if (m_txMachineState == BUSY)
            return; // Quit if channel busy
        Ptr<Packet> p;
        if (m_node->GetNodeType() == 0)
        {
            int qIndex = m_rdmaEQ->GetNextQindex(m_paused);
            if (qIndex != -1024)
            {
                if (qIndex == -1)
                { // high prio
                    p = m_rdmaEQ->DequeueQindex(qIndex);
                    m_traceDequeue(p, 0);
                    TransmitStart(p);
                    return;
                }
                // a qp dequeue a packet
                Ptr<MpRdmaQueuePair> lastQp = m_rdmaEQ->GetQp(qIndex);
                p = m_rdmaEQ->DequeueQindex(qIndex);

                // transmit
                m_traceQpDequeue(p, lastQp);
                TransmitStart(p);

                // update for the next avail time
                m_rdmaPktSent(lastQp, p, m_tInterframeGap);
            }
            else
            { // no packet to send
                NS_LOG_INFO("PAUSE prohibits send at node " << m_node->GetId());
                Time t = Simulator::GetMaximumSimulationTime();
                for (uint32_t i = 0; i < m_rdmaEQ->GetFlowCount(); i++)
                {
                    Ptr<MpRdmaQueuePair> qp = m_rdmaEQ->GetQp(i);
                    if (qp->GetBytesLeft() == 0)
                        continue;
                    t = Min(qp->m_nextAvail, t);
                }
                if (m_nextSend.IsExpired() && t < Simulator::GetMaximumSimulationTime() && t > Simulator::Now())
                {
                    m_nextSend = Simulator::Schedule(t - Simulator::Now(), &MpQbbNetDevice::DequeueAndTransmit, this);
                }
            }
            return;
        }
        else
        {                                     // switch, doesn't care about qcn, just send
            p = m_queue->DequeueRR(m_paused); // this is round-robin
            if (p != 0)
            {
                m_snifferTrace(p);
                m_promiscSnifferTrace(p);
                Ipv4Header h;
                Ptr<Packet> packet = p->Copy();
                uint16_t protocol = 0;
                ProcessHeader(packet, protocol);
                packet->RemoveHeader(h);
                FlowIdTag t;
                uint32_t qIndex = m_queue->GetLastQueue();
                if (qIndex == 0)
                { // this is a pause or cnp, send it immediately!
                    m_node->SwitchNotifyDequeue(m_ifIndex, qIndex, p);
                    p->RemovePacketTag(t);
                }
                else
                {
                    m_node->SwitchNotifyDequeue(m_ifIndex, qIndex, p);
                    p->RemovePacketTag(t);
                }
                m_traceDequeue(p, qIndex);
                TransmitStart(p);
                return;
            }
            else
            { // No queue can deliver any packet
                NS_LOG_INFO("PAUSE prohibits send at node " << m_node->GetId());
                if (m_node->GetNodeType() == 0 && m_qcnEnabled)
                { // nothing to send, possibly due to qcn flow control, if so reschedule sending
                    Time t = Simulator::GetMaximumSimulationTime();
                    for (uint32_t i = 0; i < m_rdmaEQ->GetFlowCount(); i++)
                    {
                        Ptr<MpRdmaQueuePair> qp = m_rdmaEQ->GetQp(i);
                        if (qp->GetBytesLeft() == 0)
                            continue;
                        t = Min(qp->m_nextAvail, t);
                    }
                    if (m_nextSend.IsExpired() && t < Simulator::GetMaximumSimulationTime() && t > Simulator::Now())
                    {
                        m_nextSend = Simulator::Schedule(t - Simulator::Now(), &MpQbbNetDevice::DequeueAndTransmit, this);
                    }
                }
            }
        }
        return;
    }

    void
    MpQbbNetDevice::Resume(unsigned qIndex)
    {
        NS_LOG_FUNCTION(this << qIndex);
        NS_ASSERT_MSG(m_paused[qIndex], "Must be PAUSEd");
        m_paused[qIndex] = false;
        NS_LOG_INFO("Node " << m_node->GetId() << " dev " << m_ifIndex << " queue " << qIndex << " resumed at " << Simulator::Now().GetSeconds());
        DequeueAndTransmit();
    }

    void
    MpQbbNetDevice::Receive(Ptr<Packet> packet)
    {
        NS_LOG_FUNCTION(this << packet);
        if (!m_linkUp)
        {
            m_traceDrop(packet, 0);
            return;
        }

        if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
        {
            //
            // If we have an error model and it indicates that it is time to lose a
            // corrupted packet, don't forward this packet up, let it go.
            //
            m_phyRxDropTrace(packet);
            return;
        }

        m_macRxTrace(packet);
        CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
        ch.getInt = 1; // parse INT header
        packet->PeekHeader(ch);
        if (ch.l3Prot == 0xFE)
        { // PFC
            if (!m_qbbEnabled)
                return;
            unsigned qIndex = ch.pfc.qIndex;
            if (ch.pfc.time > 0)
            {
                m_tracePfc(1);
                m_paused[qIndex] = true;
            }
            else
            {
                m_tracePfc(0);
                Resume(qIndex);
            }
        }
        else
        { // non-PFC packets (data, ACK, NACK, CNP...)
            if (m_node->GetNodeType() > 0)
            { // switch
                packet->AddPacketTag(FlowIdTag(m_ifIndex));
                m_node->SwitchReceiveFromDevice(this, packet, ch);
            }
            else
            { // NIC
                // send to RdmaHw
                int ret = m_rdmaReceiveCb(packet, ch);
                // TODO we may based on the ret do something
            }
        }
        return;
    }

    bool MpQbbNetDevice::Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
    {
        NS_ASSERT_MSG(false, "QbbNetDevice::Send not implemented yet\n");
        return false;
    }

    bool MpQbbNetDevice::SwitchSend(uint32_t qIndex, Ptr<Packet> packet, CustomHeader &ch)
    {
        m_macTxTrace(packet);
        m_traceEnqueue(packet, qIndex);
        m_queue->Enqueue(packet, qIndex);
        DequeueAndTransmit();
        return true;
    }

    void MpQbbNetDevice::SendPfc(uint32_t qIndex, uint32_t type)
    {
        Ptr<Packet> p = Create<Packet>(0);
        PauseHeader pauseh((type == 0 ? m_pausetime : 0), m_queue->GetNBytes(qIndex), qIndex);
        p->AddHeader(pauseh);
        Ipv4Header ipv4h; // Prepare IPv4 header
        ipv4h.SetProtocol(0xFE);
        ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
        ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
        ipv4h.SetPayloadSize(p->GetSize());
        ipv4h.SetTtl(1);
        ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
        p->AddHeader(ipv4h);
        AddHeader(p, 0x800);
        CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
        p->PeekHeader(ch);
        SwitchSend(0, p, ch);
    }

    bool
    MpQbbNetDevice::Attach(Ptr<MpQbbChannel> ch)
    {
        NS_LOG_FUNCTION(this << &ch);
        m_channel = ch;
        m_channel->Attach(this);
        NotifyLinkUp();
        return true;
    }

    bool MpQbbNetDevice::TransmitStart(Ptr<Packet> p)
    {
        NS_LOG_FUNCTION(this << p);
        NS_LOG_LOGIC("UID is " << p->GetUid() << ")");
        //
        // This function is called to start the process of transmitting a packet.
        // We need to tell the channel that we've started wiggling the wire and
        // schedule an event that will be executed when the transmission is complete.
        //
        NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
        m_txMachineState = BUSY;
        m_currentPkt = p;
        m_phyTxBeginTrace(m_currentPkt);
        Time txTime = Seconds(m_bps.CalculateTxTime(p->GetSize()));
        Time txCompleteTime = txTime + m_tInterframeGap;
        NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds() << "sec");
        Simulator::Schedule(txCompleteTime, &MpQbbNetDevice::TransmitComplete, this);

        bool result = m_channel->TransmitStart(p, this, txTime);
        if (result == false)
        {
            m_phyTxDropTrace(p);
        }
        return result;
    }

    Ptr<Channel>
    MpQbbNetDevice::GetChannel(void) const
    {
        return m_channel;
    }

    bool MpQbbNetDevice::IsQbb(void) const
    {
        return true;
    }

    void MpQbbNetDevice::NewQp(Ptr<MpRdmaQueuePair> qp)
    {
        qp->m_nextAvail = Simulator::Now();
        DequeueAndTransmit();
    }
    void MpQbbNetDevice::ReassignedQp(Ptr<MpRdmaQueuePair> qp)
    {
        DequeueAndTransmit();
    }
    void MpQbbNetDevice::TriggerTransmit(void)
    {
        DequeueAndTransmit();
    }

    void MpQbbNetDevice::SetQueue(Ptr<BEgressQueue> q)
    {
        NS_LOG_FUNCTION(this << q);
        m_queue = q;
    }

    Ptr<BEgressQueue> MpQbbNetDevice::GetQueue()
    {
        return m_queue;
    }

    Ptr<MpRdmaEgressQueue> MpQbbNetDevice::GetRdmaQueue()
    {
        return m_rdmaEQ;
    }

    void MpQbbNetDevice::RdmaEnqueueHighPrioQ(Ptr<Packet> p)
    {
        m_traceEnqueue(p, 0);
        m_rdmaEQ->EnqueueHighPrioQ(p);
    }

    void MpQbbNetDevice::TakeDown()
    {
        // TODO: delete packets in the queue, set link down
        if (m_node->GetNodeType() == 0)
        {
            // clean the high prio queue
            m_rdmaEQ->CleanHighPrio(m_traceDrop);
            // notify driver/RdmaHw that this link is down
            m_rdmaLinkDownCb(this);
        }
        else
        { // switch
            // clean the queue
            for (uint32_t i = 0; i < qCnt; i++)
                m_paused[i] = false;
            while (1)
            {
                Ptr<Packet> p = m_queue->DequeueRR(m_paused);
                if (p == 0)
                    break;
                m_traceDrop(p, m_queue->GetLastQueue());
            }
            // TODO: Notify switch that this link is down
        }
        m_linkUp = false;
    }

    void MpQbbNetDevice::UpdateNextAvail(Time t)
    {
        if (!m_nextSend.IsExpired() && t < m_nextSend.GetTs())
        {
            Simulator::Cancel(m_nextSend);
            Time delta = t < Simulator::Now() ? Time(0) : t - Simulator::Now();
            m_nextSend = Simulator::Schedule(delta, &MpQbbNetDevice::DequeueAndTransmit, this);
        }
    }
} // namespace ns3
