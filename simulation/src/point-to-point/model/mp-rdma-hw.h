
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
    };

    class MpRdmaHw : public RdmaHw
    {
    public:
        // Constructor
        MpRdmaHw();
        TypeId GetTypeId(void);
        Ptr<Packet> GetNextPacket(Ptr<RdmaQueuePair> qp);

        std::queue<VirtualPath> m_vpQueue; // virtual path queue
    };
} /* namespace ns3 */

#endif // MP_RDMA_HW_H
