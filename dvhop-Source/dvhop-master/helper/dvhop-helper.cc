/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "dvhop-helper.h"
#include "ns3/dvhop.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/dvhop.h"

namespace ns3 {

  DVHopHelper::DVHopHelper():Ipv4RoutingHelper()
  {
    m_agentFactory.SetTypeId ("ns3::dvhop::RoutingProtocol");
  }

  DVHopHelper*
  DVHopHelper::Copy () const
  {
    return new DVHopHelper(*this);
  }

  Ptr<Ipv4RoutingProtocol>
  DVHopHelper::Create (Ptr<Node> node) const
  {
    Ptr<dvhop::RoutingProtocol> agent = m_agentFactory.Create<dvhop::RoutingProtocol> ();
    node->AggregateObject (agent);
    return agent;
  }

  void
  DVHopHelper::Set (std::string name, const AttributeValue &value)
  {
    m_agentFactory.Set (name, value);
  }

  int64_t
  DVHopHelper::AssignStreams (NodeContainer c, int64_t stream)
  {
    int64_t currentStream = stream;
      Ptr<Node> node;
      for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
        {
          node = (*i);
          Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
          NS_ASSERT_MSG (ipv4, "Ipv4 not installed on node");
          Ptr<Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol ();
          NS_ASSERT_MSG (proto, "Ipv4 routing not installed on node");
          Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
          if (dvhop)
            {
              currentStream += dvhop->AssignStreams (currentStream);
              continue;
            }
        }
      return (currentStream - stream);
  }



  void
  DVHopHelper::PrintDistanceTableAllAt(Time printTime, Ptr<OutputStreamWrapper> stream) const
  {
    for(uint32_t i=0; i < NodeList::GetNNodes (); i++)
      {
        Ptr<Node> node = NodeList::GetNode (i);
        Simulator::Schedule(printTime, &DVHopHelper::Print, this, node, stream);
      }

  }

  void
  DVHopHelper::Print (Ptr<Node> node, Ptr<OutputStreamWrapper> stream) const
  {
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ptr<dvhop::RoutingProtocol> rp = DynamicCast<dvhop::RoutingProtocol>(ipv4->GetRoutingProtocol ());
    //Ptr<Ipv4RoutingProtocol> rp = ipv4->GetRoutingProtocol ();
    NS_ASSERT (rp);
    rp->PrintDistances(stream, node);
  }

}

