#include "cluster-node-app.h"
#include "ns3/fanet-communication.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("ClusterNodeCHPromo");

    void ClusterNodeApp::NotifyGDT(bool isCH)
    {
        m_isClusterHead = isCH;

        if (isCH)
        {
            Simulator::ScheduleNow(&ClusterNodeApp::SendNotifyGdtMessage, this);
            //SendMessage();
            // NS_LOG_DEBUG("Node " << GetNode()->GetId() << " scheduled to notify GDT of its promotion to CH");

        }     
        else
        {
            NS_LOG_DEBUG("Node " << GetNode()->GetId() << " demoted back to CH");
        }
    }

    void ClusterNodeApp::SendNotifyGdtMessage()
    {
        FANETHeader header;
        header.SetType(HELLO);
        header.SetClusterId(m_clusterIndex);
        header.SetNodeId(GetNode()->GetId());
        header.SetService(CH_PROMO);

        std::ostringstream message;
        message << "PROMOTED";


        Ptr<Packet> packet = Create<Packet>((uint8_t*) message.str().c_str(), message.str().length());

        packet->AddHeader(header);

        if (FANETCommunication::SendPacket(this, packet, m_destAddr) > 0)
        {
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() 
                << " Cluster " << m_clusterIndex 
                << " Node " << GetNode()->GetId() 
                << " scheduled to notify GDT of its promotion to CH");        
        }
        else
        {
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() 
                << " Cluster " << m_clusterIndex 
                << " Node " << GetNode()->GetId() 
                << " failed to notify GDT of its promotion to CH");     
        }
    }


    void ClusterNodeApp::HandleCHPromo(FANETHeader* header, Ptr<Packet> packet, Address from)
    {
        uint8_t buffer[128] = {0};
        packet->CopyData(buffer, packet->GetSize());
        std::string msg((char*)buffer);

        NS_LOG_DEBUG("Node " << GetNode()->GetId() << " application received CH status notification: " << msg);

        if (msg == "BECOME_CH")
        {
            NotifyGDT(true);
            m_isClusterHead = true;
        }
        else if (msg == "STOP_CH")
        {
            NotifyGDT(false);
            m_isClusterHead = false;
        }
    }

    bool ClusterNodeApp::GetCHStatus() { return m_isClusterHead; }
}