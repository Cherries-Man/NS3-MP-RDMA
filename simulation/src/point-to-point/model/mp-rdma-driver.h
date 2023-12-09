#ifndef MP_RDMA_DRIVER_H
#define MP_RDMA_DRIVER_H

#include <ns3/node.h>
#include <ns3/qbb-net-device.h>
#include <ns3/rdma.h>
#include <ns3/mp-rdma-queue-pair.h>
#include <ns3/mp-rdma-hw.h>

namespace ns3
{
    class MpRdmaDriver : public Object
    {
    public:
        Ptr<Node> m_node;
        Ptr<MpRdmaHw> m_rdma;

        // trace
        TracedCallback<Ptr<MpRdmaQueuePair>> m_traceQpComplete;

        static TypeId GetTypeId(void);
        MpRdmaDriver();

        // This function init the m_nic according to the NetDevice
        // So this must be called after all NICs are installed
        void Init(void);

        // Set Node
        void SetNode(Ptr<Node> node);

        // Set RdmaHw
        void SetRdmaHw(Ptr<MpRdmaHw> mpRdma);

        // add a queue pair
        void AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport, uint32_t win, uint64_t baseRtt, Callback<void> notifyAppFinish);

        // callback when qp completes
        void QpComplete(Ptr<MpRdmaQueuePair> q);
    };
}

#endif /* MP_RDMA_DRIVER_H */