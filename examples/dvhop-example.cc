/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
    Header Documentation:
    Adapted DV-Hop example to simulate DV-hop on NS3.30.1

    For Ideal Conditions Implementation

    Contributors:
      Graham Coates-Farely    -- Team Lead
      Mikaila Flavell
      Ahmad Suleiman
      Russell Peter 
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
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

/**
 * \brief Test script.
 *
 * This script creates 1-dimensional grid topology and then ping last node from the first one:
 *
 *
 */
class DVHopExample
{
public:
  /// Default constructor 
  DVHopExample ();
  /// NonDefault Constructor with time of simulation passed
  DVHopExample (double time);
  /// Configure script parameters, \return true on successful configuration, passed seed for configuration
  bool Configure (int argc, char **argv, u_int32_t seed);
  /// Run simulation, boolean to determine ideal/critical scenario
  void Run (bool crit);
  /// Report results
  void Report (std::ostream & os);
  /// Sets the simulation time (primary use in critical condition but does not effect ideal)
  void SetSimTime ();


private:
  ///\name parameters
  //\{
  /// Number of nodes
  uint32_t size;
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
  void MakeCritical();
};

//----------------------------------------------------------------------------------------------------------------------------------

// Constants for ease of size
const u_int32_t SIZE = 200;               // Number of nodes
const u_int32_t DEFAULT_TIME = 50;      // Default simulation time
const u_int32_t DEFAULT_SEED = 12345;   // Default simulation seed
const double SENT = -1.0;               // Sentinel Value

int main (int argc, char **argv)                          // Main loop invitation 
{
  char ansA, ansB;
  bool crit = false;
  double time = 0.0;
  u_int32_t seed = 0;

  // User determination of whether the simulation is critical or ideal
  std::cout << "\nShould this be a critical simulation? (Y or N)\n";
  std::cin.get(ansA);
  ansA = toupper(ansA);

  if(ansA == 'Y')
    crit = true;

  // User determination of simulation time
  std::cout << "\nPlease input simulation time-span in seconds. For default time (10s) input -1\n";
  std::cin >> time;

  // User determination of execution seed
  std::cout << "\nShould the simulation run on a default or random seed? (R for Random, anything else for Default)\n";
  std::cin >> ansB;
  ansB = toupper(ansB);

  if (ansB == 'R')
  {
    srand(time * DEFAULT_SEED);
    seed = rand()%99999 + 10000;
  }
  else
    seed = DEFAULT_SEED;

  std::cout << "\n\n" << seed <<"\n\n";

  DVHopExample test;                                      // Creates DVHop 
  // Sets new simulation time if requested other than default (input not SENT)
  if(time != SENT)
    test = DVHopExample(abs(time)); // Ensures a non negative simulation time


  if (!test.Configure (argc, argv, seed))                       // Triggers in the event test objects configuration fails 
    NS_FATAL_ERROR ("Configuration failed. Aborted.");    // Declares error if the trigger condition is met.

  test.Run (crit);                                   // Initiates running sequence of DVhop simulation
  test.Report (std::cout);
  Simulator::Destroy ();                                  // Recycles simulation resources post execution
  return 0;                                               // Return successful execution 
}

//-----------------------------------------------------------------------------
DVHopExample::DVHopExample () :
  size (SIZE),              			// Sets number of nodes
  totalTime (DEFAULT_TIME),         // Sets simulation run time
  pcap (true),            // Enables pcap generation  
  printRoutes (true)      // Enables route printing
{
}

DVHopExample::DVHopExample (double time) :
  size (SIZE),              			// Sets number of nodes
  totalTime (time),         // Sets simulation run time
  pcap (false),            // Enables pcap generation
  printRoutes (true)      // Enables route printing
{
}


bool
DVHopExample::Configure (int argc, char **argv, u_int32_t seed)
{
  // Enable DVHop logs by default. Comment this if too noisy
  //LogComponentEnable("DVHopRoutingProtocol", LOG_LEVEL_ALL);
  
  SeedManager::SetSeed (seed);
  
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);

  cmd.Parse (argc, argv);
  return true;
}

void
DVHopExample::Run (bool crit)
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();                  // Creates nodes for simulation
  CreateDevices ();                // Installs devices on Nodes
  InstallInternetStack ();         // Establishes Internet topology

  if(crit)
    MakeCritical();

  CreateBeacons();                  // Converts a number of nodes to beacons
  SetSimTime();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  Simulator::Stop (Seconds (totalTime));      // Establishes the Stop time for the simulation
  
  
  AnimationInterface anim("anim_ideal.xml");   // Establishes the file for animation generation of simulation    

  Simulator::Run ();        // Runs the sim
}

void
DVHopExample::Report (std::ostream &)                            
{
  // Go through all non anchor nodes and calculate the localization error
  double totalLE = 0;
  u_int32_t totalBeacons = 0;
  u_int32_t tempSize = SIZE;

  for(uint32_t i=0; i < size; i++) {
    Ptr <Ipv4RoutingProtocol> proto = nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol();
    Ptr <dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol>(proto);
    if(dvhop->IsBeacon()) {
      totalBeacons += 1;
      continue; // Dont calculate beacon error
    }
    Ptr <MobilityModel> mob = nodes.Get(i)->GetObject<MobilityModel>();

    double dx = dvhop->GetXPosition() - mob->GetPosition().x;
    double dy = dvhop->GetYPosition() - mob->GetPosition().y;
    double LE = pow(pow(dx,2) + pow(dy,2), 0.5);

    std::cout << "Localization Error LE for Node " << i << " = " << LE << std::endl;

    if (!isinf(LE)){
      totalLE += LE;
    }
    else{
      tempSize -= 1;
    }
  }
  std::cout << "Average Localization Error LE = "<<(totalLE/(tempSize-totalBeacons)) << std::endl;
}

void
DVHopExample::CreateNodes ()
{
  std::cout << "Creating RandomRectangle Nodes" << (unsigned)size << " nodes\n";
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
  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=100]"),
                                 "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=100]"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
}

void
DVHopExample::MakeCritical ()
{
  for(uint32_t i = 0; i < size; ++i)
  {
    Ptr<Ipv4RoutingProtocol> proto = nodes.Get (i)->GetObject<Ipv4>()->GetRoutingProtocol ();
  	Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
  	dvhop->SetIsCritical (true);

  }
}

void
DVHopExample::CreateBeacons ()
{
  // This is Currently hardcoded to create beacons, can use rand() between 0 and maxNode 
  // a number of times, maybe 10-15% of the max nodes as beacons?

  uint32_t beaconCount = 3;
  uint32_t beacons[3] = {(uint32_t)((double)size * 0.12), (uint32_t)((double)size * 0.48), (uint32_t)((double)size * 0.82)}; // Node ID of our beacons

  for(uint32_t i=0; i < beaconCount; i++) {
    Ptr <Ipv4RoutingProtocol> proto = nodes.Get(beacons[i])->GetObject<Ipv4>()->GetRoutingProtocol();
    Ptr <dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol>(proto);
    dvhop->SetIsBeacon(true);

    Ptr <MobilityModel> mob = nodes.Get(beacons[i])->GetObject<MobilityModel>();
    dvhop->SetPosition(mob->GetPosition().x, mob->GetPosition().y);
  }

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
  dvhop.PrintDistanceTableAllAt(Seconds(totalTime), distStream);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("dvhop.routes", std::ios::out);
      dvhop.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}

void
DVHopExample::SetSimTime ()
{
  for(uint32_t i = 0; i < size; ++i)
  {
    Ptr<Ipv4RoutingProtocol> proto = nodes.Get (i)->GetObject<Ipv4>()->GetRoutingProtocol ();
  	Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
  	dvhop->SetSimulationTime (totalTime);

  }
}
