#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/random-variable.h"
#include "ns3/mp-qbb-net-device.h"
#include "ns3/ipv4-end-point.h"
#include "ns3/mp-rdma-client.h"
#include "ns3/seq-ts-header.h"
#include "ns3/mp-rdma-driver.h"
#include <stdlib.h>
#include <stdio.h>

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("MpRdmaClient");
    NS_OBJECT_ENSURE_REGISTERED(MpRdmaClient);

    TypeId
    MpRdmaClient::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::MpRdmaClient")
                                .SetParent<Application>()
                                .AddConstructor<MpRdmaClient>()
                                .AddAttribute("WriteSize",
                                              "The number of bytes to write",
                                              UintegerValue(10000),
                                              MakeUintegerAccessor(&MpRdmaClient::m_size),
                                              MakeUintegerChecker<uint64_t>())
                                .AddAttribute("SourceIP",
                                              "Source IP",
                                              Ipv4AddressValue("0.0.0.0"),
                                              MakeIpv4AddressAccessor(&MpRdmaClient::m_sip),
                                              MakeIpv4AddressChecker())
                                .AddAttribute("DestIP",
                                              "Dest IP",
                                              Ipv4AddressValue("0.0.0.0"),
                                              MakeIpv4AddressAccessor(&MpRdmaClient::m_dip),
                                              MakeIpv4AddressChecker())
                                .AddAttribute("SourcePort",
                                              "Source Port",
                                              UintegerValue(0),
                                              MakeUintegerAccessor(&MpRdmaClient::m_sport),
                                              MakeUintegerChecker<uint16_t>())
                                .AddAttribute("DestPort",
                                              "Dest Port",
                                              UintegerValue(0),
                                              MakeUintegerAccessor(&MpRdmaClient::m_dport),
                                              MakeUintegerChecker<uint16_t>())
                                .AddAttribute("PriorityGroup", "The priority group of this flow",
                                              UintegerValue(0),
                                              MakeUintegerAccessor(&MpRdmaClient::m_pg),
                                              MakeUintegerChecker<uint16_t>())
                                .AddAttribute("Window",
                                              "Bound of on-the-fly packets",
                                              UintegerValue(0),
                                              MakeUintegerAccessor(&MpRdmaClient::m_win),
                                              MakeUintegerChecker<uint32_t>())
                                .AddAttribute("BaseRtt",
                                              "Base Rtt",
                                              UintegerValue(0),
                                              MakeUintegerAccessor(&MpRdmaClient::m_baseRtt),
                                              MakeUintegerChecker<uint64_t>());
        return tid;
    }

    MpRdmaClient::MpRdmaClient()
    {
        NS_LOG_FUNCTION_NOARGS();
    }

    MpRdmaClient::~MpRdmaClient()
    {
        NS_LOG_FUNCTION_NOARGS();
    }

    void MpRdmaClient::SetRemote(Ipv4Address ip, uint16_t port)
    {
        m_dip = ip;
        m_dport = port;
    }

    void MpRdmaClient::SetLocal(Ipv4Address ip, uint16_t port)
    {
        m_sip = ip;
        m_sport = port;
    }

    void MpRdmaClient::SetPG(uint16_t pg)
    {
        m_pg = pg;
    }

    void MpRdmaClient::SetSize(uint64_t size)
    {
        m_size = size;
    }

    void MpRdmaClient::Finish()
    {
        m_node->DeleteApplication(this);
    }

    void MpRdmaClient::DoDispose(void)
    {
        NS_LOG_FUNCTION_NOARGS();
        Application::DoDispose();
    }

    void MpRdmaClient::StartApplication(void)
    {
        NS_LOG_FUNCTION_NOARGS();
        // get RDMA driver and add up queue pair
        Ptr<Node> node = GetNode();
        Ptr<MpRdmaDriver> rdma = node->GetObject<MpRdmaDriver>();
        rdma->AddQueuePair(m_size, m_pg, m_sip, m_dip, m_sport, m_dport, m_win, m_baseRtt, MakeCallback(&MpRdmaClient::Finish, this));
    }

    void MpRdmaClient::StopApplication()
    {
        NS_LOG_FUNCTION_NOARGS();
        // TODO stop the queue pair
    }

} // Namespace ns3
