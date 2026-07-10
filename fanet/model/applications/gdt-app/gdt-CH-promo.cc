#include "gdt-app.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("GdtChPromo");

    void GDTApp::HandleCHPromo(FANETHeader* header, Ptr<Packet> packet, Address from)
    {
        uint8_t buffer[128] = {0};
        packet->CopyData(buffer, packet->GetSize());
        std::string msg((char*)buffer);

        uint32_t clusterId = header->GetClusterId();
        uint32_t nodeId = header->GetNodeId();

        m_clusterHeads[clusterId] = nodeId;

        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
            << ", GDT received notification from Cluster " << clusterId
            << " Node " << nodeId
            << " that it " << msg << " to CH");
    }

    void GDTApp::PrintCHTable()
    {
        NS_LOG_INFO("=== Cluster Head Table ===");
        for (const auto& ch : m_clusterHeads)
        {
            NS_LOG_INFO("Cluster " << ch.first << " → CH Node " << ch.second);
        }
    }
}