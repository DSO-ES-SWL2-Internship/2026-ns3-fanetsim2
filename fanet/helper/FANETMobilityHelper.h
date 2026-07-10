#ifndef FANET_MOBILITY_HELPER_H
#define FANET_MOBILITY_HELPER_H

#include "FANETTopologyHelper.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/mobility-model.h"


#include <cstdint>

namespace ns3
{
    /**
     * @class FANETMobilityHelper
     * @brief Provides mobility configuration for FANET nodes.
     */
    class FANETMobilityHelper : public Object {
        public:
            /**
             * @brief Constructs a FANETMobilityHelper object.
             */
            FANETMobilityHelper();   

            /**
             * @brief Destroys the FANETMobilityHelper object.
             */                                                                                                   
            ~FANETMobilityHelper();       

            static  TypeId GetTypeId();

            /**
             * @brief Sets the Ground Data Terminal (GDT) node at a fixed position.
             * @param gdtNode The NodeContainer containing the GDT node.
             */
            void SetGDTMobility(NodeContainer& gdtNode);          

            /**
             * @brief Applies mobility settings to the FANET nodes in a wireless topology.
             * @param fanet A pointer to the FANETTopologyHelper object containing node groups.
             */                                                                                                                                                
            void ApplyMobilityWireless(Ptr<FANETTopologyHelper> fanet);

            /**
             * @brief Finds and returns the closest node to a given target node.
             * @param target The target node.
             * @param nodes A NodeContainer with candidate nodes.
             * @return A pointer to the closest node.
             */
            static Ptr<Node> GetClosestNode(Ptr<Node> target, NodeContainer nodes);

            /**
             * @brief Sets random mobility for cluster members within a specified sector.
             * @param clusterMembers The NodeContainer with cluster member nodes.
             * @param xCenter The x-coordinate of the mobility region center.
             * @param yCenter The y-coordinate of the mobility region center.
             */
            void SetClusterMemberMobility(NodeContainer& clusterMembers, double xCenter, double yCenter);      

            // void SetClusterHeadMobility(NodeContainer& clusterHeads, double x, double y, uint32_t nClusterHeads, double radius);        
            // void ApplyMobilityP2P(FANETTopologyHelper* fanet);   

        private:
            MobilityHelper mobility;
    };
}

#endif