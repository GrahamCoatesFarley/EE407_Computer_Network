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
      //				X		     Y		Record Timestamp
      os << h.GetHops () << "\t(" << pos.first << ","<< pos.second << ")\t"<< h.GetTime ()<<"\n";
      return os;
    }

    // Calculate the ho size of a beacon = Sum (all other anchors as i) SQRT((x-xi)^2 + (y-yi)^2)
    double
    DistanceTable::CalculateHopSize (uint16_t x, uint16_t y) const
    {
      double up = 0;
      double down = 0;
      for(std::map<Ipv4Address,BeaconInfo>::const_iterator j = m_table.begin (); j != m_table.end (); ++j)
      {
        uint16_t x_i = j->second.GetPosition().first;
        uint16_t y_i = j->second.GetPosition().second;

        up += sqrt(pow(x-x_i, 2) + pow(y-y_i, 2));
        down += j->second.GetHops();
      }

      double hop_size = up/down;
      return hop_size;
    }

    Point
    DistanceTable::Trilateration(Ptr<OutputStreamWrapper> os, std::vector<double> hopSize) const {
      Point points[3];
      double distances[3];

      uint16_t counter = 0;
      *os->GetStream () << "Distance entries\n";
      for(std::map<Ipv4Address,BeaconInfo>::const_iterator j = m_table.begin (); j != m_table.end (); ++j)
      {
        points[counter] = {j->second.GetPosition().first, j->second.GetPosition().second};
        distances[counter] = hopSize[counter] * j->second.GetHops();

        // Beacon IP Address     Distance
        *os->GetStream () << j->first << "\t" << distances[counter] << std::endl;
        counter++;
	if(counter==3) break;
      }

      Point location;
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
        *os->GetStream () << "The points are collinear or too close for trilateration!" << std::endl;
      }

      location.x = (ez * fy - ey * fz) / denominator;
      location.y = (ex * fz - ez * fx) / denominator;

      return location;

    }

    Data
    DistanceTable::ComputeData(Ptr<OutputStreamWrapper> os, std::vector<double> hopSizes) const{
      Point points[3];
      double distances[3];

      double totalDist = 0.0;
      double totalLat = 0.0;
      double totalHops = 0.0;

      Data output;

      uint16_t counter = 0;
      *os->GetStream () << "Distance entries\n";
      for(std::map<Ipv4Address,BeaconInfo>::const_iterator j = m_table.begin (); j != m_table.end (); ++j)
        {
          points[counter] = {j->second.GetPosition().first, j->second.GetPosition().second};
          distances[counter] = hopSizes[counter] * j->second.GetHops();

          totalDist += (double)distances[counter];
          totalLat += j->second.GetTime().GetDouble();
          totalHops += j->second.GetHops();

          // Beacon IP Address     Distance
          *os->GetStream () << j->first << "\t" << distances[counter] << std::endl;
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



