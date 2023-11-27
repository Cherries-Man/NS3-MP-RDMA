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

} // namespace ns3
