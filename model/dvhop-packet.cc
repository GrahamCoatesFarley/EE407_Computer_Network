#include "dvhop-packet.h"
#include "ns3/packet.h"
#include "ns3/address-utils.h"

namespace ns3
{
  namespace dvhop
  {

    NS_OBJECT_ENSURE_REGISTERED (FloodingHeader);

    FloodingHeader::FloodingHeader()  // Default Constructor
    {
    }

    FloodingHeader::FloodingHeader(double xPos, double yPos, uint16_t seqNo, uint16_t hopCount, double hopSize, Ipv4Address beacon)
    {
      // Set (X,Y) Coordinate of Node
      m_xPos     = xPos;        
      m_yPos     = yPos;      
      // Set sequence Number of Packet
      m_seqNo    = seqNo;      
      // Set the current hop count
      m_hopCount = hopCount;
      // Set the current hop size
      m_hopSize = hopSize;
      // Sets the ID of the beacon (node of reference)
      m_beaconId = beacon;     
    }

    TypeId
    FloodingHeader::GetTypeId ()      // Sets the interface ID for the packet
    {
      static TypeId tid = TypeId("ns3::dvhop::FloodingHeader")
          .SetParent<Header> ()
          .AddConstructor<FloodingHeader>();
      return tid;
    }

    TypeId
    FloodingHeader::GetInstanceTypeId () const  // Gets the interfaces ID
    {
      return GetTypeId ();
    }

    uint32_t
    FloodingHeader::GetSerializedSize () const
    {
      return 32; //Total number of bytes when serialized
    }

    void
    FloodingHeader::Serialize (Buffer::Iterator start) const  // Converts the coordinates of the node into its integral byte representation
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

      double hopSize = m_hopSize;
      char *const p3 = reinterpret_cast<char*>(&hopSize);
      std::copy(p3, p3+sizeof(uint64_t), reinterpret_cast<char*>(&dst));
      start.WriteHtonU64 (dst);

      start.WriteU16 (m_seqNo);      
      start.WriteU16 (m_hopCount);  
      WriteTo(start, m_beaconId);
    }

    uint32_t
    FloodingHeader::Deserialize (Buffer::Iterator start)    // Returns byte representation of thecoordinates, sequence number and hop count to double values
    {
      Buffer::Iterator i = start;


      uint64_t midX = i.ReadNtohU64 ();
      char *const p = reinterpret_cast<char*>(&midX);
      std::copy(p, p + sizeof(double), reinterpret_cast<char*>(&m_xPos));

      uint64_t midY = i.ReadNtohU64 ();
      char* const p2 = reinterpret_cast<char*>(&midY);
      std::copy(p2, p2 + sizeof(double), reinterpret_cast<char*>(&m_yPos));

      uint64_t midHopSize = i.ReadNtohU64 ();
      char* const p3 = reinterpret_cast<char*>(&midHopSize);
      std::copy(p3, p3 + sizeof(double), reinterpret_cast<char*>(&m_hopSize));

//      std::cout << "Deserializing coordinates ("<<m_xPos <<","<<m_yPos<<")"<<std::endl;

      m_seqNo = i.ReadU16 ();
      m_hopCount = i.ReadU16 ();
      ReadFrom (i, m_beaconId);

      //Validate the read bytes match the serialized size
      uint32_t dist = i.GetDistanceFrom (start);
      NS_ASSERT (dist == GetSerializedSize () );
      return dist;
    }

    // Prints the Beacons IP address and the hop count, hop size and coordinates
    void
    FloodingHeader::Print (std::ostream &os) const  
    {
      //                  "Reference" Node            Current HopCount   HopSize       X                  Y
      os << "\nBeacon: " << m_beaconId << " ,hopCount: " << m_hopCount <<" ,hopSize: " << m_hopSize <<", (" << m_xPos << ", "<< m_yPos<< ")\n";

    }

    // Overloads the output stream extraction operator to call the defined Print function
    std::ostream &
    operator<< (std::ostream &os, FloodingHeader const &h)  
    {
      // Print the packet header
      h.Print (os);
      return os;
    }



  }
}
