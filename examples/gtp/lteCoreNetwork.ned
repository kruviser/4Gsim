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

package inet.examples.gtp;

import inet.nodes._4gsim.ENB;
import inet.nodes._4gsim.PGW;
import inet.nodes._4gsim.SGW;
import inet.nodes.inet.Router;
import inet.util.ThruputMeteringChannel;
import inet.world.radio.ChannelControl;
import inet.nodes.inet.StandardHost;
import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;

network lteCoreNetwork
{
    @display("bgb=900,371");
    types:
        channel networkConnection extends ThruputMeteringChannel
        {
            delay = 0.1us;
            datarate = 10Mbps;
            thruputDisplayFormat = "u";
        }
    submodules:
        iPv4NetworkConfigurator: IPv4NetworkConfigurator {
            @display("p=95,42");
            config = xmldoc("lteCoreConfigFiles/lteIpAddresses.xml");
        }
        pgwStandard: PGW {
            @display("p=638,135");
        }
        sgwStandard2: SGW {
            @display("p=488,257");
        }
        router: Router {
            @display("p=488,135");
        }
        InternetHost: StandardHost {
            @display("p=826,296");
        }
        router1: Router {
            @display("p=808,166");
        }
        InternetHost1: StandardHost {
            @display("p=808,50");
        }
        eNB: ENB {
            @display("p=200,257");
        }
        router2: Router {
            @display("p=352,331");
        }
        channelControl: ChannelControl {
            @display("p=217,42");
        }
        UE1: StandardHost {
            @display("p=58,296;i=device/cellphone");
        }
    connections:
        pgwStandard.pppg++ <--> networkConnection <--> router.pppg++;
        router.pppg++ <--> networkConnection <--> sgwStandard2.pppg++;
        InternetHost.ethg++ <--> networkConnection <--> router1.ethg++;
        router1.ethg++ <--> networkConnection <--> pgwStandard.ethg++;
        InternetHost1.ethg++ <--> networkConnection <--> pgwStandard.ethg++;
        sgwStandard2.pppg++ <--> networkConnection <--> router2.pppg++;
        router2.pppg++ <--> networkConnection <--> eNB.pppg++;
        UE1.ethg++ <--> networkConnection <--> eNB.ethg++;
}
