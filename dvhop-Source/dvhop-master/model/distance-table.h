#ifndef DISTANCETABLE_H
#define DISTANCETABLE_H

#include <map>
#include <vector>
#include "ns3/ipv4.h"
#include "ns3/nstime.h"
#include "ns3/output-stream-wrapper.h"

namespace ns3
{
  namespace dvhop
  {


    typedef std::pair<double, double> Position;

    class BeaconInfo
    {
    public:
      uint16_t  GetHops()     const   { return m_hops;     }
      Position  GetPosition() const   { return m_pos;      }
      Time      GetTime()     const   { return m_updatedAt;}

      void SetHops    (uint16_t hops) { m_hops = hops;  }
      void SetPosition(Position p)    { m_pos  = p;     }
      void SetTime    ( Time t )      { m_updatedAt = t;}

    private:
      uint16_t m_hops;
      Position m_pos;
      Time     m_updatedAt;
    };

    std::ostream & operator<< (std::ostream & os, BeaconInfo const &);




    /**
     * @brief The DistanceTable class stores local
     *information about the beacons known to the node.
     */
    class DistanceTable
    {
    public:
      DistanceTable();

      /**
       * @brief GetSize The number of entries stored in this table
       * @return The size
       */
      size_t  GetSize() const  { return m_table.size (); }


      /**
       * @brief GetHopsTo Gets the last known hops to a certain beacon
       * @param beacon The beacon address
       * @return The hop count to the beacon, or 0 if there is no such information
       */
      uint16_t    GetHopsTo(Ipv4Address beacon) const;

      /**
       * @brief GetBeaconPosition Get the ordered pair representing the absolute position of the beacon
       * @param beacon The beacon address
       * @return The coordinates of the beacon
       */
      Position    GetBeaconPosition(Ipv4Address beacon) const;

      /**
       * @brief LastUpdatedAt Gets the time in which the information for the beacon was updated for the last time
       * @param beacon The address of the beacon
       * @return The time
       */
      Time LastUpdatedAt(Ipv4Address beacon) const;

      /**
       * @brief GetKnownBeacons
       * @return A vector containing the known beacons
       */
      std::vector<Ipv4Address> GetKnownBeacons() const;

      /**
       * @brief Print Print this DistanceTable to the output stream provided
       * @param os The stream
       */
      void Print(Ptr<OutputStreamWrapper> os) const;

      /**
       * @brief AddBeacon Creates or updates an entry for a newly discovered beacon
       * @param beacon The beacon address
       * @param hops Hops to the beacon
       * @param xPos X coordinate
       * @param yPos Y coordinate
       */
      void AddBeacon(Ipv4Address beacon, uint16_t hops, double xPos, double yPos);
    private:
      std::map<Ipv4Address, BeaconInfo>  m_table;
    };





  }
}


#endif // DISTANCETABLE_H
