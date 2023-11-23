
#ifndef ROCEV2_HEADER_H
#define ROCEV2_HEADER_H

#include "ns3/header.h"

namespace ns3
{

    class RoCEv2Header : public Header
    {
    public:
        RoCEv2Header();

        // Add your additional methods and member variables here

    private:
        // Add your member variables here
        uint8_t synchronise;
        uint8_t ReTx;
    };

} // namespace ns3

#endif // ROCEV2_HEADER_H
