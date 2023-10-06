/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "ns3/core-module.h"       // provides access to features such as Attributes, Callbacks, Configuration, Testing and Tracing
#include "ns3/network-module.h"    // provides access to network features such as Address, Data Rate, Network Deive, Packet, and Socket
#include "ns3/csma-module.h"	   // provides access to CSMA system tests
#include "ns3/internet-module.h"   // provides access to IPv4 and IPv6 helper classes, routing protocols, TCP and UDP, etc.
#include "ns3/point-to-point-module.h"		// provides access to the point to point data link
#include "ns3/applications-module.h"		// provides access to features such as OnOffApplication, PacketSin, etc.
#include "ns3/ipv4-global-routing-helper.h"	// provides access to pre-simulation static route computation for IPV4 topology

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0


using namespace ns3;		// The current namespace of the project (helpful if the program needs to be used by another project)

NS_LOG_COMPONENT_DEFINE ("SecondScriptExample");

int
main (int argc, char *argv[])		// Start of the main function of the project
{
  bool verbose = true;	// Boolean variable to indicate true or false in logic expressions
  uint32_t nCsma = 3;	// Unsigned 32-bit integer value to indicate the number of CSMA devices

  CommandLine cmd;	// Command line object instantiation
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma); 	// adds 3 Csma program arguments, with descriptive t ext indicating they are extra CSMA devices 
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);   // adds an argument with boolean value as the callback called verbose

  cmd.Parse (argc,argv);  // parses program arguments, from the input command-line and will attempt to handle them all

  if (verbose)	// If verbose is TRUE
    {
	// Enables logging output associated with  the UdpEcho client and server applications
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  nCsma = nCsma == 0 ? 1 : nCsma;	// ternary operation, if nCsma's value is 0, it gets 1, otherwise the value does not change

  NodeContainer p2pNodes;	// Instantiates a node container class object for point to point nodes
  p2pNodes.Create (2);		// Creates 2 p2p nodes within the class object

  NodeContainer csmaNodes;		// Instantiates a node container class object for csma nodes
  csmaNodes.Add (p2pNodes.Get (1));	// Retrieves and adds ONE of the p2p nodes to the csma node class object
  csmaNodes.Create (nCsma);		// Creates a number (indicated by the value of nCsma) of nodes within the class object

  PointToPointHelper pointToPoint;	// Instantiates a point to point helper class object
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));	// Sets the Data Rate device attribute of the p2p helper object to 5Mbps
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));	// Sets the Delay channel attribute to 2ms

  NetDeviceContainer p2pDevices;	// Instantiates a net device container class object for p2p devices
  p2pDevices = pointToPoint.Install (p2pNodes);	// Installs each p2p node in the p2pNodes class object as p2p devices

  CsmaHelper csma;	// Instantiates a csma helper class object
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps")); // Sets the DataRate channel attribute of the csma helper object to 100Mbps
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560))); // Set the Delay channel attribute of the csma helper boject to 6560ns

  NetDeviceContainer csmaDevices;		// Instantiates a net device container class object for csma devices
  csmaDevices = csma.Install (csmaNodes);	// Installs each csma node in the csmaNodes class object as csma devices

  InternetStackHelper stack; 		// Instantiates an internet stack helper class object stack
  stack.Install (p2pNodes.Get (0));	// Installs the FIRST (indexed from 0) p2pNode to the stack object
  stack.Install (csmaNodes);		// Installs ALL of the csmaNodes to the stack object

  Ipv4AddressHelper address;				// Instantiates an Ipv4 address helper class object address
  address.SetBase ("10.1.1.0", "255.255.255.0");	// Sets the base IP address and mask 
  Ipv4InterfaceContainer p2pInterfaces;			// Instatiates an Ipv4 interface container class object for p2p interfaces
  p2pInterfaces = address.Assign (p2pDevices);		// Assigns the set address to the p2p interfaces

  address.SetBase ("10.1.2.0", "255.255.255.0");	// Sets a new base IP address and mask
  Ipv4InterfaceContainer csmaInterfaces;		// Instantiates an Ipv4 interface container class object for csma interfaces
  csmaInterfaces = address.Assign (csmaDevices);	// Assigns the set address to the csma interfaces

 // Setup of the Server on the CSMA device:
  UdpEchoServerHelper echoServer (9);                   // Instantiates a Udp echo server helper class object with port #9 for the server

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma)); // Instantiates a container for the echoServer with the last csma device node
  serverApps.Start (Seconds (1.0));     // Sets the server application start time to 1 second
  serverApps.Stop (Seconds (10.0));     // Sets the server application stop time to 10 seconds

  // Setup of the Client Application:
  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9); // Instantiates a udp echo client helper class object with (to send packets) the
                                                                         // base Ipv4 address and mask of the last Csma interfaces and port 9
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));            // Sets the clients MaxPackets to unsigned integer value 1
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));      // Sets the clients Interval for packets to 1 second
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));         // Sets the clients PacketSize to unsigned integer value 1024

  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0)); // Creates a client application class object and sets it equal to the echo client 1st p2p node installed
  clientApps.Start (Seconds (2.0));	// Sets the client application start time to 2 seconds
  clientApps.Stop (Seconds (10.0));	// Sets the client application stop time to 10 seconds

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();	// This creates a routing table to facilitate communication between the different routing paths

  pointToPoint.EnablePcapAll ("second");		 // Enables pcap tracing of p2p helper object
  csma.EnablePcap ("second", csmaDevices.Get (1), true); // Enables pcap tracing for the csma helper object 

  Simulator::Run ();		// Runs the simulation
  Simulator::Destroy ();	// Ends the simulation
  return 0;	//  Return value to indicate successful execution of the project/program
}
