/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
    Header Documentation:
    Adapted DV-Hop example to simulate DV-hop on NS3.30.1

    For Critical Conditons Implementation

    Contributors:
      Graham Coates-Farely    -- Team Lead
      Mikaila Flavell
      Ahmad Suleiman
      Peter Russell
      Vincent Cifone
      Jashitha Boyapati
      Bhavani Bandi


*/

#include "ns3/dvhop-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include <iostream>
#include <cmath>

using namespace ns3;

/**
 * \brief Test script.
 *
 * This script creates 1-dimensional grid topology and then ping last node from the first one:
 *
 * [10.0.0.1] <-- step --> [10.0.0.2] <-- step --> [10.0.0.3] <-- step --> [10.0.0.4]
 *
 *
 */
class DVHopExample
{
public:
  DVHopExample ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /// Report results
  void Report (std::ostream & os);

private:
  ///\name parameters
  //\{
  /// Number of nodes
  uint32_t size;
  /// Distance between nodes, meters
  double step;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;
  //\}

  ///\name network
  //\{
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
  //\}

private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();
  void CreateBeacons();
};

int main (int argc, char **argv)                          // Main loop invitation 
{
  DVHopExample test;                                      // Creates DVHop 
  if (!test.Configure (argc, argv))                       // Triggers in the event test objects configuration fails 
    NS_FATAL_ERROR ("Configuration failed. Aborted.");    // Delcares error if the trigger condition is met.

  test.Run ();                                            // Initiates running sequence of DVhop simulation
  test.Report (std::cout);                                
  return 0;                                               // Return successful execution 
}

//-----------------------------------------------------------------------------
DVHopExample::DVHopExample () :
  size (20),              			// Sets number of nodes
  step (10),             // Set step size between nodes
  totalTime (10),         // Sets simulation run time
  pcap (true),            // Enables pcap generation  
  printRoutes (true)      // Enables route printing
{
}

bool
DVHopExample::Configure (int argc, char **argv)
{
  // Enable DVHop logs by default. Comment this if too noisy
  LogComponentEnable("DVHopRoutingProtocol", LOG_LEVEL_ALL);

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("step", "Grid step, m", step);

  cmd.Parse (argc, argv);
  return true;
}

void
DVHopExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();                  // Creates nodes for simulation
  CreateDevices ();                // Installs devices on Nodes
  InstallInternetStack ();         // Establishes Internet topoglogy

  CreateBeacons();                  // Converts a number of nodes to beacons

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  Simulator::Stop (Seconds (totalTime));      // Establishes the Stop time for the simulation
  
  AnimationInterface anim("animV.xml");   // Establishes the file for animation generation of simulation    

  Simulator::Run ();        // Runs the sim
  Simulator::Destroy ();    // Recycles simulation resources post execution
}


// TODO: Report simulation result 
void
DVHopExample::Report (std::ostream &)                            
{
}

void
DVHopExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes " << step << " m apart.\n";
  nodes.Create (size);	// Create all nodes + beacons
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      std::cout << "Creating node: "<< os.str ()<< std::endl ;
      Names::Add (os.str (), nodes.Get (i));
    }
  // Create static grid
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),        // Minimum Coordinate Grid Positions  (0.0,0.0)
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (step /3),      // Delta (change in) Coordinate Grid Positions (step, 0)
                                 "DeltaY", DoubleValue (step /2),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
}

void
DVHopExample::CreateBeacons ()
{
  // This is Currently hardcoded to create beacons, can use rand() between 0 and maxNode 
  // a number of times, maybe 10-15% of the max nodes as beacons?

  //for(uint32_t i = size; i < (size +sizeB); i++)
  
    Ptr<Ipv4RoutingProtocol> proto = nodes.Get (0)->GetObject<Ipv4>()->GetRoutingProtocol ();
  	Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
  	dvhop->SetIsBeacon (true);
  	dvhop->SetPosition (0, 2);
  


  proto = nodes.Get (4)->GetObject<Ipv4>()->GetRoutingProtocol ();
  dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
  dvhop->SetIsBeacon (true);
  dvhop->SetPosition (25, 10);


  proto = nodes.Get (9)->GetObject<Ipv4>()->GetRoutingProtocol ();
  dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
  dvhop->SetIsBeacon (true);
  dvhop->SetPosition (30, 15);

}


void
DVHopExample::CreateDevices ()
{
  WifiMacHelper wifiMac = WifiMacHelper();
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi = WifiHelper();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes);

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
}

void
DVHopExample::InstallInternetStack ()
{
  DVHopHelper dvhop;
  // you can configure DVhop attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (dvhop); // has effect on the next Install ()
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");                                                            // Adjust the IP address to the following 
  interfaces = address.Assign (devices);

  Ptr<OutputStreamWrapper> distStream = Create<OutputStreamWrapper>("dvhop.distances", std::ios::out);
  dvhop.PrintDistanceTableAllAt(Seconds(9), distStream);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("dvhop.routes", std::ios::out);
      dvhop.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}
