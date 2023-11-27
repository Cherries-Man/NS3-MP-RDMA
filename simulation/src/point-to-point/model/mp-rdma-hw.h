
#ifndef MP_RDMA_HW_H
#define MP_RDMA_HW_H

#include "rdma-hw.h"
#include <queue>
#include <vector>
#include "mp-rdma-queue-pair.h"

namespace ns3
{
    // VP: Virtual Path
    // struct VirtualPath
    // {
    //     uint32_t sPort;  // source port
    //     uint8_t numSend; // number allowed to send in this path
    //     bool ReTx;       // whether this path is used for retransmission
    // };

    class MpRdmaHw : public RdmaHw
    {
    public:
        // Constructor
        MpRdmaHw();
        TypeId GetTypeId(void);
        Ptr<Packet> GetNextPacket(Ptr<MpRdmaQueuePair> qp);
        int ReceiveUdp(Ptr<Packet> p, CustomHeader &ch);
        bool doSynch(Ptr<MpRdmaRxQueuePair> rxMpQp);
        void moveRcvWnd(Ptr<MpRdmaRxQueuePair> rxMpQp, uint32_t distance);
        Ptr<MpRdmaRxQueuePair> GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, bool create);

        // enum Mode // mode of transport
        // {
        //     MP_RDMA_HW_MODE_NORMAL = 0,
        //     MP_RDMA_HW_MODE_RECOVERY = 1
        // };
        // std::queue<VirtualPath> m_vpQueue; // virtual path queue
        // Mode m_mode;
        // uint32_t m_cwnd;     // congestion window
        // Time m_lastSyncTime; // last time of synchronisation

        // std::vector<uint8_t> m_bitmap; // bitmap for out of order packets
        // int32_t aack;                  // accumulative acknoledged sequence number
        // int32_t aack_idx;              // bitmap index of aack
        // int32_t max_rcv_seq;           // the highest seq number received
        // uint32_t m_bitmapSize = 64;    // bitmap size

        /**
         * set hyper parameters about MP-RDMA-HW
         */
        double m_alpha = 1.0;  // alpha for calculating synchronisation time
        uint32_t m_delta = 32; // out of order degree parameter

        std::unordered_map<uint64_t, Ptr<MpRdmaRxQueuePair>> m_rxMpQpMap; // mapping from uint64_t to rx qp
    };
} /* namespace ns3 */

#endif // MP_RDMA_HW_H
