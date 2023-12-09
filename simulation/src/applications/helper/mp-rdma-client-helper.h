#ifndef MP_RDMA_CLIENT_SERVER_HELPER_H
#define MP_RDMA_CLIENT_SERVER_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include <ns3/mp-rdma-client.h>

namespace ns3
{

    /**
     * \brief Create a client application which does RDMA write
     */
    class MpRdmaClientHelper
    {

    public:
        /**
         * Create RdmaClientHelper which will make life easier for people trying
         * to set up simulations with udp-client-server.
         *
         */
        MpRdmaClientHelper();

        /**
         *  Create RdmaClientHelper which will make life easier for people trying
         * to set up simulations with udp-client-server.
         *
         * \param ip The IP address of the remote udp server
         * \param port The port number of the remote udp server
         */

        MpRdmaClientHelper(uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint64_t size, uint32_t win, uint64_t baseRtt);

        /**
         * Record an attribute to be set in each Application after it is is created.
         *
         * \param name the name of the attribute to set
         * \param value the value of the attribute to set
         */
        void SetAttribute(std::string name, const AttributeValue &value);

        /**
         * \param c the nodes
         *
         * Create one udp client application on each of the input nodes
         *
         * \returns the applications created, one application per input node.
         */
        ApplicationContainer Install(NodeContainer c);

    private:
        ObjectFactory m_factory;
    };

} // namespace ns3

#endif /* MP_RDMA_CLIENT_SERVER_HELPER_H */
