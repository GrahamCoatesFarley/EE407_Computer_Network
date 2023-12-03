#include "distance-table.h"
#include "ns3/simulator.h"
#include <algorithm>

namespace ns3
{
  namespace dvhop
  {


    DistanceTable::DistanceTable()		// Default Constructor
    {
    }

    // Returns the number of hops to get to the passed beacon
    uint16_t
    DistanceTable::GetHopsTo (Ipv4Address beacon) const
    {
      std::map<Ipv4Address, BeaconInfo>::const_iterator it = m_table.find (beacon);
      if( it != m_table.end ())
        {
          return ((BeaconInfo)it->second).GetHops ();
        }

      else return 0;
    }

    // Returns the hop size of the passed beacon
    double
    DistanceTable::GetHopSizeOf (Ipv4Address beacon) const
    {
      std::map<Ipv4Address, BeaconInfo>::const_iterator it = m_table.find (beacon);
      if( it != m_table.end ())
      {
        return ((BeaconInfo)it->second).GetHopSize ();
      }

      else return -1.0;
    }

    // Returns the position of the passed beacon
    Position
    DistanceTable::GetBeaconPosition (Ipv4Address beacon) const
    {
      std::map<Ipv4Address, BeaconInfo>::const_iterator it = m_table.find (beacon);
      if( it != m_table.end ())
        {
          return ((BeaconInfo)it->second).GetPosition ();
        }

      else return Position(-1.0,-1.0);
    }

    // Adds a new Beacon to the data table, assigning its BeaconInfo
    void
    DistanceTable::AddBeacon (Ipv4Address beacon, uint16_t hops, double hopSize, double xPos, double yPos)
    {
      std::map<Ipv4Address, BeaconInfo>::iterator it = m_table.find (beacon);
      BeaconInfo info;
      if( it != m_table.end ())
        {
          info.SetPosition (it->second.GetPosition ());
          info.SetHops (hops);
          info.SetHopSize(hopSize);
          info.SetTime (Simulator::Now ());
          it->second = info;
        }
      else
        {
          info.SetHops (hops);
          info.SetHopSize(hopSize);
          info.SetPosition (Position(xPos, yPos));
          info.SetTime (Simulator::Now ());
	        m_table[beacon] = info;
          //m_table.insert (std::pair<Ipv4Address, BeaconInfo>(beacon&, info));
        }
    }

    // Returns the time at which the passed beacon information was
    // last updated
    Time
    DistanceTable::LastUpdatedAt (Ipv4Address beacon) const
    {
      std::map<Ipv4Address, BeaconInfo>::const_iterator it = m_table.find (beacon);
      if( it != m_table.end ())
        {
          return ((BeaconInfo)it->second).GetTime ();
        }

      else return Time::Max ();
    }

    // Creates a linked list (as a stack) of each node for access
    std::vector<Ipv4Address>
    DistanceTable::GetKnownBeacons() const
    {
      std::vector<Ipv4Address> theBeacons;
      for(std::map<Ipv4Address, BeaconInfo>::const_iterator j = m_table.begin (); j != m_table.end (); ++j)
        {
          theBeacons.push_back (j->first);
        }
      return theBeacons;
    }

    // Prints the beacon Address and Information to the output stream
    void
    DistanceTable::Print (Ptr<OutputStreamWrapper> os) const
    {
      *os->GetStream () << m_table.size () << " entries\n";
      for(std::map<Ipv4Address,BeaconInfo>::const_iterator j = m_table.begin (); j != m_table.end (); ++j)
        {
          //                    BeaconAddr           BeaconInfo
          *os->GetStream () <<  j->first << "\t" << j->second;
        }
    }

    // Overload of the extraction operator to print the known beacon information
    std::ostream &
    operator<< (std::ostream &os, BeaconInfo const &h)
    {
      std::pair<float,float>  pos = h.GetPosition ();
      //	Hops    HopSize			X		     Y		Record Timestamp
      os << h.GetHops () << "\t"<< h.GetHopSize () << "\t(" << pos.first << ","<< pos.second << ")\t"<< h.GetTime ()<<"\n";
      return os;
    }


  }
}



