#include "FANETMobilityHelper.h"
#include "FANETMobilityHelper.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/rectangle.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("FANETMobilityHelper");

    TypeId FANETMobilityHelper::GetTypeId()
    { 
        static TypeId tid =
            TypeId("ns3::FANETMobilityHelper")
                .SetParent<ns3::Object>()
                .AddConstructor<FANETMobilityHelper>();
        return tid;
    }

    // Constructor
    FANETMobilityHelper::FANETMobilityHelper() {}

    // Destructor
    FANETMobilityHelper::~FANETMobilityHelper() {}

    // Set the GDT node at a fixed position
    void FANETMobilityHelper::SetGDTMobility(NodeContainer& gdtNode) {

        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
        positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // Static GDT Node
        mobility.SetPositionAllocator(positionAlloc);

        // mobility.SetPositionAllocator("ns3::ListPositionAllocator",
        //                             "Positions", VectorValue({Vector(0.0, 0.0, 0.0)}));  // GDT at (0,0)
        
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(gdtNode);

        NS_LOG_INFO("GDT mobility set");
    }


    // Set random mobility for cluster members within a sector around their CH
    void FANETMobilityHelper::SetClusterMemberMobility(NodeContainer& clusterMembers, double xCenter, double yCenter) {
            // mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", 
            // "Bounds", RectangleValue(Rectangle(-100, 100, -100, 100)), // Increase bounds
            // "Speed", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=200.0]"));
            // mobility.Install(fanet.clusters[1]);
        mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator", 
            "X", StringValue("ns3::UniformRandomVariable[Min=" + std::to_string(xCenter - 200.0) + "|Max=" + std::to_string(xCenter + 200.0) + "]"),
            "Y", StringValue("ns3::UniformRandomVariable[Min=" + std::to_string(yCenter - 200.0) + "|Max=" + std::to_string(yCenter + 200.0) + "]"));

        // Set the Mobility Model with larger bounds for Random Walk
        mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", 
            "Bounds", RectangleValue(Rectangle(xCenter - 500.0, xCenter + 500.0, yCenter - 500.0, yCenter + 500.0)),
            "Speed", StringValue("ns3::UniformRandomVariable[Min=10.0|Max=60.0]")
        );


        mobility.Install(clusterMembers);
    }

    // // Apply mobility to all nodes in the FANET topology, cluster heads are set to be not mobile in this model as P2P is used
    // void FANETMobilityHelper::ApplyMobilityP2P(FANETTopologyHelper* fanet) {
    //     NodeContainer singleCH;
    //     SetGDTMobility(fanet->GDTNode);

    //     double radius = 100.0;  // Distance of cluster heads from GDT
    //     double angleStep = 360.0 / fanet->clusters.size();  // Evenly space CHs in a circular pattern

    //     for (size_t i = 0; i < fanet->clusters.size(); i++) {
    //         double angleRad = (angleStep * i) * (M_PI / 180.0);
    //         double xCH = radius * cos(angleRad);
    //         double yCH = radius * sin(angleRad);
            
    //         // Set mobility for members of this cluster around their CH
    //         SetClusterMemberMobility(fanet->clustersCMNodes[i], xCH, yCH);
    //         //SetClusterMemberMobility(fanet->clusters[i], xCH, yCH);
    //     }

    //     SetClusterHeadMobility(fanet->clusterHeadNodes, 0.0, 0.0, fanet->clusters.size(), radius);
    // }

    void FANETMobilityHelper::ApplyMobilityWireless(Ptr<FANETTopologyHelper> fanet) {
        NodeContainer singleCH;
        SetGDTMobility(fanet->GDTNode);

        double radius = 100.0;  // Distance of cluster heads from GDT
        double angleStep = 360.0 / fanet->clusters.size();  // Evenly space CHs in a circular pattern

        for (size_t i = 0; i < fanet->clusters.size(); i++) {
            double angleRad = (angleStep * i) * (M_PI / 180.0);
            double xCH = radius * cos(angleRad);
            double yCH = radius * sin(angleRad);
            
            // Set mobility for members of this cluster to be around the area of their assignment
            SetClusterMemberMobility(fanet->clusters[i], xCH, yCH);
        }

        NS_LOG_INFO("Cluster Nodes mobility set");
    }


    Ptr<Node> FANETMobilityHelper::GetClosestNode(Ptr<Node> target, NodeContainer nodes)
    {
        Ptr<MobilityModel> targetMobility = target->GetObject<MobilityModel>();

        double minDistance = std::numeric_limits<double>::max();
        Ptr<Node> closestNode = nullptr;

        // Iterate through all nodes in the cluster to find the closest one
        for (uint32_t j = 0; j < nodes.GetN(); j++)
        {
            Ptr<Node> node = nodes.Get(j);
            Ptr<MobilityModel> nodeMobility = node->GetObject<MobilityModel>();

            if (nodeMobility && targetMobility)
            {
                double distance = targetMobility->GetDistanceFrom(nodeMobility);

                if (distance < minDistance)
                {
                    minDistance = distance;
                    closestNode = node;
                }
            }
        }

        return closestNode;
    }

}