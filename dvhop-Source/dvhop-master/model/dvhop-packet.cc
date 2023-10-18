#include "dvhop-packet.h"
#include "ns3/packet.h"
#include "ns3/address-utils.h"

namespace ns3
{
  namespace dvhop
  {

    NS_OBJECT_ENSURE_REGISTERED (FloodingHeader);

    FloodingHeader::FloodingHeader()
    {
    }

    FloodingHeader::FloodingHeader(double xPos, double yPos, uint16_t seqNo, uint16_t hopCount, Ipv4Address beacon)
    {
      m_xPos     = xPos;
      m_yPos     = yPos;
      m_seqNo    = seqNo;
      m_hopCount = hopCount;
      m_beaconId = beacon;
    }

    TypeId
    FloodingHeader::GetTypeId ()
    {
      static TypeId tid = TypeId("ns3::dvhop::FloodingHeader")
          .SetParent<Header> ()
          .AddConstructor<FloodingHeader>();
      return tid;
    }

    TypeId
    FloodingHeader::GetInstanceTypeId () const
    {
      return GetTypeId ();
    }

    uint32_t
    FloodingHeader::GetSerializedSize () const
    {
      return 24; //Total number of bytes when serialized
    }

    void
    FloodingHeader::Serialize (Buffer::Iterator start) const
    {
      //The position info are serialized as uint64_t, though they're doubles
      //We convert the double to a unsigned long and then serialize that number
      double x = m_xPos;
      uint64_t dst;
      char *const p = reinterpret_cast<char*>(&x);
      std::copy(p, p+sizeof(uint64_t), reinterpret_cast<char*>(&dst));
      start.WriteHtonU64 (dst);

      double y = m_yPos;
      char* const p2 = reinterpret_cast<char*>(&y);
      std::copy(p2, p2+sizeof(uint64_t), reinterpret_cast<char*>(&dst));
      start.WriteHtonU64 (dst);

      start.WriteU16 (m_seqNo);
      start.WriteU16 (m_hopCount);
      WriteTo(start, m_beaconId);
    }

    uint32_t
    FloodingHeader::Deserialize (Buffer::Iterator start)
    {
      Buffer::Iterator i = start;


      uint64_t midX = i.ReadNtohU64 ();
      char *const p = reinterpret_cast<char*>(&midX);
      std::copy(p, p + sizeof(double), reinterpret_cast<char*>(&m_xPos));

      uint64_t midY = i.ReadNtohU64 ();
      char* const p2 = reinterpret_cast<char*>(&midY);
      std::copy(p2, p2 + sizeof(double), reinterpret_cast<char*>(&m_yPos));


      std::cout << "Deserializing coordinates ("<<m_xPos <<","<<m_yPos<<")"<<std::endl;

      m_seqNo = i.ReadU16 ();
      m_hopCount = i.ReadU16 ();
      ReadFrom (i, m_beaconId);

      //Validate the readed bytes match the serialized size
      uint32_t dist = i.GetDistanceFrom (start);
      NS_ASSERT (dist == GetSerializedSize () );
      return dist;
    }

    void
    FloodingHeader::Print (std::ostream &os) const
    {
      os << "Beacon: " << m_beaconId << " ,hopCount: " << m_hopCount << ", (" << m_xPos << ", "<< m_yPos<< ")\n";

    }

    std::ostream &
    operator<< (std::ostream &os, FloodingHeader const &h)
    {
      h.Print (os);
      return os;
    }



  }
}
