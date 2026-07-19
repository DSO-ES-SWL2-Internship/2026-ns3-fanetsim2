#ifndef FANET_ANIMATION_HELPER_H
#define FANET_ANIMATION_HELPER_H

#include "ns3/animation-interface.h"
#include "FANETTopologyHelper.h"

#include <cstdint>
#include <string>

namespace ns3 
{
    /**
     * @class FANETAnimationHelper
     * @brief Helper class for animating FANET network topology using ns-3 NetAnim.
     */
    class FANETAnimationHelper : public AnimationInterface {
        private:
            /// @brief Stores RGB color values for each cluster
            std::vector<std::array<uint8_t, 3>> clustersColor;
            /// @brief Stores current cluster head nodes
            std::vector<Ptr<Node>> curCHNodes;
            
        public:

            /**
             * @brief Constructor for FANETAnimationHelper.
             * @param name The filename to save the animation trace.
             */
            FANETAnimationHelper(std::string name);

            /**
             * @brief Destructor for FANETAnimationHelper.
             */
            ~FANETAnimationHelper(); 

            //static TypeId GetTypeId();
            /**
             * @brief Assigns animation colors and names to the cluster nodes
             * @param clusterIndex The index of the cluster.
             * @param cluster The container of nodes belonging to the cluster.
             */
            void AssignClusterAnim(uint32_t clusterIndex, NodeContainer cluster);

            /**
             * @brief Assign the color of the GDT node
             * @param gdtNode The node container holding the GDT node
             */
            void AssignGDTAnim(NodeContainer gdtNode);

            /**
             * @brief Updates the color of the nodes that are assigned to be CH
             * @param CHNodes A vector of pointers to the current cluster head nodes.
             * 
             * The function stores its the current CH nodes into its own vector
             */
            void AssignCHAnim(std::vector<Ptr<Node>> CHNodes);

            /**
             * @brief Updates the animation for changing cluster head nodes.
             * @param CHNodes A vector of pointers to the new cluster head nodes.
             */
            void UpdateCHAnim(std::vector<Ptr<Node>> CHNodes);

            /**
             * @brief Animates the entire FANET topology.
             * @param fanet Pointer to the FANETTopologyHelper object containing network topology details.
             */
            void AnimateFANET(Ptr<FANETTopologyHelper> fanet);
    };
}

#endif