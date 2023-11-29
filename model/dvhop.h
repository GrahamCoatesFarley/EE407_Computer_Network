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
      static TypeId GetTypeId (void);  // Develops a routing protocol ID


      RoutingProtocol();
      virtual ~RoutingProtocol();    // Destructor, reallocates dynamically allocated memory from runtime
      virtual void DoDispose();      // Closes every socket in the node (one per interface)

      //From Ipv4RoutingProtocol
      Ptr<Ipv4Route>  RouteOutput(Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
      bool            RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                                  UnicastForwardCallback   ufcb,
                                  MulticastForwardCallback mfcb,
                                  LocalDeliverCallback     ldcb,
                                  ErrorCallback            errcb);
      // Sets the nodes IP, function and schedule
      virtual void SetIpv4(Ptr<Ipv4> ipv4);                                                           
      // Interface Formating and  Installation for Node Sockets
      virtual void NotifyInterfaceUp (uint32_t interface);                                             // Used to format and install an interface on a socket
      virtual void NotifyInterfaceDown (uint32_t interface);                                           // Used to remove the interface
      // IP Address assignment for broadcast recieval
      virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);                // Assigns an IP address to allow for broadcast recieval
      virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);             // Removes bound IP addresses
      virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const;         // Prints node ID to the stream with formatting
      // Assigns a random value to the stream
      int64_t AssignStreams(int64_t stream);

      // Stes if the Simulation is Critical
      void SetIsCritical(bool isCritical) { m_isCrit = isCritical; } 

      //Sets if node is a Beacon
      void SetIsBeacon(bool isBeacon)    { m_isBeacon = isBeacon; }
      //Sets beacon hop size
      void SetHopSize(double hopSize)    { m_hopSize = hopSize; }
      // Sets coordinate location of a node
      void SetPosition(double x, double y) { m_xPosition = x; m_yPosition = y; }         
      // Gets node coordinates
      double GetXPosition()               { return m_xPosition;}                        
      double GetYPosition()               { return m_yPosition;}                       
      // Predicates on if a node is a beacon
      bool  IsBeacon()                   { return m_isBeacon;}                            // Determines in the node is flagged as a beacon (knows its location)
      // Returns the hop size of the beacon
      double GetHopSize()               { return m_hopSize;}

      // Prints the node ID,Beacon andress and Info from the Distance Table
      void  PrintDistances(Ptr<OutputStreamWrapper> stream, Ptr<Node> node) const;        
    private:
      //Start protocol operation (timer initialization)
      void        Start    ();
      // Sends a packet to a Socket at IP address
      void        SendTo   (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination);
      // Interacts with Recieved packets, 
      // updating hop count and beacon address from packet header
      void        RecvDvhop(Ptr<Socket> socket);
      // Middle Functionn to allow for Critical Simulation
      void        Recieve(Ptr<Socket> socket);
      // Finds socket based on Interface IP
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
      // Hop size of a beacon node
      double m_hopSize;

      // Boolean to indicate if the node is still alive
      bool m_isAlive;
      bool m_isCrit;

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

