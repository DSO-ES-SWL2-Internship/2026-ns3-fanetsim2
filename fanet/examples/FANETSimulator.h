#ifndef FANET_SIMULATOR_H
#define FANET_SIMULATOR_H

// Helper Classes Includes
#include "ns3/fanet-module.h"
#include "ns3/tdma-wifi-mac.h"
#include "ns3/FANETDeviceHelper.h"

namespace ns3 
{
    struct TrafficWindow {
        double startTime; // Time to start the profile
        double endTime;   // Time to end the profile
        std::vector<uint32_t> newClusterHeads; // List of new cluster head node IDs for this window
        std::vector<TrafficProfile> profiles; // Traffic profiles to apply during this duration
    };
    
    class FANETSimulator : public Object
    {
        private:

            // Variables to store basic information of the network

            /// @brief Number of clusters in the FANET.
            uint32_t nClusters;

            /// @brief Number of cluster nodes in each cluster.
            std::vector<uint32_t> nClusterNodes;

            std::string nClusterNodesString;

            /// @brief File name for NetAnim to run on.
            std::string fileName;

            /// @brief Duration of each cycle in TDMA
            uint32_t cycleDuration;

            double simDuration;
            double m_updateTime;
            uint32_t m_targetClusterIndex;
            uint32_t m_targetNodeIndex;
            uint16_t m_targetPort;
            std::string m_commandString;

            std::unordered_map<std::string, WifiStandard> wifiStandardMap = {
                {"WIFI_STANDARD_80211a", WIFI_STANDARD_80211a},
                {"WIFI_STANDARD_80211b", WIFI_STANDARD_80211b},
                {"WIFI_STANDARD_80211g", WIFI_STANDARD_80211g},
                {"WIFI_STANDARD_80211p", WIFI_STANDARD_80211p},
                {"WIFI_STANDARD_80211n", WIFI_STANDARD_80211n},
                {"WIFI_STANDARD_80211ac", WIFI_STANDARD_80211ac},
                {"WIFI_STANDARD_80211ax", WIFI_STANDARD_80211ax},
                {"WIFI_STANDARD_80211ad", WIFI_STANDARD_80211ad},
                {"WIFI_STANDARD_80211be", WIFI_STANDARD_80211be}
            };

            std::unordered_map<std::string, RoutingProtocol> routingProtocolMap = {
                {"AODV", AODV},
                {"OLSR", OLSR},
                {"DSDV", DSDV},
                {"DSR", DSR}
            };

            // Track which nodes have applications installed and their current state
            struct NodeAppState {
                Ptr<Node> node;
                ApplicationContainer videoApp;
                ApplicationContainer highResVideoApp;
                ApplicationContainer statusApp;
                ApplicationContainer cmdApp;
                bool isCurrentlyCH = false;
            };
            std::map<uint32_t, NodeAppState> m_nodeApps; // Key = Node ID

            // Methods
            
            /// @brief Obtain from user, the number of clusters to simulate
            void GetNClusters();
            
            /// @brief Obtain from user, the number of nodes in each cluster to simulator
            void GetNClusterNodes();

            /**
             * @brief Setting all the clusters to have nClusterMem nodes
             * 
             * @param nClusters Number of clusters
             * 
             * @param nClusterMem Number of nodes per cluster
             */
            void SetConstNClusterNodes(uint32_t nClusters, uint32_t nClusterNodes);

            /// @brief Obtain from user, the duration of each frame in TDMA
            void GetCycleDuration();

            void GetSimulationDuration();

            void CreateNetwork();
            void InstallDevices();
            void SetMobility();
            void SetRoutingProtocol();
            void AssignAddress();
            void SetUpNetAnim();
            void CommandCallBack(Ptr<Socket> socket); // Callback method to handle the reception of a dynamic command at the drone
            void ExecuteProfileSwap(std::vector<TrafficProfile> profilesToApply, std::vector<uint32_t> newCHs, std::string stageName);
            void PrintTdmaGridMap(Ptr<Node> node, Ptr<WifiNetDevice> wifiDev, Ptr<TdmaWifiMac> tdmaMac);
            void HandleCommand(Ptr<Node> rxNode, Ptr<Packet> packet);
            void ConfigureInterfaceMetrics(); 
            Ptr<WifiNetDevice> getWifiNetDevice(uint32_t nodeId, bool is_inter_else_intra);

            std::vector<TrafficProfile> m_currentActiveProfiles; //To keep track of currently active profiles for logging purposes
            void PeriodicTopologySync();//Method to periodically synchronize the topology and print the TDMA grid map for each node
            std::deque<ClusterMacConfig> m_intraClusterConfigs; 
            std::deque<ClusterMacConfig> m_interClusterConfigs;
            std::deque<ClusterMacConfig> m_chIntraConfigs;
            std::vector<uint32_t> m_initialCHs; // To keep track of the initial cluster heads for logging purposes
            
        public:
            static TypeId GetTypeId();
            /// @brief Create a FANET Simulator
            FANETSimulator();
            /// @brief Destroy the FANET Simulator
            ~FANETSimulator();

            std::vector<TrafficProfile> m_trafficProfiles;
            std::vector<TrafficProfile> m_updateProfiles;
            std::vector<TrafficWindow> m_scheduledtrafficWindows;

            /// @brief Helper for managing FANET topology.
            Ptr<FANETTopologyHelper> fanet;

            /// @brief Helper for configuring FANET devices.
            Ptr<FANETDeviceHelper> fanetDevices;

            /// @brief Helper for handling FANET node mobility.
            FANETMobilityHelper* mobility;

            /// @brief Helper for setting up FANET routing protocols.
            Ptr<FANETRoutingHelper> router;

            /// @brief Helper for managing FANET IPv4 addressing.
            Ptr<FANETAddressHelper> ipv4;

            /// @brief Pointer to the FANET animation helper for visualization.
            FANETAnimationHelper *anim;

            void ParseClusterNodesString();

            void Setup();

            /**
             * @brief Run basic simulation of the fanet
             * 
             * @param fileName Name of output file neccessary for NetAnim
             */
            void RunSimulation();

            void SetupSimulation(std::string jsonFilePath);
            void UpdateNodeApplications(Ptr<Node> node, bool isNowCH);

    };
}

#endif