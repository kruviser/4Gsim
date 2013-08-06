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

    // reading and setting owner type
    ownerType_ = selectOwnerType(par("ownerType"));

    //============= Reading XML files =============
    const char *filename = par("filterFileName");
    if (filename == NULL || (!strcmp(filename, "")))
        error("TrafficFlowFilter::initialize - Error reading configuration from file %s", filename);
    loadFilterTable(filename);
    //=============================================
}

EpcNodeType TrafficFlowFilter::selectOwnerType(const char * type)
{
    EV << "TrafficFlowFilter::selectOwnerType - setting owner type to " << type << endl;
    if(strcmp(type,"ENB") == 0)
        return ENB;
    else if(strcmp(type,"PGW") == 0)
        return PGW;

    error("TrafficFlowFilter::selectOwnerType - unknown owner type [%s]. Aborting...",type);
}

void TrafficFlowFilter::handleMessage(cMessage *msg)
{
    EV << "TrafficFlowFilter::handleMessage - Received Packet:" << endl;
    EV << "name: "<< msg->getFullName() << endl;

    // receive and read IP datagram
    IPv4Datagram * datagram = check_and_cast<IPv4Datagram *>(msg);
    IPv4Address &destAddr = datagram->getDestAddress();
    IPv4Address &srcAddr = datagram->getSrcAddress();
    IPv4Address primaryKey , secondaryKeyAddr;

    EV << "TrafficFlowFilter::handleMessage - Received datagram : " << datagram->getName() << " - src[" << destAddr << "] - dest[" << srcAddr << "]\n";

    // run packet filter and associate a flowId to the connection (default bearer?)
    // search within tftTable the proper entry for this destination

    // check the ownerType and create the primary and secondary key consequently
    if(ownerType_==PGW)
    {
        primaryKey = destAddr;
        secondaryKeyAddr = srcAddr;
    }
    else//(ownerType_==ENB)
    {
        primaryKey = srcAddr;
        secondaryKeyAddr = destAddr;
    }

    // TODO decide whether specifying src and dest ports or not
    TrafficFlowTemplate secondaryKey( secondaryKeyAddr , UNSPECIFIED_PORT , UNSPECIFIED_PORT );

    // search for the tftId in the table
    unsigned int tftId = findTrafficFlow( primaryKey , secondaryKey );
    if(tftId == UNSPECIFIED_TFT)
        error("TrafficFlowFilter::handleMessage - Cannot find corresponding tftId. Aborting...");

    // add control info to the normal ip datagram. This info will be read by the GTP-U application
    TftControlInfo * tftInfo = new TftControlInfo();
    tftInfo->setTft(tftId);
    datagram->setControlInfo(tftInfo);

    EV << "TrafficFlowFilter::handleMessage - setting tft=" << tftId << endl;

    // send the datagram to the GTP-U module
    send(datagram,"gtpUserGateOut");
}



TrafficFlowTemplateId TrafficFlowFilter::findTrafficFlow(IPvXAddress firstKey , TrafficFlowTemplate secondKey)
{
    TrafficFilterTemplateTable::iterator tftIt;
    tftIt = filterTable_.find(firstKey);

    // if no entry found
    if( tftIt==filterTable_.end() )
    {
       EV << "TrafficFlowFilter::findTrafficFlow - Cannot find tft list for destAddress " << firstKey << "." << endl;
       return UNSPECIFIED_TFT;
    }

    // obtain the filter list associated to the given address
    TrafficFilterTemplateList & filterList = tftIt->second;

    // search for the second key within the list
    TrafficFilterTemplateList::iterator templIt = filterList.begin() ,
                                        templEt = filterList.end()   ;

    for( ; templIt != templEt ; templIt++ )
    {
        if( (*templIt)==secondKey )
            return templIt->tftId;
    }

    EV << "TrafficFlowFilter::findTrafficFlow - Cannot find entry for destAddress " << firstKey << " and values: ["
       <<  secondKey.addr << "," <<  secondKey.destPort << ","<< secondKey.srcPort << "]" << endl;

    return UNSPECIFIED_TFT;
}


bool TrafficFlowFilter::addTrafficFlow( IPvXAddress firstKey , TrafficFlowTemplate tft )
{
    EV << "TrafficFlowFilter::addTrafficFlow - checking existence of an entry for destAddress " << firstKey << " and values: ["
       <<  tft.addr << "," <<  tft.destPort << ","<< tft.srcPort << "]" << endl;

    // check if an entry for the given keys already exists
    TrafficFlowTemplateId ret= findTrafficFlow(firstKey,tft);
    if( ret ==! UNSPECIFIED_TFT )
    {
        EV << "TrafficFlowFilter::addTrafficFlow - skipping duplicate entry  with destAddress " << firstKey << " and values: ["
           <<  tft.addr << "," <<  tft.destPort << ","<< tft.srcPort << "]" << endl;
        return false;
    }

    filterTable_[firstKey].push_back(tft);

    EV << "TrafficFlowFilter::addTrafficFlow - inserted entry: destAddr[" << firstKey <<"] - TFT[" << tft.tftId <<"]" << endl;
    return true;
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

    // create default entries
    IPvXAddress destAddr("0.0.0.0"), srcAddr("0.0.0.0");
    unsigned int destPort = UNSPECIFIED_PORT;
    unsigned int srcPort = UNSPECIFIED_PORT;

    unsigned int tftId;
    IPvXAddress primaryKey, secondaryKeyAddr;

    // tft attributes management
    const unsigned int numAttributes = 7;
    char const * attributes[numAttributes] = { "destAddr" , "tftId" , "destName" , "srcAddr" , "srcName" , "srcPort" , "destPort" };
    enum attributes                          { DEST_ADDR  , TFT_ID  , DEST_NAME  , SRC_ADDR  , SRC_NAME  , SRC_PORT  , DEST_PORT  };

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

            // read each attribute into the temp vector
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

            // read src and dest port values
            if(temp[DEST_PORT]!=NULL)
                destPort = atoi(temp[DEST_PORT]);
            if(temp[SRC_PORT]!=NULL)
                srcPort = atoi(temp[SRC_PORT]);

            //===================== Source and Destination addresses management =====================
            // NOTE that the behavior of this part depends on the node type of trafficFlowFilter owner:
            // - in case of ENB, the filterTable will be indexed by srcAddr at the first level. Thus srcAddress or srcName fields are mandatory
            // - in case of PGW, the filterTable will be indexed by destAddr at the first level. Thus destAddress or destName fields are mandatory

            // at least one between destAddr and destName MUST be specified in case of PGW
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
                else if(ownerType_==PGW)
                    error("TrafficFlowFilter::loadFilterTable - unable to resolve any address for tftID[%i] in PGW.",tftId);
            }

            // at least one between srcAddr and srcName MUST be specified in case of ENB
            // try to read the source address for first
            if(temp[SRC_ADDR]!=NULL)
                srcAddr.set(temp[SRC_ADDR]);
            else // if no src address has been specified, try to resolve node name
            {
                if(temp[SRC_NAME]!=NULL)
                {
                    EV << "TrafficFlowFilter::loadFilterTable - resolving IP address for host name " << temp[SRC_NAME] << endl;
                    srcAddr = IPvXAddressResolver().resolve(temp[SRC_NAME]);
                }
                else if(ownerType_==ENB)
                    error("TrafficFlowFilter::loadFilterTable - unable to resolve any address for tftID[%i] in ENB",tftId);
            }

            // check the owner type, and choose the primary and secondary key values accordingly
            if(ownerType_==ENB)
            {
                primaryKey = srcAddr;
                secondaryKeyAddr = destAddr;
            }
            else // (ownerType_==PGW)
            {
                primaryKey = destAddr;
                secondaryKeyAddr = srcAddr;
            }


            // create the new entry...finally
            TrafficFlowTemplate secondaryKey( secondaryKeyAddr , destPort , srcPort );
            secondaryKey.tftId = tftId;

            // TODO decide what to do in case of duplicate entries
            if( !addTrafficFlow(primaryKey,secondaryKey) )
                ;
            else
                ;
        }
    }
}
