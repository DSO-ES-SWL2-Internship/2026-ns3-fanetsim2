#ifndef FANET_ROUTING_HELPER_H
#define FANET_ROUTING_HELPER_H

#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/node-container.h"

namespace ns3
{
    /**
     * @enum RoutingProtocol
     * @brief Enumeration of supported routing protocols.
     */
    enum RoutingProtocol {
        AODV,
        OLSR,
        DSDV,
        DSR,
    };
    
    /**
     * @class FANETRoutingHelper
     * @brief Provides helper functions to configure routing protocols in FANET networks.
     */
    class FANETRoutingHelper : public Object {
        private:
            /// @brief Internet stack helper for protocol installation.
            InternetStackHelper internet;
            /// @brief List of routing protocols for the network.
            Ipv4ListRoutingHelper list;

            RoutingProtocol m_protocol;

            /**
             * @brief Installs the internet stack with the selected routing helper on all nodes.
             * @param nodes The container of nodes to install the stack on.
             */           
            void InstallInternetStackToAllNodes(NodeContainer nodes);

        public:
            FANETRoutingHelper();
            ~FANETRoutingHelper();

            static TypeId GetTypeId();

            // Methods to set the routing protocol used
            /**
             * @brief Configures and installs the AODV routing protocol on the given nodes.
             * @param nodes The container of nodes to configure.
             */
            void SetAODV(NodeContainer nodes);

            /**
             * @brief Configures and installs the OLSR routing protocol on the given nodes.
             * @param nodes The container of nodes to configure.
             */
            void SetOLSR(NodeContainer nodes);

            /**
             * @brief Configures and installs the DSDV routing protocol on the given nodes.
             * @param nodes The container of nodes to configure.
             */
            void SetDSDV(NodeContainer nodes);
            //void SetDSR(NodeContainer nodes);

            RoutingProtocol GetRoutingProtocol();

            void InstallRoutingProtocol(NodeContainer nodes);
    };
}

#endif