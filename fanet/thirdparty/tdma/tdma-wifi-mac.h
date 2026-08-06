#ifndef TDMA_WIFI_MAC_H
#define TDMA_WIFI_MAC_H

#include "ns3/wifi-mac.h"
#include "ns3/wifi-mac-header.h"
#include <string>
#include <vector>
#include <queue>
#include <map>
#include "ns3/mac48-address.h"

namespace ns3
{

    struct TdmaBufferItem
    {
        Ptr<WifiMpdu> mpdu; // The packet to be transmitted
        WifiMacHeader hdr;  // The MAC header for the packet
        Mac48Address to;    // The destination address
    };

    /// Structure to represent a mini-slot in the TDMA schedule
    struct MiniSlot 
    {
        bool isOccupied; // Indicates if the mini-slot is occupied
        std::string trafficType; // Type of traffic assigned to this mini-slot (e.g., "video", "audio", "data")
        uint32_t ownerNodeId; // ID of the node that owns this mini-slot
    };

    struct TrafficProfile 
    {
        std::string type;
        uint32_t priority; // Higher value means higher priority
        uint32_t bandwidthKb; // Bandwidth requirement, store as integer
    };

    //Configuration structure for the TDMA MAC, which can be populated from JSON data
    struct ClusterMacConfig 
    {
        std::vector<TrafficProfile> trafficProfiles;
        uint32_t totalMiniSlots = 12;
        uint32_t kbPerMiniSlot = 1;
    };

    class TdmaWifiMac : public WifiMac
    {
        public:
            static TypeId GetTypeId();
            TdmaWifiMac();
            virtual ~TdmaWifiMac();

            void SetTdmaParameters(uint32_t numSlots, Time cycleDuration, uint32_t assignedSlot);
            void StartTdma();
            void Enqueue(Ptr<WifiMpdu> mpdu, Mac48Address to, Mac48Address from) override;
            // void Enqueue(Ptr<Packet> packet, Mac48Address to) override;
            bool CanForwardPacketsTo(Mac48Address to) const override;

            void AllocateMiniSlots();

            std::string GetSlotTrafficType(uint32_t slotId) const;  
            std::string GetSlotHistory(uint32_t slotId) const;


            // Function to set the cluster configuration, which includes traffic profiles and mini-slot allocation 
            //(to wire up the reference from the JSON data to the MAC layer))
            void SetClusterConfig(const ClusterMacConfig* sharedConfig);
            void SetIsClusterHead(bool isCH);
            void SetIsInterCluster(bool isInter);
            void ClearQueueTracking() { m_nodeQueueSizes.clear(); }
            const std::map<Mac48Address, uint16_t>& GetNodeQueueSizes() const { return m_nodeQueueSizes; }
            void setName(std::string& name)
            {
                m_name = name;
            }
        private:
            void TdmaScheduleNextSlot();
            void TdmaTransmit();
            void UpdateSlotDuration();
            void Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId) override;
            void DoCompleteConfig() override;
            void TdmaTransmit(std::string scheduledTraffic);
        
            std::string m_name;
            uint32_t m_numSlots;          // Total number of TDMA slots (equal to the number of nodes)
            Time m_cycleDuration;         // Duration of one TDMA cycle
            Time m_slotDuration;          // Duration of each slot (calculated as cycleDuration / numSlots)
            uint32_t m_assignedSlot;      // Slot assigned to this node
            uint32_t m_currentSlot;       // Current slot in the TDMA cycle
            EventId m_tdmaEvent;          // Event for scheduling the next slot
            bool m_isMySlot;              // Flag to indicate if it's the node's slot
            uint32_t m_totalMiniSlots;    // 
            uint32_t m_kbPerMiniSlot;
            bool m_isInterCluster;        // Flag to indicate if this interface is Inter-Cluster
            bool m_isClusterHead;         // Flag to indicate if this node is a cluster head

            const ClusterMacConfig* m_clusterConfig = nullptr; // Pointer to the shared cluster configuration

            std::vector<MiniSlot> m_allocationTable;
            std::map<Mac48Address, std::queue<TdmaBufferItem>> m_nodeQueues; // Map of node MAC addresses to their respective queues
            std::map<Mac48Address, uint16_t> m_nodeQueueSizes; // Map to track the latest queue size for each node (keyed by MAC address)  
            std::vector<std::string> m_slotHistory;

            //MAC-Level WFQ Queues
            std::queue<TdmaBufferItem> m_pri1_status2Queue;
            std::queue<TdmaBufferItem> m_pri1_cmd3Queue;
            std::queue<TdmaBufferItem> m_pri2_status1Queue;
            std::queue<TdmaBufferItem> m_pri2_cmd2Queue;
            std::queue<TdmaBufferItem> m_pri3_highResQueue;
            std::queue<TdmaBufferItem> m_pri5_lowResQueue;
            std::queue<TdmaBufferItem> m_pri3_cmd1Queue;

            //Leaky Bucket Limits (Max packets allowed to wait)
            uint32_t m_maxQueueSize = 500;
            
            // Map to track the latest queue size for each neighbour node (keyed by MAC address) 
            std::map<Mac48Address, uint16_t> m_neighbourQueueSizes;  

            // Threshold before begging neighbours for bandwidth
            uint16_t m_offloadThreshold = 5; // Threshold for offloading packets to the next node
    };
}
#endif