#include "FANETAddressHelper.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("FANETAddressHelper");
    
    FANETAddressHelper::FANETAddressHelper(){

    }

    FANETAddressHelper::FANETAddressHelper(Ipv4Address network, Ipv4Mask mask)
        : network(network), mask(mask)
    {

    }

    void FANETAddressHelper::SetUp(Ipv4Address network, Ipv4Mask mask){
        this->network = network;
        this->mask = mask;
    }

    FANETAddressHelper::~FANETAddressHelper(){

    }

    TypeId FANETAddressHelper::GetTypeId()
    {
        static TypeId tid =
            TypeId("ns3::FANETAddressHelper")
                .SetParent<ns3::Object>()
                .AddConstructor<FANETAddressHelper>()
                .AddAttribute(  "baseNetworkAddress",
                                "Base Network Address for the simulator to set the IP for the entire fanet",
                                Ipv4AddressValue(Ipv4Address("0.0.0.0")),
                                MakeIpv4AddressAccessor(&FANETAddressHelper::network),
                                MakeIpv4AddressChecker())
                .AddAttribute(  "baseSubnetMask",
                                "Subnet mask to be used for the network",
                                Ipv4MaskValue(Ipv4Mask("255.255.255.255")),
                                MakeIpv4MaskAccessor(&FANETAddressHelper::mask),
                                MakeIpv4MaskChecker());

        return tid;
    }

    void FANETAddressHelper::IncrementNetwork(){
        uint32_t address = network.Get();

        // Extract the octets
        uint8_t octet1 = (address >> 24) & 0xFF;
        uint8_t octet2 = (address >> 16) & 0xFF;
        uint8_t octet3 = (address >> 8) & 0xFF;
        // uint8_t octet4 = address & 0xFF;

        // Increment the third octet
        octet3++;   

        // Reconstruct the IP address with last octet set to 0
        network = Ipv4Address((octet1 << 24) | (octet2 << 16) | (octet3 << 8) | 0);   

        return;
    }

    void FANETAddressHelper::SetBases(NetDeviceContainer GDTDevice, std::vector<NetDeviceContainer> allIntraClusterNetDevices, std::vector<std::vector<NetDeviceContainer>> allInterClusterNetDevices)
        {
            //assign f_0 first
            //gdt will be 10.1.0.1, CH1 will be 10.1.0.2
            ipv4.SetBase(network, mask);
            this->GDTInterface = ipv4.Assign(GDTDevice);
            
            // Iterate over each cluster
            for (size_t clusterId = 0; clusterId < allInterClusterNetDevices.size(); clusterId++) {
                std::vector<Ipv4InterfaceContainer> clusterLinkInterfaces;
                for (size_t localNodeId = 0; localNodeId < allInterClusterNetDevices[clusterId].size(); localNodeId++) {
                    Ipv4InterfaceContainer linkInterface = ipv4.Assign(allInterClusterNetDevices[clusterId][localNodeId]);
                    clusterLinkInterfaces.push_back(linkInterface);
                }
                clustersLinkInterfaces.push_back(clusterLinkInterfaces);
            }

            // To assign IP address for cluster member devices
            IncrementNetwork(); //move to the next subnet
        
            //then assign Intra-cluster (f_n) subnets 
            for (size_t clusterId = 0; clusterId < allIntraClusterNetDevices.size(); clusterId++){
                ipv4.SetBase(network, mask);
                Ipv4InterfaceContainer clusterInterface = ipv4.Assign(allIntraClusterNetDevices[clusterId]);
                clustersInterfaces.push_back(clusterInterface);
                StoreClusterBaseIP(network);
                IncrementNetwork();
            }
        }

    void FANETAddressHelper::StoreClusterBaseIP(Ipv4Address addr)
    {
        clustersBaseIP.push_back(addr);
    }

    Ipv4Address FANETAddressHelper::GetClusterBaseIP(uint32_t clusterIndex)
    {
        return clustersBaseIP[clusterIndex];
    }

    Ipv4Address FANETAddressHelper::GetBaseAddress(Ipv4Address ip)
    {
        uint32_t ipInt = ip.Get(); // Convert IP to integer

        // Mask out the last octet (set it to 0)
        uint32_t baseIPInt = ipInt & 0xFFFFFF00; 

        return Ipv4Address(baseIPInt); // Convert back to Ipv4Address
    }

    Ipv4Address FANETAddressHelper::GetBroadcastIP(Ipv4Address addr)
    {

        { return Ipv4Address( addr.Get() | 0x000000FF); }
    }
}