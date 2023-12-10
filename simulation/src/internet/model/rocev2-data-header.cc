#include "ns3/rocev2-data-header.h"
#include "ns3/simulator.h"

namespace ns3
{
    // 实现Rocev2DataHeader类的成员函数

    RoCEv2DataHeader::RoCEv2DataHeader() : m_synchronise(0),
                                           m_ReTx(0)
    {
    }

    void RoCEv2DataHeader::SetSynchronise(uint8_t sync)
    {
        m_synchronise = sync;
    }

    uint8_t RoCEv2DataHeader::GetSynchronise(void) const
    {
        return m_synchronise;
    }

    void RoCEv2DataHeader::SetReTx(uint8_t reTx)
    {
        m_ReTx = reTx;
    }

    uint8_t RoCEv2DataHeader::GetReTx(void) const
    {
        return m_ReTx;
    }

    TypeId RoCEv2DataHeader::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::RoCEv2DataHeader")
                                .SetParent<Header>()
                                .AddConstructor<RoCEv2DataHeader>();
        return tid;
    }

    TypeId RoCEv2DataHeader::GetInstanceTypeId(void) const
    {
        return GetTypeId();
    }

    void RoCEv2DataHeader::Print(std::ostream &os) const
    {
        // 打印函数的实现
        os << "synchronise: " << (uint32_t)m_synchronise << std::endl;
        os << "ReTx: " << (uint32_t)m_ReTx << std::endl;
    }

    uint32_t RoCEv2DataHeader::GetSerializedSize(void) const
    {
        return 2;
    }

    void RoCEv2DataHeader::Serialize(Buffer::Iterator start) const
    {
        start.WriteU8(m_synchronise);
        start.WriteU8(m_ReTx);
    }

    uint32_t RoCEv2DataHeader::Deserialize(Buffer::Iterator start)
    {
        m_synchronise = start.ReadU8();
        m_ReTx = start.ReadU8();
        return GetSerializedSize();
    }

} // namespace ns3