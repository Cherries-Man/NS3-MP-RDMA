
#ifndef ROCEV2_HEADER_H
#define ROCEV2_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3
{

    class RoCEv2Header : public Header
    {
    public:
        RoCEv2Header();

        /**
         * \param sync the sync signal to the receiver
         */
        void SetSynchronise(uint8_t sync);
        uint8_t GetSynchronise(void) const;

        /**
         * \param reTx the retransmission signal from the receiver
         */
        void SetReTx(uint8_t reTx);
        uint8_t GetReTx(void) const;

        static TypeId GetTypeId(void);
        virtual TypeId GetInstanceTypeId(void) const;
        virtual void Print(std::ostream &os) const;
        /**
         * \return Returns the serialized size
         */
        virtual uint32_t GetSerializedSize(void) const;

    private:
        virtual void Serialize(Buffer::Iterator start) const;
        virtual uint32_t Deserialize(Buffer::Iterator start);

        uint8_t m_synchronise;
        uint8_t m_ReTx;
    };

} // namespace ns3

#endif // ROCEV2_HEADER_H
