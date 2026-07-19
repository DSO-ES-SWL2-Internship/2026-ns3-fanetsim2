#ifndef ADD_SERVER_H
#define ADD_SERVER_H

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
#include "ns3/random-variable-stream.h"
#include <queue>

namespace ns3
{
    /**
     * @class AddServer
     * 
     * @brief Network Application that receive addition questions from clients
     * 
     * This application listens for incoming packets from clients, queue them for processing
     * where the app then add the 2 numbers and sends the result back to the client
     */
    class AddServer : public Application {
        private:
            /// @brief UDP socket for receiving and sending data
            Ptr<Socket> m_socket;

            /// @brief Queue to store received packets for processing
            std::queue<Ptr<Packet>> m_packetQueue;

            /// @brief Queue to store sender addresses associated with received packets
            std::queue<Address> m_addressQueue;

            /**
             * @brief Callback function to handle incoming packets
             * 
             * This functions triggers when packet is received, storing it in the queue
             * for further processing
             * 
             * @param socket Socket that received the packet
             */
            void HandleRead(Ptr<Socket> socket);

            /**
             * @brief Process the next packet in the queue
             * 
             * Extracts the next packet from te queue and performs the necessary 
             * operations on it
             */
            void ProcessNextPacket();


        public:

            /// @brief Constructor for AddServer
            AddServer() {}

            /**
             * @brief Starts the application
             * 
             *  Initializes the socket, binds it to a port and prepares the
             *  the application to receive packets 
             */
            void StartApplication() override;

            /**
             * @brief Stops the application
             * 
             * Close the socket
             */
            void StopApplication() override;

            /// @brief Enable NS_LOG_INFO for this component
            void EnableInfoLog();

            /// @brief Enable NS_LOG_DEBUG for this component
            void EnableDebugLog();
    };
}

#endif