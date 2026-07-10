#ifndef ADD_CLIENT_H
#define ADD_CLIENT_H

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


using namespace ns3;

namespace ns3
{
    /**
     * @class AddClient
     * @brief A custom network application that sends packets to a specified peer.
     * 
     * This application establishes a socket connection and sends data packets 
     * at randomized intervals, to ask the server to make a simple calculation and 
     * send the results back. 
     */
    class AddClient : public Application {

        private:
            /// @brief Socket for communication
            Ptr<Socket> m_socket;                
            /// @brief Destination address of peer
            Address m_peerAddress;               
            /// @brief Destination port address
            uint16_t m_peerPort;               
            EventId m_sendEvent;                 
            /// @brief Random variabe for inter-packet delays
            Ptr<UniformRandomVariable> m_randomTime;  
            /// @brief Random variable for packet data values
            Ptr<UniformRandomVariable> m_randomValue; 


            /**
             * @brief Send packet containing question to peer
             * 
             * This method generates the packet with a random addition question 
             * between 2 numbers and sends it to peers at random time
             */
            void SendPacket();

            /**
             * @brief Handles incoming packets from the socket
             * 
             * @param socket the socket receiving the packet
             * 
             * This function triggers when the socket receive the a packet 
             * and process and log out the result of the question
             */
            void HandleRead(Ptr<Socket> socket);



        public:
            /// @brief Constructor of AddClient Class
            AddClient();

            /**
             * @brief Configures the application with destination address and port
             * 
             * @param peerAddress Destination IP Address
             * @param peerPort Destination port number
             */
            void Setup(Address peerAddress, uint16_t peerPort);

            /**
             * @brief Starts the AddClient application.
             * 
             * This function initializes the UDP socket, binds it to a unique port,
             * connects to a peer address, sets up a callback to handle incoming packets,
             * and starts the packet transmission process.
             */
            void StartApplication() override;

            /**
             * @brief Stop the application
             * 
             * Close the socket
             */
            void StopApplication() override;

            /**
             * @brief Enable NS_LOG_INFO for this component
             */
            void EnableInfoLog();

            /**
             * @brief Enable NS_LOG_DEBUG for this component
             */
            void EnableDebugLog();

    };
}

#endif
