#include "mp-rdma-driver.h"

namespace ns3
{
    /*************************************
     * MpRdmaDriver class implementation
     ************************************/
    TypeId MpRdmaDriver::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::MpRdmaDriver")
                                .SetParent<Object>()
                                .AddTraceSource("QpComplete",
                                                "A qp completes.",
                                                MakeTraceSourceAccessor(&MpRdmaDriver::m_traceQpComplete));
        return tid;
    }

    MpRdmaDriver::MpRdmaDriver()
    {
    }

    void MpRdmaDriver::Init(void)
    {
        for (uint32_t i = 0; i < m_node->GetNDevices(); i++)
        {
            Ptr<QbbNetDevice> dev = NULL;
            if (m_node->GetDevice(i)->IsQbb())
                dev = DynamicCast<QbbNetDevice>(m_node->GetDevice(i));
            m_rdma->m_nic.push_back(MpRdmaInterfaceMgr(dev));
            m_rdma->m_nic.back().qpGrp = CreateObject<MpRdmaQueuePairGroup>();
        }
        // RdmaHw do setup
        m_rdma->SetNode(m_node);
        m_rdma->Setup(MakeCallback(&MpRdmaDriver::QpComplete, this));
    }

    void MpRdmaDriver::SetNode(Ptr<Node> node)
    {
        m_node = node;
    }

    void MpRdmaDriver::SetRdmaHw(Ptr<MpRdmaHw> mpRdma)
    {
        m_rdma = mpRdma;
    }

    void MpRdmaDriver::AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint32_t win, uint64_t baseRtt, Callback<void> notifyAppFinish)
    {
        m_rdma->AddQueuePair(size, pg, sip, dip, sport, dport, win, baseRtt, notifyAppFinish);
    }

    void MpRdmaDriver::QpComplete(Ptr<MpRdmaQueuePair> q)
    {
        m_traceQpComplete(q);
    }
}