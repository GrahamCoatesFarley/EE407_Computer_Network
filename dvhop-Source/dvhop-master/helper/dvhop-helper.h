/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DVHOP_HELPER_H
#define DVHOP_HELPER_H

#include "ns3/ipv4-routing-helper.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/node-container.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-list-routing.h"

namespace ns3 {

  //Forward declarations
  class Node;
  class Ipv4RoutingProtocol;


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

    /*The factory to create DVHope Routing object*/
    ObjectFactory m_agentFactory;
  };

}

#endif /* DVHOP_HELPER_H */

