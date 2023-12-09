#include "ns3/mp-rdma-client-helper.h"
#include "ns3/mp-rdma-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3
{

    MpRdmaClientHelper::MpRdmaClientHelper()
    {
    }

    MpRdmaClientHelper::MpRdmaClientHelper(uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint64_t size, uint32_t win, uint64_t baseRtt)
    {
        m_factory.SetTypeId(MpRdmaClient::GetTypeId());
        SetAttribute("PriorityGroup", UintegerValue(pg));
        SetAttribute("SourceIP", Ipv4AddressValue(sip));
        SetAttribute("DestIP", Ipv4AddressValue(dip));
        SetAttribute("SourcePort", UintegerValue(sport));
        SetAttribute("DestPort", UintegerValue(dport));
        SetAttribute("WriteSize", UintegerValue(size));
        SetAttribute("Window", UintegerValue(win));
        SetAttribute("BaseRtt", UintegerValue(baseRtt));
    }

    void MpRdmaClientHelper::SetAttribute(std::string name, const AttributeValue &value)
    {
        m_factory.Set(name, value);
    }

    ApplicationContainer MpRdmaClientHelper::Install(NodeContainer c)
    {
        ApplicationContainer apps;
        for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
        {
            Ptr<Node> node = *i;
            Ptr<MpRdmaClient> client = m_factory.Create<MpRdmaClient>();
            node->AddApplication(client);
            apps.Add(client);
        }
        return apps;
    }

} // namespace ns3
