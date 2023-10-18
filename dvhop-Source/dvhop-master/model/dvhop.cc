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
      HelloInterval (Seconds (1)),         //Send HELLO each second
      m_htimer (Timer::CANCEL_ON_DESTROY), //Set timer for HELLO
      m_isBeacon(false),
      m_xPosition(12.56),
      m_yPosition(468.5),
      m_seqNo (0)
    {
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
    RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
    {
      *stream->GetStream () << "(" << m_ipv4->GetObject<Node>()->GetId() << " - Not implemented yet";
    }


    void
    RoutingProtocol::PrintDistances (Ptr<OutputStreamWrapper> stream, Ptr<Node> node) const
    {
      *stream->GetStream ()<<"----------------- Node "<<node->GetId ()<<"-----------------"<<"\n";
      m_disTable.Print (stream);
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

      SendHello ();

      m_htimer.Cancel ();
      m_htimer.Schedule (RoutingProtocol::HelloInterval);
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
          /*TODO: Remove the hardcoded position*/

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
                                         m_seqNo++,                   //Sequence Numbr
                                         0,                           //Hop Count
                                         iface.GetLocal ());          //Beacon Address
              std::cout <<__FILE__<< __LINE__ << helloHeader << std::endl;

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
      UpdateHopsTo (fHeader.GetBeaconAddress (), fHeader.GetHopCount () + 1, fHeader.GetXPosition (), fHeader.GetYPosition ());



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
    RoutingProtocol::UpdateHopsTo (Ipv4Address beacon, uint16_t newHops, double x, double y)
    {
      uint16_t oldHops = m_disTable.GetHopsTo (beacon);
      if (m_ipv4->GetInterfaceForAddress (beacon) >= 0){
          NS_LOG_DEBUG ("Local Address, not updating in table");
          return;
        }

      if( oldHops > newHops || oldHops == 0) //Update only when a shortest path is found
        m_disTable.AddBeacon (beacon, newHops, x, y);
    }




  }
}

