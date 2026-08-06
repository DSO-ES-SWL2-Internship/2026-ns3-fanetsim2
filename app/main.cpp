#include "wfq.h"

int main()
{
    WFQ wfq;
    Packet p;
    wfq.enqueue(&p);
    return 0;
}



