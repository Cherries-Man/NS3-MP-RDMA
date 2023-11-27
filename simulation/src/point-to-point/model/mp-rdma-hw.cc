
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
    MpRdmaHw::MpRdmaHw() : RdmaHw(),
                           m_mode(MP_RDMA_HW_MODE_NORMAL),
                           m_cwnd(1),
                           m_lastSyncTime(Seconds(0)),
                           m_bitmap(m_bitmapSize, 0),
                           aack(-1),
                           aack_idx(-1),
                           max_rcv_seq(-1)
    {
        // 初始化成员变量
        srand(1000);
    }

    Ptr<Packet> MpRdmaHw::GetNextPacket(Ptr<RdmaQueuePair> qp)
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
        if (m_lastSyncTime + m_alpha * (m_delta / m_cwnd) * qp->m_baseRtt > Simulator::Now() || qp->GetBytesLeft() == 0)
        {
            rocev2.SetSynchronise(1);
        }
        if (!m_vpQueue.empty() && m_vpQueue.front().ReTx)
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
        if (m_vpQueue.empty())
        {
            // generate new virtual path
            VirtualPath vp;
            // source should be from 49152 to 65535
            vp.sPort = rand() % (65535 - 49152 + 1) + 49152;
            vp.numSend = 1;
            m_vpQueue.push(vp);
            qp->sport = vp.sPort;
        }
        else
        {
            VirtualPath vp = m_vpQueue.front();
            qp->sport = vp.sPort;
            if (vp.numSend == 1)
            {
                m_vpQueue.pop();
            }
            else
            {
                vp.numSend--;
                m_vpQueue.front() = vp;
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

    int MpRdmaHw::ReceiveUdp(Ptr<Packet> p, CustomHeader &ch)
    {
        if (ch.udp.seq > aack + m_bitmapSize)
        {
            // out of window, drop the packet
            return 1;
        }
        if (ch.udp.seq <= aack ||
            m_bitmap[(aack_idx + (ch.udp.seq - aack)) % m_bitmapSize] == 1)
        {
            // duplicate packet, drop it
            return 2;
        }
        if (ch.udp.seq > max_rcv_seq)
        {
            max_rcv_seq = ch.udp.seq;
        }
        // get Ipv4 ECN bits
        uint8_t ecnbits = ch.GetIpv4EcnBits();
        // calculate payload_size
        uint32_t payload_size = p->GetSize() - ch.GetSerializedSize();
        // TODO find corresponding rx queue pair
        Ptr<RdmaRxQueuePair> rxQp = GetRxQp(ch.dip, ch.sip, ch.udp.dport, ch.udp.sport, ch.udp.pg, true);
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
        m_bitmap[(aack_idx + ch.udp.seq - aack) % m_bitmapSize] = 1;
        if (ch.udp.synchronise && !doSynch())
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
    bool MpRdmaHw::doSynch()
    {
        for (int i = 0; i < std::min(m_delta, static_cast<uint32_t>(max_rcv_seq - aack)); i++)
        {
            if (m_bitmap[(aack_idx + i) % m_bitmapSize] == 0)
            {
                return false;
            }
        }
        moveRcvWnd(std::min(m_delta, static_cast<uint32_t>(max_rcv_seq - aack)));
        return true;
    }

    void MpRdmaHw::moveRcvWnd(uint32_t distance)
    {
        // update aack
        aack += distance;
        // update aack_idx
        aack_idx += distance % m_bitmapSize;
    }

} /* namespace ns3 */
