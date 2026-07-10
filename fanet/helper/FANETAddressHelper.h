#ifndef FANET_ADDRESS_HELPER_H
#define FANET_ADDRESS_HELPER_H

#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-global-routing-helper.h"


#include <cstdint>
#include <vector>

#define NETWORK_BROADCAST Ipv4Address("255.255.255.255")

namespace ns3
{
    /**
     * @class FANETAddressHelper
     * @brief Manages the allocation of IP addresses for FANET networks.
     */
    class FANETAddressHelper : public Object {
        private:
            /// @brief Base network address
            Ipv4Address network;
            /// @brief Subnet mask
            Ipv4Mask mask;
            /// @brief NS-3 IPv4 Address Helper
            Ipv4AddressHelper ipv4;

            /**
             * @brief Increments the network address for the next subnet
             */
            void IncrementNetwork();                                           
            
        public:
            /// @brief Interface container for the GDT device
            Ipv4InterfaceContainer GDTInterface;
            /// @brief Vector containing the Interface container of each cluster
            std::vector<Ipv4InterfaceContainer> clustersInterfaces;
            /// @brief Vector containing the base IP of each cluster
            std::vector<Ipv4Address> clustersBaseIP;
            /// @brief Interfaces of the links between cluster nodes and gdt. Outer vecter being the clusters, inner vector being the links of each cluster node to the gdt
            std::vector<std::vector<Ipv4InterfaceContainer>> clustersLinkInterfaces;
            /// @brief Interface details of CH-GDT links
            std::vector<std::vector<std::pair<Ptr<Ipv4>, uint32_t>>> linksInterfaces;

            /**
             * @brief Default constructor
             */
            FANETAddressHelper();         

            /**
             * @brief Parameterized constructor
             * @param network Base network address
             * @param mask Subnet mask 
            */                                     
            FANETAddressHelper(Ipv4Address network, Ipv4Mask mask);  
            /**
             * @brief Destructor 
             * */          
            ~FANETAddressHelper();                       

            static TypeId GetTypeId();                      

            /**
             * @brief Initialize the network base address and subnet mask
             * @param network Base network address
             * @param mask Subnet mask
             */
            void SetUp(Ipv4Address network, Ipv4Mask mask);                    
            
            /**
             * @brief Assigns IP addresses to the devices in the FANET network
             * @param GDTDevice The GDT network device
             * @param allIntraClusterNetDevices Vector of network devices in clusters
             * @param allInterClusterNetDevices 2D vector of network devices linking the GDT to cluster nodes
             */
            void SetBases(NetDeviceContainer GDTDevice, std::vector<NetDeviceContainer> allIntraClusterNetDevices, std::vector<std::vector<NetDeviceContainer>> allInterClusterNetDevices);
            
            /**
             * @brief Computes the base network address from a given IP address.
             * @param ip The IP address to extract the base address from.
             * @return The base network address.
             */
            static Ipv4Address GetBaseAddress(Ipv4Address ip);

            void StoreClusterBaseIP(Ipv4Address addr);

            Ipv4Address GetClusterBaseIP(uint32_t clusterIndex);

            Ipv4Address GetBroadcastIP(Ipv4Address addr);
        };
}

#endif