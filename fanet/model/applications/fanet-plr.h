#ifndef FANET_PLR_H
#define FANET_PLR_H

#include "ns3/FANETHeader.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/packet.h"

namespace ns3
{
    class FANETApplication;
    class PLRManager
    {
        public:
            PLRManager();
            virtual ~PLRManager();

            void Setup(uint32_t nNodes);

            void StartTest(Ptr<FANETApplication> app, double startTime, uint32_t nodeId, Ipv4Address destAddress, uint32_t pktsToSend, double interval);

            
            void HandleData(Ptr<FANETApplication> app, FANETHeader* header, Ptr<Packet> packet, Address from);
            void HandleRequest(Ptr<FANETApplication> app, FANETHeader* header, Ptr<Packet> packet, Address from);
            void HandleResponse(Ptr<FANETApplication> app, FANETHeader* header, Ptr<Packet> packet, Address from);

            void SendData(Ptr<FANETApplication> app, uint32_t nodeId, Ipv4Address destAddress, uint32_t pktsToSend, double interval);
            void SendRequest(Ptr<FANETApplication> app, uint32_t nodeId, Ipv4Address destAddress);
            void SendResponse(Ptr<FANETApplication> app, uint32_t nodeId, Ipv4Address destAddress);
            
            double GetPLR(uint32_t nodeId);

            bool HasReceivedNetworkBroadcast(Ptr<FANETApplication> app, FANETHeader* header);
            void HandleNetworkBroadcast(Ptr<FANETApplication> app, FANETHeader* header, Ptr<Packet> packet, Address from);

        private:
            std::vector<uint32_t> m_sequenceNumberPLR;
            std::vector<uint32_t> m_expectedSeqPLR;
            std::vector<uint32_t> m_receivedPacketsPLR;
            std::vector<uint32_t> m_lostPacketsPLR;
            std::vector<uint32_t> m_packetsSentPLR;
    };
}


#endif