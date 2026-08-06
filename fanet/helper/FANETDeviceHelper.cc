#include "FANETDeviceHelper.h"
#include "ns3/FANETMobilityHelper.h"
#include "ns3/FANETHeader.h"
#include "ns3/tdma-wifi-mac.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/config.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("FANETDeviceHelper");

    TypeId FANETDeviceHelper::GetTypeId()
    {
        static TypeId tid = 
            TypeId("ns3::FANETDeviceHelper")
                .SetParent<ns3::Object>()
                .AddConstructor<FANETDeviceHelper>()
                .AddAttribute(  "clusterWifiStandard",
                                "Wifi standard to use for clusters",
                                EnumValue(WIFI_STANDARD_80211n),
                                MakeEnumAccessor<WifiStandard>(&FANETDeviceHelper::clusterWifiStandard),
                                MakeEnumChecker(
                                    WIFI_STANDARD_UNSPECIFIED, "WIFI_STANDARD_UNSPECIFIED",
                                    WIFI_STANDARD_80211a, "WIFI_STANDARD_80211A",
                                    WIFI_STANDARD_80211b, "WIFI_STANDARD_80211B",
                                    WIFI_STANDARD_80211g, "WIFI_STANDARD_80211G",
                                    WIFI_STANDARD_80211p, "WIFI_STANDARD_80211P",
                                    WIFI_STANDARD_80211n, "WIFI_STANDARD_80211N",
                                    WIFI_STANDARD_80211ac, "WIFI_STANDARD_80211AC",
                                    WIFI_STANDARD_80211ad, "WIFI_STANDARD_80211AD",
                                    WIFI_STANDARD_80211ax, "WIFI_STANDARD_80211AX",
                                    WIFI_STANDARD_80211be, "WIFI_STANDARD_80211BE"
                                ))
                .AddAttribute(  "clusterWifiChannelPropagationDelay",
                                "Propagation delay model of cluster wifi",
                                StringValue("ns3::ConstantSpeedPropagationDelayModel"),
                                MakeStringAccessor(&FANETDeviceHelper::clusterWifiChannelPropagationDelay),
                                MakeStringChecker()
                                )
                .AddAttribute(  "clusterPropagationLossModel",
                                "Propagation loss model of cluster wifi",
                                StringValue("ns3::FriisPropagationLossModel"),
                                MakeStringAccessor(&FANETDeviceHelper::clusterPropagationLossModel),
                                MakeStringChecker()
                                )
                .AddAttribute(  "clusterMacType",
                                "MAC type of the cluster",
                                StringValue("ns3::TdmaWifiMac"),
                                MakeStringAccessor(&FANETDeviceHelper::clusterMacType),
                                MakeStringChecker()
                                )      
                .AddAttribute(  "linkWifiStandard",
                                "Wifi standard to use for links",
                                EnumValue(WIFI_STANDARD_80211n),
                                MakeEnumAccessor<WifiStandard>(&FANETDeviceHelper::linkWifiStandard),
                                MakeEnumChecker(
                                    WIFI_STANDARD_UNSPECIFIED, "WIFI_STANDARD_UNSPECIFIED",
                                    WIFI_STANDARD_80211a, "WIFI_STANDARD_80211A",
                                    WIFI_STANDARD_80211b, "WIFI_STANDARD_80211B",
                                    WIFI_STANDARD_80211g, "WIFI_STANDARD_80211G",
                                    WIFI_STANDARD_80211p, "WIFI_STANDARD_80211P",
                                    WIFI_STANDARD_80211n, "WIFI_STANDARD_80211N",
                                    WIFI_STANDARD_80211ac, "WIFI_STANDARD_80211AC",
                                    WIFI_STANDARD_80211ad, "WIFI_STANDARD_80211AD",
                                    WIFI_STANDARD_80211ax, "WIFI_STANDARD_80211AX",
                                    WIFI_STANDARD_80211be, "WIFI_STANDARD_80211BE"
                                ))
                .AddAttribute(  "linkWifiChannelPropagationDelay",
                                "Propagation delay model of link wifi",
                                StringValue("ns3::ConstantSpeedPropagationDelayModel"),
                                MakeStringAccessor(&FANETDeviceHelper::linkWifiChannelPropagationDelay),
                                MakeStringChecker()
                                )
                .AddAttribute(  "linkPropagationLossModel",
                                "Propagation loss model of link wifi",
                                StringValue("ns3::FriisPropagationLossModel"),
                                MakeStringAccessor(&FANETDeviceHelper::linkPropagationLossModel),
                                MakeStringChecker()
                                )
                .AddAttribute(  "linkMacType",
                                "MAC type of the link",
                                StringValue("ns3::TdmaWifiMac"),
                                MakeStringAccessor(&FANETDeviceHelper::linkMacType),
                                MakeStringChecker()
                                );               
        return tid;
    }

    // Constructor
    FANETDeviceHelper::FANETDeviceHelper() {
    }

    // Destructor
    FANETDeviceHelper::~FANETDeviceHelper() {

    }

    void FANETDeviceHelper::DefaultWifi() {
        clusterWifiStandard = WIFI_STANDARD_80211n;
        clusterWifiChannelPropagationDelay = "ns3::ConstantSpeedPropagationDelayModel";
        clusterPropagationLossModel = "ns3::FriisPropagationLossModel";
        clusterMacType = "ns3::TdmaWifiMac";
    }

    void FANETDeviceHelper::TdmaWifi(){
        clusterWifiStandard = WIFI_STANDARD_80211n;
        clusterWifiChannelPropagationDelay = "ns3::ConstantSpeedPropagationDelayModel";
        clusterPropagationLossModel = "ns3::FriisPropagationLossModel";
        clusterMacType = "ns3::TdmaWifiMac";
    }

    void FANETDeviceHelper::SetupGDTWifi(NodeContainer GDTNode)
    {
        WifiHelper localWifiGDT;
        localWifiGDT.SetStandard(clusterWifiStandard);
        
        // Create a separate WiFi channel for the dedicated GDT WiFi device
        YansWifiChannelHelper wifiChannelGDT;
        if (!clusterWifiChannelPropagationDelay.empty()) {
            wifiChannelGDT.SetPropagationDelay(clusterWifiChannelPropagationDelay);
        }
        if (!clusterPropagationLossModel.empty()) {
            wifiChannelGDT.AddPropagationLoss(clusterPropagationLossModel);
        }

        // Setup the PHY layer for the GDT WiFi device
        YansWifiPhyHelper wifiPhyGDT;
        wifiPhyGDT.SetChannel(wifiChannelGDT.Create());

        // Configure the MAC layer for AdHoc mode
        WifiMacHelper wifiMacAdHocGDT;
        wifiMacAdHocGDT.SetType(clusterMacType, 
            "QosSupported", BooleanValue(false), 
            "Ssid", SsidValue(Ssid("GDT-WiFi")));

        // Install the WiFi device on the GDT node
        NetDeviceContainer gdtWiFiDevice = localWifiGDT.Install(wifiPhyGDT, wifiMacAdHocGDT, GDTNode.Get(0));

        // Store the new device
        this->GDTDevice = gdtWiFiDevice;
    }

    //Intra-cluster f_n
    // @param clusters: Node container of all cluster members/heads in the network, exclusive of GDT 
    void FANETDeviceHelper::SetUpIntraClusterWifi(std::vector<NodeContainer> clusters) {
        
        WifiHelper localWifiIntra;
        // Ensure the WiFi standard is set
        localWifiIntra.SetStandard(clusterWifiStandard);
        //wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("DsssRate11Mbps"), "ControlMode", StringValue("DsssRate11Mbps"));

        // Install WiFi devices for each cluster
        for (size_t i = 0; i < clusters.size(); i++) 
        {
            
            // Create a unique Wi-Fi channel for this cluster
            YansWifiChannelHelper wifiChannelintra;
            if (!clusterWifiChannelPropagationDelay.empty()) {
                wifiChannelintra.SetPropagationDelay(clusterWifiChannelPropagationDelay);
            }
            if (!clusterPropagationLossModel.empty()) {
                wifiChannelintra.AddPropagationLoss(clusterPropagationLossModel);
            }

            // Setup the PHY layer for this cluster with the unique channel
            YansWifiPhyHelper wifiPhyCluster;
            wifiPhyCluster.SetChannel(wifiChannelintra.Create());

            //Dynamically assign channels to clusters to avoid interference, using a simple round-robin approach
            int channelNum = 1 + (i % 3) * 5; 
            std::ostringstream channelStr;
            channelStr << "{" << channelNum << ", 20, BAND_2_4GHZ, 0}";
            wifiPhyCluster.Set("ChannelSettings", StringValue(channelStr.str()));

            // Configure SSID for the cluster
            std::ostringstream ssidStream;
            ssidStream << "Cluster_" << i;
            std::string ssid = ssidStream.str();

            // Configure the MAC layer for AdHoc (for both GDT and CH)
            WifiMacHelper wifiMacCM;
            wifiMacCM.SetType(clusterMacType, 
                  "Ssid", SsidValue(Ssid(ssid)),
                  "QosSupported", BooleanValue(false),
                  "TotalMiniSlots", UintegerValue(12),
                  "KbPerMiniSlot", UintegerValue(1));

            // Install WiFi devices on nodes in the current cluster
            NetDeviceContainer clusterDevices = localWifiIntra.Install(wifiPhyCluster, wifiMacCM, clusters[i]);

            this->allIntraClusterNetDevices.push_back(clusterDevices);
            
            // LKW: Set name for TDMAWifiMac
            for (uint32_t j = 0; j < clusterDevices.GetN(); j++) {
                // Grab the generic device
                Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(clusterDevices.Get(j));
                if (wifiDev) {
                    // Cast the generic MAC into our custom TdmaWifiMac
                    std::string devname = std::string("wlan-intra-node") + std::to_string(clusters[i].Get(j)->GetId());
                    Ptr<TdmaWifiMac> tdmaMac = DynamicCast<TdmaWifiMac>(wifiDev->GetMac());
                    tdmaMac->setName(devname);
                }
            }
        }
        NS_LOG_DEBUG("Devices for intra-cluster communication installed on the nodes");
    }

    //Creates a channel representing inter-cluster(f_0), attach it to the GDT and all nodes
    void FANETDeviceHelper::SetUpInterClusterWifi(Ptr<FANETTopologyHelper> fanet) {
        
        WifiHelper localWifiInter;
        localWifiInter.SetStandard(linkWifiStandard);

        YansWifiChannelHelper wifiChannelinter;
    
        if (!linkWifiChannelPropagationDelay.empty()) {
            wifiChannelinter.SetPropagationDelay(linkWifiChannelPropagationDelay);
        }
        if (!linkPropagationLossModel.empty()) {
            wifiChannelinter.AddPropagationLoss(linkPropagationLossModel);
        }

        YansWifiPhyHelper wifiPhyInter;
        wifiPhyInter.SetChannel(wifiChannelinter.Create());
        
        //Assign Inter cluster antenna to 5 GHz Band (Channel 36) to avoid interference with intra-cluster communication on 2.4 GHz band.
        wifiPhyInter.Set("ChannelSettings", StringValue("{36, 20, BAND_5GHZ, 0}"));

        // Configure 1 common MAC and SSID for the Inter-cluster network
        WifiMacHelper wifiMacInter;
        wifiMacInter.SetType(linkMacType, 
                     "Ssid", SsidValue(Ssid("InterCluster_f0")),
                     "QosSupported", BooleanValue(false), 
                     "TotalMiniSlots", UintegerValue(24),
                     "KbPerMiniSlot", UintegerValue(1));

        // Install this f0 radio on the GDT
        NetDeviceContainer gdtInterDevice = localWifiInter.Install(wifiPhyInter, wifiMacInter, fanet->GDTNode.Get(0));    
        // If you have a variable to hold the GDT device, save it here
        this->GDTDevice = gdtInterDevice; 

        // LKW: Set name for TDMAWifiMac
        for (uint32_t j = 0; j < gdtInterDevice.GetN(); j++) {
            Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(gdtInterDevice.Get(j));
            if (wifiDev) {
                // Cast the generic MAC into our custom TdmaWifiMac
                std::string devname = std::string("wlan-inter-gdt");
                Ptr<TdmaWifiMac> tdmaMac = DynamicCast<TdmaWifiMac>(wifiDev->GetMac());
                tdmaMac->setName(devname);
            }
        }

        // Install this SAME f0 radio on ALL cluster nodes.
        // CH assignment is dynamic, every node needs the hardware, 
        // but the TDMA MAC logic will keep it silent unless they are promoted to CH.
        for (size_t i = 0; i < fanet->clusters.size(); i++) {
        
// #define TEST_MODE
#ifdef TEST_MODE
            YansWifiPhyHelper wifiPhyInterTest;
            wifiPhyInterTest.SetChannel(wifiChannelinter.Create());
            
            //Assign Inter cluster antenna to 5 GHz Band (Channel 36) to avoid interference with intra-cluster communication on 2.4 GHz band.
            wifiPhyInterTest.Set("ChannelSettings", StringValue("{0, 0, BAND_2_4GHZ, 0}"));

            // Configure 1 common MAC and SSID for the Inter-cluster network
            WifiMacHelper wifiMacInter;
            wifiMacInter.SetType(linkMacType, 
                        "Ssid", SsidValue(Ssid("InterCluster_f0")),
                        "TotalMiniSlots", UintegerValue(24),
                        "KbPerMiniSlot", UintegerValue(1));
            // Install on the whole cluster at once
            NetDeviceContainer clusterInterDevices = localWifiInter.Install(wifiPhyInterTest, wifiMacInter, fanet->clusters[i]);
#else
            // Install on the whole cluster at once
            NetDeviceContainer clusterInterDevices = localWifiInter.Install(wifiPhyInter, wifiMacInter, fanet->clusters[i]);
#endif
            // LKW: Set name for TDMAWifiMac
            for (uint32_t j = 0; j < clusterInterDevices.GetN(); j++) {
                Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(clusterInterDevices.Get(j));
                if (wifiDev) {
                    // Cast the generic MAC into our custom TdmaWifiMac
                    std::string devname = std::string("wlan-inter-node") + std::to_string(fanet->clusters[i].Get(j)->GetId());
                    Ptr<TdmaWifiMac> tdmaMac = DynamicCast<TdmaWifiMac>(wifiDev->GetMac());
                    tdmaMac->setName(devname);
                }
            }
            // Create an inner vector to satisfy the 2D requirement of allInterClusterNetDevices
            std::vector<NetDeviceContainer> innerVector;
            innerVector.push_back(clusterInterDevices);

            // Store the devices. Note: I changed this to push back the whole container
            // rather than doing it node-by-node to match standard ns-3 topology structures.
            this->allInterClusterNetDevices.push_back(innerVector);
        }

    NS_LOG_DEBUG("Devices for GDT-Cluster communication (f_0) installed");
    }

    void FANETDeviceHelper::AssignTdmaSlots(Ptr<FANETTopologyHelper> fanet, 
                                            Time cycleDuration, 
                                            std::deque<ClusterMacConfig>& intraConfigs, 
                                            std::deque<ClusterMacConfig>& interConfigs) 
    {
        for (size_t i = 0; i < fanet->clusters.size(); ++i)
        {
            for (uint32_t j = 0; j < fanet->clusters[i].GetN(); ++j)
            {
                Ptr<Node> node = fanet->clusters[i].Get(j);
                
                //Check if node is a Cluster Head (for inter-cluster config)
                bool isCH = false;
                for (const auto& chNode : fanet->CHNodes) {
                    if (chNode && chNode->GetId() == node->GetId()) {
                        isCH = true;
                        break;
                    }
                }

                //Iterate over the devices of the node to find the WiFi device and configure TDMA parameters
                for (uint32_t d = 0; d < node->GetNDevices(); ++d) 
                {
                    Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice>(node->GetDevice(d));
                    if (wifiDevice) 
                    {
                        Ptr<TdmaWifiMac> tdmaMac = DynamicCast<TdmaWifiMac>(wifiDevice->GetMac());
                        if (tdmaMac) 
                        {
                            //Set standard params
                            tdmaMac->SetTdmaParameters(fanet->allNodes.GetN(), cycleDuration, node->GetId());
                            
                            //Determine if this device is for intra-cluster or inter-cluster communication based on SSID
                            std::string ssid = wifiDevice->GetMac()->GetSsid().PeekString();
                            bool isInterCluster = (ssid.find("InterCluster") != std::string::npos);
                            //sync MAC layer identity
                            tdmaMac->SetIsInterCluster(isInterCluster);

                            if (isInterCluster) {
                                tdmaMac->SetClusterConfig(isCH ? &interConfigs.at(i) : nullptr);
                            } else {
                                tdmaMac->SetClusterConfig(&intraConfigs.at(i));
                            }

                            tdmaMac->AllocateMiniSlots();
                            tdmaMac->StartTdma();
                        }
                    }
                }
            }
        }
        //Configure the GDT's TDMA parameters and start its clock so it can start transmitting to the CHs in the inter-cluster network
        Ptr<Node> gdtNode = fanet->GDTNode.Get(0);
        for (uint32_t d = 0; d < gdtNode->GetNDevices(); ++d) 
        {
            Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice>(gdtNode->GetDevice(d));
            if (wifiDevice) 
            {
                Ptr<TdmaWifiMac> tdmaMac = DynamicCast<TdmaWifiMac>(wifiDevice->GetMac());
                if (tdmaMac) 
                {
                    //Give the GDT Slot 0 out of the total nodes
                    tdmaMac->SetTdmaParameters(fanet->allNodes.GetN(), cycleDuration, gdtNode->GetId());
                    tdmaMac->StartTdma();
                }
            }
        }
    }

    void FANETDeviceHelper::AssignClusterHeads(Ptr<FANETTopologyHelper> fanet,
                                               Ptr<FANETAddressHelper> ipv4,
                                               FANETAnimationHelper* anim,
                                               CHStatusChangeCallback callback)
    {
        fanet->CHNodes.clear(); // Clear previous CH assignments before reassigning
                
        for (size_t i = 0; i < fanet->clusters.size(); i++)
        {
            // Obtain the closest node of the cluster to the GDT 
            Ptr<Node> closestNode = FANETMobilityHelper::GetClosestNode(fanet->GDTNode.Get(0), fanet->clusters[i]);

            if (closestNode==nullptr && fanet->clusters[i].GetN() > 0) {
                closestNode = fanet->clusters[i].Get(0); // Fallback to the first node if GetClosestNode fails
            }

            for (uint32_t j = 0; j < fanet->clusters[i].GetN(); j++) {
                Ptr<Node> n = fanet->clusters[i].Get(j);
                bool isCH = (n == closestNode);
                
                // Loop through devices to find the TDMA MAC
                for (uint32_t d = 0; d < n->GetNDevices(); d++) {
                    Ptr<WifiNetDevice> wifi = DynamicCast<WifiNetDevice>(n->GetDevice(d));
                    if (wifi) {
                        Ptr<TdmaWifiMac> mac = DynamicCast<TdmaWifiMac>(wifi->GetMac());
                        if (mac) {
                            mac->SetIsClusterHead(isCH);
                            mac->AllocateMiniSlots(); // Refresh slot table based on role
                        }
                    }
                }
            }

            fanet->CHNodes.push_back(closestNode); // Store the closest node as the CH for this cluster
            
            if (closestNode != nullptr) {
                 // Notify the application layer of the CH assignment
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s, Node " 
                        << closestNode->GetId() << " selected as cluster head");
                
                //Invoke the callback instead of calling simulator directly
                if(!callback.IsNull())
                {
                    callback(closestNode, true);
                }
            }
        }
        if (anim != nullptr && !fanet->CHNodes.empty()) {
            anim->AssignCHAnim(fanet->CHNodes);
        }
        
        Simulator::Schedule(Seconds(5.0), [this, fanet, ipv4, anim, callback]() {
        this->ReassignClusterHeads(fanet, ipv4, anim, callback);
        });
    }

    void FANETDeviceHelper::ReassignClusterHeads(Ptr<FANETTopologyHelper> fanet,
                                                 Ptr<FANETAddressHelper> ipv4,
                                                 FANETAnimationHelper* anim,
                                                 CHStatusChangeCallback callback)
    {
        for (size_t i = 0; i < fanet->clusters.size(); i++)
        {
            Ptr<Node> closestNode = FANETMobilityHelper::GetClosestNode(fanet->GDTNode.Get(0), fanet->clusters[i]);
            
            if (closestNode == nullptr && fanet->clusters[i].GetN() > 0) {
                closestNode = fanet->clusters[i].Get(0); // Fallback to the first node if GetClosestNode fails
            }
            
            if (closestNode == nullptr || i > fanet->CHNodes.size() || fanet->CHNodes[i] == nullptr) 
                continue; 
            Ptr<Node> clusterHeadNode = fanet->CHNodes[i];

            if (closestNode->GetId() != clusterHeadNode->GetId()) {
            
                // Invoke callback for demotion
                if (!callback.IsNull()) {
                    callback(clusterHeadNode, false);
                }
                
                fanet->CHNodes[i] = closestNode;
                
                // Invoke callback for promotion
                if (!callback.IsNull()) {
                    callback(closestNode, true);
                }  

                // Notify the application layer of the CH reassignment 
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds() << "s, Node " 
                            << closestNode->GetId() << " reassigned as cluster head");
                }
        }

        if (anim != nullptr) {
            anim->UpdateCHAnim(fanet->CHNodes);
        }

        //Pass callback to recursive call
        Simulator::Schedule(Seconds(5.0), [this, fanet, ipv4, anim, callback]() {
        this->ReassignClusterHeads(fanet, ipv4, anim, callback);
    }); 
    }

    void FANETDeviceHelper::NotifyCHStatusChange(Ptr<Node> node, std::string status)
    {
        // Ptr<Socket> socket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
        // InetSocketAddress addr = InetSocketAddress(Ipv4Address("127.0.0.1"), 8080);

        // FANETHeader header;
        // header.SetNodeId(node->GetId());
        // header.SetClusterId(999);
        // header.SetType(HELLO);
        // header.SetService(CH_PROMO);

        // Ptr<Packet> packet = Create<Packet>((uint8_t*) status.c_str(), status.length());
        // packet->AddHeader(header);

        // socket->Connect(addr);
        // socket->Send(packet);
        // socket->Close();

        //NS_LOG_DEBUG("Node " << node->GetId() <<" notifying application of CH status change");
    }
}

