/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*

	Outline of Methods defined within:

	Forward Declarationa:
		class Node			-- a network Node object class, 
					provides NetDevice objects for network interfacing between other Nodes connected by Channel Instances
					provides Application objects to represent userspace traffic that interact with Nodes through Socket API
					any Node created is added to the NodeList automatically
		class Ipv4RoutingHelper		-- used to create Ipv4 routing protocol objects

	PDMs
		void Print( node, stream )	-- Print function which retrieves an Ipv4 class object as a node, the routing protocol from the DVHop class
					printd ID's of a the node to the output stream
		ObjectFactory m_agentFactory	-- a subclass of the ns3::Object, can hold set attributes to set automatically during object construction 

	PUBLIC METHODS:
		DVHopHelper		-- Default Constructor to instantiate the class object
					utilizes the Ipv4RoutingHelper to set the PDM m_agentFactory type id
		Copy			-- copy constructor to instantiate deep copy of DVHopHelper, referenced by pointer
		Create			-- creates and returns pointer to a new routing protocol from the current agentFactory pdm
		Set			-- sets the name and attribute value of the agentFactory pdm
		AssignStreams		-- installs Ipv4 and routing to nodes add new streams to current DVHop stream
		PrintDistanceTableAllAt -- prints the distance table and times


*/
#ifndef DVHOP_HELPER_H
#define DVHOP_HELPER_H

#include "ns3/ipv4-routing-helper.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/node-container.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-list-routing.h"

namespace ns3 {

  //Forward declarations -- indicate needed classes for the class to work properly
  class Node;	// Node object
  class Ipv4RoutingProtocol;	// IP Protocol


  class DVHopHelper : public Ipv4RoutingHelper
  {
  public:
    DVHopHelper();

    /**
     *Returns a pointer to clone of this DVHopHelper
     */
    DVHopHelper* Copy() const;

    /**
     *Returns a newly created routing protocol, it's called by ns3::InternetStackHelper::Install
     */
    virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

    /**
     *Controls the attributes of ns3::dvhop::RoutingProtocol
     */
    void Set(std::string name, const AttributeValue &value);

    /**
     *Assign a fixed random variable stream number to the random variables used by this model
     */
    int64_t  AssignStreams(NodeContainer c, int64_t stream);

    /**
     *Print the distance table to each reference at a given time
     */
    void PrintDistanceTableAllAt (Time printTime, Ptr<OutputStreamWrapper> stream) const;

  private:
    void Print (Ptr<Node> node, Ptr<OutputStreamWrapper> stream) const;

    /*The factory to create DVHop Routing object*/
    ObjectFactory m_agentFactory;
  };

}

#endif /* DVHOP_HELPER_H */

