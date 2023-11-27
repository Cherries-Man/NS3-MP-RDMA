#include <ns3/rdma-queue-pair.h>
#include <queue>
#include <vector>

namespace ns3
{
    // VP: Virtual Path
    struct VirtualPath
    {
        uint32_t sPort;  // source port
        uint8_t numSend; // number allowed to send in this path
        bool ReTx;       // whether this path is used for retransmission
    };

    class MpRdmaQueuePair : public RdmaQueuePair
    {
    public:
        MpRdmaQueuePair(uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport);
        static TypeId GetTypeId(void);

        enum Mode // mode of transport
        {
            MP_RDMA_HW_MODE_NORMAL = 0,
            MP_RDMA_HW_MODE_RECOVERY = 1
        };
        std::queue<VirtualPath> m_vpQueue; // virtual path queue
        Mode m_mode;
        uint32_t m_cwnd;     // congestion window
        Time m_lastSyncTime; // last time of synchronisation
    };

    class MpRdmaRxQueuePair : public RdmaRxQueuePair
    {
    public:
        MpRdmaRxQueuePair();
        static TypeId GetTypeId(void);

        std::vector<uint8_t> m_bitmap; // bitmap for out of order packets
        int32_t aack;                  // accumulative acknoledged sequence number
        int32_t aack_idx;              // bitmap index of aack
        int32_t max_rcv_seq;           // the highest seq number received
        uint32_t m_bitmapSize = 64;    // bitmap size
    };
}