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

#include "GTPUser.h"
#include "LTEUtils.h"
#include "TunnelEndpointTableAccess.h"
#include "GTPPathTableAccess.h"
#include "BearerContext.h"
#include "IPv4Datagram.h"
#include "GTPUtils.h"

Define_Module(GTPUser);

GTPUser::GTPUser() {
	// TODO Auto-generated constructor stub
	localCounter = 1;
	tunnIds = 5 + uniform(5, 10);
	plane = GTP_USER;
}

GTPUser::~GTPUser() {
	// TODO Auto-generated destructor stub
}

void GTPUser::initialize(int stage) {
	GTP::initialize(stage);

	nb->subscribe(this, NF_SUB_NEEDS_TUNN);
}

void GTPUser::handleMessage(cMessage *msg) {
	GTP::handleMessage(msg);

	// message that has to be encapsulated in a GTP user plane message
	if (msg->arrivedOn("ipIn")) {
		IPv4Datagram *datagram = check_and_cast<IPv4Datagram*>(msg);

		TunnelEndpoint *te = teT->findEntryTunnelEndpoint(datagram);
		if (te) {
			EV << "GTPUser: Subscriber found. Encapsulating and sending the GTP user message.\n";
			GTPPath *path = te->getPath();

			GTPv1Header *header = GTPUtils().createHeader(GPDU, 1, 0, 0, 0, te->getRemoteId(), 0, 0, 0, std::vector<GTPv1Extension>());
			GTPMessage *gtpMsg = new GTPMessage(msg->getName());
			gtpMsg->setHeader(header);
			gtpMsg->encapsulate(PK(msg));

			path->send(gtpMsg);
		} else {
			EV << "GTPUser: Unknown subscriber IP address. Dropping the message.\n";
			delete msg;
		}
	}
}

void GTPUser::processGTPMessage(GTPMessage *msg) {
	// encapsulated GTP user message

	GTPv1Header *header = check_and_cast<GTPv1Header*>(msg->getHeader());
	unsigned teid = header->getTeid();

	delete msg->removeControlInfo();

	TunnelEndpoint *te = teT->findTransitTunnelEndpoint(teid);
	if (te) {	// transit tunnel in SGW, have to change the tunnel id and forward it to eNB
		EV << "GTPUser: Transit tunnel found for teid = " << te->getRemoteId() << ". Forwarding the GTP user message.\n";
		GTPPath *path = te->getPath();
		header->setTeid(te->getRemoteId());
		path->send(msg);
	} else {
		// should be tunnel exit
		IPv4Datagram *datagram = check_and_cast<IPv4Datagram*>(msg->decapsulate());

		delete msg;

		TunnelEndpoint *te = teT->findEntryTunnelEndpoint(datagram);
		if (te) {
			EV << "GTPUser: Subscriber found. Decapsulating and sending the IP packet.\n";
			this->send(datagram, gate("ipOut"));
		} else {
			EV << "GTPUser: Unknown subscriber IP address. Dropping the message.\n";
			delete datagram;
		}
	}
}

void GTPUser::receiveChangeNotification(int category, const cPolymorphic *details) {

	Enter_Method_Silent();
//	if (category == NF_SUB_NEEDS_TUNN) {
//		EV << "GTPUser: Received NF_SUB_NEEDS_TUNN notification. Processing notification.\n";
//		BearerContext *bearer = check_and_cast<BearerContext*>(details);
//		TunnelEndpoint *userTe = bearer->getENBTunnEnd();
//		GTPPath *path = pT->findPath(userTe->getRemoteAddr(), S1_U_eNodeB_GTP_U);
//		if (path == NULL) {
//			nb->fireChangeNotification(NF_SUB_TUNN_NACK, bearer);
//			return;
//		}
//		TunnelEndpoint *newTe = new TunnelEndpoint(path);
//		newTe->setRemoteId(userTe->getRemoteId());
//		teT->push_back(newTe);
//		bearer->setENBTunnEnd(newTe);
//		delete userTe;
//		nb->fireChangeNotification(NF_SUB_TUNN_ACK, bearer);
//	}
}

