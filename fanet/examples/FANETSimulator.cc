#include "FANETSimulator.h"
#include "ns3/applications-module.h"
#include "ns3/add-client.h"
#include "ns3/add-server.h"
#include "ns3/cluster-node-app.h"
#include "ns3/gdt-app.h"
#include "ns3/FANETAppHelper.h"
#include "ns3/json.hpp"
#include "../thirdparty/tdma/dtdma-queue-header.h"
#include "ns3/node.h"

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("FANETSimulator");

    NS_OBJECT_ENSURE_REGISTERED(DtdmaQueueHeader);

    using json = nlohmann::json;

    TypeId FANETSimulator::GetTypeId()
    {
        static TypeId tid =
            TypeId("ns3::FANETSimulator")
                .SetParent<ns3::Object>()
                .AddConstructor<FANETSimulator>()
                .AddAttribute("nClusters",
                              "The number of clusters in the FANET Network",
                              UintegerValue(3),
                              MakeUintegerAccessor(&FANETSimulator::nClusters),
                              MakeUintegerChecker<uint32_t>())
                .AddAttribute("nClusterNodes",
                              "Space-separated list of number of nodes in each cluster: 'x1 y1 z1 x2 y2 z2 ...'",
                              StringValue("3 3 3"),
                              MakeStringAccessor(&FANETSimulator::nClusterNodesString),
                              MakeStringChecker())
                .AddAttribute("cycleDuration",
                              "Period of each cycle",
                              UintegerValue(200),
                              MakeUintegerAccessor(&FANETSimulator::cycleDuration),
                              MakeUintegerChecker<uint32_t>())
                .AddAttribute("filename",
                              "Output .xml file name for NetAnim",
                              StringValue("animation.xml"),
                              MakeStringAccessor(&FANETSimulator::fileName),
                              MakeStringChecker())
                .AddAttribute("simulationDuration",
                              "Duration of simulation",
                              DoubleValue(0.0),
                              MakeDoubleAccessor(&FANETSimulator::simDuration),
                              MakeDoubleChecker<double>());

        return tid;
    }

    FANETSimulator::FANETSimulator()
    {
    }

    FANETSimulator::~FANETSimulator()
    {
        delete this->mobility;
    }

    void FANETSimulator::Setup()
    {
        this->fanet = CreateObject<FANETTopologyHelper>();
        if (this->fanet == nullptr)
        {
            NS_FATAL_ERROR("Failed to create FANETTopologyHelper object!");
        }

        this->fanetDevices = CreateObject<FANETDeviceHelper>();
        this->router = CreateObject<FANETRoutingHelper>();
        this->ipv4 = CreateObject<FANETAddressHelper>();
    }

    void FANETSimulator::GetNClusters()
    {
        std::cout << "Number of clusters to simulate: ";
        std::string input;
        std::getline(std::cin, input); // Read input as a string

        this->nClusters = std::stoul(input); // Convert string to uint32_t
    }

    void FANETSimulator::GetNClusterNodes()
    {
        std::string input;
        uint32_t temp;

        for (uint32_t i = 0; i < this->nClusters; i++)
        {
            std::cout << "Number of cluster nodes in Cluster " << i << ": ";
            std::getline(std::cin, input);

            temp = std::stoul(input);
            this->nClusterNodes.push_back(temp);
        }
    }

    void FANETSimulator::SetConstNClusterNodes(uint32_t nClusters, uint32_t nClusterNodes)
    {
        for (uint32_t i = 0; i < nClusters; i++)
            this->nClusterNodes.push_back(nClusterNodes);
    }

    void FANETSimulator::GetCycleDuration()
    {
        std::string input;
        uint32_t temp;

        std::cout << "Duration of each cycle for TDMA (ms) " << ": ";
        std::getline(std::cin, input);

        temp = std::stoul(input);
        this->cycleDuration = temp;
    }

    void FANETSimulator::GetSimulationDuration()
    {
        std::string input;
        double duration;

        std::cout << "Duration of simulation (s) : ";
        std::getline(std::cin, input);

        duration = std::stod(input);
        this->simDuration = duration;
    }

    void FANETSimulator::CreateNetwork()
    {

        // this->mobility = new FANETMobilityHelper();
        this->fanet->CreateFANET(this->nClusters, this->nClusterNodes);
    }

    void FANETSimulator::SetMobility()
    {
        this->mobility = new FANETMobilityHelper();
        mobility->ApplyMobilityWireless(this->fanet);
    }

    void FANETSimulator::InstallDevices()
    {
        // this->fanetDevices->SetupGDTWifi(this->fanet->GDTNode);
        this->fanetDevices->SetUpIntraClusterWifi(this->fanet->clusters);
        this->fanetDevices->SetUpInterClusterWifi(this->fanet);
        this->fanetDevices->AssignTdmaSlots(this->fanet,
                                            MilliSeconds(this->cycleDuration),
                                            this->m_intraClusterConfigs,
                                            this->m_interClusterConfigs);
        // Print the GDT interfaces
        Ptr<Node> gdt = this->fanet->GDTNode.Get(0);
        std::cout << "GDT (Node " << gdt->GetId() << ") has " << gdt->GetNDevices() << " hardware interfaces." << std::endl;
    }

    void FANETSimulator::SetRoutingProtocol()
    {
        this->router->InstallRoutingProtocol(this->fanet->allNodes);
    }

    void FANETSimulator::AssignAddress()
    {
        this->ipv4->SetBases(this->fanetDevices->GDTDevice, this->fanetDevices->allIntraClusterNetDevices, this->fanetDevices->allInterClusterNetDevices);
    }

    void FANETSimulator::SetUpNetAnim()
    {
        this->anim = new FANETAnimationHelper(this->fileName);
        anim->SetMaxPktsPerTraceFile(5000000);
    }

    void FANETSimulator::ParseClusterNodesString()
    {
        std::stringstream ss(nClusterNodesString);
        uint32_t value;
        while (ss >> value)
            nClusterNodes.push_back(value);
    }

    // Role-Based Routing Hierarchy(pre-simulation)
    void FANETSimulator::ConfigureInterfaceMetrics()
    {
        Ptr<Node> gdtNode = this->fanet->GDTNode.Get(0);

        // Iterate through all nodes in the network
        for (uint32_t i = 0; i < this->fanet->allNodes.GetN(); i++)
        {
            Ptr<Node> node = this->fanet->allNodes.Get(i);
            Ptr<Ipv4> ipv4Stack = node->GetObject<Ipv4>();

            if (!ipv4Stack) continue;

            bool isGdt = (node->GetId() == gdtNode->GetId());

            // Iterate over all hardware devices on this specific node
            for (uint32_t d = 0; d < node->GetNDevices(); d++)
            {
                Ptr<NetDevice> dev = node->GetDevice(d);
                Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(dev);

                if (wifiDev)
                {
                    // Find the IP interface index that corresponds to this hardware MAC
                    int32_t ifIndex = ipv4Stack->GetInterfaceForDevice(dev);
                    if (ifIndex >= 0)
                    {
                        std::string ssid = wifiDev->GetMac()->GetSsid().PeekString();

                        // If it's the Inter-Cluster interface, make it less preferred
                        if (ssid.find("InterCluster") != std::string::npos)
                        {
                            ipv4Stack->SetMetric(ifIndex, 10);
                            if (!isGdt) {
                                ipv4Stack->SetDown(ifIndex); // Disable al CM's interfaces for routing purposes
                        }
                        // If it's the Intra-Cluster interface, make it highly preferred
                        } else if (ssid.find("Cluster_") != std::string::npos)
                            {
                                ipv4Stack->SetMetric(ifIndex, 1);
                            }
                    }
                }
            }
        }
        std::cout << "[ROUTING] Interface metrics successfully configured for Role-Based Routing." << std::endl;
    }

    void FANETSimulator::RunSimulation()
    {
        ns3::PacketMetadata::Enable();

        // See if the Application is successfully pushing data out
        LogComponentEnable("UdpSocketImpl", LOG_LEVEL_INFO);

        // See if AODV is desperately crying out for a route but failing
        LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_LOGIC);

        // See if the TDMA MAC layer is properly prioritizing and scheduling packets according to the traffic profiles
        LogComponentEnable("TdmaWifiMac", LOG_LEVEL_FUNCTION);

        // See if GDTApp can send command and if node listender can received command
        LogComponentEnable("GDTApp", LOG_LEVEL_INFO);

        this->CreateNetwork();
        InternetStackHelper internet;
        internet.Install(this->fanet->allNodes);

        this->SetMobility();

        size_t actualClusters = this->fanet->clusters.size();

        // Resize the deques to match the exact number of created clusters
        m_intraClusterConfigs.resize(actualClusters);
        m_interClusterConfigs.resize(actualClusters);
        m_chIntraConfigs.resize(actualClusters);

        // DEBUG: Verify profiles were loaded from JSON
        std::cout << "[DEBUG] Loaded " << this->m_trafficProfiles.size() << " traffic profiles from JSON" << std::endl;
        for (const auto &profile : this->m_trafficProfiles)
        {
            std::cout << "  - " << profile.type << ": " << profile.bandwidthKb << " Kbps, Priority " << profile.priority << std::endl;
        }

        // Initialize the TDMA MAC configurations for each cluster
        for (size_t i = 0; i < actualClusters; i++)
        {
            // CM - Full traffic allowed
            m_intraClusterConfigs[i].trafficProfiles = this->m_trafficProfiles;

            // CH Local - Cmd relay only
            std::vector<TrafficProfile> chLocalProfiles;
            for (const auto &tp : this->m_trafficProfiles)
            {
                if (tp.type.find("Cmd") != std::string::npos)
                    chLocalProfiles.push_back(tp);
            }
            m_chIntraConfigs[i].trafficProfiles = chLocalProfiles;
            m_chIntraConfigs[i].kbPerMiniSlot = 1;

            // CH Backbone - Aggregated Traffic
            // Ensure memberCount is calculated correctly based on cluster size
            uint32_t clusterNodeCount = this->fanet->clusters[i].GetN();
            uint32_t memberCount = (clusterNodeCount > 0) ? clusterNodeCount - 1 : 0;

            std::vector<TrafficProfile> backboneProfiles;
            for (const auto &tp : this->m_trafficProfiles)
            {
                TrafficProfile agg = tp;
                if (tp.type.find("Video") != std::string::npos || tp.type.find("Status") != std::string::npos)
                {
                    agg.bandwidthKb = tp.bandwidthKb * memberCount;
                }
                backboneProfiles.push_back(agg);
            }
            m_interClusterConfigs[i].trafficProfiles = backboneProfiles;
            m_interClusterConfigs[i].kbPerMiniSlot = 1;
        }

        this->InstallDevices();
        this->SetRoutingProtocol();
        this->AssignAddress();
        this->ConfigureInterfaceMetrics();

        // Declare targetNode
        Ptr<Node> targetNode = this->fanet->clusters[this->m_targetClusterIndex].Get(this->m_targetNodeIndex);

        // Create the App and install it on the target node
        Ptr<ClusterNodeApp> targetApp = CreateObject<ClusterNodeApp>();
        targetNode->AddApplication(targetApp);

        // Link the App's callback back to the Simulator's HandleCommand function
        targetApp->SetCommandCallback(MakeCallback(&FANETSimulator::HandleCommand, this));

        // Start the application
        targetApp->SetStartTime(Seconds(0.0));
        targetApp->SetStopTime(Seconds(this->simDuration));

        // Use template to ensure the compiler resolves the pointer correctly
        Ptr<Ipv4> ipv4 = targetNode->template GetObject<Ipv4>();
        Ipv4Address targetIp = ipv4->GetAddress(1, 0).GetLocal();

        LogComponentDisable("ClusterNodeCHPromo", LOG_LEVEL_DEBUG);

        this->SetUpNetAnim();

        // Schedule the cluster head assignment to run shortly after the simulation starts
        // to ensure all devices are installed and ready, but before the applications start sending data,
        // so that the cluster heads are properly assigned and can manage the TDMA scheduling from the get-go.
        Simulator::Schedule(Seconds(0.001), [this]()
                            {
            auto callback = MakeCallback(&FANETSimulator::UpdateNodeApplications, this);
            this->fanetDevices->AssignClusterHeads(this->fanet, this->ipv4, this->anim, callback); });
        // Schedule the periodic topology sync to run every 2ms to print the TDMA grid map of each node and verify that the mini-slot allocations are correct and updating as expected based on the traffic profiles.
        Simulator::Schedule(Seconds(0.002), &FANETSimulator::PeriodicTopologySync, this);

        // Dynamically fetch the GDT's IP address to use as the destination for the applications instead of hardcoding it.
        // This also serves as a demonstration of how the GDT can be aware of the cluster nodes and their addresses right from the start,
        // which is crucial for the GDT to perform its management functions effectively.
        Ptr<Node> gcsNode = this->fanet->GDTNode.Get(0);
        Ptr<Ipv4> gdtIpv4 = gcsNode->GetObject<Ipv4>();
        // Interface 1 is the physical f_0 radio linking to the FANET
        Ipv4Address gcsIp = gdtIpv4->GetAddress(1, 0).GetLocal();

        std::cout << "[APPLICATION] GDT Target IP Address is: " << gcsIp << std::endl;

        // Set up a PacketSink on the GDT to receive the traffic from the cluster nodes and verify that data is being received.
        uint16_t port = 9999;
        PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApp = sinkHelper.Install(gcsNode);
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(this->simDuration));

        // Configure the traffic profiles for the applications based on the JSON configuration.
        // For simplicity, just set up three types of traffic: Video, Status, and Command, each with different bandwidth requirements and priorities.
        // The TDMA MAC layer will use the priorities to allocate mini-slots accordingly.
        // VIDEO: 1.2K High Res -> 1200 bytes, 9.6Kbps | TID 5 (DSCP 0xA0)
        // OnOffHelper videoApp("ns3::UdpSocketFactory", InetSocketAddress(gcsIp, port));
        // videoApp.SetConstantRate(DataRate("50Kbps"), 1200);
        // videoApp.SetAttribute("Tos", UintegerValue(0xA0));

        // //STATUS 1: 0.1K -> 100 bytes, 0.8Kbps | TID 4 (DSCP 0x80)
        // OnOffHelper statusApp("ns3::UdpSocketFactory", InetSocketAddress(gcsIp, port));
        // statusApp.SetConstantRate(DataRate("0.8Kbps"), 100);
        // statusApp.SetAttribute("Tos", UintegerValue(0x80));

        // //CMD 1: 0.1K -> 100 bytes, 0.8Kbps | TID 6 (DSCP 0xC0)
        // OnOffHelper cmdApp("ns3::UdpSocketFactory", InetSocketAddress(gcsIp, port));
        // cmdApp.SetConstantRate(DataRate("0.8Kbps"), 100);
        // cmdApp.SetAttribute("Tos", UintegerValue(0xC0));

        // Isntall applications on all the cluster nodes with staggered start times to prevent collisions and ensure the GDT is ready to receive when the apps start sending
        for (size_t i = 0; i < this->fanet->clusters.size(); i++)
        {
            for (uint32_t j = 0; j < this->fanet->clusters[i].GetN(); j++)
            {
                Ptr<Node> currentNode = this->fanet->clusters[i].Get(j);
                uint32_t nodeId = currentNode->GetId();

                // Create a struct to track the applications installed on this node and their state
                NodeAppState appState;
                appState.node = currentNode;

                double staggerOffset = (i * 0.1) + (j * 0.05);

                // Bind physical interface to the local address of the node to ensure that the traffic is sent from the correct interface
                Ipv4Address localAddr;
                Ptr<Ipv4> cmIpv4 = currentNode->GetObject<Ipv4>();
                for (uint32_t d = 0; d < currentNode->GetNDevices(); d++)
                {
                    Ptr<WifiNetDevice> wDev = DynamicCast<WifiNetDevice>(currentNode->GetDevice(d));
                    if (wDev)
                    {
                        std::string ssidStr = wDev->GetMac()->GetSsid().PeekString();
                        if(ssidStr.find("Cluster_") != std::string::npos && ssidStr.find("InterCluster") == std::string::npos)
                        {
                            int32_t idx = cmIpv4->GetInterfaceForDevice(wDev);
                            if (idx >= 0) {
                                localAddr = cmIpv4->GetAddress(idx, 0).GetLocal();
                                break;
                            }
                        }
                    }
                }

                AddressValue localSocketAddr(InetSocketAddress(localAddr, 0));
                std::cout << "Bound Client Node " << nodeId << " Local Source to: " << localAddr << std::endl; 
                
                // LOW RES VIDEO [Pri 5 | ToS: 0x50] (Continuous Default)
                // Ipv4Address chIp = Ipv4Address("10.1.1.1");
                OnOffHelper lowResApp("ns3::UdpSocketFactory", InetSocketAddress(gcsIp, port));
                // lowResApp.SetConstantRate(DataRate("48Kbps"), 600);
                if (nodeId == 2) {
                    lowResApp.SetConstantRate(DataRate("500Kbps"), 600);
                } else {
                    lowResApp.SetConstantRate(DataRate("1bps"), 600); 
                }
                lowResApp.SetAttribute("Local", localSocketAddr);
                lowResApp.SetAttribute("Tos", UintegerValue(0x50)); 
                appState.videoApp = lowResApp.Install(currentNode); // Tracked for muting during CH promotion
                appState.videoApp.Start(Seconds(4.0 + staggerOffset));
                appState.videoApp.Stop(Seconds(this->simDuration));

                // HIGH RES VIDEO [Pri 3 | ToS: 0x30] (Dormant Default, waiting for trigger)
                OnOffHelper highResApp("ns3::UdpSocketFactory", InetSocketAddress(gcsIp, port));
                highResApp.SetConstantRate(DataRate("1bps"), 1200); // 0bps so it doesn't transmit until commanded
                highResApp.SetAttribute("Local", localSocketAddr);
                highResApp.SetAttribute("Tos", UintegerValue(0x30));
                appState.highResVideoApp = highResApp.Install(currentNode);
                appState.highResVideoApp.Start(Seconds(4.0 + staggerOffset));
                appState.highResVideoApp.Stop(Seconds(this->simDuration));

                // STATUS 1 [Pri 2 | ToS: 0x20] (Continuous Telemetry)
                OnOffHelper status1App("ns3::UdpSocketFactory", InetSocketAddress(gcsIp, port));
                status1App.SetConstantRate(DataRate("0.8Kbps"), 100);
                status1App.SetAttribute("Local", localSocketAddr);
                status1App.SetAttribute("Tos", UintegerValue(0x20));
                appState.statusApp = status1App.Install(currentNode);
                appState.statusApp.Start(Seconds(1.1 + staggerOffset));
                appState.statusApp.Stop(Seconds(this->simDuration));

                // STATUS 2 [Pri 1 | ToS: 0x10] (Asynchronous Burst/Alert)
                OnOffHelper status2App("ns3::UdpSocketFactory", InetSocketAddress(gcsIp, port));
                status2App.SetConstantRate(DataRate("2.4Kbps"), 300); // Higher rate to push 300 bytes in 1s
                status2App.SetAttribute("Local", localSocketAddr);
                status2App.SetAttribute("Tos", UintegerValue(0x10));
                ApplicationContainer s2 = status2App.Install(currentNode);
                s2.Start(Seconds(10.0 + staggerOffset)); // Emergency burst at 10s
                s2.Stop(Seconds(12.0 + staggerOffset));

                // Store the app references
                m_nodeApps[nodeId] = appState;
            }
        }

        // Schedule the dynamic update of traffic profiles based on the JSON configuration.
        for (const auto &block : this->m_scheduledtrafficWindows)
        {
            // Schedule the activation of this specific profile block
            Simulator::Schedule(Seconds(block.startTime),
                                &FANETSimulator::ExecuteProfileSwap, this,
                                block.profiles, "ACTIVATING UPDATE PROFILE WINDOW");

            // Schedule the automatic tear-down to revert back to baseline trafficProfiles
            Simulator::Schedule(Seconds(block.endTime),
                                &FANETSimulator::ExecuteProfileSwap, this,
                                this->m_trafficProfiles, "WINDOW ENDED - REVERTING TO BASELINE");
        }

        this->anim->AnimateFANET(this->fanet);

        // Give the GDT an active schedule so it can transmit Commands and ARP replies
        Ptr<Node> activeGcsNode = this->fanet->GDTNode.Get(0);
        for (uint32_t d = 0; d < activeGcsNode->GetNDevices(); d++)
        {
            Ptr<WifiNetDevice> gdtWifi = DynamicCast<WifiNetDevice>(activeGcsNode->GetDevice(d));
            if (gdtWifi)
            {
                Ptr<TdmaWifiMac> gdtMac = DynamicCast<TdmaWifiMac>(gdtWifi->GetMac());
                if (gdtMac)
                {
                    gdtMac->SetClusterConfig(&m_interClusterConfigs[0]);
                }
            }
        }

        // Create the GDTapp
        Ptr<GDTApp> gdtApp = CreateObject<GDTApp>();
        gcsNode->AddApplication(gdtApp);
        gdtApp->SetStartTime(Seconds(0.0));
        gdtApp->SetStopTime(Seconds(this->simDuration));

        // Define GDT Local Socket
        Ipv4Address gdtLocalIp = gcsNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        AddressValue gdtSocketAddr(InetSocketAddress(gdtLocalIp, 0));

        // CMD 1 [Pri 3 | ToS: 0x31] (Continuous GDT Heartbeat)
        OnOffHelper cmd1App("ns3::UdpSocketFactory", InetSocketAddress(targetIp, this->m_targetPort));
        cmd1App.SetConstantRate(DataRate("0.8Kbps"), 100); 
        cmd1App.SetAttribute("Local", gdtSocketAddr);
        cmd1App.SetAttribute("Tos", UintegerValue(0x31));
        ApplicationContainer c1 = cmd1App.Install(gcsNode);
        c1.Start(Seconds(2.0));
        c1.Stop(Seconds(this->simDuration));

        // CMD 2 [Pri 2 | ToS: 0x21] (Asynchronous Target Update)
        OnOffHelper cmd2App("ns3::UdpSocketFactory", InetSocketAddress(targetIp, this->m_targetPort));
        cmd2App.SetConstantRate(DataRate("2.4Kbps"), 300); 
        cmd2App.SetAttribute("Local", gdtSocketAddr);
        cmd2App.SetAttribute("Tos", UintegerValue(0x21));
        ApplicationContainer c2 = cmd2App.Install(gcsNode);
        c2.Start(Seconds(15.0)); // Asynchronous firing at t=15s
        c2.Stop(Seconds(17.0));

        Ptr<NetDevice> gdtInterClusterRadio;
        for (uint32_t d = 0; d < gcsNode->GetNDevices(); d++)
        {
            Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(gcsNode->GetDevice(d));
            if (wifiDev)
            {
                std::string ssid = wifiDev->GetMac()->GetSsid().PeekString();
                if (ssid.find("InterCluster") != std::string::npos)
                {
                    gdtInterClusterRadio = wifiDev;
                    break;
                }
            }
        }
        // Schedule the GDT App to dispatch the command through the Inter-Cluster radio
        Simulator::Schedule(Seconds(this->m_updateTime + 5.0),
                            &GDTApp::SendCommand,
                            gdtApp, targetIp, this->m_targetPort, this->m_commandString, gdtInterClusterRadio);

        Simulator::Stop(Seconds(simDuration));
        Simulator::Run();

        // The verification of GDT receiving data can be done by checking the total bytes received in the sink application on the GDT node.
        // If it's greater than 0, it means the GDT successfully received some data from the Nodes.
        Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApp.Get(0));
        std::cout << "\n"
                  << std::endl;
        std::cout << "[VERIFICATION] GDT successfully received: " << sink->GetTotalRx() << " bytes." << std::endl;
        std::cout << "\n"
                  << std::endl;

        Simulator::Destroy();
    }

    // Node is only allowed to generate data if it is a CM
    void FANETSimulator::UpdateNodeApplications(Ptr<Node> node, bool isNowCH)
    {
        uint32_t nodeId = node->GetId();

        if (nodeId == this->fanet->GDTNode.Get(0)->GetId()) {
            return; 
        }
        
        Ptr<Ipv4> ipv4Stack = node->GetObject<Ipv4>();
        if (ipv4Stack)
        {
            for (uint32_t d = 0; d < node->GetNDevices(); d++)
            {
                Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(node->GetDevice(d));
                if (wifiDev)
                {
                    std::string ssid = wifiDev->GetMac()->GetSsid().PeekString();
                    if (ssid.find("InterCluster") != std::string::npos)
                    {
                        int32_t ifIndex = ipv4Stack->GetInterfaceForDevice(wifiDev);
                        if (ifIndex >= 0)
                        {
                            if (isNowCH)
                                ipv4Stack->SetUp(ifIndex); //Turn on the Inter-Cluster interface of CH for routing purposes
                            else
                                ipv4Stack->SetDown(ifIndex); //Turn off the Inter-Cluster interface of CM for routing purposes
                        }
                    }
                }
            }
        }

        if (m_nodeApps.find(nodeId) == m_nodeApps.end())
            return;

        NodeAppState &appState = m_nodeApps[nodeId];
        if (appState.isCurrentlyCH == isNowCH)
            return;

        // Application muting is handled dynamically by ExecuteProfileSwap DataRate limits.
        if (isNowCH)
        {
            NS_LOG_INFO("Node " << nodeId << " promoted to CH.");
            appState.isCurrentlyCH = true;
        }
        else
        {
            NS_LOG_INFO("Node " << nodeId << " demoted from CM.");
            appState.isCurrentlyCH = false;
        }
    }

    void FANETSimulator::SetupSimulation(std::string jsonFilePath)
    {
        std::ifstream file(jsonFilePath);
        json config;
        file >> config;

        // Read the Bandwidth Table from JSON
        if (config.contains("trafficProfiles"))
        {
            for (const auto &item : config["trafficProfiles"])
            {
                TrafficProfile tp;
                tp.type = item["type"].get<std::string>();
                tp.priority = item["priority"].get<uint32_t>();
                tp.bandwidthKb = item["bandwidthKb"].get<uint32_t>();

                this->m_trafficProfiles.push_back(tp);
            }
        }

        // Sort the profiles by priority (1 comes first) so the MAC layer handles them in order
        std::sort(this->m_trafficProfiles.begin(), this->m_trafficProfiles.end(),
                  [](const TrafficProfile &a, const TrafficProfile &b)
                  {
                      return a.priority < b.priority;
                  });

        this->m_scheduledtrafficWindows.clear(); // clear existing blocks

        // Read update profile from json
        if (config.contains("updateProfiles"))
        {
            for (const auto &block : config["updateProfiles"])
            {
                TrafficWindow tw;
                tw.startTime = block["startTime"].get<double>();
                tw.endTime = block["endTime"].get<double>();

                // Parse the traffic profiles for this update block
                for (const auto &item : block["profiles"])
                {
                    TrafficProfile tp;
                    tp.type = item["type"].get<std::string>();
                    tp.priority = item["priority"].get<uint32_t>();
                    tp.bandwidthKb = item["bandwidthKb"].get<uint32_t>();
                    tw.profiles.push_back(tp);
                    this->m_updateProfiles.push_back(tp);
                }

                // Sort the profiles within this block by priority
                std::sort(tw.profiles.begin(), tw.profiles.end(),
                          [](const TrafficProfile &a, const TrafficProfile &b)
                          {
                              return a.priority < b.priority;
                          });

                // Add this block to the list of update profiles
                this->m_scheduledtrafficWindows.push_back(tw);
            }
        }

        // Sort the update profiles by priority
        std::sort(this->m_updateProfiles.begin(), this->m_updateProfiles.end(),
                  [](const TrafficProfile &a, const TrafficProfile &b)
                  {
                      return a.priority < b.priority;
                  });

        // Read dynamic injection time from json (default set to 15s if not provided)
        if (config.contains("updateTime"))
        {
            this->m_updateTime = config["updateTime"].get<double>();
        }
        else
        {
            this->m_updateTime = 15.0;
        }

        // Parse GDT command configuration from JSON.
        // This includes which node should be the target of the command, what port to listen on, and what command string to send.
        if (config.contains("gdtCommandConfig"))
        {
            this->m_targetClusterIndex = config["gdtCommandConfig"]["targetClusterIndex"].get<uint32_t>();
            this->m_targetNodeIndex = config["gdtCommandConfig"]["targetNodeIndex"].get<uint32_t>();
            this->m_targetPort = config["gdtCommandConfig"]["targetPort"].get<uint16_t>();
            this->m_commandString = config["gdtCommandConfig"]["commandString"].get<std::string>();
        }
        else
        {
            // Safe defaults if the block is missing from the JSON
            this->m_targetClusterIndex = 0;
            this->m_targetNodeIndex = 0;
            this->m_targetPort = 9999;
            this->m_commandString = "HIGH_RES";
        }

        Setup();
        SetAttribute("nClusters", UintegerValue(config["nClusters"]));
        SetAttribute("nClusterNodes", StringValue(config["nClusterNodes"].get<std::string>()));
        ParseClusterNodesString();
        SetAttribute("cycleDuration", UintegerValue(config["cycleDuration"]));
        SetAttribute("filename", StringValue(config["filename"].get<std::string>()));
        SetAttribute("simulationDuration", DoubleValue(config["simulationDuration"]));
        ;

        fanetDevices->SetAttribute("clusterWifiStandard", EnumValue(wifiStandardMap[config["fanetDevices"]["clusterWifiStandard"].get<std::string>()]));
        fanetDevices->SetAttribute("clusterWifiChannelPropagationDelay", StringValue(config["fanetDevices"]["clusterWifiChannelPropagationDelay"].get<std::string>()));
        fanetDevices->SetAttribute("clusterPropagationLossModel", StringValue(config["fanetDevices"]["clusterPropagationLossModel"].get<std::string>()));
        fanetDevices->SetAttribute("clusterMacType", StringValue(config["fanetDevices"]["clusterMacType"].get<std::string>()));
        fanetDevices->SetAttribute("linkWifiStandard", EnumValue(wifiStandardMap[config["fanetDevices"]["linkWifiStandard"].get<std::string>()]));
        fanetDevices->SetAttribute("linkWifiChannelPropagationDelay", StringValue(config["fanetDevices"]["linkWifiChannelPropagationDelay"].get<std::string>()));
        fanetDevices->SetAttribute("linkPropagationLossModel", StringValue(config["fanetDevices"]["linkPropagationLossModel"].get<std::string>()));
        fanetDevices->SetAttribute("linkMacType", StringValue(config["fanetDevices"]["linkMacType"].get<std::string>()));

        router->SetAttribute("routingProtocol", EnumValue(routingProtocolMap[config["router"]["routingProtocol"].get<std::string>()]));

        ipv4->SetAttribute("baseNetworkAddress", Ipv4AddressValue(Ipv4Address(config["ipv4"]["baseNetworkAddress"].get<std::string>().c_str())));
        ipv4->SetAttribute("baseSubnetMask", Ipv4MaskValue(Ipv4Mask(config["ipv4"]["baseSubnetMask"].get<std::string>().c_str())));

        this->m_currentActiveProfiles = this->m_trafficProfiles; // Initialize the current active profiles to the baseline profiles at the start of the simulation
    }

    // This callback is triggered when the target node receives the command from the GCS.
    // It identifies which cluster the node belongs to, updates the central configuration objects
    // with the new traffic profiles, and signals all nodes in that cluster to refresh their TDMA slot allocations based on the new profiles.
    void FANETSimulator::HandleCommand(Ptr<Node> rxNode, Ptr<Packet> packet)
    {
        uint32_t rxNodeId = rxNode->GetId();
        std::cout << "\n[SIMULATOR MANAGER] Received command trigger from Node " << rxNode->GetId() << std::endl;

        // extract the command string
        uint32_t pktSize = packet->GetSize();
        uint8_t *buffer = new uint8_t[pktSize];
        packet->CopyData(buffer, pktSize);
        std::string command(reinterpret_cast<char *>(buffer), pktSize);
        delete[] buffer;

        std::cout << "[SIMULATOR MANAGER] Received command [" << command << "] from Node " << rxNode->GetId() << std::endl;

        // Only applies profile if command is high_res
        if (command != "HIGH_RES")
        {
            NS_LOG_WARN("Unknown command: " << command << ". No profile swap performed.");
            return;
        }

        // Identify which cluster this node belongs to
        int targetClusterId = -1;
        uint32_t clusterSize = 0;
        for (size_t i = 0; i < this->fanet->clusters.size(); i++)
        {
            for (uint32_t j = 0; j < this->fanet->clusters[i].GetN(); j++)
            {
                if (this->fanet->clusters[i].Get(j)->GetId() == rxNodeId)
                {
                    targetClusterId = i;
                    clusterSize = this->fanet->clusters[i].GetN();
                    break;
                }
            }
            if (targetClusterId != -1)
                break;
        }

        if (targetClusterId < 0 || (size_t)targetClusterId >= m_intraClusterConfigs.size())
        {
            NS_LOG_ERROR("FATAL: targetClusterId " << targetClusterId << " is out of bounds!");
            return;
        }

        // Update the central cluster configuration objects with the new traffic profiles
        this->m_intraClusterConfigs[targetClusterId].trafficProfiles = this->m_updateProfiles;

        // Filter Video for CH
        std::vector<TrafficProfile> chIntraProfiles;
        for (const auto &p : this->m_updateProfiles)
        {
            if (p.type.find("Video") == std::string::npos)
            {
                chIntraProfiles.push_back(p);
            }
        }
        this->m_chIntraConfigs[targetClusterId].trafficProfiles = chIntraProfiles;

        // Generate the aggregated traffic profiles for the inter-cluster communication
        std::vector<TrafficProfile> aggregatedProfiles;
        for (const auto &baseProfile : this->m_updateProfiles)
        {
            TrafficProfile clusterDemand = baseProfile;
            uint32_t memberCount = (clusterSize > 0) ? clusterSize - 1 : 0;
            clusterDemand.bandwidthKb = (baseProfile.bandwidthKb * memberCount);
            aggregatedProfiles.push_back(clusterDemand);
        }
        this->m_interClusterConfigs[targetClusterId].trafficProfiles = aggregatedProfiles;

        // Signal all nodes in this cluster to refresh their TDMA mini-slot allocations
        for (uint32_t j = 0; j < this->fanet->clusters[targetClusterId].GetN(); j++)
        {
            Ptr<Node> clusterNode = this->fanet->clusters[targetClusterId].Get(j);
            uint32_t nodeId = clusterNode->GetId();

            bool isClusterHead = false;
            for (size_t c = 0; c < this->fanet->CHNodes.size(); c++)
            {
                if (this->fanet->CHNodes[c] != nullptr && this->fanet->CHNodes[c]->GetId() == nodeId)
                {
                    isClusterHead = true;
                    break;
                }
            }

            for (uint32_t d = 0; d < clusterNode->GetNDevices(); d++)
            {
                Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(clusterNode->GetDevice(d));
                if (wifiDev)
                {
                    Ptr<TdmaWifiMac> tdmaMac = DynamicCast<TdmaWifiMac>(wifiDev->GetMac());
                    if (tdmaMac)
                    {
                        std::string ssid = wifiDev->GetMac()->GetSsid().PeekString();
                        bool isInterCluster = (ssid.find("InterCluster") != std::string::npos);

                        tdmaMac->SetIsClusterHead(isClusterHead);
                        tdmaMac->SetIsInterCluster(isInterCluster);

                        if (isInterCluster)
                        {
                            if (isClusterHead)
                            {
                                tdmaMac->SetClusterConfig(&m_interClusterConfigs[targetClusterId]);
                            }
                            else
                            {
                                tdmaMac->SetClusterConfig(nullptr);
                            }
                        }
                        else
                        {
                            if (isClusterHead)
                            {
                                tdmaMac->SetClusterConfig(&m_chIntraConfigs[targetClusterId]);
                            }
                            else
                            {
                                tdmaMac->SetClusterConfig(&m_intraClusterConfigs[targetClusterId]);
                            }
                        }
                        tdmaMac->AllocateMiniSlots();
                        PrintTdmaGridMap(clusterNode, wifiDev, tdmaMac);
                    }
                }
            }
        }
    }

    void FANETSimulator::ExecuteProfileSwap(std::vector<TrafficProfile> profilesToApply, std::string stageName)
    {
        if (!this->fanet || m_intraClusterConfigs.size() != this->fanet->clusters.size())
        {
            NS_LOG_WARN("Skipping ProfileSwap: Topology or Config vectors not synchronized. "
                        << "Topology Clusters: " << (this->fanet ? std::to_string(this->fanet->clusters.size()) : "NULL")
                        << " Config Size: " << m_intraClusterConfigs.size());
            return;
        }

        this->m_currentActiveProfiles = profilesToApply;

        NS_LOG_UNCOND("[DEBUG] ExecuteProfileSwap called. Cluster count: " << (this->fanet ? this->fanet->clusters.size() : 0));

        for (size_t i = 0; i < this->fanet->clusters.size(); i++)
        {
            // Dynamically get the number of nodes in this specific cluster
            uint32_t clusterSize = this->fanet->clusters[i].GetN();
            uint32_t memberCount = (clusterSize > 0) ? clusterSize - 1 : 0;

            // Intra-cluster profiles for CM
            m_intraClusterConfigs[i].trafficProfiles = profilesToApply;

            // Intra-cluster profiles for CH, relay cmds
            std::vector<TrafficProfile> chIntraProfiles;
            for (const auto &p : profilesToApply)
            {
                if (p.type.find("Cmd") != std::string::npos)
                {
                    chIntraProfiles.push_back(p);
                }
            }
            m_chIntraConfigs[i].trafficProfiles = chIntraProfiles;

            // Inter-cluster for CH - relay Video/Status, base Cmd
            std::vector<TrafficProfile> aggregatedProfiles;
            for (const auto &baseProfile : profilesToApply)
            {
                TrafficProfile clusterDemand = baseProfile;
                if (baseProfile.type.find("Video") != std::string::npos ||
                    baseProfile.type.find("Status") != std::string::npos)
                {
                    clusterDemand.bandwidthKb = (baseProfile.bandwidthKb * memberCount);
                }
                else
                {
                    clusterDemand.bandwidthKb = baseProfile.bandwidthKb;
                }

                if (clusterDemand.bandwidthKb > 0)
                {
                    aggregatedProfiles.push_back(clusterDemand);
                }
            }
            m_interClusterConfigs[i].trafficProfiles = aggregatedProfiles;

            for (uint32_t j = 0; j < clusterSize; j++)
            {
                Ptr<Node> node = this->fanet->clusters[i].Get(j);
                uint32_t nodeId = node->GetId();

                // Check if this node is currently elected as a Cluster Head
                bool isClusterHead = false;
                for (size_t c = 0; c < this->fanet->CHNodes.size(); c++)
                {
                    if (this->fanet->CHNodes[c] != nullptr && this->fanet->CHNodes[c]->GetId() == nodeId)
                    {
                        isClusterHead = true;
                        break;
                    }
                }

                // Iterate through all applications on the node to safely find the Video App
                if (m_nodeApps.find(nodeId) != m_nodeApps.end())
                {
                    NodeAppState &appState = m_nodeApps[nodeId];

                    if (isClusterHead) {
                        // CHs are muted (they only act as relays)
                        appState.videoApp.Get(0)->SetAttribute("DataRate", StringValue("1bps"));
                        appState.highResVideoApp.Get(0)->SetAttribute("DataRate", StringValue("1bps"));
                    } else {
                        // Check if the HIGH_RES profile is actively commanded by the GDT
                        bool activateHighRes = false;
                        for (const auto& p : profilesToApply) {
                            if (p.type == "Video_HIGH_RES") activateHighRes = true;
                        }
                        
                        // Toggle the appropriate video resolution
                        if (nodeId == 2) {
                            if (activateHighRes) {
                                appState.videoApp.Get(0)->SetAttribute("DataRate", StringValue("1bps"));
                                appState.highResVideoApp.Get(0)->SetAttribute("DataRate", StringValue("500Kbps"));
                            } else {
                                appState.videoApp.Get(0)->SetAttribute("DataRate", StringValue("500Kbps"));
                                appState.highResVideoApp.Get(0)->SetAttribute("DataRate", StringValue("1bps"));
                            }
                        } else {
                            appState.videoApp.Get(0)->SetAttribute("DataRate", StringValue("1bps"));
                            appState.highResVideoApp.Get(0)->SetAttribute("DataRate", StringValue("1bps"));
                        }
                    }
                }

                // Iterate through the devices of this node to find the WiFi interfaces and update their TDMA MAC configurations
                // based on whether they are inter-cluster or intra-cluster interfaces, and whether this node is a cluster head or not.
                for (uint32_t d = 0; d < node->GetNDevices(); d++)
                {
                    Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(node->GetDevice(d));
                    if (wifiDev)
                    {
                        Ptr<TdmaWifiMac> tdmaMac = DynamicCast<TdmaWifiMac>(wifiDev->GetMac());
                        if (tdmaMac)
                        {
                            tdmaMac->ClearQueueTracking();
                            std::string ssid = wifiDev->GetMac()->GetSsid().PeekString();
                            bool isInterCluster = (ssid.find("InterCluster") != std::string::npos);
                            tdmaMac->SetIsClusterHead(isClusterHead);
                            tdmaMac->SetIsInterCluster(isInterCluster);
                            // Based on whether this is the inter-cluster or intra-cluster interface,
                            // and whether this node is a cluster head or not, we apply the appropriate traffic profiles
                            // and re-allocate the TDMA mini-slots accordingly. Cluster heads get the aggregated profiles on their inter-cluster interfaces,
                            // plain members get an empty profile to maintain their dormant state on the inter-cluster link,
                            // and all nodes get the regular profiles on their intra-cluster interfaces.
                            if (isInterCluster)
                            {
                                if (isClusterHead)
                                {
                                    // CH backbone profiles for the inter-cluster communication
                                    tdmaMac->SetClusterConfig(&m_interClusterConfigs.at(i));
                                }
                                else
                                {
                                    tdmaMac->SetClusterConfig(nullptr);
                                }
                            }
                            else
                            {
                                if (isClusterHead)
                                {
                                    // CH Local, get Cmd only profiles
                                }
                                else
                                {
                                    // CM Local, get standard video profile
                                    tdmaMac->SetClusterConfig(&m_intraClusterConfigs.at(i));
                                }
                            }
                        }
                        tdmaMac->AllocateMiniSlots();
                        PrintTdmaGridMap(node, wifiDev, tdmaMac);
                    }
                }
            }
        }
        Ptr<Node> syncGcsNode = this->fanet->GDTNode.Get(0);
        for (uint32_t d = 0; d < syncGcsNode->GetNDevices(); d++)
        {
            Ptr<WifiNetDevice> gdtWifi = DynamicCast<WifiNetDevice>(syncGcsNode->GetDevice(d));
            if (gdtWifi)
            {
                Ptr<TdmaWifiMac> gdtMac = DynamicCast<TdmaWifiMac>(gdtWifi->GetMac());
                if (gdtMac)
                {
                    gdtMac->SetClusterConfig(&m_interClusterConfigs[0]);
                    gdtMac->AllocateMiniSlots();
                }
            }
        }
    }

    // Method to periodically synchronize the topology and print the TDMA grid map for each node, demonstrating how the MAC layer adapts to any changes in the network over time (e.g., nodes moving, cluster head changes, etc.)
    void FANETSimulator::PeriodicTopologySync()
    {
        // Re-trigger the profile swap loop to apply rules to any newly promoted/demoted nodes
        ExecuteProfileSwap(this->m_currentActiveProfiles, "PERIODIC TOPOLOGY HARDWARE SYNC");

        // Reschedule the synchronization to execute 5 seconds from now
        Simulator::Schedule(Seconds(5.0), &FANETSimulator::PeriodicTopologySync, this);
    }

    // Enhanced logging function to visualize the TDMA slot allocation in a grid format, along with the node's role and current traffic profile.
    void FANETSimulator::PrintTdmaGridMap(Ptr<Node> node, Ptr<WifiNetDevice> wifiDev, Ptr<TdmaWifiMac> tdmaMac)
    {
        uint32_t nodeId = node->GetId();
        std::string ssid = wifiDev->GetMac()->GetSsid().PeekString();
        std::string simTime = std::to_string((uint32_t)Simulator::Now().GetSeconds()) + "s";

        std::stringstream macSs;
        macSs << wifiDev->GetMac()->GetAddress();
        std::string macAddr = macSs.str();

        std::string ipAddr = "Unassigned";
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();

        //
        if (ipv4)
        {
            //
            int32_t ifIndex = ipv4->GetInterfaceForDevice(wifiDev);

            //
            if (ifIndex >= 0 && ipv4->GetNAddresses(ifIndex) > 0)
            {
                //
                Ipv4Address localIp = ipv4->GetAddress(ifIndex, 0).GetLocal();

                //
                std::stringstream ipSs;
                ipSs << localIp;
                ipAddr = ipSs.str();
            }
        }

        // Check if this specific node is an active Cluster Head
        bool isClusterHead = false;
        for (uint32_t i = 0; i < this->fanet->CHNodes.size(); i++)
        {
            if (this->fanet->CHNodes[i] != nullptr && this->fanet->CHNodes[i]->GetId() == nodeId)
            {
                isClusterHead = true;
                break;
            }
        }

        bool isInterCluster = (ssid.find("InterCluster") != std::string::npos);
        bool isDormant = isInterCluster && !isClusterHead;

        // Build Header String
        std::stringstream headerSs;
        if (isDormant)
        {
            headerSs << "[TDMA HARDWARE STATE CHANGE]  Node: " << nodeId
                     << "  |  IP: " << ipAddr
                     << "  |  MAC: " << macAddr
                     << "  |  SSID: " << ssid << " | Sim Time: " << simTime;
        }
        else
        {
            headerSs << "[TDMA HARDWARE STATE CHANGE]  Node: " << nodeId
                     << "  |  IP: " << ipAddr
                     << "  |  MAC: " << macAddr
                     << "  |  SSID: " << ssid << "  | Sim Time: " << simTime;
        }
        std::string headerContent = headerSs.str();

        // Build Grid Map Row
        std::stringstream gridSs;
        gridSs << "GRID MAP | ";

        if (isDormant)
        {
            gridSs << "[ --- DORMANT INTERFACE (WAITING FOR PROMOTION) --- ]";
        }
        else
        {
            // Check if we should print 24 slots or 12 slots
            uint32_t slotsToPrint = isInterCluster ? 24 : 12;
            for (uint32_t i = 0; i < slotsToPrint; i++)
            {
                std::string slotName = tdmaMac->GetSlotTrafficType(i);

                if (slotName == "Status1")
                    gridSs << "[STA1] ";
                else if (slotName == "Status2")
                    gridSs << "[STA2] ";
                else if (slotName == "Cmd1")
                    gridSs << "[CMD1] ";
                else if (slotName == "Cmd2")
                    gridSs << "[CMD2] ";
                else if (slotName == "Cmd3")
                    gridSs << "[CMD3] ";
                else if (slotName == "Video_LOW_RES")
                    gridSs << "[V-LOW] ";
                else if (slotName == "Video_HIGH_RES")
                    gridSs << "[V-HI] ";
                else
                    gridSs << "[IDLE] ";
            }
        }
        std::string gridContent = gridSs.str();

        size_t internalWidth = std::max(headerContent.length(), gridContent.length()) + 2;

        // Render box
        std::cout << "+" << std::string(internalWidth, '-') << "+" << std::endl;
        std::cout << "| " << headerContent << std::string(internalWidth - headerContent.length() - 1, ' ') << "|" << std::endl;
        std::cout << "+" << std::string(internalWidth, '-') << "+" << std::endl;
        std::cout << "| " << gridContent << std::string(internalWidth - gridContent.length() - 1, ' ') << "|" << std::endl;
        std::cout << "+" << std::string(internalWidth, '-') << "+" << std::endl;
        std::cout << std::endl;

        if (isClusterHead && !tdmaMac->GetNodeQueueSizes().empty())
        {
            std::cout << "[D-TDMA STATUS] ";

            // CHANGE THIS LOOP:
            for (auto const &pair : tdmaMac->GetNodeQueueSizes())
            {
                // Access via pair.first (the MAC address) and pair.second (the size)
                std::cout << "Node " << pair.first << " Q=" << pair.second << " | ";
            }
            std::cout << std::endl;
        }
    }
}
