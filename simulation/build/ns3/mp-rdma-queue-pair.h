#ifndef MP_RDMA_QUEUE_PAIR_H
#define MP_RDMA_QUEUE_PAIR_H

#include <queue>
#include <vector>
#include <ns3/object.h>
#include <ns3/ipv4-address.h>
#include <ns3/timer.h>
#include <ns3/data-rate.h>

namespace ns3
{
    // VP: Virtual Path
    struct VirtualPath
    {
        uint16_t sPort;  // source port
        uint8_t numSend; // number allowed to send in this path
        bool ReTx;       // whether this path is used for retransmission
    };

    class MpRdmaQueuePair : public Object
    {
    public:
        MpRdmaQueuePair(uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport, uint32_t _mtu);
        static TypeId GetTypeId(void);
        uint32_t GetPacketsLeft();
        uint64_t GetBytesLeft();
        void SetSize(uint64_t size);
        void SetBaseRtt(uint64_t baseRtt);
        void SetAppNotifyCallback(Callback<void> notifyAppFinish);
        uint32_t GetHash(void);
        bool IsFinished();
        bool IsWinBound();

        enum Mode // mode of transport
        {
            MP_RDMA_HW_MODE_NORMAL = 0,
            MP_RDMA_HW_MODE_RECOVERY = 1
        };
        std::queue<VirtualPath> m_vpQueue; // virtual path queue
        Mode m_mode;
        uint64_t recovery;       // set the snd_nxt to this value when entering recovery mode
        double m_cwnd;           // congestion window
        uint32_t m_inflate;      // number of acked packets between snd_una and snd_nxt
        Time m_lastSyncTime;     // last time of synchronisation
        uint64_t snd_una;        // the highest unacked seq(begin from 0)
        uint64_t snd_retx;       // number of retransmitted packets(specify by the NACK packet)
        int64_t max_acked_seq;   // the highest acked number received(begin from -1)
        uint64_t snd_nxt;        // next sequence number to send(begin from 0)
        uint64_t snd_done;       // not in the m_vpQueue yet, going to go into m_vpQueue
        Time m_lastProbpathTime; // last time of probing path
        uint64_t m_size;         // size of data to be sent over the RDMA Queue Pair
        uint64_t m_baseRtt;      // Base Round-Trip Time (RTT) of this Queue Pair
        uint16_t m_pg;           // Priority Group (PG) and IP Identification (IPID)
        uint16_t m_ipid;
        Ipv4Address sip, dip; // Source and destination IP addresses and ports
        uint16_t sport, dport;
        // Callback to notify the application when the transmission is finished
        Callback<void> m_notifyAppFinish;
        Time m_nextAvail; // Soonest time of next send
        uint32_t m_mtu;   // Maximum Transmission Unit (MTU) of the Queue Pair
        DataRate m_rate;  // Current rate
        Time startTime;   // flow start time
    };

    class MpRdmaRxQueuePair : public Object
    {
    public:
        MpRdmaRxQueuePair();
        static TypeId GetTypeId(void);
        uint32_t GetHash(void);

        uint32_t m_bitmapSize = 64;    // bitmap size
        std::vector<uint8_t> m_bitmap; // bitmap for out of order packets
        int32_t aack;                  // accumulative acknoledged sequence number
        int32_t aack_idx;              // bitmap index of aack
        int32_t max_rcv_seq;           // the highest seq number received
        uint16_t m_ipid;
        uint32_t sip, dip;
        uint16_t sport, dport;
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

#endif /* MP_RDMA_QUEUE_PAIR_H */