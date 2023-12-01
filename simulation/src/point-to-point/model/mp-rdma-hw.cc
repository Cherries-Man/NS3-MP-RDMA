
#include "mp-rdma-hw.h"
#include "rocev2-data-header.h"
#include <ns3/seq-ts-header.h>
#include <random>
#include <ctime>
#include <ns3/ppp-header.h>
#include <ns3/qbb-header.h>
#include <algorithm>
#include "rocev2-ack-header.h"

namespace ns3
{
    // 注册类型
    TypeId MpRdmaHw::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::MpRdmaHw")
                                .SetParent<RdmaHw>()
                                .AddConstructor<MpRdmaHw>();
        return tid;
    }

    // 构造函数
    MpRdmaHw::MpRdmaHw() : RdmaHw()
    {
        // 初始化成员变量
        srand(1000);
    }

    Ptr<Packet> MpRdmaHw::GetNextPacket(Ptr<MpRdmaQueuePair> qp)
    {
        // return null if no more packets to send
        if (qp->m_vpQueue.empty())
        {
            return nullptr;
        }

        Ptr<Packet> p;
        // create RoCEv2DataHeader
        RoCEv2DataHeader rocev2;
        // create SeqTsHeader
        SeqTsHeader seqTs;
        // create udp header
        UdpHeader udpHeader;
        // create ipv4 header
        Ipv4Header ipHeader;
        // create ppp header
        PppHeader ppp;

        if (qp->m_mode == MpRdmaQueuePair::MP_RDMA_HW_MODE_NORMAL)
        {
            // normal mode
            uint32_t payload_size = qp->GetPacketsLeft(m_mtu) == 1 ? qp->GetBytesLeft(m_mtu) : m_mtu;
            p = Create<Packet>(payload_size);
            // set synchronise and ReTx
            if (qp->m_lastSyncTime + m_alpha * (m_delta / qp->m_cwnd) * qp->m_baseRtt < Simulator::Now() || qp->m_size / m_mtu == qp->snd_done)
            {
                rocev2.SetSynchronise(1);
                qp->m_lastSyncTime = Simulator::Now();
            }
            seqTs.SetSeq(qp->snd_done);
            qp->snd_done++;
        }
        else if (qp->m_mode == MpRdmaQueuePair::MP_RDMA_HW_MODE_RECOVERY)
        {
            uint32_t payload_size = qp->snd_retx == qp->m_size / m_mtu ? qp->m_size % m_mtu : m_mtu;
            p = Create<Packet>(payload_size);
            // recovery mode
            rocev2.SetReTx(1);
            seqTs.SetSeq(qp->snd_retx);
        }
        seqTs.SetPG(qp->m_pg);

        VirtualPath vp = qp->m_vpQueue.front();
        qp->sport = vp.sPort;
        if (vp.numSend == 1)
        {
            qp->m_vpQueue.pop();
        }
        else
        {
            vp.numSend--;
            qp->m_vpQueue.front() = vp;
        }
        if (Simulator::Now() - qp->m_lastProbpathTime > qp->m_baseRtt && rand() % 100 == 0)
        {
            qp->sport = rand() % (65535 - 49152 + 1) + 49152;
        }
        udpHeader.SetDestinationPort(qp->dport); // destination port
        udpHeader.SetSourcePort(qp->sport);

        ipHeader.SetSource(qp->sip);            // source IP addresses
        ipHeader.SetDestination(qp->dip);       // destination IP addresses
        ipHeader.SetProtocol(0x11);             // protocol type (UDP)
        ipHeader.SetPayloadSize(p->GetSize());  // payload size
        ipHeader.SetTtl(64);                    // TTL
        ipHeader.SetTos(0);                     // Type of Service (Tos)
        ipHeader.SetIdentification(qp->m_ipid); // identification

        ppp.SetProtocol(0x0021); // EtherToPpp(0x800), see point-to-point-net-device.cc

        p->AddHeader(rocev2);
        p->AddHeader(seqTs);
        p->AddHeader(udpHeader);
        p->AddHeader(ipHeader);
        p->AddHeader(ppp);

        // update state
        qp->m_ipid++;

        return p;
    }

    int MpRdmaHw::ReceiveUdp(Ptr<Packet> p, CustomHeader &ch)
    {
        Ptr<MpRdmaRxQueuePair> rxMpQp = GetRxQp(ch.dip, ch.sip, ch.udp.dport, ch.udp.sport, ch.udp.pg, true);
        if (ch.udp.seq > rxMpQp->aack + rxMpQp->m_bitmapSize)
        {
            // out of window, drop the packet
            return 1;
        }
        if (ch.udp.seq <= rxMpQp->aack ||
            rxMpQp->m_bitmap[(rxMpQp->aack_idx + (ch.udp.seq - rxMpQp->aack)) % rxMpQp->m_bitmapSize] == 1)
        {
            // duplicate packet, drop it
            return 2;
        }
        if (ch.udp.seq > rxMpQp->max_rcv_seq)
        {
            rxMpQp->max_rcv_seq = ch.udp.seq;
        }
        /**
         * In here, we only mark the packet as received. We don't
         * consider Tail and Tail with completion in the simulation.
         * But in the real world, they should be considered.
         */
        rxMpQp->m_bitmap[(rxMpQp->aack_idx + ch.udp.seq - rxMpQp->aack) % rxMpQp->m_bitmapSize] = 1;
        // get Ipv4 ECN bits
        uint8_t ecnbits = ch.GetIpv4EcnBits();
        // calculate payload_size
        uint32_t payload_size = p->GetSize() - ch.GetSerializedSize();
        RoCEv2AckHeader roCEv2AckH;
        qbbHeader seqh;
        seqh.SetSeq(ch.udp.seq);
        seqh.SetPG(ch.udp.pg);
        seqh.SetSport(ch.udp.dport);
        seqh.SetDport(ch.udp.sport);
        seqh.SetIntHeader(ch.udp.ih);
        if (ecnbits)
        {
            seqh.SetCnp();
        }
        Ipv4Header head;
        head.SetDestination(Ipv4Address(ch.sip));
        head.SetSource(Ipv4Address(ch.dip));
        Ptr<Packet> newp = Create<Packet>(std::max(60 - 14 - 20 -
                                                       (int)seqh.GetSerializedSize() -
                                                       (int)roCEv2AckH.GetSerializedSize(),
                                                   0));
        // default set ACK
        head.SetProtocol(0xFC);
        if (ch.udp.synchronise && !doSynch(rxMpQp))
        {
            // if sync fail set NACK
            head.SetProtocol(0xFD);
        }
        head.SetTtl(64);
        head.SetPayloadSize(newp->GetSize());
        head.SetIdentification(rxMpQp->m_ipid++);
        roCEv2AckH.SetReTx(ch.udp.ReTx);  // set ReTx rely on the send packet
        roCEv2AckH.SetAACK(rxMpQp->aack); // set AACK
        // add headers in order
        newp->AddHeader(roCEv2AckH);
        newp->AddHeader(seqh);
        newp->AddHeader(head);
        AddHeader(newp, 0x800); // Attach PPP header
        // send
        uint32_t nic_idx = GetNicIdxOfRxQp(rxMpQp);
        m_nic[nic_idx].dev->RdmaEnqueueHighPrioQ(newp);
        m_nic[nic_idx].dev->TriggerTransmit();
        return 0;
    }

    int MpRdmaHw::ReceiveAck(Ptr<Packet> p, CustomHeader &ch)
    {
        Ptr<MpRdmaQueuePair> qp = GetQp(ch.sip, ch.ack.sport, ch.udp.pg);
        uint32_t nic_idx = GetNicIdxOfQp(qp);
        Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;

        uint8_t cnp = (ch.ack.flags >> qbbHeader::FLAG_CNP) & 1;
        if (cnp)
        {
            qp->m_cwnd -= 1 / 2;
        }
        else
        {
            qp->m_cwnd += 1 / qp->m_cwnd;
        }

        // ACK
        if (ch.ack.seq >= qp->snd_una && ch.ack.seq < qp->snd_nxt)
        {
            qp->m_inflate++;
        }
        else
        {
            // Ghost ACK, return 1
            return 1;
        }

        if (ch.ack.seq <= qp->max_acked_seq - m_delta && ch.ack.ReTx == 0)
        {
            // out of order ACK, drop it
            return 2;
        }
        if (ch.ack.AACK + 1 > qp->snd_una)
        {
            qp->snd_una = ch.ack.AACK + 1;
            if (qp->m_mode == MpRdmaQueuePair::Mode::MP_RDMA_HW_MODE_RECOVERY && qp->snd_una > qp->recovery)
            {
                qp->m_mode = MpRdmaQueuePair::Mode::MP_RDMA_HW_MODE_NORMAL;
            }
        }
        if (ch.ack.seq > qp->max_acked_seq)
        {
            qp->max_acked_seq = ch.ack.seq;
        }

        if (ch.l3Prot == 0xFD)
        { // NACK
            qp->m_mode = MpRdmaQueuePair::MP_RDMA_HW_MODE_RECOVERY;
            qp->snd_retx = ch.ack.seq;
            qp->recovery = qp->snd_nxt;
        }
        else if (ch.l3Prot == 0xFC)
        { // ACK
            uint32_t awnd = qp->m_cwnd - ((qp->snd_nxt - qp->snd_una) - qp->m_inflate);
            uint8_t numSend = std::min(std::min(awnd, 2u), qp->GetPacketsLeft(dev->GetMtu()));
            qp->m_vpQueue.push({ch.ack.dport, numSend, 0});
        }

        return 0;
    }

    // 其他辅助函数的实现...

    /**
     * Do the Synch Procedure
     */
    bool MpRdmaHw::doSynch(Ptr<MpRdmaRxQueuePair> qp)
    {
        for (int i = 0; i < std::min(m_delta, static_cast<uint32_t>(qp->max_rcv_seq - qp->aack)); i++)
        {
            if (qp->m_bitmap[(qp->aack_idx + i) % qp->m_bitmapSize] == 0)
            {
                moveRcvWnd(qp, i);
                return false;
            }
        }
        moveRcvWnd(qp, std::min(m_delta, static_cast<uint32_t>(qp->max_rcv_seq - qp->aack)));
        return true;
    }

    void MpRdmaHw::moveRcvWnd(Ptr<MpRdmaRxQueuePair> qp, uint32_t distance)
    {
        // update aack
        qp->aack += distance;
        // update aack_idx
        qp->aack_idx += distance % qp->m_bitmapSize;
    }

    Ptr<MpRdmaQueuePair> MpRdmaHw::GetQp(uint32_t dip, uint16_t dport, uint16_t pg)
    {
        auto it = m_MpQpMap.find(GetQpKey(dip, dport, pg));
        if (it != m_MpQpMap.end())
            return it->second;
        return NULL;
    }

    Ptr<MpRdmaRxQueuePair> MpRdmaHw::GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, bool create)
    {
        uint64_t key = GetQpKey(dip, dport, pg);
        auto it = m_rxMpQpMap.find(key);
        if (it != m_rxMpQpMap.end())
            return it->second;
        if (create)
        {
            // create new rx qp
            Ptr<MpRdmaRxQueuePair> q = CreateObject<MpRdmaRxQueuePair>();
            // init the qp
            q->sip = sip;
            q->dip = dip;
            q->sport = sport;
            q->dport = dport;
            q->m_ecn_source.qIndex = pg;
            // store in map
            m_rxQpMap[key] = q;
            return q;
        }
        return NULL;
    }

    void MpRdmaHw::AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport,
                                uint32_t win, uint32_t baseRtt, Callback<void> notifyAppFinish)
    {
        // create qp
        Ptr<MpRdmaQueuePair> qp = CreateObject<MpRdmaQueuePair>(pg, sip, dip, sport, dport);
        qp->SetSize(size);
        qp->SetBaseRtt(baseRtt);
        qp->SetAppNotifyCallback(notifyAppFinish);

        // add qp
        uint32_t nic_idx = GetNicIdxOfQp(qp);
        m_nic[nic_idx].qpGrp->AddQp(qp);
        m_MpQpMap[GetQpKey(dip.Get(), dport, pg)] = qp;

        // set init variables
        // DataRate m_bps = m_nic[nic_idx].dev->GetDataRate();
        // qp->m_rate = m_bps;
    }

    uint32_t MpRdmaHw::GetNicIdxOfQp(Ptr<MpRdmaQueuePair> qp)
    {
        auto &v = m_rtTable[qp->dip.Get()];
        if (v.size() > 0)
        {
            return v[qp->GetHash() % v.size()];
        }
        else
        {
            NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
        }
    }

    uint64_t MpRdmaHw::GetQpKey(uint32_t dip, uint16_t dport, uint16_t pg)
    {
        return ((uint64_t)dip << 32) | ((uint64_t)dport << 16) | (uint64_t)pg;
    }

} /* namespace ns3 */
