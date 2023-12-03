#include "mp-rdma-queue-pair.h"
#include <ns3/simulator.h>

namespace ns3
{
    /**
     * MpRdmaQueuePair class implementation
     */
    MpRdmaQueuePair::MpRdmaQueuePair(uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport)
        : m_mode(MP_RDMA_HW_MODE_NORMAL),
          m_cwnd(1.0),
          m_lastSyncTime(Simulator::Now()),
          snd_una(0),
          snd_retx(0),
          max_acked_seq(-1),
          snd_nxt(0),
          snd_done(0),
          m_lastProbpathTime(Simulator::Now()),
          m_size(0),
          m_baseRtt(0),
          m_pg(pg),
          m_ipid(0),
          sip(_sip),
          dip(_dip),
          sport(_sport),
          dport(_dport)

    {
        // generate new virtual path
        VirtualPath vp;
        // source should be from 49152 to 65535
        vp.sPort = rand() % (65535 - 49152 + 1) + 49152;
        vp.numSend = 1;
        m_vpQueue.push(vp);
    }

    TypeId MpRdmaQueuePair::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::MpRdmaQueuePair")
                                .SetParent<RdmaQueuePair>()
                                .AddConstructor<MpRdmaQueuePair>();
        return tid;
    }

    uint32_t MpRdmaQueuePair::GetPacketsLeft(uint32_t mtu)
    {
        return ((m_size - snd_nxt) + (mtu - 1)) / mtu;
    }

    uint64_t MpRdmaQueuePair::GetBytesLeft(uint32_t mtu)
    {
        return m_size - snd_done * mtu;
    }

    void MpRdmaQueuePair::SetSize(uint64_t size)
    {
        m_size = size;
    }

    void MpRdmaQueuePair::SetBaseRtt(uint64_t baseRtt)
    {
        m_baseRtt = baseRtt;
    }

    void MpRdmaQueuePair::SetAppNotifyCallback(Callback<void> notifyAppFinish)
    {
        m_notifyAppFinish = notifyAppFinish;
    }

    uint32_t MpRdmaQueuePair::GetHash(void)
    {
        union
        {
            struct
            {
                uint32_t sip, dip;
                uint16_t sport, dport;
            };
            char c[12];
        } buf;
        buf.sip = sip.Get();
        buf.dip = dip.Get();
        buf.sport = sport;
        buf.dport = dport;
        return Hash32(buf.c, 12);
    }

    /**
     * MpRdmaRxQueuePair class implementation
     */
    MpRdmaRxQueuePair::MpRdmaRxQueuePair()
        : m_bitmap(m_bitmapSize, 0),
          aack(-1),
          aack_idx(-1),
          max_rcv_seq(-1),
          m_ipid(0),
          sip(0),
          dip(0),
          sport(0),
          dport(0)
    {
    }

    TypeId MpRdmaRxQueuePair::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::MpRdmaRxQueuePair")
                                .SetParent<RdmaRxQueuePair>()
                                .AddConstructor<MpRdmaRxQueuePair>();
        return tid;
    }

    uint32_t MpRdmaRxQueuePair::GetHash(void)
    {
        union
        {
            struct
            {
                uint32_t sip, dip;
                uint16_t sport, dport;
            };
            char c[12];
        } buf;
        buf.sip = sip;
        buf.dip = dip;
        buf.sport = sport;
        buf.dport = dport;
        return Hash32(buf.c, 12);
    }

    /**
     * MpRdmaQueuePairGroup class implementation
     */
    TypeId MpRdmaQueuePairGroup::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::MpRdmaQueuePairGroup")
                                .SetParent<Object>();
        return tid;
    }

    MpRdmaQueuePairGroup::MpRdmaQueuePairGroup(void)
    {
    }

    uint32_t MpRdmaQueuePairGroup::GetN(void)
    {
        return m_qps.size();
    }

    Ptr<MpRdmaQueuePair> MpRdmaQueuePairGroup::Get(uint32_t idx)
    {
        return m_qps[idx];
    }

    Ptr<MpRdmaQueuePair> MpRdmaQueuePairGroup::operator[](uint32_t idx)
    {
        return m_qps[idx];
    }

    void MpRdmaQueuePairGroup::AddQp(Ptr<MpRdmaQueuePair> qp)
    {
        m_qps.push_back(qp);
    }

    void MpRdmaQueuePairGroup::Clear(void)
    {
        m_qps.clear();
    }
} // namespace ns3
