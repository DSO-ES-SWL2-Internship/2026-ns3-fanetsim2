#include "FANETRoutingHelper.h"
#include "ns3/ff-mac-common.h"
#include "ns3/aodv-helper.h"
#include "ns3/dsdv-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/dsr-helper.h"
#include "ns3/dsr-main-helper.h"
#include "ns3/log.h"
#include "ns3/string.h"

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("FANETRoutingHelper");

    TypeId FANETRoutingHelper::GetTypeId()
    {
        static TypeId tid =
            TypeId("ns3::FANETRoutingHelper")
                .SetParent<ns3::Object>()
                .AddConstructor<FANETRoutingHelper>()
                .AddAttribute(  "routingProtocol",
                "Set routing protocol used by the fanet",
                EnumValue(AODV),
                MakeEnumAccessor<RoutingProtocol>(&FANETRoutingHelper::m_protocol),
                MakeEnumChecker(
                    AODV, "AODV",
                    OLSR, "OLSR",
                    DSDV, "DSDV",
                    DSR,  "DSR"
                ));

        return tid;
    }

    FANETRoutingHelper::FANETRoutingHelper(){

    }

    FANETRoutingHelper::~FANETRoutingHelper(){

    }

    void FANETRoutingHelper::InstallInternetStackToAllNodes(NodeContainer nodes){
        this->internet.SetRoutingHelper(this->list);
        this->internet.Install(nodes);
    }

    void FANETRoutingHelper::SetAODV(NodeContainer nodes){
        AodvHelper aodv;

        // Adjust aodv parameters here

        this->list.Add (aodv, 100);

        InstallInternetStackToAllNodes(nodes);
    }

    void FANETRoutingHelper::SetOLSR(NodeContainer nodes){
        OlsrHelper oslr;

        // Adjust OSLR parameters here
        
        this->list.Add(oslr, 100);
        InstallInternetStackToAllNodes(nodes);
    }

    void FANETRoutingHelper::SetDSDV(NodeContainer nodes){
        DsdvHelper dsdv;

        // Adjust dsdv parameters here
        
        this->list.Add(dsdv, 100);
        InstallInternetStackToAllNodes(nodes);
    }

    // void FANETRoutingHelper::SetDSR(NodeContainer nodes){
    //     DsrHelper dsr;

    //     // Adjust dsr parameters here
        
    //     this->list.Add(dsr, 100);
    //     InstallInternetStackToAllNodes(nodes);
    // }

    RoutingProtocol FANETRoutingHelper::GetRoutingProtocol() { return m_protocol; }

    void FANETRoutingHelper::InstallRoutingProtocol(NodeContainer nodes)
    {
        switch (GetRoutingProtocol())
        {
            case AODV: {
                // TODO: add functionality to include all the different settings for AODV
                SetAODV(nodes);
                break;
            }

            case OLSR: {
                SetOLSR(nodes);
                break;
            }

            case DSDV: {
                SetDSDV(nodes);
                break;
            }

            default: {
                NS_LOG_UNCOND("Invalid Routing Protocol");
                exit(FAILURE);
            }
        }
    }
}