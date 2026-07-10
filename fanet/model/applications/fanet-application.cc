#include "fanet-application.h"
#include "ns3/log.h"
#include "ns3/fanet-communication.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"

namespace ns3 
{
    NS_LOG_COMPONENT_DEFINE("FANETApplication");

    FANETApplication::FANETApplication()
        : m_plrManager(std::make_unique<PLRManager>()),
        m_socket(nullptr), 
        m_destAddr(Ipv4Address("0.0.0.0")), 
        m_port(8080)
        
    {}


    FANETApplication::~FANETApplication(){}

    void FANETApplication::SetDestAddr(Ipv4Address destAddr) { m_destAddr = destAddr; }

    void FANETApplication::SetPort(uint16_t port) { m_port = port; }

    Ptr<Socket> FANETApplication::GetSocket() { return m_socket; }

    Ipv4Address FANETApplication::GetDestAddr() { return m_destAddr; }

    uint16_t FANETApplication::GetPort() { return m_port; }

    std::queue<Ptr<Packet>>& FANETApplication::GetPacketQueue() { return m_packetQueue; }

    std::queue<Address>& FANETApplication::GetAddressQueue() { return m_addressQueue; }

    void FANETApplication::ProcessNextPacket()
    {
        if (m_packetQueue.empty())
            return;

        Ptr<Packet> packet = m_packetQueue.front();
        Address addr = m_addressQueue.front();
        m_packetQueue.pop();
        m_addressQueue.pop();

        FANETHeader header;
        packet->RemoveHeader(header);

        // Use the dynamically registered handlers
        switch (header.GetType())
        {
            case HELLO:
                ProcessHelloPacket(&header, packet, addr);
                break;
            case DATA:
                ProcessDataPacket(&header, packet, addr);
                break;
            case REQUEST:
                ProcessRequestPacket(&header, packet, addr);
                break;
            case RESPONSE:
                ProcessResponsePacket(&header, packet, addr);
                break;
            default:
                NS_LOG_WARN("Unknown packet type received!");
                return;
        }

        if (!m_packetQueue.empty()) {
            Simulator::ScheduleNow(&FANETApplication::ProcessNextPacket, this);
        }
    }

    void FANETApplication::ProcessHelloPacket(FANETHeader* header, Ptr<Packet> packet, Address from) {
        auto it = helloServiceHandlers.find(header->GetService());
        if (it != helloServiceHandlers.end()) {
            it->second(header, packet, from);  
        } else {
            NS_LOG_DEBUG("No Hello handler registered for service type: " << header->GetService());
        }
    }

    void FANETApplication::ProcessDataPacket(FANETHeader* header, Ptr<Packet> packet, Address from) {
        auto it = dataServiceHandlers.find(header->GetService());
        if (it != dataServiceHandlers.end()) {
            it->second(header, packet, from);
        } else {
            NS_LOG_DEBUG("No Data handler registered for service type: " << header->GetService());
        }
    }

    void FANETApplication::ProcessRequestPacket(FANETHeader* header, Ptr<Packet> packet, Address from) {
        auto it = requestServiceHandlers.find(header->GetService());
        if (it != requestServiceHandlers.end()) {
            it->second(header, packet, from);
        } else {
            NS_LOG_DEBUG("No Request handler registered for service type: " << header->GetService());
        }
    }

    void FANETApplication::ProcessResponsePacket(FANETHeader* header, Ptr<Packet> packet, Address from) {
        auto it = responseServiceHandlers.find(header->GetService());
        if (it != responseServiceHandlers.end()) {
            it->second(header, packet, from);
        } else {
            NS_LOG_DEBUG("No Response handler registered for service type: " << header->GetService());
        }
    }

    void FANETApplication::EnableInfoLog()
    {
        LogComponentEnable("FANETPlr", LOG_LEVEL_INFO);
        LogComponentEnable("FANETApplication", LOG_LEVEL_INFO);
    }

    void FANETApplication::EnableDebugLog()
    {
        LogComponentEnable("FANETPlr", LOG_LEVEL_DEBUG);
        LogComponentEnable("FANETApplication", LOG_LEVEL_DEBUG);
    }

    void FANETApplication::RegisterHandlers()
    {
        dataServiceHandlers[PLR] = [this] (FANETHeader* header, Ptr<Packet> packet, Address from) { m_plrManager->HandleData(this, header, packet, from); };
        requestServiceHandlers[PLR] = [this] (FANETHeader* header, Ptr<Packet> packet, Address from) { m_plrManager->HandleRequest(this, header, packet, from ); };
        responseServiceHandlers[PLR] = [this] (FANETHeader* header, Ptr<Packet> packet, Address from) { m_plrManager->HandleResponse(this, header, packet, from ); };
    }

    void FANETApplication::HandleRead(Ptr<Socket> socket)
    {
        FANETCommunication::ReceivePacket(this, socket);
    }

    void FANETApplication::DoInitialize()
    {
        if (!m_socket)
        {
            m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
            InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
            m_socket->Bind(local);
            m_socket->SetAllowBroadcast(true);
            m_socket->SetRecvCallback(MakeCallback(&FANETApplication::HandleRead, this));
            m_socket->SetAttribute("RcvBufSize", UintegerValue(65536));
        }

        Application::DoInitialize();
    }
}