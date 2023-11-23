
#include "mp-rdma-hw.h"
#include "rocev2-header.h"
#include <ns3/seq-ts-header.h>
#include <random>
#include <ctime>
#include <ns3/ppp-header.h>

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
    MpRdmaHw::MpRdmaHw()
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

    // 其他辅助函数的实现...

} /* namespace ns3 */
