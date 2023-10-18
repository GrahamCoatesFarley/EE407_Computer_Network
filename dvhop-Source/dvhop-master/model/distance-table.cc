#include "distance-table.h"
#include "ns3/simulator.h"
#include <algorithm>

namespace ns3
{
  namespace dvhop
  {


    DistanceTable::DistanceTable()
    {
    }

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

    Position
    DistanceTable::GetBeaconPosition (Ipv4Address beacon) const
    {
      std::map<Ipv4Address, BeaconInfo>::const_iterator it = m_table.find (beacon);
      if( it != m_table.end ())
        {
          return ((BeaconInfo)it->second).GetPosition ();
        }

      else return std::make_pair<double,double>(-1.0,-1.0);
    }


    void
    DistanceTable::AddBeacon (Ipv4Address beacon, uint16_t hops, double xPos, double yPos)
    {
      std::map<Ipv4Address, BeaconInfo>::iterator it = m_table.find (beacon);
      BeaconInfo info;
      if( it != m_table.end ())
        {
          info.SetPosition (it->second.GetPosition ());
          info.SetHops (hops);
          info.SetTime (Simulator::Now ());
          it->second = info;
        }
      else
        {
          info.SetHops (hops);
          info.SetPosition (std::make_pair<double,double>(xPos, yPos));
          info.SetTime (Simulator::Now ());
          m_table.insert (std::make_pair<Ipv4Address, BeaconInfo>(beacon, info));
        }
    }


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


    std::ostream &
    operator<< (std::ostream &os, BeaconInfo const &h)
    {
      std::pair<float,float> pos = h.GetPosition ();
      os << h.GetHops () << "\t(" << pos.first << ","<< pos.second << ")\t"<< h.GetTime ()<<"\n";
      return os;
    }

  }
}



