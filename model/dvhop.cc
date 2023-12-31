/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "dvhop.h"
#include "dvhop-packet.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/node-list.h"



NS_LOG_COMPONENT_DEFINE ("DVHopRoutingProtocol");


namespace ns3 {

  namespace dvhop{

    NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);


    TypeId
    RoutingProtocol::GetTypeId (){
      static TypeId tid = TypeId ("ns3::dvhop::RoutingProtocol")             //Unique string identifying the class
          .SetParent<Ipv4RoutingProtocol> ()                                   //Base class
          .AddConstructor<RoutingProtocol> ()                                  //Set of accessible constructors
          .AddAttribute ("HelloInterval",                                      //Bind a value to a string
                         "HELLO messages emission interval.",                  // with this description
                         TimeValue (Seconds (1)),                              // default value
                         MakeTimeAccessor (&RoutingProtocol::HelloInterval),   // accessed through
                         MakeTimeChecker ())
          .AddAttribute ("UniformRv",
                         "Access to the underlying UniformRandomVariable",
                         StringValue ("ns3::UniformRandomVariable"),
                         MakePointerAccessor (&RoutingProtocol::m_URandom),
                         MakePointerChecker<UniformRandomVariable> ());                                  // the checker is used to set bounds in values
      return tid;
    }


    /// UDP Port for DV-Hop
    const uint32_t RoutingProtocol::DVHOP_PORT = 1234;


    RoutingProtocol::RoutingProtocol () :
      HelloInterval (Seconds (1)),          //Send HELLO each second
      m_htimer (Timer::CANCEL_ON_DESTROY),  //Set timer for HELLO
      m_isBeacon(false),                    // If the node is a beacon
      m_isAlive(true),                      // If the node is alive
      m_isCrit(false),                      // If critical conditions have been intitialized for the simulation
      m_hopSize(-1.0),                      // Hop Size
      m_xPosition(-1.0),                   // X Coordinate
      m_yPosition(-1.0),                   // Y Coordinate
      m_seqNo (0),                          // Current packet sequence number
      m_totalTime(10)                       // 10 second simulation time by default
    {
          srandom(m_totalTime);   // For use in random number generation
    }



    RoutingProtocol::~RoutingProtocol ()
    {
    }

    void
    RoutingProtocol::DoDispose ()
    {
      m_ipv4 = 0;
      //Close every raw socket in the node (one per interface)
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::iterator iter =
           m_socketAddresses.begin (); iter != m_socketAddresses.end (); iter++)
        {
          iter->first->Close ();
        }
      m_socketAddresses.clear ();
      Ipv4RoutingProtocol::DoDispose ();
    }

    /**
 *Outgoing packets arrive at this function to acquire a route
 */
    Ptr<Ipv4Route>
    RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
    {
      NS_LOG_DEBUG("Outgoing packet through interface: " << (oif ? oif->GetIfIndex () : 0));
      if(!p)
        {
          sockerr = Socket::ERROR_INVAL;
          NS_LOG_WARN ("Packet is NULL");
          Ptr<Ipv4Route> route;
          return route;
        }

      if (m_socketAddresses.empty ())
        {
          sockerr = Socket::ERROR_NOROUTETOHOST;
          NS_LOG_LOGIC ("No DVHop interfaces");
          Ptr<Ipv4Route> route;
          return route;
        }


      int32_t ifIndex = m_ipv4->GetInterfaceForDevice(oif); //Get the interface for this device
      if(ifIndex < 0 )
        {
          sockerr = Socket::ERROR_NOROUTETOHOST;
          NS_LOG_LOGIC ("Could not get the address for this NetDevice");
          Ptr<Ipv4Route> route;
          return route;
        }

      Ipv4InterfaceAddress iface = m_ipv4->GetAddress(ifIndex, 0);
      sockerr = Socket::ERROR_NOTERROR;
      Ipv4Address dst = header.GetDestination ();

      NS_LOG_DEBUG("Sending packet to: " << dst<< ", From:"<< iface.GetLocal ());

      //Construct a route object to return
      //TODO: Remove hardcoded routes
      Ptr<Ipv4Route> route = Create<Ipv4Route>();

      route->SetDestination (dst);
      route->SetGateway (iface.GetBroadcast ());//nextHop
      route->SetSource (iface.GetLocal ());
      route->SetOutputDevice (oif);
      return route;

    }


    /**
 *Incoming packets on this node arrive here, they're processed to upper layers or the protocol itself
 */
    bool
    RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev, UnicastForwardCallback ufcb, MulticastForwardCallback mfcb, LocalDeliverCallback ldcb, ErrorCallback errcb)
    {
      NS_LOG_FUNCTION ("Packet received: " << p->GetUid () << header.GetDestination () << idev->GetAddress ());

      if(m_socketAddresses.empty ())
        {//No interface is listening
          NS_LOG_LOGIC ("No DVHop interfaces");
          return false;
        }

      NS_ASSERT (m_ipv4 != 0);                               //The IPv4 Stack is running in this node
      NS_ASSERT (p != 0);                                    //The packet is not null
      int32_t iif = m_ipv4->GetInterfaceForDevice (idev);    //Get the interface index
      NS_ASSERT (iif >= 0);                                  //The input device also supports IPv4


      Ipv4Address dst = header.GetDestination ();
      Ipv4Address src = header.GetSource ();

      if(dst.IsMulticast ())
        {//Deal with the multicast packet
          NS_LOG_INFO ("Multicast destination...");

          //mfcb(p,header,iif);
        }

      //Broadcast local delivery or forwarding
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
        {
          Ipv4InterfaceAddress iface = j->second;
          if(m_ipv4->GetInterfaceForAddress (iface.GetLocal ()) == iif)
            {//We got the interface that received the packet
              if(dst == iface.GetBroadcast () || dst.IsBroadcast ())
                {//...and it's a broadcasted packet
                  Ptr<Packet> packet = p->Copy ();
                  if(  ! ldcb.IsNull () )
                    {//Forward the packet to further processing to the LocalDeliveryCallback defined
                      NS_LOG_DEBUG("Forwarding packet to Local Delivery Callback");
                      ldcb(packet,header,iif);
                    }
                  else
                    {
                      NS_LOG_ERROR("Unable to deliver packet: LocalDeliverCallback is null.");
                      errcb(packet,header,Socket::ERROR_NOROUTETOHOST);
                    }
                  if (header.GetTtl () > 1)
                    {
                      NS_LOG_LOGIC ("Forward broadcast...");
                      //Get a route and call UnicastForwardCallback
                    }
                  else
                    {
                      NS_LOG_LOGIC ("TTL Exceeded, drop packet");
                    }
                  return true;
                }
            }
        }


      //Unicast local delivery
      if( m_ipv4->IsDestinationAddress (dst, iif))
        {
          if (ldcb.IsNull () == false)
            {
              NS_LOG_LOGIC ("Unicast local delivery to " << dst);
              ldcb (p, header, iif);
            }
          else
            {
              NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << src);
              errcb (p, header, Socket::ERROR_NOROUTETOHOST);
            }
          return true;
        }

      //Forwarding
      return Forwarding (p,header,ufcb,errcb);
    }

    void
    RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
    {
      NS_ASSERT (ipv4 != 0);
      NS_ASSERT (m_ipv4 == 0);

      m_htimer.SetFunction (&RoutingProtocol::HelloTimerExpire, this);
      m_htimer.Schedule (RoutingProtocol::HelloInterval);

      m_ipv4 = ipv4;

      Simulator::ScheduleNow (&RoutingProtocol::Start, this);

    }

    void
    RoutingProtocol::NotifyInterfaceUp (uint32_t interface)
    {
      NS_LOG_FUNCTION (this << m_ipv4->GetAddress (interface, 0).GetLocal ());
      Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
      if (l3->GetNAddresses (interface) > 1)
        {
          NS_LOG_WARN ("DVHop does not work with more then one address per each interface.");
        }
      Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
      if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
        return;

      // Create a socket to listen only on this interface
      Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                 UdpSocketFactory::GetTypeId ());
      NS_ASSERT (socket != 0);
      socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvDvhop, this));
      socket->BindToNetDevice (l3->GetNetDevice (interface));
      socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), DVHOP_PORT));
      socket->SetAllowBroadcast (true);
      socket->SetAttribute ("IpTtl", UintegerValue (1));
      m_socketAddresses.insert (std::make_pair (socket, iface));

    }


    void
    RoutingProtocol::NotifyInterfaceDown (uint32_t interface)
    {
      NS_LOG_FUNCTION (this << m_ipv4->GetAddress (interface, 0).GetLocal ());

      // Close socket
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (m_ipv4->GetAddress (interface, 0));
      NS_ASSERT (socket);
      socket->Close ();
      m_socketAddresses.erase (socket);
      if (m_socketAddresses.empty ())
        {
          NS_LOG_LOGIC ("No DV-Hop interfaces");
          m_htimer.Cancel ();
          return;
        }
    }

    void
    RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
    {
      NS_LOG_FUNCTION (this << " interface " << interface << " address " << address);
      Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
      if (!l3->IsUp (interface))
        return;
      if (l3->GetNAddresses (interface) == 1)
        {
          Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
          Ptr<Socket> socket = FindSocketWithInterfaceAddress (iface);
          if (!socket)
            {
              if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
                return;
              // Create a socket to listen only on this interface
              Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                         UdpSocketFactory::GetTypeId ());
              NS_ASSERT (socket != 0);
              socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvDvhop,this));
              socket->BindToNetDevice (l3->GetNetDevice (interface));
              // Bind to any IP address so that broadcasts can be received
              socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), DVHOP_PORT));
              socket->SetAllowBroadcast (true);
              m_socketAddresses.insert (std::make_pair (socket, iface));
            }
        }
      else
        {
          NS_LOG_LOGIC ("DV-Hop does not work with more then one address per each interface. Ignore added address");
        }

    }

    void
    RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
    {
      NS_LOG_FUNCTION (this);
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (address);
      if (socket)
        {
          m_socketAddresses.erase (socket);
          Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
          if (l3->GetNAddresses (interface))
            {
              Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
              // Create a socket to listen only on this interface
              Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                         UdpSocketFactory::GetTypeId ());
              NS_ASSERT (socket != 0);
              socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvDvhop, this));
              // Bind to any IP address so that broadcasts can be received
              socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), DVHOP_PORT));
              socket->SetAllowBroadcast (true);
              m_socketAddresses.insert (std::make_pair (socket, iface));
            }
          if (m_socketAddresses.empty ())
            {
              NS_LOG_LOGIC ("No aodv interfaces");
              m_htimer.Cancel ();
              return;
            }
        }
      else
        {
          NS_LOG_LOGIC ("Remove address not participating in AODV operation");
        }
    }


    void
    RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const 
    {
      *stream->GetStream () << "(" << m_ipv4->GetObject<Node>()->GetId() << " - Not implemented yet";
    }


    void
    RoutingProtocol::PrintDistances (Ptr<OutputStreamWrapper> stream, Ptr<Node> node) const
    {
      *stream->GetStream ()<<"----------------- Node "<<node->GetId ()<<"-----------------"<<"\n";
      m_disTable.Print (stream);

      if(m_isBeacon) 
      {
        *stream->GetStream() << "Hop Size: " << m_hopSize << "\n";
      } 
      else 
      {
        Data info = ComputeData();

        Ptr <Ipv4RoutingProtocol> proto = node->GetObject<Ipv4>()->GetRoutingProtocol();
        Ptr <dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol>(proto);
        if(dvhop->GetXPosition() == -1 && dvhop->GetYPosition() == -1) { // Making sure we were able to calculate location
          *stream->GetStream() << "Unable to perform Trilateration due either beacons being <3, on the same line or too close" << std::endl;
        }else {
          Ptr <MobilityModel> mob = node->GetObject<MobilityModel>();
          *stream->GetStream() << "Actual Position: (" << mob->GetPosition().x << ", " << mob->GetPosition().y << ")" << std::endl;
          *stream->GetStream() << "Estimated Position: (" << dvhop->GetXPosition() << ", " << dvhop->GetYPosition() << ")" << std::endl;
        }

        *stream->GetStream() << "Average distance from beacons: " << info.avgDist << std::endl;
        *stream->GetStream() << "Average number of hops from beacons: " << info.avgHops<< std::endl;
        *stream->GetStream() << "Average latency from beacons: " << info.avgLat<< std::endl;
      }
    }

    int64_t
    RoutingProtocol::AssignStreams (int64_t stream)
    {
      NS_LOG_FUNCTION (this << stream);
      m_URandom->SetStream (stream);
      return 1;
    }


    void
    RoutingProtocol::Start ()
    {
      NS_LOG_FUNCTION (this);
      //Initialize timers and extra behaviour not initialized in the constructor
    }


    void
    RoutingProtocol::HelloTimerExpire ()
    {
      NS_LOG_DEBUG ("HelloTimer expired");

        // Determine if the protocl has has critical conditions enabled
       if(m_isCrit)
      {
        if(m_isAlive) // IF the current Node is flagged as alive
        {
          SendHello (); 
          
          // Determine if the node survives after sending the hello
          double currTime = (Simulator::Now()).GetSeconds();
          u_int32_t chance = (rand()%100) + 1;
          //std::cout << std::endl<< chance << std::endl<< std::endl;   //<-- Output was used to allow for testing of death chance
          //td::cout << std::endl<< currTime << "Time of Possible Death" << std::endl<< std::endl;

          // Nodes have a lower chance of dying at later times in the simulation
          if(!m_isBeacon && 
          (((currTime > (m_totalTime * 0.15) && currTime < (m_totalTime * 0.30)) && chance < 15) || 
          ((currTime > (m_totalTime * 0.30) && currTime < (m_totalTime * 0.45)) && chance < 5) || 
          (((currTime > (m_totalTime * 0.45) && currTime < (m_totalTime)) && chance < 1))))
          {
            m_isAlive = false;
            NS_LOG_LOGIC ("\n\nA Node has Died at time: " << currTime << std::endl); 
          }
          else
          {
            // Ensure the next time gets scheduled
            m_htimer.Cancel ();
            m_htimer.Schedule (RoutingProtocol::HelloInterval);
          }

        }  
        else
          NS_LOG_LOGIC ("\n\n@" << Simulator::Now() << " , Hello fails.\n\n");
      }
      else
      {
        // Send Hello without critical conditions
        SendHello ();
        m_htimer.Cancel ();
        m_htimer.Schedule (RoutingProtocol::HelloInterval);
      }
    }

    bool
    RoutingProtocol::Forwarding(Ptr<const Packet> p, const Ipv4Header &header, Ipv4RoutingProtocol::UnicastForwardCallback ufcb, Ipv4RoutingProtocol::ErrorCallback errcb)
    {
      //Not forwarding packets for the moment
     return false;
    }



    void
    RoutingProtocol::SendHello ()
    {
      //NS_LOG_FUNCTION (this);
      /* Broadcast a HELLO packet the message fields set as follows:
   *   Sequence Number    The node's latest sequence number.
   *   Hop Count                      0
   */

      for(std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin(); j != m_socketAddresses.end (); ++j)
        {
          Ptr<Socket> socket = j->first;
          Ipv4InterfaceAddress iface = j->second;

          std::vector<Ipv4Address> knownBeacons = m_disTable.GetKnownBeacons ();
          std::vector<Ipv4Address>::const_iterator addr;
          for (addr = knownBeacons.begin (); addr != knownBeacons.end (); ++addr)
            {
              //Create a HELLO Packet for each known Beacon to this node
              Position beaconPos = m_disTable.GetBeaconPosition (*addr);
              FloodingHeader helloHeader(beaconPos.first,              //X Position
                                         beaconPos.second,             //Y Position
                                         m_seqNo++,                    //Sequence Numbr
                                         m_disTable.GetHopsTo (*addr), //Hop Count
                                         m_disTable.GetHopSizeOf (*addr), //Hop Size
                                         *addr);                       //Beacon Address
              NS_LOG_DEBUG (iface.GetLocal ()<< " Sending Hello...");
              Ptr<Packet> packet = Create<Packet>();
              packet->AddHeader (helloHeader);
              // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
              Ipv4Address destination;
              if (iface.GetMask () == Ipv4Mask::GetOnes ())
                {
                  destination = Ipv4Address ("255.255.255.255");
                }
              else
                {
                  destination = iface.GetBroadcast ();
                }
              Time jitter = Time (MilliSeconds (m_URandom->GetInteger (0, 10)));
              Simulator::Schedule (jitter, &RoutingProtocol::SendTo, this , socket, packet, destination);
            }

          /*If this node is a beacon, it should broadcast its position always*/
          NS_LOG_DEBUG ("Node "<< iface.GetLocal () << " isBeacon? " << m_isBeacon);
          if (m_isBeacon){
              //Create a HELLO Packet for each known Beacon to this node
              FloodingHeader helloHeader(m_xPosition,                 //X Position
                                         m_yPosition,                 //Y Position
                                         m_seqNo++,                   //Sequence Number
                                         0,                           //Hop Count
                                         m_hopSize,                   //Hop Size
                                         iface.GetLocal ());          //Beacon Address
//              std::cout <<__FILE__<< __LINE__ << helloHeader << std::endl;

              NS_LOG_DEBUG (iface.GetLocal ()<< " Sending Hello...");
              Ptr<Packet> packet = Create<Packet>();
              packet->AddHeader (helloHeader);
              // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
              Ipv4Address destination;
              if (iface.GetMask () == Ipv4Mask::GetOnes ())
                {
                  destination = Ipv4Address ("255.255.255.255");
                }
              else
                {
                  destination = iface.GetBroadcast ();
                }
              Time jitter = Time (MilliSeconds (m_URandom->GetInteger (0, 10)));
              Simulator::Schedule (jitter, &RoutingProtocol::SendTo, this , socket, packet, destination);


            }
        }
    }


    void
    RoutingProtocol::SendTo (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
    {
      socket->SendTo (packet, 0, InetSocketAddress (destination, DVHOP_PORT));
        
    }

    /**
     *Callback to receive DVHop Packets
     */
    void
    RoutingProtocol::RecvDvhop (Ptr<Socket> socket)
    {
      // Determine if critical condition, if true, a dead node cannot recieve packets
      if(m_isCrit)
      {
        if(m_isAlive)
          Recieve(socket);
        else
          NS_LOG_LOGIC ("\n\nCritical error on node communication.\n\n"); 
      }
      else
        Recieve(socket);

    }

    void
    RoutingProtocol::Recieve(Ptr<Socket> socket)
    {
      Address sourceAddress;
      Ptr<Packet> packet = socket->RecvFrom (sourceAddress); //Read a single packet from 'socket' and retrieve the 'sourceAddress'

      InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
      Ipv4Address sender = inetSourceAddr.GetIpv4 ();
      Ipv4Address receiver = m_socketAddresses[socket].GetLocal ();

      NS_LOG_DEBUG ("sender:           " << sender);
      NS_LOG_DEBUG ("receiver:         " << receiver);


      FloodingHeader fHeader;
      packet->RemoveHeader (fHeader);
      NS_LOG_DEBUG ("Update the entry for: " << fHeader.GetBeaconAddress ());
      UpdateHopsTo (fHeader.GetBeaconAddress (), fHeader.GetHopCount () + 1, fHeader.GetHopSize (), fHeader.GetXPosition (), fHeader.GetYPosition ());
      NS_LOG_LOGIC ( "Header Dump Post Recieve (Beacon IP/Hop Count/ (X,Y) of Beacon): " << fHeader.GetBeaconAddress() 
        << " / " << fHeader.GetHopCount() << " / ( "  << fHeader.GetXPosition() << " , " << fHeader.GetYPosition() << " ) \n"); 

    }

    Ptr<Socket>
    RoutingProtocol::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress addr) const
    {
      NS_LOG_FUNCTION (this << addr);
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
           m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
        {
          Ptr<Socket> socket = j->first;
          Ipv4InterfaceAddress iface = j->second;
          if (iface == addr)
            return socket;
        }
      Ptr<Socket> socket;
      return socket;
    }

    void
    RoutingProtocol::UpdateHopsTo (Ipv4Address beacon, uint16_t newHops, double newHopSize, double x, double y)
    {
      uint16_t oldHops = m_disTable.GetHopsTo (beacon);
      double oldHopSize = m_disTable.GetHopSizeOf (beacon);
      if (m_ipv4->GetInterfaceForAddress (beacon) >= 0){
          NS_LOG_DEBUG ("Local Address, not updating in table");
          return;
        }

      if( oldHops > newHops || oldHops == 0) {//Update only when a shortest path is found
        m_disTable.AddBeacon(beacon, newHops, newHopSize > 0? newHopSize:oldHopSize, x, y);

        if(m_isBeacon) { // Recalculate hop sizes to other beacons
          RecalculateHopSize();
        }else if( newHopSize > 0 || oldHopSize > 0 ) {
          Trilateration();
        }
      } else if( newHopSize > 0 && !m_isBeacon) {//Also update hop size if its available only for regular nodes, but hop counts remains
        m_disTable.AddBeacon(beacon, oldHops, newHopSize, x, y);
        Trilateration();
      }
    }

    // Calculate the ho size of a beacon = Sum (all other anchors as i) SQRT((x-xi)^2 + (y-yi)^2)
    void
    RoutingProtocol::RecalculateHopSize ()
    {
      double up = 0;
      double down = 0;

      std::vector<Ipv4Address> knownBeacons = m_disTable.GetKnownBeacons ();
      std::vector<Ipv4Address>::const_iterator addr;
      for (addr = knownBeacons.begin (); addr != knownBeacons.end (); ++addr) {
        Position beaconPos = m_disTable.GetBeaconPosition(*addr);
        double hops = m_disTable.GetHopsTo(*addr);

        up += sqrt(pow(m_xPosition-beaconPos.first, 2) + pow(m_yPosition-beaconPos.second, 2));
        down += hops;
      }

      m_hopSize = up/down;
    }

    void
    RoutingProtocol::Trilateration() {
      Point points[3];
      double distances[3];

      uint16_t counter = 0;
//      *os->GetStream () << "Trilateration Distance entries\n";

      std::vector<Ipv4Address> knownBeacons = m_disTable.GetKnownBeacons ();
      std::vector<Ipv4Address>::const_iterator addr;
      for (addr = knownBeacons.begin (); addr != knownBeacons.end (); ++addr) 
      {
        Position beaconPos = m_disTable.GetBeaconPosition(*addr);
        if(m_disTable.GetHopSizeOf(*addr) < 0) {
          continue; // Ignore Beacon with no valid hop size
        }
        points[counter] = {beaconPos.first, beaconPos.second};
        distances[counter] = m_disTable.GetHopSizeOf(*addr) * m_disTable.GetHopsTo(*addr);

        // Beacon IP Address     Distance
//        *os->GetStream () << addr << "\t" << distances[counter] << std::endl;
        counter++;
        if(counter==3) break;
      }

      if(counter<3) 
      { 
        // We did not get upto 3 beacons to trilaterate
//        *os->GetStream () << "No enough points for trilateration!" << std::endl;
        return;
      }
      double ex = points[1].x - points[0].x;
      double ey = points[1].y - points[0].y;
      double ez = points[1].x * points[1].x - points[0].x * points[0].x +
                  points[1].y * points[1].y - points[0].y * points[0].y +
                  distances[0] * distances[0] - distances[1] * distances[1];

      double fx = points[2].x - points[0].x;
      double fy = points[2].y - points[0].y;
      double fz = points[2].x * points[2].x - points[0].x * points[0].x +
                  points[2].y * points[2].y - points[0].y * points[0].y +
                  distances[0] * distances[0] - distances[2] * distances[2];

      double denominator = 2 * (ex * fy - ey * fx);
      if (std::abs(denominator) < 1e-6) {
//        *os->GetStream () << "The points are collinear or too close for trilateration!" << std::endl;
        return;
      }

      m_xPosition = (ez * fy - ey * fz) / denominator;
      m_yPosition = (ex * fz - ez * fx) / denominator;

      // Bounding the position to the simulation area 100 x 100
      if(m_xPosition < 0) m_xPosition = 0;
      else if(m_xPosition > 100) m_xPosition = 100;
      if(m_yPosition < 0) m_yPosition = 0;
      else if(m_yPosition > 100) m_yPosition = 100;
    }

    Data
    RoutingProtocol::ComputeData() const{
      Point points[3];
      double distances[3];

      double totalDist = 0.0;
      double totalLat = 0.0;
      double totalHops = 0.0;

      Data output;

      uint16_t counter = 0;
      std::vector<Ipv4Address> knownBeacons = m_disTable.GetKnownBeacons ();
      std::vector<Ipv4Address>::const_iterator addr;
      for (addr = knownBeacons.begin (); addr != knownBeacons.end (); ++addr) {
        Position beaconPos = m_disTable.GetBeaconPosition(*addr);
        points[counter] = {beaconPos.first, beaconPos.second};
        distances[counter] = m_disTable.GetHopSizeOf(*addr) * m_disTable.GetHopsTo(*addr);

        totalDist += (double)distances[counter];
        totalLat +=  m_disTable.LastUpdatedAt(*addr).GetDouble();
        totalHops += m_disTable.GetHopsTo(*addr);
        counter++;
        if(counter==3) break;
      }

      output.avgDist = totalDist / 3.0;
      output.avgLat = totalLat / 3.0;
      output.avgHops = totalHops / 3.0;

      return output;

    }

  }
}

