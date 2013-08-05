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

#include "TrafficFlowFilter.h"
#include "IPv4ControlInfo.h"
#include "IPv4Datagram.h"
#include "IPvXAddressResolver.h"

Define_Module(TrafficFlowFilter);

void TrafficFlowFilter::initialize(int stage)
{
    // wait until all the IP addresses are configured
    if ( stage!=3 )
        return;
    //============= Reading XML files =============
    const char *filename = par("filterFileName");
    if (filename == NULL || (!strcmp(filename, "")))
        error("TrafficFlowFilter::initialize - Error reading configuration from file %s", filename);
    loadFilterTable(filename);
    //=============================================
}

void TrafficFlowFilter::handleMessage(cMessage *msg)
{
    EV << "TrafficFlowFilter::handleMessage - Received Packet:" << endl;
    EV << "name: "<< msg->getFullName() << endl;

    // receive and read IP datagram
    IPv4Datagram * datagram = check_and_cast<IPv4Datagram *>(msg);
    IPv4Address &destAddr = datagram->getDestAddress();
    IPv4Address &srcAddr = datagram->getSrcAddress();

    EV << "Received datagram : " << datagram->getName() << " - src[" << destAddr << "] - dest[" << srcAddr << "]\n";

    // run packet filter and associate a flowId to the connection (default bearer?)
    // search within tftTable the proper entry for this destination
    TrafficFilterTemplateTable::iterator tftIt;
    tftIt = filterTable_.find(destAddr);
    if(tftIt==filterTable_.end())
    {
        EV << "TrafficFlowFilter::handleMessage - Cannot find entry for destAddress " << destAddr << ". Discarding packet;" << endl;
        return;
    }
    unsigned int tftId = tftIt->second; // this is the flow identifier

    // add control info to the normal ip datagram. This info will be read by the GTP-U application
    TftControlInfo * tftInfo = new TftControlInfo();
    tftInfo->setTft(tftId);
    datagram->setControlInfo(tftInfo);

    EV << "TrafficFlowFilter::handleMessage - setting tft=" << tftId << endl;

    // send the datagram to the GTP-U module
    send(datagram,"gtpUserGateOut");
}

void TrafficFlowFilter::loadFilterTable(const char * filterTableFile)
{
    // open and check xml file
    EV << "TrafficFlowFilter::loadFilterTable - reading file " << filterTableFile << endl;
    cXMLElement* config = ev.getXMLDocument(filterTableFile);
    if (config == NULL)
        error("TrafficFlowFilter::loadFilterTable: Cannot read configuration from file: %s", filterTableFile);

    // obtain reference to teidTable
    cXMLElement* filterNode = config->getElementByPath("filterTable");
    if (filterNode == NULL)
        error("TrafficFlowFilter::loadFilterTable: No configuration for teidTable");

    cXMLElementList tftList = filterNode->getChildren();

    IPvXAddress destAddr;

    unsigned int tftId;

    // tft attributes management
    const unsigned int numAttributes = 3;
    char const * attributes[numAttributes] = { "destAddr" , "tftId" , "destName" };
    enum attributes                          { DEST_ADDR  , TFT_ID  , DEST_NAME } ;

    // attribute iterator
    unsigned int attrId = 0;

    char const * temp[numAttributes];

    // foreach tft element in the list, read the parameters and fill the tft table
    for (cXMLElementList::iterator tftIt = tftList.begin(); tftIt != tftList.end(); tftIt++)
    {
        std::string elementName = (*tftIt)->getTagName();

        if ((elementName == "filter"))
        {
            // clean attributes
            for( attrId = 0 ; attrId<numAttributes ; ++attrId)
                temp[attrId] = NULL;

            for( attrId = 0 ; attrId<numAttributes ; ++attrId)
            {
                temp[attrId] = (*tftIt)->getAttribute(attributes[attrId]);
                if(temp[attrId]==NULL && attributes[attrId])
                {
                    EV << "TrafficFlowFilter::loadFilterTable - attribute " << attributes[attrId] << " unspecified."<< endl;
                }
            }

            // check and save tftID
            if(temp[TFT_ID]==NULL)
            {
                error("TrafficFlowFilter::loadFilterTable - attribute tftId MUST be specified for every traffic filter.");
            }
            else
                tftId = atoi(temp[TFT_ID]);

            // at least one between destAddr and destName MUST be specified
            // try to read the destination address for first
            if(temp[DEST_ADDR]!=NULL)
                destAddr.set(temp[DEST_ADDR]);
            else // if no dest address has been specified, try to resolve node name
            {
                if(temp[DEST_NAME]!=NULL)
                {
                    EV << "TrafficFlowFilter::loadFilterTable - resolving IP address for host name " << temp[DEST_NAME] << endl;
                    destAddr = IPvXAddressResolver().resolve(temp[DEST_NAME]);
                }
                else
                    error("TrafficFlowFilter::loadFilterTable - unable to resolve any address for tftID[%i]",tftId);
            }

            std::pair<TrafficFilterTemplateTable::iterator,bool> ret;
            ret = filterTable_.insert(std::pair<IPvXAddress,unsigned int>(destAddr,tftId));
            if (ret.second==false)
              EV << "TrafficFlowFilter::loadFilterTable - skipping duplicate entry  with destAddr "<<  ret.first->first << '\n';
            else
                EV << "TrafficFlowFilter::loadFilterTable - inserted entry: destAddr[" << destAddr <<"] - TFT[" << tftId <<"]" << endl;
        }
    }
}
