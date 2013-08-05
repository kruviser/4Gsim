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
 * The traffic filter uses a TrafficFilterTemplateTable that maps destrination address to TFT identifiers
 * This table is specified via (part of) a XML configuration file
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
    cGate * gtpUserGate_;

    TrafficFilterTemplateTable filterTable_;

    void loadFilterTable(const char * filterTableFile);
  protected:
    virtual int numInitStages() const {return 4;}
    virtual void initialize(int stage);

    // TrafficFlowFilter module may receive messages only from the input interface of its compound module
    virtual void handleMessage(cMessage *msg);
};

#endif
