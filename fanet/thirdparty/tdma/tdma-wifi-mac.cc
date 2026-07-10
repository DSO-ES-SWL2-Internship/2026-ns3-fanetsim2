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

    // This method calculates the duration of each slot based on the total cycle duration and the number of slots.
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

        uint8_t tos = 0;
        // Decipher the IPv4 Header directly to get the ToS byte
        Ptr<Packet> packetCopy = mpdu->GetPacket()->Copy();
        LlcSnapHeader llc;

        // Remove the LLC/SNAP header (8 bytes) to expose the IP layer
        if (packetCopy->RemoveHeader(llc)) {
            // 0x0800 is the standard hex code for IPv4 traffic
            if (llc.GetType() == 0x0800) {
                Ipv4Header ipv4Hdr;
                packetCopy->PeekHeader(ipv4Hdr);
                tos = ipv4Hdr.GetTos();
            }
        }

        // Assign Priority based on the ToS value (custom mapping)
        uint8_t tid = 0;
        if (tos == 0x10) { tid = 1; } 
        else if (tos == 0x11) { tid = 2; }
        else if (tos == 0x20 || tos == 0x80) { tid = 3; } 
        else if (tos == 0x21) { tid = 4; }
        else if (tos == 0x30 || tos == 0xA0) { tid = 5; } 
        else if (tos == 0x31 || tos == 0xC0) { tid = 6; }
        else if (tos == 0x50) { tid = 7; }

        // Map the custom TID back to the standard hardware AC for the MAC Header
        // AC_BK = 0, AC_BE = 1, AC_VI = 2, AC_VO = 3
        uint8_t ac_tid = 0;
        if (tid == 1 || tid == 2) ac_tid = 0;      
        else if (tid == 3 || tid == 4) ac_tid = 1; 
        else if (tid == 5 || tid == 6) ac_tid = 2; 
        else if (tid == 7) ac_tid = 3;

        // Setting QoS in the header if its supported else just use WIFI_MAC_DATA
        if (GetQosSupported())
        {
            hdr.SetType(WIFI_MAC_QOSDATA);
            hdr.SetQosTid(tid);
            hdr.SetQosAckPolicy(WifiMacHeader::NORMAL_ACK);
            hdr.SetQosNoEosp();
            hdr.SetQosNoAmsdu();
            // Transmission of multiple frames in the same TXOP is not
            // supported for now
            hdr.SetQosTxopLimit(0);
        } else {
            hdr.SetType(WIFI_MAC_DATA);
        }

        Ptr<WifiMpdu> modifiedMpdu = Create<WifiMpdu>(mpdu->GetPacket(), hdr);
        // Handle bypass for small/control packets (AODV routing broadcasts, ARP requests)
        if (to.IsBroadcast() || tid == 0) {
            tid = 6;
            if (GetQosSupported()) {
                hdr.SetType(WIFI_MAC_QOSDATA);
                hdr.SetQosTid(tid);
            }
        }

        // Traffic flow trace
        if (tid >= 1 && tid <= 7) {
            std::string role = m_isClusterHead ? "CH" : "CM";
            std::string iface = m_isInterCluster ? "Backbone (5GHz)" : "Local (2.4GHz)";
            
            std::cout << "[FLOW TRACE - TX] Node: " << GetDevice()->GetNode()->GetId() 
                      << " (" << role << " | " << iface << ") " 
                      << "TID: " << (int)tid << " | Dest: " << to 
                      << " | Size: " << mpdu->GetPacket()->GetSize() << std::endl;
        }

        // Calculate current total TDMA load
        uint16_t myTotalQueue = m_pri1_status2Queue.size() + m_pri1_cmd3Queue.size() + 
                                m_pri2_status1Queue.size() + m_pri2_cmd2Queue.size() + 
                                m_pri3_highResQueue.size() + m_pri3_cmd1Queue.size() + 
                                m_pri5_lowResQueue.size();

        bool isRelayPacket = false;
        Mac48Address finalDestination = to;

        // Cooperative Relay Logic (Only for Video Traffic: TIDs 5 and 7)
        uint16_t panicThreshold = 10; // Hardcode a safe threshold guarantee
        if (!m_isClusterHead && !m_isInterCluster &&( tid == 5 || tid == 7) && myTotalQueue >= panicThreshold)
        {
            Mac48Address bestHelper = Mac48Address::GetBroadcast();

            // Find all neighbours that has available bandwidth (Queue < Offload Threshold)
            std::vector<Mac48Address> availableHelpers;
            for (auto const& neighbour : m_neighbourQueueSizes) {
                // Ensure we don't offload to the original destination (CH) or Broadcast,
                // and verify the neighbor is completely idle/ready to help
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
            
            // If we found a valid helper, alter the MAC routing
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
        DtdmaQueueHeader qHeader;
        qHeader.SetQueueSize(myTotalQueue);
        qHeader.SetIsRelay(isRelayPacket);
        qHeader.SetFinalDest(finalDestination);
        mutablePacket->AddHeader(qHeader);

        // Rebuild the MPDU with the new header
        Ptr<WifiMpdu> finalMpdu = Create<WifiMpdu>(mutablePacket, hdr);

        // Create a TdmaBufferItem to hold the MPDU and its associated information
        TdmaBufferItem item;
        item.mpdu = finalMpdu;

        // Push the item into the appropriate queue based on the TID
        if (tid == 1) { m_pri1_status2Queue.push(item); }
        else if (tid == 2) { m_pri1_cmd3Queue.push(item); }
        else if (tid == 3) { m_pri2_status1Queue.push(item); }
        else if (tid == 4) { m_pri2_cmd2Queue.push(item); }
        else if (tid == 5) { m_pri3_highResQueue.push(item); }
        else if (tid == 6) { m_pri3_cmd1Queue.push(item); }
        else if (tid == 7) { m_pri5_lowResQueue.push(item); }

        if (m_isMySlot) {
            TdmaTransmit();
        }
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

        // Check if it's the node's slot
        m_isMySlot = (m_currentSlot == m_assignedSlot);

        // Schedule the next slot
        m_tdmaEvent = Simulator::Schedule(m_slotDuration, &TdmaWifiMac::TdmaScheduleNextSlot, this);

        // If it's the node's slot, start transmitting
        if (m_isMySlot)
        {
            TdmaTransmit();
        }

        // Increment the slot counter
        m_currentSlot = (m_currentSlot + 1) % m_numSlots;
    }

    // This method transmits all packets in the buffer during the node's assigned slot.
    // It checks for slot overruns to ensure that the node does not exceed its allocated time.
    void TdmaWifiMac::TdmaTransmit()
    {
        Time slotStartTime = Simulator::Now();
        
        //Evaluate how many packets of each traffic type we can send based on the allocation table for this slot
        uint32_t status1Quota = 0, status2Quota = 0;
        uint32_t cmd1Quota = 0, cmd2Quota = 0, cmd3Quota = 0;
        uint32_t lowResQuota = 0, highResQuota = 0;

        //Scans m_allocationTable and tallies up the weights for the current cycle
        for(const auto& slot : m_allocationTable) {
            if (!slot.isOccupied) {
                continue; // Skip empty slots
            }
            // Use find() so it matches "Video_HIGH_RES", "Video_LOW_RES", etc.
            if (slot.trafficType == "Status1") status1Quota++;
            else if (slot.trafficType == "Status2") status2Quota++;
            else if (slot.trafficType == "Cmd1") cmd1Quota++;
            else if (slot.trafficType == "Cmd2") cmd2Quota++;
            else if (slot.trafficType == "Cmd3") cmd3Quota++;
            else if (slot.trafficType == "Video_LOW_RES") lowResQuota++;
            else if (slot.trafficType == "Video_HIGH_RES") highResQuota++;
        }

        //Lambda function to decrease a specific queue safely
        auto drainQueue = [&](std::queue<TdmaBufferItem>& queue, uint32_t& quota, uint8_t destTid) {
            while (!queue.empty() && quota > 0)
            {
                if (Simulator::Now() - slotStartTime >= m_slotDuration)
                {
                    NS_LOG_WARN("Slot overrun detected... Stopping transmission.");
                    return; 
                }
                TdmaBufferItem item = queue.front();
                Ptr mpdu = item.mpdu;

                if (GetQosSupported())
                {
                    // Directly use destTid instead of calculating it
                    GetQosTxop(destTid)->Queue(mpdu);
                }
                else
                {
                    GetTxop()->Queue(mpdu);
                }
                NS_LOG_FUNCTION(this << "transmit-" << m_name);
                queue.pop();
                quota--; 
            }
        };

        // if (!m_videoQueue.empty() || !m_statusQueue.empty() || !m_cmdQueue.empty()) {
        //     std::cout << "[WFQ ENFORCER] Node Slot Active | Sending Max: " 
        //               << videoQuota << " Video, " 
        //               << statusQuota << " Status, " 
        //               << cmdQuota << " Cmd." << std::endl;
        // }    

        // Pass the explicit 802.11e TIDs (0-3) to ensure GetQosTxop() returns a valid pointer
        // Priority 1/2 traffic -> Maps to Access Category BK/BE/VI
        // Priority 1
        drainQueue(m_pri1_status2Queue, status2Quota, 3); // Map to AC_VO (Highest priority hardware queue)
        drainQueue(m_pri1_cmd3Queue, cmd3Quota, 3);       // Map to AC_VO
        
        // Priority 2
        drainQueue(m_pri2_status1Queue, status1Quota, 2); // Map to AC_VI (Video queue)
        drainQueue(m_pri2_cmd2Queue, cmd2Quota, 2);       // Map to AC_VI
        
        // Priority 3
        drainQueue(m_pri3_highResQueue, highResQuota, 0); // Map to AC_BE (Best effort)
        drainQueue(m_pri3_cmd1Queue, cmd1Quota, 0);       // Map to AC_BE
        
        // Priority 5
        drainQueue(m_pri5_lowResQueue, lowResQuota, 1);   // Map to AC_BK (Background)
    }

    // This method updates the slot duration whenever the number of slots or cycle duration changes.
    void TdmaWifiMac::UpdateSlotDuration()
    {
        NS_LOG_FUNCTION(this);
        m_slotDuration = m_cycleDuration / m_numSlots;
        NS_LOG_DEBUG("Slot duration updated to " << m_slotDuration.As(Time::MS));
    }

    // This method is called when a packet is received.
    // It extracts the source and destination addresses and processes the packet accordingly.
    void TdmaWifiMac::AllocateMiniSlots() {
        //check if the cluster configuration reference has been set before trying to allocate mini-slots
        if (!m_clusterConfig) {
            NS_LOG_WARN("No ClusterMacConfig reference attached yet!");
            return;
        }
        
        //Reset the table to 12 empty slots
        m_allocationTable.clear();
        m_allocationTable.resize(m_clusterConfig->totalMiniSlots, {false, ""});

        std::vector<TrafficProfile> allowedProfiles;
        //Filter for CH
        for (const auto& p : m_clusterConfig->trafficProfiles) {
            if (m_isClusterHead && !m_isInterCluster) {
                if (p.type.find("Cmd") != std::string::npos) {
                    //Case 1: CH local antenna, pure relay for Cmds
                    allowedProfiles.push_back(p);
                }
            } else if(m_isClusterHead && m_isInterCluster) {
                //Case 2: CH Backbone antenna, full relay
                allowedProfiles.push_back(p);
            } else {
                // All other cases (CM local OR CH backbone): Allow everything
                allowedProfiles.push_back(p);
            }
        }

        //
        uint32_t slotsAvailable = m_clusterConfig->totalMiniSlots;
        //Loop through the JSON profiles (already sorted highest priority first)
        for (const auto& profile : allowedProfiles) 
        {
            //Calculate how many mini-slots this traffic needs (e.g., 0.6K / 0.1 = 6 slots)
            uint32_t slotsNeeded = std::ceil(static_cast<double>(profile.bandwidthKb / m_kbPerMiniSlot));
            
            //If we don't have enough slots left, it gets whatever is remaining (e.g., if only 4 slots left but needs 6, it gets 4 and is marked as partially allocated)
            uint32_t slotsToAllocate = std::min(slotsNeeded, slotsAvailable);
            
            //Fill the slots in the table accordingly with the traffic type and mark them as occupied
            for (uint32_t i = 0; i < slotsToAllocate; i++) {
                // Find the next available empty slot
                for (auto& slot : m_allocationTable) {
                    if (!slot.isOccupied) {
                        slot.isOccupied = true;
                        slot.trafficType = profile.type;
                        slotsAvailable--;
                        break;
                    }
                }
            }

            // If the table is full, stop allocating. Lower priorities get dropped.
            if (slotsAvailable == 0) {
                break; 
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
            uint8_t rxTid = hdr->IsQosData() ? hdr->GetQosTid() : 0;

            // Strip the Cooperative MAC Header to get the neighbour's queue size and relay information
            if (rxTid >= 1 && rxTid <= 7)
            {
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

                            uint8_t tos = 0;
                            Ptr<Packet> packetCopy = packet->Copy();
                            LlcSnapHeader llc;
                            if (packetCopy->RemoveHeader(llc)) {
                                if (llc.GetType() == 0x0800) {
                                    Ipv4Header ipv4Hdr;
                                    packetCopy->PeekHeader(ipv4Hdr);
                                    tos = ipv4Hdr.GetTos();
                                }
                        }

                        uint8_t tid = 0;
                        if (tos == 0x10) { tid = 1; } 
                        else if (tos == 0x11) { tid = 2; }
                        else if (tos == 0x20) { tid = 3; } 
                        else if (tos == 0x21) { tid = 4; }
                        else if (tos == 0x30) { tid = 5; } 
                        else if (tos == 0x31) { tid = 6; }
                        else if (tos == 0x50) { tid = 7; }

                        WifiMacHeader relayHdr = *hdr;
                        relayHdr.SetAddr1(qHeader.GetFinalDest()); 
                        relayHdr.SetAddr2(GetAddress());           
                        Ptr<WifiMpdu> cleanMpdu = Create<WifiMpdu>(packet, relayHdr);

                        TdmaBufferItem item;
                        item.mpdu = cleanMpdu;
                        
                        if (tid == 1) { m_pri1_status2Queue.push(item); }
                        else if (tid == 2) { m_pri1_cmd3Queue.push(item); }
                        else if (tid == 3) { m_pri2_status1Queue.push(item); }
                        else if (tid == 4) { m_pri2_cmd2Queue.push(item); }
                        else if (tid == 5) { m_pri3_highResQueue.push(item); }
                        else if (tid == 6) { m_pri3_cmd1Queue.push(item); }
                        else if (tid == 7) { m_pri5_lowResQueue.push(item); }
                    }
                }
            }
            if (to != GetAddress() && !to.IsBroadcast()) {
            return;
            }
            // Forward the clean packet to the higher layers
            //Ptr<WifiMpdu> cleanMpdu = Create<WifiMpdu>(cleanPacket, *hdr);
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
}


