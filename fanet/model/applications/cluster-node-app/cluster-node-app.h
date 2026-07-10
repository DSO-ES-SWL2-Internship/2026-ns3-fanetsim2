#ifndef CLUSTER_NODE_PROMOTION_APP_H
#define CLUSTER_NODE_PROMOTION_APP_H

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
#include "ns3/FANETHeader.h"
#include "ns3/fanet-application.h"
#include "ns3/callback.h"

namespace ns3
{
    class ClusterNodeApp : public FANETApplication
    {
        /**
         * @class ClusterNodeApp
         * @brief FANET Application that handles cluster node operations
         */
        public:
            /// @brief Constructor for ClusterNodeApp
            ClusterNodeApp();
            /// @brief Destructor for ClusterNodeApp
            virtual ~ClusterNodeApp();

            /**
             * @brief Configures the application with GDT IP, port and cluster Index
             * @param gdtIp The IP address of the GDT
             * @param port The port number used for communication
             * @param clusterIndex The index of the cluster
             */
            void SetUp(Ipv4Address gdtIp, uint16_t port, uint32_t clusterIndex, Ipv4Address clusterBaseIP);

            /**
             * @brief Schedules the message to notify the GDT of the promotion to CH
             * @param isCH Boolean indicating whether the node is promoted to CH or not
             */
            void NotifyGDT(bool isCH);
            /// @brief Enable NS_LOG_INFO for this component
            void EnableInfoLog() override;
            /// @brief Enable NS_LOG_DEBUG for this component
            void EnableDebugLog() override;

            uint32_t GetClusterIndex();
            Ipv4Address GetClusterBaseIP();
            Ipv4Address GetClusterBroadcastIP();
            Ipv4Address GetGdtIp();

            bool GetCHStatus();

            typedef Callback<void, Ptr<Node>, Ptr<Packet>> CommandReceivedCallback;
            void SetCommandCallback(CommandReceivedCallback cb);
            void CommandCallBack(Ptr<Socket> socket);
            void SetupCommandSocket(uint16_t);

        private:
            /// @brief Flag indication if the node is a cluster head
            bool m_isClusterHead;
            /// @brief Event ID for scheduled message sending
            EventId m_sendEvent; 
            /// @brief Cluster index the node belongs to
            uint32_t m_clusterIndex;

            Ipv4Address m_clusterBaseIP;

            Ipv4Address m_gdtIp;

            /// @brief Starts the application
            virtual void StartApplication() override;

            /**
             * Stop the application
             */
            virtual void StopApplication() override;

            /**
             * @brief Initialise the application
             * 
             * Creates the socket and bind it to m_port and set up the call back function
             * to handle incoming packets
             */
            virtual void DoInitialize() override;

            void SendNotifyGdtMessage();

            void RegisterHandlers() override;

            void HandleCHPromo(FANETHeader* header, Ptr<Packet> packet, Address from);

            CommandReceivedCallback m_commandCallback;
            Ptr<Socket> m_cmdSocket;
    };
}

#endif