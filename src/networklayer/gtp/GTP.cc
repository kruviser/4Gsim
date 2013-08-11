//
// Copyright (C) 2012 Calin Cerchez
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

#include "GTP.h"
#include "TunnelEndpointTableAccess.h"
#include "GTPPathTableAccess.h"
#include "LTEUtils.h"

Define_Module(GTP);

GTP::GTP() {
	// TODO Auto-generated constructor stub

}

GTP::~GTP() {
	// TODO Auto-generated destructor stub
	if (plmnId != NULL)
		delete plmnId;
}

void GTP::initialize(int stage) {
	teT = TunnelEndpointTableAccess().get();
	pT = GTPPathTableAccess().get();
	nb = NotificationBoardAccess().get();

	const char *filename = par("configFile");
    if (filename == NULL || (!strcmp(filename, "")))
        error("GTP: Error reading configuration from file %s", filename);

    cXMLElement* config = ev.getXMLDocument(filename);
    if (config == NULL)
        error("GTP: Cannot read configuration from file: %s", filename);

    cXMLElement* gtpNode = config->getElementByPath("GTP");
	if (gtpNode == NULL)
		error("GTP: No configuration for GTP module");

	const char *mcc = gtpNode->getAttribute("mcc");
	if (mcc == NULL)
		error("GTP: Module has no mcc attribute");

	const char *mnc = gtpNode->getAttribute("mnc");
	if (mnc == NULL)
		error("GTP: Module has no mnc attribute");
	this->plmnId = LTEUtils().toPLMNId(mcc, mnc);

    socket.setOutputGate(gate("udpOut"));
	if (plane == GTP_CONTROL)
		socket.bind(GTP_CONTROL_PORT);
	else if (plane == GTP_USER)
		socket.bind(GTP_USER_PORT);

	loadPathsFromXML(*gtpNode);

	if (plane == GTP_USER)
		loadTunnelsFromXML(*gtpNode);
}

void GTP::handleMessage(cMessage *msg) {
	GTPPath *path;
	EV << "GTP: Received ";
	if (msg->isSelfMessage()) {
		EV << "self message.\n";
		path = (GTPPath*)msg->getContextPointer();
		path->processEchoTimer();
	} else if (msg->arrivedOn("udpIn")) {
		EV << "network message.";
		path = pT->findPath(msg);
		if (path != NULL) {
		    EV << " Passing the message to the correct path.\n";
		    path->processMessage(msg);
		} else {
		    EV << " Unknown path, dropping the message.\n";
			delete msg;
		}
	}
}

void GTP::loadPathsFromXML(const cXMLElement& gtpNode) {
	cXMLElement* pathsNode = gtpNode.getElementByPath("Paths");
	if (pathsNode != NULL) {
	    cXMLElementList pathsList = pathsNode->getChildren();
	    for (cXMLElementList::iterator pathsIt = pathsList.begin(); pathsIt != pathsList.end(); pathsIt++) {

	    	std::string elementName = (*pathsIt)->getTagName();
	        if ((elementName == "Path")) {
	        	const char *localAddr = (*pathsIt)->getAttribute("localAddr");
	        	if (!localAddr)
	        		error("GTP: Path has no <localAddr> attribute");
	        	const char *remoteAddr = (*pathsIt)->getAttribute("remoteAddr");
	        	if (!remoteAddr)
	        		error("GTP: Path has no <remoteAddr> attribute");
	        	if (!(*pathsIt)->getAttribute("type"))
	        		error("GTP: Path has no <type> attribute");
	        	unsigned char type = atoi((*pathsIt)->getAttribute("type"));
	        	const char *ctrlAddr = (*pathsIt)->getAttribute("ctrlAddr");

	        	if (plane == GTP_USER) {
	        		if (type == S5_S8_SGW_GTP_U || type == S5_S8_PGW_GTP_U || type == S1_U_eNodeB_GTP_U || type == S1_U_SGW_GTP_U) {
	        			GTPPath *path = new GTPPath(this, IPvXAddress(localAddr), IPvXAddress(remoteAddr), type);
	        			if (ctrlAddr != NULL)
	        				path->setCtrlAddr(IPvXAddress(ctrlAddr));
	        			pT->push_back(path);
	        		}
	        	} else {
	        		if (type == S5_S8_SGW_GTP_C || type == S5_S8_PGW_GTP_C || type == S11_MME_GTP_C || type == S11_S4_SGW_GTP_C) {
	        			GTPPath *path = new GTPPath(this, IPvXAddress(localAddr), IPvXAddress(remoteAddr), type);
	        			pT->push_back(path);
	        		}
				}
	        }
	    }
	}
}

void GTP::loadTunnelsFromXML(const cXMLElement& gtpNode) {
	cXMLElement* tunnelsNode = gtpNode.getElementByPath("Tunnels");
	if (tunnelsNode != NULL) {
	    cXMLElementList tunnelsList = tunnelsNode->getChildren();
	    for (cXMLElementList::iterator tunnelsIt = tunnelsList.begin(); tunnelsIt != tunnelsList.end(); tunnelsIt++) {

	    	std::string elementName = (*tunnelsIt)->getTagName();
	        if ((elementName == "TunnelEntry")) {

				const char *subAddr = (*tunnelsIt)->getAttribute("subAddr");
				if (!subAddr)
					error("GTP: Tunnel has no subAddr attribute");

	        	if (!(*tunnelsIt)->getAttribute("remoteId"))
	        		error("GTP: Tunnel has no <remoteId> attribute");
	        	unsigned remoteId = atoi((*tunnelsIt)->getAttribute("remoteId"));

	        	if (!(*tunnelsIt)->getAttribute("path"))
	        		error("GTP: Tunnel has no <path> attribute");
	        	unsigned path = atoi((*tunnelsIt)->getAttribute("path"));

	        	GTPPath *gtpPath = pT->at(path);
	        	if (gtpPath != NULL) {
	        		TunnelEndpoint *te = new TunnelEndpoint();
	        		te->setRemoteId(remoteId);
	        		te->setPath(gtpPath);
	        		teT->addEntryTunnelEndpoint(IPvXAddress(subAddr), te);
	        	} else {
	        		error("GTP: Path id does not match any one from the table");
	        	}
	        }

	        if ((elementName == "TunnelTransit")) {

	        	if (!(*tunnelsIt)->getAttribute("localId"))
	        		error("GTP: Tunnel has no <localId> attribute");
	        	unsigned localId = atoi((*tunnelsIt)->getAttribute("localId"));

	        	if (!(*tunnelsIt)->getAttribute("remoteId"))
	        		error("GTP: Tunnel has no <remoteId> attribute");
	        	unsigned remoteId = atoi((*tunnelsIt)->getAttribute("remoteId"));

	        	if (!(*tunnelsIt)->getAttribute("path"))
	        		error("GTP: Tunnel has no <path> attribute");
	        	unsigned path = atoi((*tunnelsIt)->getAttribute("path"));

	        	GTPPath *gtpPath = pT->at(path);
	        	if (gtpPath != NULL) {
	        		TunnelEndpoint *te = new TunnelEndpoint();
	        		te->setRemoteId(remoteId);
	        		te->setPath(gtpPath);
	        		teT->addTransitTunnelEndpoint(localId, te);
	        	} else {
	        		error("GTP: Path id does not match any one from the table");
	        	}
	        }
	    }
	}
}

