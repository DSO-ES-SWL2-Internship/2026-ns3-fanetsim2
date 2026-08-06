#ifndef DTDMA_QUEUE_HEADER_H
#define DTDMA_QUEUE_HEADER_H

#include "ns3/header.h"
#include "ns3/mac48-address.h"

namespace ns3 {

class DtdmaQueueHeader : public Header 
{
public:
    DtdmaQueueHeader();
    virtual ~DtdmaQueueHeader();

    // ns-3 Object requirements
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const override;
    
    // Core Serialization Methods
    virtual uint32_t GetSerializedSize(void) const override;
    virtual void Serialize(Buffer::Iterator start) const override;
    virtual uint32_t Deserialize(Buffer::Iterator start) override;
    virtual void Print(std::ostream &os) const override;

    // Getters and Setters
    void SetQueueSize(uint16_t size);
    uint16_t GetQueueSize(void) const;

    void SetIsRelay(bool isRelay);
    bool GetIsRelay() const;

    void SetFinalDest(Mac48Address dest);
    Mac48Address GetFinalDest() const;

private:
    uint16_t m_queueSize; // The actual 2 bytes of data
    uint8_t m_isRelay; // 0 for false, 1 for true
    Mac48Address m_finalDest; // 6 bytes
};

} 

#endif 