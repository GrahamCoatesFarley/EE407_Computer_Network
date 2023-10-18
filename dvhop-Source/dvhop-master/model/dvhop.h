/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DVHOP_H
#define DVHOP_H

#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"

#include "distance-table.h"

#include <map>


namespace ns3 {
  namespace dvhop{

    class RoutingProtocol : public Ipv4RoutingProtocol{
    public:
      static const uint32_t DVHOP_PORT;
      static TypeId GetTypeId (void);


      RoutingProtocol();
      virtual ~RoutingProtocol();
      virtual void DoDispose();

      //From Ipv4RoutingProtocol
      Ptr<Ipv4Route>  RouteOutput(Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
      bool            RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                                  UnicastForwardCallback   ufcb,
                                  MulticastForwardCallback mfcb,
                                  LocalDeliverCallback     ldcb,
                                  ErrorCallback            errcb);
      virtual void SetIpv4(Ptr<Ipv4> ipv4);
      virtual void NotifyInterfaceUp (uint32_t interface);
      virtual void NotifyInterfaceDown (uint32_t interface);
      virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
      virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
      virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const;

      int64_t AssignStreams(int64_t stream);

      //Getters and Setters for protocol parameters
      void SetIsBeacon(bool isBeacon)    { m_isBeacon = isBeacon; }
      void SetPosition(double x, double y) { m_xPosition = x; m_yPosition = y; }

      double GetXPosition()               { return m_xPosition;}
      double GetYPosition()               { return m_yPosition;}
      bool  IsBeacon()                   { return m_isBeacon;}

      void  PrintDistances(Ptr<OutputStreamWrapper> stream, Ptr<Node> node) const;

    private:
      //Start protocol operation
      void        Start    ();
      void        SendTo   (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination);
      void        RecvDvhop(Ptr<Socket> socket);
      Ptr<Socket> FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;
      //In case there exists a route to the destination, the packet is forwarded
      bool        Forwarding(Ptr<const Packet> p, const Ipv4Header &header, UnicastForwardCallback ufcb, ErrorCallback errcb);
      void        SendUnicastTo(Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination);

      //HELLO intervals and timers
      Time   HelloInterval;
      Timer  m_htimer;
      void   SendHello();
      void   HelloTimerExpire();

      //Table to store the hopCount to each beacon
      DistanceTable  m_disTable;
      void UpdateHopsTo (Ipv4Address beacon, uint16_t hops, double x, double y);


      //Boolean to identify if this node acts as a Beacon
      bool m_isBeacon;

      //This node's position info
      double m_xPosition;
      double m_yPosition;

      //IPv4 Protocol
      Ptr<Ipv4>   m_ipv4;
      // Raw socket per each IP interface, map socket -> iface address (IP + mask)
      std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;

      uint32_t    m_seqNo;



      //Used to simulate jitter
      Ptr<UniformRandomVariable> m_URandom;


    };
  }

}

#endif /* DVHOP_H */

