#include "gdt-app.h"
#include "ns3/fanet-communication.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 
{
    NS_LOG_COMPONENT_DEFINE("GDTApp");

    GDTApp::GDTApp()
    {
    }

    GDTApp::~GDTApp()
    {
        if (m_socket)
        {
            m_socket->Close();
            m_socket = nullptr;
        }
        if (m_cmdSocket)
        {
            m_cmdSocket->Close();
            m_cmdSocket = nullptr;
        }
    }

    void GDTApp::SetPort(uint16_t port)
    {
        m_port = port;
    }

    void GDTApp::StartApplication()
    {
        NS_LOG_DEBUG("GDT App started on GDT");
        //Simulator::Schedule(Seconds(19), &GDTApp::PrintCHTable, this);
    }

    void GDTApp::StopApplication()
    {
        if (m_socket) 
        {
            m_socket->Close();
            m_socket = nullptr;
        }
        if (m_cmdSocket)
        {
            m_cmdSocket->Close();
            m_cmdSocket = nullptr;
        }
    }

    void GDTApp::DoInitialize()
    {
        if (!m_socket)
        {
            RegisterHandlers();
        }

        FANETApplication::DoInitialize();
    }

    void GDTApp::RegisterHandlers()
    {
        FANETApplication::RegisterHandlers();

        helloServiceHandlers[CH_PROMO] = [this] (FANETHeader* header, Ptr<Packet> packet, Address from) { HandleCHPromo(header, packet, from); };
    }

    void GDTApp::SendCommand(Ipv4Address targetIp, uint16_t targetPort, std::string commandStr, Ptr<NetDevice> egressDevice)
    {
        //Create and bind the socket only if it doesn't exist yet by checking if m_cmdSocket is nullptr. 
        //This ensures that the socket is created only once and reused for subsequent command sends.
        if (!m_cmdSocket) {
            m_cmdSocket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());

            m_cmdSocket->SetIpTos(0x11);
            
            // //Apply SO_BINDTODEVICE once
            // if (egressDevice != nullptr) {
            //     m_cmdSocket->BindToNetDevice(egressDevice);
            //     NS_LOG_INFO("Command socket bound to physical interface ID: " << egressDevice->GetIfIndex());
            // }
            m_cmdSocket->Bind();
            std::cout << "[GDT DISPATCH] Time: " << Simulator::Now().As(Time::S) << std::endl;
            std::cout << "[GDT DISPATCH] GDT fired command [" << commandStr << "] directly to IP: " << targetIp << std::endl;
        }

        //Creating the payload
        //This allows the single socket to send commands to any drone dynamically
        Ptr<Packet> packet = Create<Packet>((uint8_t*)commandStr.c_str(), commandStr.length());
        m_cmdSocket->SendTo(packet, 0, InetSocketAddress(targetIp, targetPort));

        NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s, GDT dispatched command [" 
                    << commandStr << "] directly to " << targetIp);
    }

    void GDTApp::EnableInfoLog()
    {
        LogComponentEnable("GDTApp", LOG_LEVEL_INFO);
        LogComponentDisable("GDTApp", LOG_LEVEL_DEBUG);
        LogComponentEnable("GdtChPromo", LOG_LEVEL_INFO);
    }

    void GDTApp::EnableDebugLog()
    {
        LogComponentEnable("GDTApp", LOG_LEVEL_DEBUG);
        LogComponentEnable("GdtChPromo", LOG_LEVEL_DEBUG);
    }
}