#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/json.hpp"
// #include "ns3/fanet_utils.h"
#include "FANETSimulator.h"
#include "fstream"

// This topology is mimicing 2 MANET clusters with its cluster head having a point to point connection with the GDT
// Issue with this topology is that if there are too many nodes, the echo packet is lost, which i highly suspect might be caused by either, AODV protocol which caused too much overhead causing the packet to be lost.
// or the nodes simply are too far away from each other resulting in poor connection
// In progress

using namespace ns3;
using json = nlohmann::json;

//NS_LOG_COMPONENT_DEFINE ("MANETNetwork");


int main (int argc, char* argv[]){

    bool verbose = true;

    if (verbose){
        //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
        //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
        //LogComponentEnable ("FANETTopologyHelper", LOG_LEVEL_INFO);
        //LogComponentEnable ("FANETMobilityHelper", LOG_LEVEL_INFO);
        
        LogComponentEnable ("FANETDeviceHelper", LOG_LEVEL_INFO);
        LogComponentDisable("FANETDeviceHelper", LOG_LEVEL_DEBUG);
        
        //LogComponentEnable ("FANETDeviceHelper", LOG_LEVEL_DEBUG);
        //LogComponentEnable("FANETRoutingHelper", LOG_LEVEL_INFO);

    }
    
    Ptr<FANETSimulator> simulator = CreateObject<FANETSimulator>();

    simulator->SetupSimulation("/home/dsointern/projects/intern0-ns3sim/2025-ns3-fanetsim/ns3/src/fanet/examples/config.json");
    simulator->RunSimulation(); 
    


    return 0;
}
