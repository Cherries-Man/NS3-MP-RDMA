
#ifndef ROCEV2_HEADER_H
#define ROCEV2_HEADER_H

#include "ns3/header.h"

namespace ns3
{

    class RoCEv2Header : public Header
    {
    public:
        RoCEv2Header();

        // Override the methods from Header class
        virtual uint32_t GetSerializedSize() const;
        virtual void Serialize(Buffer::Iterator start) const;
        virtual uint32_t Deserialize(Buffer::Iterator start);

        // Add your additional methods and member variables here

    private:
        // Add your member variables here
    };

} // namespace ns3

#endif // ROCEV2_HEADER_H
