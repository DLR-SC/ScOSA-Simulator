cplusplus {{
  	#include <vector>
  	#include <string>
    #include <queue>
    
    typedef std::string strg;
    typedef std::queue<int> que;
    typedef std::vector<int> vec;  
}}

class noncobject strg;
class noncobject que;
class noncobject vec;

packet TaskMsg {
    int taskId;
}

packet TaskActivation extends TaskMsg{bool status;}
packet TaskReady extends TaskMsg{}
packet TaskTrigger extends TaskMsg{}
packet TaskDone extends TaskMsg{
	vec destIds;
	int messageSize;
	double startTime;
	double endTime;
}
