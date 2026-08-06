#include "dtdma-queue-header.h"
#include "ns3/object-factory.h"

namespace ns3 
{

    NS_OBJECT_ENSURE_REGISTERED(DtdmaQueueHeader);

    DtdmaQueueHeader::DtdmaQueueHeader() : m_queueSize(0),
                                           m_isRelay(0),
                                           m_finalDest(Mac48Address::GetBroadcast()) 
    {}

    DtdmaQueueHeader::~DtdmaQueueHeader() {}

    TypeId DtdmaQueueHeader::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::DtdmaQueueHeader")
            .SetParent<Header>()
            .SetGroupName("Wifi")
            .AddConstructor<DtdmaQueueHeader>();
        return tid;
    }

    TypeId DtdmaQueueHeader::GetInstanceTypeId(void) const {
        return GetTypeId();
    }

    //Tell ns-3 exactly how many bytes to allocate for this header
    uint32_t DtdmaQueueHeader::GetSerializedSize(void) const {
        // 2 bytes(Queue Size) + 1 byte(IsRelay) + 6 bytes(FinalDest) = 9 bytes total
        return 2 + 1 + 6;
    }

    //Convert the C++ variable into physical bytes
    void DtdmaQueueHeader::Serialize(Buffer::Iterator start) const {
        start.WriteU16(m_queueSize);    // Write 2 bytes
        start.WriteU8(m_isRelay);       // Write 1 byte

        // Mac48Address is 6 bytes, so we need to convert it to a byte array (due to no "WriteMac" func.)
        uint8_t buf[6];
        m_finalDest.CopyTo(buf);
        start.Write(buf, 6);            // Write 6 bytes
    }

    //Extract the physical bytes back into the C++ variable
    uint32_t DtdmaQueueHeader::Deserialize(Buffer::Iterator start) {
        m_queueSize = start.ReadU16(); // Read 2 bytes
        m_isRelay = start.ReadU8();    // Read 1 byte

        // Read the next 6 bytes and reconstruct the Mac48Address
        uint8_t buf[6];
        start.Read(buf, 6);
        m_finalDest.CopyFrom(buf);

        return GetSerializedSize();
    }

    void DtdmaQueueHeader::Print(std::ostream &os) const {
        os << "QueueSize=" << m_queueSize 
           << ", IsRelay=" << (m_isRelay ? "True" : "False") 
           << ", FinalDest=" << m_finalDest;
    }

    // Getters and Setters

    void DtdmaQueueHeader::SetQueueSize(uint16_t size) {
        m_queueSize = size;
    }

    uint16_t DtdmaQueueHeader::GetQueueSize(void) const {
        return m_queueSize;
    }

    void DtdmaQueueHeader::SetIsRelay(bool isRelay) {
        // Convert the boolean to a uint8_t (0 or 1) for serialization
        m_isRelay = isRelay ? 1 : 0;
    }

    bool DtdmaQueueHeader::GetIsRelay(void) const {
        return m_isRelay == 1; // Convert back to boolean
    }

    void DtdmaQueueHeader::SetFinalDest(Mac48Address dest) {
        m_finalDest = dest;
    }

    Mac48Address DtdmaQueueHeader::GetFinalDest(void) const {
        return m_finalDest;
    }
}