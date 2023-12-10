
#ifndef ROCEV2_ACK_HEADER_H
#define ROCEV2_ACK_HEADER_H

#include "ns3/header.h"

namespace ns3
{

    /**
     * \ingroup internet
     * \brief RoCEv2 ACK Header
     *
     * This class represents the RoCEv2 ACK header.
     * It inherits from the ns3::Header class.
     */
    class RoCEv2AckHeader : public Header
    {
    public:
        RoCEv2AckHeader();

        void SetReTx(uint8_t reTx);
        uint8_t GetReTx(void) const;

        void SetAACK(uint32_t aack);
        uint32_t GetAACK(void) const;

        static TypeId GetTypeId(void);
        virtual TypeId GetInstanceTypeId(void) const;
        virtual void Print(std::ostream &os) const;
        virtual uint32_t GetSerializedSize(void) const;

    private:
        virtual void Serialize(Buffer::Iterator start) const;
        virtual uint32_t Deserialize(Buffer::Iterator start);

        uint8_t m_ReTx;
        uint32_t m_AACK;
    };

} // namespace ns3

#endif // ROCEV2_ACK_HEADER_H
