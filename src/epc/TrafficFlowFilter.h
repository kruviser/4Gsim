//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef ___4GSIM_TRAFFICFLOWFILTER_H_
#define ___4GSIM_TRAFFICFLOWFILTER_H_

#include <omnetpp.h>
//#include "trafficFlowTemplateMsg_m.h"
#include "TftControlInfo.h"
#include "gtp_common.h"


/**
 * The traffic filter uses a TrafficFilterTemplateTable that maps IP 4-Tuples to TFT identifiers.
 * The TrafficFilterTemplateTable has two levels:
 *  - at the first level there up to one entry for each IPvXAddress. This may be a destination (on the P-GW side) or source (on the eNB side) address
 *  - at the second level each entry is a list of TrafficFlowTemplates structures, with a src/dest address (depending on the first level key,
 *    a dest and src port, and a tftId;
 *
 * This table is specified via (part of) a XML configuration file. Note that the fields of the TrafficFlowTemplates (except for the tftId) may
 * be left unspecified
 *
 * Example format for traffic filter XML configuration
    </config>
        <filterTable>
            <filter
                destName   ="Host2"
                tftId      = "1"
            />
            <filter
                destAddr   = "10.1.1.1"
                tftId      = "2"
            />
        </filterTable>
   </config>
 *
 * Each entry of the filter table is specified with a "filter" tag
 * For each filter entry the "tftId" and one between "destName" and "destAddr" must be specified
 * In case of both "destName" and "destAddr" values, the "destAddr" will be used
 *
 */
class TrafficFlowFilter : public cSimpleModule
{
    // specifies the type of the node that contains this filter (it can be ENB or PGW
    // the filterTable_ will be indexed differently depending on this parameter
    EpcNodeType ownerType_;

    // gate for connecting with the GTP-U module
    cGate * gtpUserGate_;

    TrafficFilterTemplateTable filterTable_;

    void loadFilterTable(const char * filterTableFile);

    EpcNodeType selectOwnerType(const char * type);
  protected:
    virtual int numInitStages() const {return 4;}
    virtual void initialize(int stage);

    // TrafficFlowFilter module may receive messages only from the input interface of its compound module
    virtual void handleMessage(cMessage *msg);

    // manage filter table
    TrafficFlowTemplateId findTrafficFlow(IPvXAddress firstKey , TrafficFlowTemplate secondKey );
    bool addTrafficFlow( IPvXAddress firstKey , TrafficFlowTemplate tft);

};

#endif
