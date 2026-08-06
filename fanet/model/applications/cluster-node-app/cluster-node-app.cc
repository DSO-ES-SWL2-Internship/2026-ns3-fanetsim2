#include "cluster-node-app.h"
#include "ns3/fanet-communication.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("ClusterNodeApp");

    ClusterNodeApp::ClusterNodeApp()
        : m_isClusterHead(false),
        m_clusterBaseIP("0.0.0.0")
    {

    }

    ClusterNodeApp::~ClusterNodeApp()
    {
        if (m_socket)
        {
            m_socket->Close();
            m_socket = nullptr;
        }
    }

    void ClusterNodeApp::SetUp(Ipv4Address gdtIp, uint16_t port, uint32_t clusterIndex, Ipv4Address clusterBaseIP)
    {
        m_destAddr = gdtIp;
        m_gdtIp = gdtIp;
        m_port = port;
        m_clusterIndex = clusterIndex;
        m_clusterBaseIP = clusterBaseIP;
    }

    void ClusterNodeApp::StartApplication()
    {
        uint16_t commandPort = 10000; 
        SetupCommandSocket(commandPort);
        NS_LOG_DEBUG("Node " << GetNode()->GetId() << " application to notify gdt that it became clusterhead started");
    }

    void ClusterNodeApp::StopApplication()
    {
        if (m_socket)
        {
            m_socket->Close();
            m_socket = nullptr;
        }
        // Safely close the command socket when the simulation ends
        if (m_cmdSocket)
        {
            m_cmdSocket->Close();
            m_cmdSocket = nullptr;
        }
    }

    void ClusterNodeApp::DoInitialize()
    {
        if (!m_socket)
        {
            RegisterHandlers();
        }

        FANETApplication::DoInitialize();
    }

    void ClusterNodeApp::RegisterHandlers() 
    {
        FANETApplication::RegisterHandlers();

        helloServiceHandlers[CH_PROMO] = [this] (FANETHeader* header, Ptr<Packet> packet, Address from ) { HandleCHPromo(header, packet, from); };
    }

    void ClusterNodeApp::EnableInfoLog()
    {
        FANETApplication::EnableInfoLog();
        LogComponentEnable("ClusterNodeApp", LOG_LEVEL_INFO);
        LogComponentEnable("ClusterNodeCHPromo", LOG_LEVEL_INFO);
    }

    void ClusterNodeApp::EnableDebugLog()
    {
        FANETApplication::EnableDebugLog();
        LogComponentEnable("ClusterNodeApp", LOG_LEVEL_DEBUG);
        LogComponentEnable("ClusterNodeCHPromo", LOG_LEVEL_DEBUG);
    }

    uint32_t ClusterNodeApp::GetClusterIndex() { return m_clusterIndex; }

    Ipv4Address ClusterNodeApp::GetClusterBaseIP() { return  m_clusterBaseIP; }

    Ipv4Address ClusterNodeApp::GetClusterBroadcastIP() { return Ipv4Address( m_clusterBaseIP.Get() | 0x000000FF); }

    Ipv4Address ClusterNodeApp::GetGdtIp() { return m_gdtIp; }

    void ClusterNodeApp::SetupCommandSocket(uint16_t port)
    {
        // Only create the socket if it doesn't already exist
        if (m_cmdSocket == nullptr) {
            m_cmdSocket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
            m_cmdSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), port));
            m_cmdSocket->SetRecvCallback(MakeCallback(&ClusterNodeApp::CommandCallBack, this));
            NS_LOG_INFO("ClusterNodeApp Command Socket listening on port " << port);
        }
    }

    void ClusterNodeApp::SetCommandCallback(CommandReceivedCallback cb)
    {
        m_commandCallback = cb;
    }

    void ClusterNodeApp::CommandCallBack(Ptr<Socket> socket)
    {
        Ptr<Packet> packet;
        Address from;
        while ((packet = socket->RecvFrom(from)))
        {
            std::cout << "[DEBUG] CommandCallBack: Socket event triggered." << std::endl;
            // Get the Node safely
            Ptr<Node> rxNode = socket->GetNode();

            std::cout << "\n[NODE LISTENER] Time: " << Simulator::Now().As(Time::S) << "s" << std::endl;
            std::cout << "[NODE LISTENER] Node " << rxNode->GetId() << " received command packet at App layer. Firing callback to Simulator..." << std::endl;

            // Fire the callback to the Simulator to handle the TDMA logic
            if (!m_commandCallback.IsNull()) {
                m_commandCallback(rxNode, packet);
            } else {
                NS_LOG_WARN("Command received, but no callback is hooked up to the Simulator!");
            }

            // Send the confirmation reply back to the GDT
            InetSocketAddress senderInetAddr = InetSocketAddress::ConvertFrom(from);
            Ipv4Address gdtIp = senderInetAddr.GetIpv4();
            
            std::cout << "[NODE LISTENER] Sending requested data back to GDT IP: " << gdtIp << std::endl;
            
            Ptr<Socket> replySocket = Socket::CreateSocket(rxNode, UdpSocketFactory::GetTypeId());
            replySocket->Connect(InetSocketAddress(gdtIp, 9999));
            Ptr<Packet> replyData = Create<Packet>((uint8_t*)"VIDEO_DATA_REPLY", 16);
            replySocket->Send(replyData);
        }
    }

}