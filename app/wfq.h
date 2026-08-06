#pragma once

#include <cstdint>

struct Packet
{
    uint8_t hdr;
};

class WFQ
{
public:
    void enqueue(Packet*);
    void algo();
};