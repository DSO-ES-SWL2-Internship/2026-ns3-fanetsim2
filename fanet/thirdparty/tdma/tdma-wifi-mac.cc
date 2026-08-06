#include "tdma-wifi-mac.h"
#include "ns3/qos-txop.h"
#include "ns3/eht-capabilities.h"
#include "ns3/he-capabilities.h"
#include "ns3/ht-capabilities.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/vht-capabilities.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/mac48-address.h"
#include "ns3/packet.h"
#include "ns3/wifi-net-device.h"
#include "ns3/node.h"
#include "dtdma-queue-header.h"
#include "ns3/socket.h"
#include "ns3/llc-snap-header.h"
#include "ns3/ipv4-header.h"
#include <map>

#define USE_NS3_QOS

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("TdmaWifiMac");

    NS_OBJECT_ENSURE_REGISTERED(TdmaWifiMac);

    TypeId TdmaWifiMac::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::TdmaWifiMac")
                                .SetParent<WifiMac>()
                                .SetGroupName("Wifi")
                                .AddConstructor<TdmaWifiMac>()
                                .AddAttribute("NumSlots", "Number of TDMA slots (equal to the number of nodes)",
                                            UintegerValue(4),
                                            MakeUintegerAccessor(&TdmaWifiMac::m_numSlots),
                                            MakeUintegerChecker<uint32_t>())
                                .AddAttribute("CycleDuration", "Duration of one TDMA cycle",
                                            TimeValue(MilliSeconds(400)), // Example: 400ms cycle
                                            MakeTimeAccessor(&TdmaWifiMac::m_cycleDuration),
                                            MakeTimeChecker())
                                .AddAttribute("TotalMiniSlots", "Total mini-slots in the frame format",
                                          UintegerValue(12),
                                          MakeUintegerAccessor(&TdmaWifiMac::m_totalMiniSlots),
                                          MakeUintegerChecker<uint32_t>())
                                .AddAttribute("KbPerMiniSlot", "Bandwidth weight per mini-slot unit",
                                          UintegerValue(1),
                                          MakeUintegerAccessor(&TdmaWifiMac::m_kbPerMiniSlot),
                                          MakeUintegerChecker<uint32_t>());
        return tid;
    }

    // the default please dont use
    TdmaWifiMac::TdmaWifiMac()
        : m_numSlots(4),
        m_cycleDuration(MilliSeconds(400)),
        m_assignedSlot(0),
        m_currentSlot(0),
        m_isMySlot(false)
    {
        NS_LOG_FUNCTION(this);
        UpdateSlotDuration(); // Initialize slot duration
        SetTypeOfStation(ADHOC_STA);
        m_slotHistory.resize(72, "IDLE");
    }

    TdmaWifiMac::~TdmaWifiMac()
    {
        NS_LOG_FUNCTION(this);
    }

    void TdmaWifiMac::SetIsClusterHead(bool isCH)
    {
        m_isClusterHead = isCH; 
    }

    void TdmaWifiMac::SetIsInterCluster(bool isInter) {
        m_isInterCluster = isInter;
    }

    // This method calculates the duration of each slot based on the total cycle duration and the number of slots.
    void TdmaWifiMac::SetTdmaParameters(uint32_t numSlots, Time cycleDuration, uint32_t assignedSlot)
    {
        m_numSlots = numSlots;
        m_cycleDuration = cycleDuration;
        m_assignedSlot = assignedSlot;
        UpdateSlotDuration(); // Recalculate slot duration
    }

    //
    void TdmaWifiMac::SetClusterConfig(const ClusterMacConfig* sharedConfig)
    {
        m_clusterConfig = sharedConfig;
    }

    // This method updates the slot duration whenever the number of slots or cycle duration changes.
    void TdmaWifiMac::StartTdma()
    {
        NS_LOG_FUNCTION(this);
        m_currentSlot = 0;
        TdmaScheduleNextSlot();
    }

    // 
    void TdmaWifiMac::Enqueue(Ptr<WifiMpdu> mpdu, Mac48Address to, Mac48Address from)
    {
        // Get existing MAC header provided by ns3
        WifiMacHeader hdr = mpdu->GetHeader();

        uint32_t packetSize = mpdu->GetPacket()->GetSize();
        std::cout << "[MAC ENQUEUE] Node " << GetDevice()->GetNode()->GetId() 
                  << " | Size: " << packetSize << " bytes | To: " << to << std::endl;

        // Setting Address
        hdr.SetAddr1(to);
        hdr.SetAddr2(GetAddress());
        hdr.SetAddr3(Mac48Address::GetBroadcast()); // For ad-hoc, we can use broadcast for the third address
        hdr.SetDsNotFrom();
        hdr.SetDsNotTo();

        if (GetHtSupported(to))
        {
            hdr.SetNoOrder(); // explicitly set to 0 for the time being since HT control field is not
                            // yet implemented (set it to 1 when implemented)
        }

        // when new packet is to be sent, check if the destination is a new location
        // FIXED: Do not register broadcast addresses as brand new unicast stations
        if (!to.IsBroadcast() && GetWifiRemoteStationManager()->IsBrandNew(to))
        {
        //     // In ad hoc mode, we assume that every destination supports all the rates we support.
        //     // Register the station with all the different capabilities
        //     // HT (High Throughput)
        //     // VHT (Very High Throughput)
        //     // HE (High Efficiency)
        //     // EHT (Extremely High Throughput)
        //     // ensure that the mac layer can support different station types
            if (GetHtSupported(to))
            {
                GetWifiRemoteStationManager()->AddAllSupportedMcs(to);
                GetWifiRemoteStationManager()->AddStationHtCapabilities(
                    to, 
                    GetHtCapabilities(SINGLE_LINK_OP_ID));
            }
            if (GetVhtSupported(SINGLE_LINK_OP_ID))
            {
                GetWifiRemoteStationManager()->AddStationVhtCapabilities(
                    to,
                    GetVhtCapabilities(SINGLE_LINK_OP_ID));
            }
            if (GetHeSupported())
            {
                GetWifiRemoteStationManager()->AddStationHeCapabilities(
                    to,
                    GetHeCapabilities(SINGLE_LINK_OP_ID));
            }
            if (GetEhtSupported())
            {
                GetWifiRemoteStationManager()->AddStationEhtCapabilities(
                    to,
                    GetEhtCapabilities(SINGLE_LINK_OP_ID));
            }
            GetWifiRemoteStationManager()->AddAllSupportedModes(to);
            // GetWifiRemoteStationManager()->RecordDisassociated(to);
        }

        uint8_t dummyTid = 7;

#ifdef USE_NS3_QOS
        // Setting QoS in the header if its supported else just use WIFI_MAC_DATA
        if (GetQosSupported())
        {
            hdr.SetType(WIFI_MAC_QOSDATA);
            hdr.SetQosTid(dummyTid);
            hdr.SetQosAckPolicy(WifiMacHeader::NORMAL_ACK);
            hdr.SetQosNoEosp();
            hdr.SetQosNoAmsdu();
            // Transmission of multiple frames in the same TXOP is not
            // supported for now
            hdr.SetQosTxopLimit(0);
        } else {
            hdr.SetType(WIFI_MAC_DATA);
        }
#else
        hdr.SetType(WIFI_MAC_DATA);
#endif

        // Extract the ToS value from the IPv4 header
        uint8_t tos = 0;
        Ptr<Packet> packetCopy = mpdu->GetPacket()->Copy();
        LlcSnapHeader llc;
        if (packetCopy->RemoveHeader(llc)) {
            if (llc.GetType() == 0x0800) { // Check if it is an IPv4 packet
                Ipv4Header ipv4Hdr;
                packetCopy->PeekHeader(ipv4Hdr);
                tos = ipv4Hdr.GetTos();
            }
        }

        // Calculate current total TDMA load
        uint16_t myTotalQueue = m_pri5_lowResQueue.size();
        [[maybe_unused]] bool isRelayPacket = false;
        Mac48Address finalDestination = to;

        // Cooperative Relay Logic
        uint16_t panicThreshold = 1000; 
        
        // Only CH do Coop-MAC offloading
        if (m_isClusterHead && myTotalQueue >= panicThreshold)
        {
            Mac48Address bestHelper = Mac48Address::GetBroadcast();

            // Find all neighbours that has available bandwidth (Queue < Offload Threshold)
            std::vector<Mac48Address> availableHelpers;
            for (auto const& neighbour : m_neighbourQueueSizes) {
                // Ensure we don't offload to the original destination or Broadcast
                if (neighbour.first != to && !neighbour.first.IsBroadcast() && neighbour.second < panicThreshold) { 
                    availableHelpers.push_back(neighbour.first);
                }
            }

            // Round-robin distribution
            if (!availableHelpers.empty()){
                // Use a map so each node maintains its own independent round-robin index for fairness
                static std::map<uint32_t, uint32_t> IndexMap; 
                uint32_t myNodeId = GetDevice()->GetNode()->GetId();

                bestHelper = availableHelpers[IndexMap[myNodeId] % availableHelpers.size()];
                IndexMap[myNodeId]++;
            }
            
            // If CH found a valid helper, alter the MAC routing
            if (bestHelper != Mac48Address::GetBroadcast()) {
                std::cout << "[COOP-MAC] Node " << GetDevice()->GetNode()->GetId() 
                          << " is OVERLOADED (Q=" << myTotalQueue 
                          << "). Offloading Video packet to Helper Node: " << bestHelper << std::endl;
                
                isRelayPacket = true;
                finalDestination = to; // Save the CH address
                to = bestHelper;       // Physically transmit to the Helper instead
                hdr.SetAddr1(to);      // Overwrite the Wi-Fi header destination
            }
        }

        // Attach the Cooperative MAC Header to the packet
        Ptr<Packet> mutablePacket = mpdu->GetPacket()->Copy();
        DtdmaQueueHeader qHeader;              // Create a new instance of the custom header
        qHeader.SetQueueSize(myTotalQueue);    // Set the current queue size for this node
        qHeader.SetIsRelay(isRelayPacket);     // Indicate if this packet is being relayed
        qHeader.SetFinalDest(finalDestination);// Set the final destination to the original CH address
        mutablePacket->AddHeader(qHeader);     // Add the custom header to the packet

        // Rebuild the MPDU with the new header
        Ptr<WifiMpdu> finalMpdu = Create<WifiMpdu>(mutablePacket, hdr);

        // Create a TdmaBufferItem to hold the MPDU and its associated information
        TdmaBufferItem item;
        item.mpdu = finalMpdu;

        // Handle bypass for control packets (AODV routing broadcasts, ARP requests)
        if (to.IsBroadcast() || tos == 0x00 || tos == 0xc0) 
        {
            if (GetQosSupported()) {
                Ptr<QosTxop> qosTxop = GetQosTxop(7);
                if (qosTxop != nullptr) {
                    qosTxop->Queue(finalMpdu); // Bypasses TDMA and transmits instantly
                    return; 
                }
            }
            Ptr<Txop> txop = GetTxop();
            if (txop != nullptr) {
                txop->Queue(finalMpdu); // Bypasses TDMA and transmits instantly
            }
            return;
        }

        // Traffic flow trace
        std::string role = m_isClusterHead ? "CH" : "CM";
        std::string iface = m_isInterCluster ? "Backbone (5GHz)" : "Local (2.4GHz)";
        std::cout << "[FLOW TRACE - TX] Node: " << GetDevice()->GetNode()->GetId() 
                    << " (" << role << " | " << iface << ") " 
                    << "ToS: 0x" << std::hex << (int)tos << std::dec << " | Dest: " << to 
                    << " | Size: " << mpdu->GetPacket()->GetSize() << std::endl;

        // Drop packet to prevent bufferbloat 
        if (tos == 0x50 && m_pri5_lowResQueue.size() >= m_maxQueueSize) {
            std::cout << "[DROP] Video Queue FULL on Node " << GetDevice()->GetNode()->GetId() << std::endl;
            return; 
        }
        if (tos == 0x20 && m_pri2_status1Queue.size() >= m_maxQueueSize) {
            std::cout << "[DROP] Status Queue FULL on Node " << GetDevice()->GetNode()->GetId() << std::endl;
            return; 
        }

        // Enqueue based on ToS value
        if (tos == 0x10) {          m_pri1_status2Queue.push(item);  // Status 2
        } else if (tos == 0x11) {   m_pri1_cmd3Queue.push(item);     // Command 3
        } else if (tos == 0x20) {   m_pri2_status1Queue.push(item);  // Status 1
        } else if (tos == 0x21) {   m_pri2_cmd2Queue.push(item);     // Command 2
        } else if (tos == 0x30) {   m_pri3_highResQueue.push(item);  // High-Res Video
        } else if (tos == 0x31) {   m_pri3_cmd1Queue.push(item);     // Command 1
        } else { m_pri5_lowResQueue.push(item); // Default to Low-Res Video for any other ToS values
        }
    }

    // This method updates the slot duration whenever the number of slots or cycle duration changes.
    void TdmaWifiMac::UpdateSlotDuration()
    {
        NS_LOG_FUNCTION(this);
        m_slotDuration = m_cycleDuration / m_numSlots;
        NS_LOG_DEBUG("Slot duration updated to " << m_slotDuration.As(Time::MS));
    }

    // Set to always return true meaning that this MAC layer allows packet forwarding to any MAC address
    bool TdmaWifiMac::CanForwardPacketsTo(Mac48Address to) const
    {
        return true;
    }

    // This method schedules the next slot in the TDMA cycle and checks if it's the node's assigned slot to transmit.
    void TdmaWifiMac::TdmaScheduleNextSlot()
    {
        NS_LOG_FUNCTION(this);

        m_currentSlot = (m_currentSlot + 1) % 72; // Force to 72 slots

        m_isMySlot = false;
        std::string scheduledTraffic = "IDLE";

        // Check the global matrix for this exact moment in time
        if (m_currentSlot < m_allocationTable.size()) 
        {
            MiniSlot currentSlotData = m_allocationTable[m_currentSlot];

            // "Transmit at this TIME... based on who I am"
            if (currentSlotData.isOccupied && currentSlotData.ownerNodeId == m_assignedSlot) 
            {
                m_isMySlot = true;
                // ""...based on the type of slot it is"
                scheduledTraffic = currentSlotData.trafficType; 
            }
        }

        // Trigger transmission if the slot strictly belongs to this node
        if (m_isMySlot && scheduledTraffic != "IDLE") 
        {
            TdmaTransmit(scheduledTraffic); // Pass the strict traffic rule to the hardware
        }

        // Schedule the next tick
        m_tdmaEvent = Simulator::Schedule(m_slotDuration, &TdmaWifiMac::TdmaScheduleNextSlot, this);
    }

    // This method transmits one packet from the appropriate queue based on the scheduled traffic type
    void TdmaWifiMac::TdmaTransmit(std::string scheduledTraffic)
    {
        // Check if the slot actually demands a traffic type or is IDLE
        if (scheduledTraffic == "IDLE")
        {
            return;
        }

        std::queue<TdmaBufferItem>* targetQueue = nullptr;
        std::string actualTrafficSent = scheduledTraffic;

        if (scheduledTraffic.find("Status") != std::string::npos) {
            if (!m_pri1_status2Queue.empty()) targetQueue = &m_pri1_status2Queue;
            else targetQueue = &m_pri2_status1Queue;
        } 
        else if (scheduledTraffic.find("Cmd") != std::string::npos) {
            if (!m_pri1_cmd3Queue.empty()) targetQueue = &m_pri1_cmd3Queue;
            else if (!m_pri2_cmd2Queue.empty()) targetQueue = &m_pri2_cmd2Queue;
            else targetQueue = &m_pri3_cmd1Queue;
        } 
        else if (scheduledTraffic.find("Video") != std::string::npos) {
            if (!m_pri3_highResQueue.empty()) targetQueue = &m_pri3_highResQueue;
            else targetQueue = &m_pri5_lowResQueue;
        } 
        else {
            targetQueue = &m_pri5_lowResQueue; // Fallback
        }
        
        if (m_currentSlot < m_slotHistory.size()) {
            m_slotHistory[m_currentSlot] = actualTrafficSent;
        }

        // Prevent transmission if the targeted queue is empty
        if (targetQueue->empty()) {
            return;
        }

        // Pull one packet per slot
        TdmaBufferItem item = targetQueue->front();
        targetQueue->pop(); 

        if (item.mpdu == nullptr) {
            return;
        }

        Ptr<WifiMpdu> mpdu = item.mpdu;

        if (GetQosSupported()) {
            Ptr<QosTxop> qosTxop = GetQosTxop(7);
            if (qosTxop != nullptr) {
                qosTxop->Queue(mpdu);
            } else {
                return; // Hardware not ready
            }
        } else {
            Ptr<Txop> txop = GetTxop();
            if (txop != nullptr) {
                txop->Queue(mpdu);
            } else {
                return; // Hardware not ready
            }
        }

        std::cout << "[TDMA TX] Node " << GetDevice()->GetNode()->GetId() << " transmitted 1 packet for slot type: [" << scheduledTraffic << "]" << std::endl;
    }

    // This method is called when a packet is received.
    // It extracts the source and destination addresses and processes the packet accordingly.
    void TdmaWifiMac::AllocateMiniSlots() {
        //check if the cluster configuration reference has been set before trying to allocate mini-slots
        if (!m_clusterConfig) {
            NS_LOG_WARN("No ClusterMacConfig reference attached yet!");
            return;
        }
        
        std::string mySsid = GetSsid().PeekString(); // Get the SSID of this node
        m_isInterCluster = (mySsid.find("InterCluster") != std::string::npos); // Determine if this node is part of the inter-cluster network based on SSID

        //Reset the table to 72 empty slots
        m_allocationTable.clear();
        m_allocationTable.resize(72, {false, "IDLE", 0});

        // Lambda function to fill a row of the allocation table with the traffic profile from the JSON config
        auto fillRowWithConfig = [&](int startSlot, int endSlot, uint32_t nodeId) {
            
            // Safety fallback if JSON is empty
            if (m_clusterConfig->trafficProfiles.empty()) {
                for (int i = startSlot; i <= endSlot; i++) m_allocationTable[i] = {true, "IDLE", nodeId};
                return;
            }

            uint32_t profileIndex = 0;
            for (int i = startSlot; i <= endSlot; i++) {
                // Fetch the dynamic application name from the JSON config
                std::string currentProfile = m_clusterConfig->trafficProfiles[profileIndex].type; 
                
                m_allocationTable[i] = {true, currentProfile, nodeId};
                
                // Distributes 12 slots evenly across all apps in config
                profileIndex = (profileIndex + 1) % m_clusterConfig->trafficProfiles.size();
            }
        };

        // INTER-CLUSTER (5GHz Backbone - 0.2K per slot)
        if (m_isInterCluster) 
        {
            if (m_assignedSlot == 0) // GDT
            {
                // GDT fills rows 2 and 4
                fillRowWithConfig(12, 23, m_assignedSlot);
                fillRowWithConfig(36, 47, m_assignedSlot);
            } 
            else if (m_isClusterHead) // ICluster Head
            {
                bool isCH1 = (m_assignedSlot >= 1 && m_assignedSlot <= 4); // Cluster 0
                bool isCH2 = (m_assignedSlot >= 5 && m_assignedSlot <= 8); // Cluster 1

                if (isCH1) {
                    // CH1 fills rows 1 and 5
                    fillRowWithConfig(0, 11, m_assignedSlot);
                    fillRowWithConfig(48, 59, m_assignedSlot);
                } 
                else if (isCH2) {
                    // CH2 fills rows 3 and 6
                    fillRowWithConfig(24, 35, m_assignedSlot);
                    fillRowWithConfig(60, 71, m_assignedSlot);
                }
            }
        }
        // INTRA-CLUSTER (2.4GHz Local - 0.1K per slot)
        else 
        {
            if (m_isClusterHead) // CH broadcasting down to CMs
            {
                // CH fills rows 2, 4, 6
                fillRowWithConfig(12, 23, m_assignedSlot);
                fillRowWithConfig(36, 47, m_assignedSlot);
                fillRowWithConfig(60, 71, m_assignedSlot);
            } 
            else // CM transmitting up to the CH
            {
                uint32_t startSlot = 0;
                
                if (m_assignedSlot == 1 || m_assignedSlot == 5) {
                    startSlot = 0;  // Row 1
                } 
                else if (m_assignedSlot == 3 || m_assignedSlot == 7) {
                    startSlot = 24; // Row 3
                } 
                else if (m_assignedSlot == 4 || m_assignedSlot == 8) {
                    startSlot = 48; // Row 5
                }
                
                // CM fills its 12-slot row
                fillRowWithConfig(startSlot, startSlot + 11, m_assignedSlot);
            }
        }
    }

    // Receive MAC protocol data unit (MPDU) and extract the source and destination address
    void TdmaWifiMac::Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
    {
        NS_LOG_FUNCTION(this << *mpdu << +linkId);
        NS_LOG_FUNCTION(this << "receive-" << m_name);
        const WifiMacHeader* hdr = &mpdu->GetHeader();
        NS_ASSERT(!hdr->IsCtl());
        Mac48Address from = hdr->GetAddr2();
        Mac48Address to = hdr->GetAddr1();

        // Check if the sender is a new station, and registers its capabilities if so
        if (GetWifiRemoteStationManager()->IsBrandNew(from))
        {
            // In ad hoc mode, we assume that every destination supports all the rates we support.
            if (GetHtSupported(to))
            {
                GetWifiRemoteStationManager()->AddAllSupportedMcs(from);
                GetWifiRemoteStationManager()->AddStationHtCapabilities(
                    from,
                    GetHtCapabilities(SINGLE_LINK_OP_ID));
            }
            if (GetVhtSupported(SINGLE_LINK_OP_ID))
            {
                GetWifiRemoteStationManager()->AddStationVhtCapabilities(
                    from,
                    GetVhtCapabilities(SINGLE_LINK_OP_ID));
            }
            if (GetHeSupported())
            {
                GetWifiRemoteStationManager()->AddStationHeCapabilities(
                    from,
                    GetHeCapabilities(SINGLE_LINK_OP_ID));
            }
            if (GetEhtSupported())
            {
                GetWifiRemoteStationManager()->AddStationEhtCapabilities(
                    from,
                    GetEhtCapabilities(SINGLE_LINK_OP_ID));
            }
            GetWifiRemoteStationManager()->AddAllSupportedModes(from);
        }

        // Control frames bypass the TDMA logic and are processed by the base class (WifiMac)
        if (hdr->IsCtl()) {
            WifiMac::Receive(mpdu, linkId);
            return;
        }

        // If the received packet is QoS A-MSDU, it is deaggregated.
        // Otherwise, it is forwarded to higher layers.
        if (hdr->IsData())
        {
            Ptr<Packet> packet = mpdu->GetPacket()->Copy();
            // uint8_t rxTid = hdr->IsQosData() ? hdr->GetQosTid() : 0;

            // Strip the Cooperative MAC Header to get the neighbour's queue size and relay information
            DtdmaQueueHeader qHeader;
            if (packet->RemoveHeader(qHeader)) 
            {
                m_neighbourQueueSizes[from] = qHeader.GetQueueSize();

                // DIRECT QUEUE INJECTION TO PREVENT PING-PONG LOOP
                if (qHeader.GetIsRelay() && to == GetAddress()) 
                {
                    std::cout << "[COOP-MAC] Node " << GetDevice()->GetNode()->GetId() 
                            << " intercepting Offloaded packet from " << from 
                            << ". Re-queuing for CH: " << qHeader.GetFinalDest() << std::endl;
                }
            }

            // Discard packets that are not intended for this node (except for broadcast)
            if (to != GetAddress() && !to.IsBroadcast()) {
                return;
            }

            // Forward the clean packet to the higher layers
            ForwardUp(packet, from, to);
            return;
        }
        // if not a data packet, it will be processed by the base class
        WifiMac::Receive(mpdu, linkId);
    }

    //
    void TdmaWifiMac::DoCompleteConfig()
    {
        //
    }

    // This method returns the traffic type allocated to a specific slot ID. 
    //If the slot is not occupied, it returns "IDLE".
    std::string TdmaWifiMac::GetSlotTrafficType(uint32_t slotId) const
    {
        // Safety check to avoid out-of-bounds crashes
        if (slotId >= m_allocationTable.size()) 
        {
            return "IDLE";
        }
        
        // Return the traffic type if occupied, otherwise return "IDLE"
        return m_allocationTable[slotId].isOccupied ? m_allocationTable[slotId].trafficType : "IDLE";
    }
    
    std::string TdmaWifiMac::GetSlotHistory(uint32_t slotId) const
    {
        if (slotId >= m_slotHistory.size()) {
            return "IDLE";
        }
        return m_slotHistory[slotId];
    }
}