cplusplus {{
#include <string>
#include <queue>
#include <vector>

typedef std::string strg;
typedef std::queue<int> que;
typedef std::vector<int> vec;  
}}

class noncobject strg;
class noncobject que;
class noncobject vec;

packet SpacewireMsg {
    int taskId = -1;
    int sender = -1;
    int destination = -1;
//    int taskDestination = -1;
	vec taskDestination;
    que path;
    double sentTime;
    double receptionTime;
}
packet FlitMsg extends SpacewireMsg{
    bool header = false;
    bool eop = false;	
    bool eep = false;
	int originalSize;
	int srcGate;
	int fltId = -1;
}
packet ChannelRequest extends SpacewireMsg{
    int srcGate;
}
packet ChannelConfirmation extends SpacewireMsg{}
packet ChannelStop extends SpacewireMsg{}

packet PortRequest{
    int taskId;
	int srcGate;
	int outGate;
	bool cReqSent = false;
	bool header = false;
}
packet WatchdogTimer{
    int sender = -1;
	int taskId;
	int srcGate;
}

packet PacketMsg {
	int originalSize = -1;
	int taskId = -1;
//	int taskDestination = -1;
	vec taskDestination;
}