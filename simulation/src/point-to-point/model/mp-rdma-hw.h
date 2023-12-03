
#ifndef MP_RDMA_HW_H
#define MP_RDMA_HW_H

#include "rdma-hw.h"
#include <queue>
#include <vector>
#include "mp-rdma-queue-pair.h"

namespace ns3
{
    struct MpRdmaInterfaceMgr
    {
        Ptr<QbbNetDevice> dev;
        Ptr<MpRdmaQueuePairGroup> qpGrp;

        MpRdmaInterfaceMgr() : dev(NULL), qpGrp(NULL) {}
        MpRdmaInterfaceMgr(Ptr<QbbNetDevice> _dev)
        {
            dev = _dev;
        }
    };

    class MpRdmaHw : public Object
    {
    public:
        // Constructor
        MpRdmaHw();
        TypeId GetTypeId(void);
        Ptr<Packet> GetNextPacket(Ptr<MpRdmaQueuePair> qp);
        int ReceiveUdp(Ptr<Packet> p, CustomHeader &ch);
        int ReceiveAck(Ptr<Packet> p, CustomHeader &ch);
        bool doSynch(Ptr<MpRdmaRxQueuePair> rxMpQp);
        void moveRcvWnd(Ptr<MpRdmaRxQueuePair> rxMpQp, uint32_t distance);
        Ptr<MpRdmaQueuePair> GetQp(uint32_t dip, uint16_t sport, uint16_t pg);
        Ptr<MpRdmaRxQueuePair> GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, bool create);
        uint32_t GetNicIdxOfRxQp(Ptr<MpRdmaRxQueuePair> q); // get the NIC index of the rxQp
        void AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport,
                          uint32_t win, uint32_t baseRtt, Callback<void> notifyAppFinish);
        uint32_t GetNicIdxOfQp(Ptr<MpRdmaQueuePair> qp);
        uint64_t GetQpKey(uint32_t dip, uint16_t dport, uint16_t pg);

        // qp complete callback
        typedef Callback<void, Ptr<MpRdmaQueuePair>> QpCompleteCallback;
        QpCompleteCallback m_qpCompleteCallback;

        void SetNode(Ptr<Node> node);
        void Setup(QpCompleteCallback cb); // setup shared data and callbacks with the QbbNetDevice

        void AddHeader(Ptr<Packet> p, uint16_t protocolNumber);
        uint16_t EtherToPpp(uint16_t proto);

        /**
         * set hyper parameters about MP-RDMA-HW
         */
        double m_alpha = 1.0;  // alpha for calculating synchronisation time
        uint32_t m_delta = 32; // out of order degree parameter

        std::unordered_map<uint64_t, Ptr<MpRdmaRxQueuePair>> m_rxMpQpMap; // mapping from uint64_t to rx qp
        std::unordered_map<uint64_t, Ptr<MpRdmaQueuePair>> m_MpQpMap;     // mapping from uint64_t to qp
        std::unordered_map<uint32_t, std::vector<int>> m_rtTable;         // map from ip address (u32) to possible ECMP port (index of dev)
        std::vector<MpRdmaInterfaceMgr> m_nic;                            // list of running nic controlled by this MpRdmaHw
        Ptr<Node> m_node;
        uint32_t m_mtu;
    };
} /* namespace ns3 */

#endif // MP_RDMA_HW_H
