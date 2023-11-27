
#include "mp-rdma-hw.h"
#include "rocev2-data-header.h"
#include <ns3/seq-ts-header.h>
#include <random>
#include <ctime>
#include <ns3/ppp-header.h>
#include <ns3/qbb-header.h>
#include <algorithm>

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
    //    m_mode(MP_RDMA_HW_MODE_NORMAL),
    //    m_cwnd(1),
    //    m_lastSyncTime(Seconds(0)),
    //    m_bitmap(m_bitmapSize, 0),
    //    aack(-1),
    //    aack_idx(-1),
    //    max_rcv_seq(-1)
    {
        // 初始化成员变量
        srand(1000);
    }

    Ptr<Packet> MpRdmaHw::GetNextPacket(Ptr<MpRdmaQueuePair> qp)
    {
        uint32_t payload_size = qp->GetBytesLeft();
        if (m_mtu < payload_size)
        {
            payload_size = m_mtu;
        }
        Ptr<Packet> p = Create<Packet>(payload_size);
        // add RoCEv2DataHeader
        RoCEv2DataHeader rocev2;

        // set synchronise and ReTx
        if (qp->m_lastSyncTime + m_alpha * (m_delta / qp->m_cwnd) * qp->m_baseRtt > Simulator::Now() || qp->GetBytesLeft() == 0)
        {
            rocev2.SetSynchronise(1);
        }
        if (!qp->m_vpQueue.empty() && qp->m_vpQueue.front().ReTx)
        {
            rocev2.SetReTx(1);
        }
        // add SeqTsHeader
        SeqTsHeader seqTs;
        seqTs.SetSeq(qp->snd_nxt);
        seqTs.SetPG(qp->m_pg);
        p->AddHeader(seqTs);
        // add udp header
        UdpHeader udpHeader;
        udpHeader.SetDestinationPort(qp->dport); // destination port
        //  || (rand() % 100 < 1) new path probing will be done in the receiver part
        if (qp->m_vpQueue.empty())
        {
            // generate new virtual path
            VirtualPath vp;
            // source should be from 49152 to 65535
            vp.sPort = rand() % (65535 - 49152 + 1) + 49152;
            vp.numSend = 1;
            qp->m_vpQueue.push(vp);
            qp->sport = vp.sPort;
        }
        else
        {
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
        }
        udpHeader.SetSourcePort(qp->sport);
        p->AddHeader(udpHeader);
        // add ipv4 header
        Ipv4Header ipHeader;
        ipHeader.SetSource(qp->sip);            // source IP addresses
        ipHeader.SetDestination(qp->dip);       // destination IP addresses
        ipHeader.SetProtocol(0x11);             // protocol type (UDP)
        ipHeader.SetPayloadSize(p->GetSize());  // payload size
        ipHeader.SetTtl(64);                    // TTL
        ipHeader.SetTos(0);                     // Type of Service (Tos)
        ipHeader.SetIdentification(qp->m_ipid); // identification
        p->AddHeader(ipHeader);
        // add ppp header
        PppHeader ppp;
        ppp.SetProtocol(0x0021); // EtherToPpp(0x800), see point-to-point-net-device.cc
        p->AddHeader(ppp);

        // update state
        qp->snd_nxt += payload_size;
        qp->m_ipid++;
        return p;
    }

    Ptr<MpRdmaRxQueuePair> MpRdmaHw::GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, bool create)
    {
        uint64_t key = ((uint64_t)dip << 32) | ((uint64_t)pg << 16) | (uint64_t)dport;
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
        // get Ipv4 ECN bits
        uint8_t ecnbits = ch.GetIpv4EcnBits();
        // calculate payload_size
        uint32_t payload_size = p->GetSize() - ch.GetSerializedSize();
        qbbHeader seqh;
        if (ecnbits)
        {
            seqh.SetCnp();
        }

        /**
         * In here, we only mark the packet as received. We don't
         * consider Tail and Tail with completion in the simulation.
         * But in the real world, they should be considered.
         */
        rxMpQp->m_bitmap[(rxMpQp->aack_idx + ch.udp.seq - rxMpQp->aack) % rxMpQp->m_bitmapSize] = 1;
        if (ch.udp.synchronise && !doSynch(rxMpQp))
        {
            // generate NACK
        }
        // generate ACK

        return RdmaHw::ReceiveUdp(p, ch);
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

} /* namespace ns3 */
