#ifndef GTP_COMMON_H
#define GTP_COMMON_H

#include "IPvXAddress.h"
#include <map>

#define LOCAL_ADDRESS_TEID 127

struct ConnectionInfo
{
    ConnectionInfo(unsigned int id, IPvXAddress hop): teid(id) , nextHop(hop){}

    unsigned int teid;
    IPvXAddress nextHop;
};

// extend the filter to the other elements of the IP 4-tuple
// We recall that: in order to use a structure as key of a std::map we need to redefine the lessThan operator (or something like that)
struct TrafficFilterTemplate
{
    TrafficFilterTemplate(IPvXAddress dest) : destAddr(dest) {}
    //IPvXAddress srcAddr;
    IPvXAddress destAddr;

    //unsigned int srcPort;
    //unsigned int destPort;
};

typedef std::map<unsigned int,ConnectionInfo> LabelTable;

// TODO udpdate the definition of TrafficFilterTemplateTable in order to use the TrafficFilterTemplate structure
//typedef std::map<TrafficFilterTemplate,unsigned int> TrafficFilterTemplateTable;
typedef std::map<IPvXAddress,unsigned int> TrafficFilterTemplateTable;

char * const * loadXmlTable(char const * attributes[] , unsigned int numAttributes);

#endif
