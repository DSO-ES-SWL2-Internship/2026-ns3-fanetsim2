#include "add-server.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

namespace ns3
{
    NS_OBJECT_ENSURE_REGISTERED(AddServer);
    NS_LOG_COMPONENT_DEFINE("AddServer");
 
    // void AddServer::HandleRead(Ptr<Socket> socket) {
    //     Ptr<Packet> packet;
    //     Address from;
        
    //     // Keep reading until the socket buffer is empty
    //     while ((packet = socket->RecvFrom(from))) {
    //         if (packet->GetSize() == 0) break; // End of messages

    //         uint32_t data[3];  // Assuming the packet format is correct
    //         packet->CopyData(reinterpret_cast<uint8_t*>(data), sizeof(data));


    //         NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s, server received request from " << data[2] << ": " << data[0] << " + " << data[1] << " = ");
            
    //         uint32_t sum = data[0] + data[1];

    //         Ptr<Packet> response = Create<Packet>(reinterpret_cast<uint8_t*>(&sum), sizeof(sum));
    //         socket->SendTo(response, 0, from);
    //     }
    // }

    void AddServer::HandleRead(Ptr<Socket> socket)
    {
        Ptr<Packet> packet;
        Address from;

        // Push all incoming packets into a queue
        while ((packet = socket->RecvFrom(from)))
        {
            m_packetQueue.push(packet);
            m_addressQueue.push(from);
        }

        // Schedule processing of packets 
        if (!m_packetQueue.empty())
        {
            Simulator::ScheduleNow(&AddServer::ProcessNextPacket, this);
        }
    }

    void AddServer::ProcessNextPacket()
    {
        if (m_packetQueue.empty())
            return;

        Ptr<Packet> packet = m_packetQueue.front();
        Address from = m_addressQueue.front();
        m_packetQueue.pop();
        m_addressQueue.pop();

        uint32_t data[3];
        packet->CopyData(reinterpret_cast<uint8_t*> (data), sizeof(data));

        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s, server processing request from " << data[2] << ": " << data[0] << " + " << data[1] << " = ");

        uint32_t sum = data[0] + data[1];

        Ptr<Packet> response = Create<Packet>(reinterpret_cast<uint8_t*>(&sum), sizeof(sum));
        m_socket->SendTo(response, 0, from);

        if (!m_packetQueue.empty())
        {
            Simulator::ScheduleNow(&AddServer::ProcessNextPacket, this);
        }
    }


    void AddServer::StartApplication() {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 8080);
        m_socket->Bind(local);
        m_socket->SetRecvCallback(MakeCallback(&AddServer::HandleRead, this));
        m_socket->SetAttribute("RcvBufSize", UintegerValue(65536));
        //m_socket->SetAttribute("SndBufSize", UintegerValue(65536));

    }

    void AddServer::StopApplication() {
        if (m_socket) {
            m_socket->Close();
        }
    }

    void AddServer::EnableInfoLog()
    {
        LogComponentEnable("AddServer", LOG_LEVEL_INFO);
    }

    void AddServer::EnableDebugLog()
    {
        LogComponentEnable("AddServer", LOG_LEVEL_DEBUG);
    }
}