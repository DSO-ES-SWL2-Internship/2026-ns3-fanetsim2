#ifndef GDT_APP_H
#define GDT_APP_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include <queue>

#include "ns3/fanet-application.h"
#include "ns3/FANETHeader.h"

namespace ns3
{
    /**
     * @class GDTApp
     * @brief An application that manages the operations in a GDT
     */
    class GDTApp : public FANETApplication
    {
        public:
            /// @brief Constructor of GDTApp
            GDTApp();
            /// @brief Destructor of GDTApp
            ~GDTApp();

            /**
             * @brief Configure the application with the port number to listen to
             * @param the port number used for communication
             */
            void SetPort(uint16_t port);

            void PrintCHTable();

            /// @brief Enable NS_LOG_INFO for this component
            void EnableInfoLog() override;
            
            /// @brief Enable NS_LOG_DEBUG for this component
            void EnableDebugLog() override;
            void SendCommand(Ipv4Address targetIp, uint16_t targetPort, std::string commandStr, Ptr<NetDevice> egressDevice = nullptr);

        private:

            std::map<uint32_t, uint32_t> m_clusterHeads;

            Ptr<Socket> m_cmdSocket;

            /// @brief Starts the application
            virtual void StartApplication() override;
            /// @brief Stop the application and close the socket
            virtual void StopApplication() override;

            /**
             * @brief Initialise the application
             * 
             * Creates the socket and bind it to m_port and set up the call back function
             * to handle incoming packets
             */
            virtual void DoInitialize() override;

            void RegisterHandlers() override;

            void HandleCHPromo(FANETHeader* header, Ptr<Packet> packet, Address from);
    };
}


#endif