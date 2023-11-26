
#ifndef MP_RDMA_HW_H
#define MP_RDMA_HW_H

#include "rdma-hw.h"
#include <queue>

namespace ns3
{
    // VP: Virtual Path
    struct VirtualPath
    {
        uint32_t sPort;  // source port
        uint8_t numSend; // number allowed to send in this path
        bool ReTx;       // whether this path is used for retransmission
    };

    class MpRdmaHw : public RdmaHw
    {
    public:
        // Constructor
        MpRdmaHw();
        TypeId GetTypeId(void);
        Ptr<Packet> GetNextPacket(Ptr<RdmaQueuePair> qp);
        int ReceiveUdp(Ptr<Packet> p, CustomHeader &ch);

        enum Mode // mode of transport
        {
            MP_RDMA_HW_MODE_NORMAL = 0,
            MP_RDMA_HW_MODE_RECOVERY = 1
        };

        std::queue<VirtualPath> m_vpQueue; // virtual path queue
        Mode m_mode;
        uint32_t m_cwnd;     // congestion window
        Time m_lastSyncTime; // last time of synchronisation
        Time m_baseRTT;      // base RTT(need to be estimated later, and implemented later)
        /**
         * set hyper parameters about MP-RDMA-HW
         */
        double m_alpha = 1.0; // alpha for calculating synchronisation time
        double m_delta = 64;  // out of order degree parameter
    };
} /* namespace ns3 */

#endif // MP_RDMA_HW_H
