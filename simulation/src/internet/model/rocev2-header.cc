#include "rocev2-header.h"
#include "ns3/simulator.h"

namespace ns3
{
    // 实现Rocev2Header类的成员函数

    RoCEv2Header::RoCEv2Header() : m_synchronise(0),
                                   m_ReTx(0)
    {
    }

    void RoCEv2Header::SetSynchronise(uint8_t sync)
    {
        m_synchronise = sync;
    }

    uint8_t RoCEv2Header::GetSynchronise(void) const
    {
        return m_synchronise;
    }

    void RoCEv2Header::SetReTx(uint8_t reTx)
    {
        m_ReTx = reTx;
    }

    uint8_t RoCEv2Header::GetReTx(void) const
    {
        return m_ReTx;
    }

    TypeId RoCEv2Header::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::RoCEv2Header")
                                .SetParent<Header>()
                                .AddConstructor<RoCEv2Header>();
        return tid;
    }

    TypeId RoCEv2Header::GetInstanceTypeId(void) const
    {
        return GetTypeId();
    }

    void RoCEv2Header::Print(std::ostream &os) const
    {
        // 打印函数的实现
        os << "synchronise: " << (uint32_t)m_synchronise << std::endl;
        os << "ReTx: " << (uint32_t)m_ReTx << std::endl;
    }

    uint32_t RoCEv2Header::GetSerializedSize(void) const
    {
        return 2;
    }

    void RoCEv2Header::Serialize(Buffer::Iterator start) const
    {
        start.WriteU8(m_synchronise);
        start.WriteU8(m_ReTx);
    }

    uint32_t RoCEv2Header::Deserialize(Buffer::Iterator start)
    {
        m_synchronise = start.ReadU8();
        m_ReTx = start.ReadU8();
        return GetSerializedSize();
    }

} // namespace ns3