#include "mp-rdma-queue-pair.h"

namespace ns3
{
    /**
     * MpRdmaQueuePair class implementation
     */
    MpRdmaQueuePair::MpRdmaQueuePair(uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport)
        : RdmaQueuePair(pg, _sip, _dip, _sport, _dport),
          m_mode(MP_RDMA_HW_MODE_NORMAL),
          m_cwnd(1),
          m_lastSyncTime(Seconds(0))
    {
    }

    TypeId MpRdmaQueuePair::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::MpRdmaQueuePair")
                                .SetParent<RdmaQueuePair>()
                                .AddConstructor<MpRdmaQueuePair>();
        return tid;
    }

    // uint32_t RdmaQueuePair::GetHash(void)
    // {
    //     union
    //     {
    //         struct
    //         {
    //             uint32_t sip, dip;
    //             uint16_t sport, dport;
    //         };
    //         char c[12];
    //     } buf;
    //     buf.sip = sip.Get();
    //     buf.dip = dip.Get();
    //     buf.sport = sport;
    //     buf.dport = dport;
    //     return Hash32(buf.c, 12);
    // }

    /**
     * MpRdmaRxQueuePair class implementation
     */
    MpRdmaRxQueuePair::MpRdmaRxQueuePair()
        : RdmaRxQueuePair(),
          m_bitmap(m_bitmapSize, 0),
          aack(-1),
          aack_idx(-1),
          max_rcv_seq(-1)
    {
    }

    TypeId MpRdmaRxQueuePair::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::MpRdmaRxQueuePair")
                                .SetParent<RdmaRxQueuePair>()
                                .AddConstructor<MpRdmaRxQueuePair>();
        return tid;
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
