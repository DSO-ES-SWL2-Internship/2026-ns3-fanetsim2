#include "ns3/fanet-plr.h"
#include "fanet-application.h"
#include "ns3/fanet-communication.h"
#include "ns3/simulator.h"
#include <cstring>
#include "ns3/gdt-app.h"
#include "ns3/cluster-node-app.h"
#include "ns3/ipv4.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("FANETPlr");

    PLRManager::PLRManager()
    {

    }

    PLRManager::~PLRManager()
    {

    }

    void PLRManager::Setup(uint32_t nNodes)
    {
        m_sequenceNumberPLR.assign(nNodes, 0);
        m_expectedSeqPLR.assign(nNodes, 0);
        m_receivedPacketsPLR.assign(nNodes, 0);
        m_lostPacketsPLR.assign(nNodes, 0);
        m_packetsSentPLR.assign(nNodes, 0);
    }

    void PLRManager::StartTest(Ptr<FANETApplication> app, double startTime, uint32_t nodeId, Ipv4Address destAddress, uint32_t pktsToSend, double interval)
    {
        Simulator::Schedule(Seconds(startTime), &PLRManager::SendData, this, app, nodeId, destAddress, pktsToSend, interval);
    }

    void PLRManager::HandleData(Ptr<FANETApplication> app, FANETHeader* header, Ptr<Packet> packet, Address from)
    {
        if (HasReceivedNetworkBroadcast(app, header)) return;

        uint32_t seqNum;
        packet->CopyData((uint8_t*) &seqNum, sizeof(uint32_t));

        uint32_t nodeId = header->GetNodeId();

        // handling for correct sequence of plr packets
        if (seqNum >= m_expectedSeqPLR[nodeId])
        {
            m_lostPacketsPLR[nodeId] += (seqNum - m_expectedSeqPLR[nodeId]);
            m_receivedPacketsPLR[nodeId]++;
            m_expectedSeqPLR[nodeId] = seqNum + 1;
        } 
        // handling for incorrect sequence of plr packets e.g. receive seq 2->3->4->1
        //if this occured, it means that the other node has scheduled a new PLR test
        else 
        {
            NS_LOG_INFO("New PLR test detected. Resetting counters.");
            m_lostPacketsPLR[nodeId] = seqNum == 0 ? 0 : seqNum;  //reset and account for the packets that could have been lost at the start of new test
            m_receivedPacketsPLR[nodeId] = 1;  // Start counting received packets
        }
            
        


        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s, Node " 
            << app->GetNode()->GetId() << " received PLR packet from Node " << header->GetNodeId() << " (Seq no. = " << seqNum << ") . Current PLR: " << GetPLR(nodeId) * 100 << "%");

        HandleNetworkBroadcast(app, header, packet, from);
    }

    void PLRManager::HandleRequest(Ptr<FANETApplication> app, FANETHeader* header, Ptr<Packet> packet, Address from)
    {
        if (HasReceivedNetworkBroadcast(app, header)) return;

        Ipv4Address senderIp;
        if (header->GetIsBroadcast())
        {
            if (!header->GetIsBroadcastForwarding())
                senderIp = InetSocketAddress::ConvertFrom(from).GetIpv4();
            else senderIp = header->GetBroadCastFrom();
        }
        else
            senderIp = InetSocketAddress::ConvertFrom(from).GetIpv4();
        
        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() 
            << "s, Node " << app->GetNode()->GetId() 
            << " received PLR request from Node " << header->GetNodeId());
        
        SendResponse(app, header->GetNodeId(), senderIp);

        HandleNetworkBroadcast(app, header, packet, from);
    }

    void PLRManager::HandleResponse(Ptr<FANETApplication> app, FANETHeader* header, Ptr<Packet> packet, Address from)
    {
        double plr;
        packet->CopyData(reinterpret_cast<uint8_t*>(&plr), sizeof(plr));

        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s, Node " 
            << app->GetNode()->GetId() << " received PLR from Node " << header->GetNodeId() << ". PLR: " << plr * 100.0 << "%");
    }

    void PLRManager::SendRequest(Ptr<FANETApplication> app, uint32_t nodeId, Ipv4Address destAddress)
    {
        FANETHeader header;
        header.SetType(REQUEST);
        header.SetService(PLR);
        if (Ptr<GDTApp> gdtapp = DynamicCast<GDTApp>(app)) header.SetClusterId(9999);
        else if (Ptr<ClusterNodeApp> clusterNodeApp = DynamicCast<ClusterNodeApp>(app)) header.SetClusterId(clusterNodeApp->GetClusterIndex());
        header.SetNodeId(app->GetNode()->GetId());
        if (destAddress.IsBroadcast()) {
            header.SetIsBroadcast(true);
            Ptr<Ipv4> ipv4 = app->GetNode()->GetObject<Ipv4>();
            header.SetBroadcastFrom(ipv4->GetAddress(1, 0).GetLocal());
            if (Ptr<GDTApp> gdtapp = DynamicCast<GDTApp>(app)) header.SetIsBroadcastForwarding(true);
        }

        Ptr<Packet> packet = Create<Packet>();
        packet->AddHeader(header);

        FANETCommunication::SendPacket(app, packet, destAddress);
        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() 
            << "s, Node " << app->GetNode()->GetId() 
            << " sent PLR request to " << nodeId);
    }

    double PLRManager::GetPLR(uint32_t nodeId) { return (double) m_lostPacketsPLR[nodeId] / (m_receivedPacketsPLR[nodeId] + m_lostPacketsPLR[nodeId]); }

    void PLRManager::SendData(Ptr<FANETApplication> app, uint32_t nodeId, Ipv4Address destAddress, uint32_t pktsToSend, double interval)
    {
        if (m_packetsSentPLR[nodeId] < pktsToSend)
        {
            FANETHeader header;
            header.SetType(DATA);
            header.SetService(PLR);
            if (Ptr<GDTApp> gdtapp = DynamicCast<GDTApp>(app)) header.SetClusterId(9999);
            else if (Ptr<ClusterNodeApp> clusterNodeApp = DynamicCast<ClusterNodeApp>(app)) header.SetClusterId(clusterNodeApp->GetClusterIndex());
            header.SetNodeId(app->GetNode()->GetId());
            if (destAddress.IsBroadcast()) {
                header.SetIsBroadcast(true);
                Ptr<Ipv4> ipv4 = app->GetNode()->GetObject<Ipv4>();
                header.SetBroadcastFrom(ipv4->GetAddress(1, 0).GetLocal());
                if (Ptr<GDTApp> gdtapp = DynamicCast<GDTApp>(app)) header.SetIsBroadcastForwarding(true);
            }

            Ptr<Packet> packet = Create<Packet>((uint8_t*) &m_sequenceNumberPLR[nodeId], sizeof(uint32_t));
            packet->AddHeader(header);
            
            FANETCommunication::SendPacket(app, packet, destAddress);
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s, Node " 
                << app->GetNode()->GetId() << " sent PLR packet to destination (Seq No. = " 
                << m_sequenceNumberPLR[nodeId] << ") Node " << nodeId);

            m_sequenceNumberPLR[nodeId]++;
            m_packetsSentPLR[nodeId]++;

            Simulator::Schedule(Seconds(interval), &PLRManager::SendData, this, app, nodeId, destAddress, pktsToSend, interval);
        } 
        else 
        {
            m_packetsSentPLR[nodeId] = 0;
            NS_LOG_DEBUG("All PLR packets have been sent");
            SendRequest(app, nodeId, destAddress);
        }
    }

    void PLRManager::SendResponse(Ptr<FANETApplication> app, uint32_t nodeId, Ipv4Address destAddress)
    {
        FANETHeader header;
        header.SetType(RESPONSE);
        header.SetService(PLR);
        if (Ptr<GDTApp> gdtapp = DynamicCast<GDTApp>(app)) header.SetClusterId(9999);
        else if (Ptr<ClusterNodeApp> clusterNodeApp = DynamicCast<ClusterNodeApp>(app)) header.SetClusterId(clusterNodeApp->GetClusterIndex());
        header.SetNodeId(app->GetNode()->GetId());

        double plr = GetPLR(nodeId);
        Ptr<Packet> packet = Create<Packet>((uint8_t*) &plr, sizeof(double));
        packet->AddHeader(header);
        
        FANETCommunication::SendPacket(app, packet, destAddress);
        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() 
            << "s, Node " << app->GetNode()->GetId() 
            << " sent PLR response to Node " << nodeId);
    
        m_expectedSeqPLR[nodeId] = 0;
        m_lostPacketsPLR[nodeId] = 0;
    }

    bool PLRManager::HasReceivedNetworkBroadcast(Ptr<FANETApplication> app, FANETHeader* header)
    {
        // if the application is GDTApp, check if the packet have past through the gdt before
        // if the packet have past through before perform no further actions
        if (Ptr<GDTApp> gdtApp = DynamicCast<GDTApp>(app))
            if (header->GetIsBroadcastForwarding()) return true;
        // if the application is ClusterNodeApp,
        if (Ptr<ClusterNodeApp> clusterNodeApp = DynamicCast<ClusterNodeApp>(app))
        {
            // check if this packet is forwarding a broadcast and if the sender of the broadcast came from a node in the same cluster
            // if both conditions holds, it means that this cluster have already received the broadcast locally and do not need to receive it again
            if (header->GetIsBroadcastForwarding() && header->GetClusterId() == clusterNodeApp->GetClusterIndex()) return true;
        }


        return false;
    }

    void PLRManager::HandleNetworkBroadcast(Ptr<FANETApplication> app, FANETHeader* header, Ptr<Packet> packet, Address from)
    {
        // if not a network broadcast no further action is required
        if (!header->GetIsBroadcast()) return;
        // if app is an GDTApp set the BroadcastFowarding flag to true
        if (Ptr<GDTApp> gdtApp = DynamicCast<GDTApp>(app))
        {
            header->SetIsBroadcastForwarding(true); 
            //header->SetBroadcastFrom(InetSocketAddress::ConvertFrom(from).GetIpv4());
            //NS_LOG_UNCOND("Broacasting...");
        }
            
        // if GDTApp, it forward the broadcast to all the cluster heads by broadcasting
        // if ClusterNodeApp and the node is a cluster head, execute the broadcast in its cluster
        if (Ptr<ClusterNodeApp> clusterNodeApp = DynamicCast<ClusterNodeApp>(app)){
            if (!clusterNodeApp->GetCHStatus()) return;
            else 
            {
                if (!header->GetIsBroadcastForwarding())
                {
                    //header->SetIsBroadcastForwarding(true);
                    //header->SetBroadcastFrom(InetSocketAddress::ConvertFrom(from).GetIpv4());
                    packet->AddHeader(*header);
                    FANETCommunication::SendPacket(app, packet, clusterNodeApp->GetGdtIp());
                    return;
                }
            }
        }
        packet->AddHeader(*header);
        FANETCommunication::SendPacket(app, packet, Ipv4Address("255.255.255.255"));
    }
}