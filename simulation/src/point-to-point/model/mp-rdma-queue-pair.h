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
        // uint32_t GetHash(void);

        enum Mode // mode of transport
        {
            MP_RDMA_HW_MODE_NORMAL = 0,
            MP_RDMA_HW_MODE_RECOVERY = 1
        };
        std::queue<VirtualPath> m_vpQueue; // virtual path queue
        Mode m_mode;
        double m_cwnd;       // congestion window
        int32_t m_inflate;   // number of acked packets between snd_una and snd_nxt
        Time m_lastSyncTime; // last time of synchronisation
        int32_t aack;        // accumulative acknoledged sequence number(begin from -1)
        uint32_t snd_retx;   // number of retransmitted packets(specify by the NACK packet)
        int64_t max_ack_seq; // the highest ack number received(begin from -1)
        uint64_t snd_nxt;    // next sequence number to send(begin from 0)
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

    class MpRdmaQueuePairGroup : public Object
    {
    public:
        std::vector<Ptr<MpRdmaQueuePair>> m_qps;

        static TypeId GetTypeId(void);
        MpRdmaQueuePairGroup(void);
        uint32_t GetN(void);
        Ptr<MpRdmaQueuePair> Get(uint32_t idx);
        Ptr<MpRdmaQueuePair> operator[](uint32_t idx);
        void AddQp(Ptr<MpRdmaQueuePair> qp);
        void Clear(void);
    };
}