#include <ns3/log.h>
#include "ns3/rocev2-ack-header.h"

NS_LOG_COMPONENT_DEFINE("Rocev2AckHeader");

namespace ns3
{
    NS_OBJECT_ENSURE_REGISTERED(RoCEv2AckHeader);

    RoCEv2AckHeader::RoCEv2AckHeader() : m_ReTx(0),
                                         m_AACK(0)
    {
    }

    void RoCEv2AckHeader::SetReTx(uint8_t reTx)
    {
        m_ReTx = reTx;
    }

    uint8_t RoCEv2AckHeader::GetReTx(void) const
    {
        return m_ReTx;
    }

    void RoCEv2AckHeader::SetAACK(uint32_t aack)
    {
        m_AACK = aack;
    }

    uint32_t RoCEv2AckHeader::GetAACK(void) const
    {
        return m_AACK;
    }

    TypeId RoCEv2AckHeader::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::RoCEv2AckHeader")
                                .SetParent<Header>()
                                .AddConstructor<RoCEv2AckHeader>();
        return tid;
    }

    TypeId RoCEv2AckHeader::GetInstanceTypeId(void) const
    {
        return GetTypeId();
    }

    void RoCEv2AckHeader::Print(std::ostream &os) const
    {
        os << "ReTx: " << (uint32_t)m_ReTx << std::endl;
        os << "AACK: " << (uint32_t)m_AACK << std::endl;
    }

    uint32_t RoCEv2AckHeader::GetSerializedSize(void) const
    {
        return sizeof(m_ReTx) + sizeof(m_AACK);
    }

    void RoCEv2AckHeader::Serialize(Buffer::Iterator start) const
    {
        start.WriteU8(m_ReTx);
        start.WriteU32(m_AACK);
    }

    uint32_t RoCEv2AckHeader::Deserialize(Buffer::Iterator start)
    {
        m_ReTx = start.ReadU8();
        m_AACK = start.ReadU32();
        return GetSerializedSize();
    }

} // namespace ns3