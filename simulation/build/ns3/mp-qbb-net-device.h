#ifndef MP_QBB_NET_DEVICE_H
#define MP_QBB_NET_DEVICE_H

#include "ns3/point-to-point-net-device.h"
#include "mp-qbb-channel.h"
#include "ns3/event-id.h"
#include "ns3/broadcom-egress-queue.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include <ns3/mp-rdma-queue-pair.h>
#include <vector>
#include <map>
#include <ns3/rdma.h>

namespace ns3
{

    class MpRdmaEgressQueue : public Object
    {
    public:
        static const uint32_t qCnt = 8;
        static uint32_t ack_q_idx;
        int m_qlast;
        uint32_t m_rrlast;
        Ptr<DropTailQueue> m_ackQ;         // highest priority queue
        Ptr<MpRdmaQueuePairGroup> m_qpGrp; // queue pairs

        // callback for get next packet
        typedef Callback<Ptr<Packet>, Ptr<MpRdmaQueuePair>> RdmaGetNxtPkt;
        RdmaGetNxtPkt m_rdmaGetNxtPkt;

        static TypeId GetTypeId(void);
        MpRdmaEgressQueue();
        Ptr<Packet> DequeueQindex(int qIndex);
        int GetNextQindex(bool paused[]);
        int GetLastQueue();
        uint32_t GetNBytes(uint32_t qIndex);
        uint32_t GetFlowCount(void);
        Ptr<MpRdmaQueuePair> GetQp(uint32_t i);
        void RecoverQueue(uint32_t i);
        void EnqueueHighPrioQ(Ptr<Packet> p);
        void CleanHighPrio(TracedCallback<Ptr<const Packet>, uint32_t> dropCb);

        TracedCallback<Ptr<const Packet>, uint32_t> m_traceRdmaEnqueue;
        TracedCallback<Ptr<const Packet>, uint32_t> m_traceRdmaDequeue;
    };

    /**
     * \class QbbNetDevice
     * \brief A Device for a IEEE 802.1Qbb Network Link.
     */
    class MpQbbNetDevice : public PointToPointNetDevice
    {
    public:
        static const uint32_t qCnt = 8; // Number of queues/priorities used

        static TypeId GetTypeId(void);

        MpQbbNetDevice();
        virtual ~MpQbbNetDevice();

        /**
         * Receive a packet from a connected PointToPointChannel.
         *
         * This is to intercept the same call from the PointToPointNetDevice
         * so that the pause messages are honoured without letting
         * PointToPointNetDevice::Receive(p) know
         *
         * @see PointToPointNetDevice
         * @param p Ptr to the received packet.
         */
        virtual void Receive(Ptr<Packet> p);

        /**
         * Send a packet to the channel by putting it to the queue
         * of the corresponding priority class
         *
         * @param packet Ptr to the packet to send
         * @param dest Unused
         * @param protocolNumber Protocol used in packet
         */
        virtual bool Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
        virtual bool SwitchSend(uint32_t qIndex, Ptr<Packet> packet, CustomHeader &ch);

        /**
         * Get the size of Tx buffer available in the device
         *
         * @return buffer available in bytes
         */
        // virtual uint32_t GetTxAvailable(unsigned) const;

        /**
         * TracedCallback hooks
         */
        void ConnectWithoutContext(const CallbackBase &callback);
        void DisconnectWithoutContext(const CallbackBase &callback);

        bool Attach(Ptr<MpQbbChannel> ch);

        virtual Ptr<Channel> GetChannel(void) const;

        void SetQueue(Ptr<BEgressQueue> q);
        Ptr<BEgressQueue> GetQueue();
        virtual bool IsQbb(void) const;
        void NewQp(Ptr<MpRdmaQueuePair> qp);
        void ReassignedQp(Ptr<MpRdmaQueuePair> qp);
        void TriggerTransmit(void);

        void SendPfc(uint32_t qIndex, uint32_t type); // type: 0 = pause, 1 = resume

        TracedCallback<Ptr<const Packet>, uint32_t> m_traceEnqueue;
        TracedCallback<Ptr<const Packet>, uint32_t> m_traceDequeue;
        TracedCallback<Ptr<const Packet>, uint32_t> m_traceDrop;
        TracedCallback<uint32_t> m_tracePfc; // 0: resume, 1: pause
    protected:
        // Ptr<Node> m_node;

        bool TransmitStart(Ptr<Packet> p);

        virtual void DoDispose(void);

        /// Reset the channel into READY state and try transmit again
        virtual void TransmitComplete(void);

        /// Look for an available packet and send it using TransmitStart(p)
        virtual void DequeueAndTransmit(void);

        /// Resume a paused queue and call DequeueAndTransmit()
        virtual void Resume(unsigned qIndex);

        /**
         * The queues for each priority class.
         * @see class Queue
         * @see class InfiniteQueue
         */
        Ptr<BEgressQueue> m_queue;

        Ptr<MpQbbChannel> m_channel;

        // pfc
        bool m_qbbEnabled; //< PFC behaviour enabled
        bool m_qcnEnabled;
        bool m_dynamicth;
        uint32_t m_pausetime; //< Time for each Pause
        bool m_paused[qCnt];  //< Whether a queue paused

        // qcn

        /* RP parameters */
        EventId m_nextSend; //< The next send event
        /* State variable for rate-limited queues */

        // qcn

        struct ECNAccount
        {
            Ipv4Address source;
            uint32_t qIndex;
            uint32_t port;
            uint8_t ecnbits;
            uint16_t qfb;
            uint16_t total;
        };

        std::vector<ECNAccount> *m_ecn_source;

    public:
        Ptr<MpRdmaEgressQueue> m_rdmaEQ;
        void RdmaEnqueueHighPrioQ(Ptr<Packet> p);

        // callback for processing packet in RDMA
        typedef Callback<int, Ptr<Packet>, CustomHeader &> RdmaReceiveCb;
        RdmaReceiveCb m_rdmaReceiveCb;
        // callback for link down
        typedef Callback<void, Ptr<MpQbbNetDevice>> RdmaLinkDownCb;
        RdmaLinkDownCb m_rdmaLinkDownCb;
        // callback for sent a packet
        typedef Callback<void, Ptr<MpRdmaQueuePair>, Ptr<Packet>, Time> RdmaPktSent;
        RdmaPktSent m_rdmaPktSent;

        Ptr<MpRdmaEgressQueue> GetRdmaQueue();
        void TakeDown(); // take down this device
        void UpdateNextAvail(Time t);

        TracedCallback<Ptr<const Packet>, Ptr<MpRdmaQueuePair>> m_traceQpDequeue; // the trace for printing dequeue
    };

} // namespace ns3

#endif // MP_QBB_NET_DEVICE_H