#include "NetworkBuilder.h"
using json = nlohmann::json;

Define_Module(NetworkBuilder);

void NetworkBuilder::initialize()
{
#if 0
    cMessage *timer = new cMessage("NetworkBuilder build");
    timer->setSchedulingPriority(-32768);
    scheduleAt(SIMTIME_ZERO, timer);
#else
    readAndBuild(this->getParentModule()->par("networkName").stringValue());
#endif
}

void NetworkBuilder::readAndBuild(const char *networkName)
{
    EV << "Initializing..." << std::endl;
    EV << "Reading input JSON..." << std::endl;

    int currentConfig = getParentModule()->par("configNum").intValue();
    EV << "Initial configuration: " << currentConfig << std::endl;

    bool systemModuleInitialized = getSystemModule()->initialized();

    typedef std::map<int, cModule *> NodeId2ModuleMap;
    NodeId2ModuleMap compNodeId2Mod;
    NodeId2ModuleMap routerNodeId2Mod;

    typedef std::pair<cChannel *, cChannel *> ChannelPair;
    typedef std::map<int, ChannelPair> LinkId2ChannelPair;
    typedef std::map<int, std::map<int, ChannelPair>> TaskId2ChannelPair;
    LinkId2ChannelPair linkId2ChannelPair;
    TaskId2ChannelPair taskId2ChannelPair;

    typedef std::map<int, std::string> NodeId2StringMap;
    typedef std::map<std::string, int> NodeName2IdMap;
    NodeId2StringMap compNodeId2NameStr;
    NodeId2StringMap compNodeId2DispStr;
    NodeId2StringMap routerNodeId2DispStr;
    NodeName2IdMap compNodeName2IdMap;

    typedef std::map<int, std::map<int, cModule *>> NodeId2TaskMap;
    typedef std::map<int, std::map<std::string, cModule *>> NodeName2TaskMap;
    typedef std::map<std::string, int> TaskName2IdMap;
    NodeId2TaskMap nodeId2TaskMap;
    NodeName2TaskMap nodeName2TaskMap;
    TaskName2IdMap taskName2IdMap;

    json j0 = loadJSON(networkName, 0,true);
    int numNodes = (j0["nodes"]).size();

    // Initiate Computational and Router Nodes
    for(int nodeId = 0; nodeId < numNodes; nodeId++){
        // read info from nominal json file
        std::string name = (j0["nodes"][nodeId])["name"];
        double utilLimit = (j0["nodes"][nodeId])["loadLimit"];

        // create router module
        cModuleType *modtype2 = cModuleType::get("RouterNode");
        cModule *mod2 = modtype2->create("RouterNode", this);

        RouterNode* rMod = check_and_cast<RouterNode *>(mod2);
        rMod->setName((name + std::string("Router")).c_str());
        rMod->setId(nodeId);

        // create comp module
        cModuleType *modtype1 = cModuleType::get("CompNode");
        cModule *mod1 = modtype1->create("CompNode", this);

        CompNode* cMod = check_and_cast<CompNode *>(mod1);
        cMod->setName(name.c_str());
        cMod->setUtilLimit(utilLimit);
        cMod->setId(nodeId);
        cMod->setCurrentConfig( this->getParentModule()->par("configNum") );

        // create task nodes
        int numTasks = (j0["tasks"]).size();
        for(int taskId = 0; taskId < numTasks; taskId++){
            cModuleType *taskModType = cModuleType::get("Task");
            Task *taskMod = (Task *) taskModType->create("Task", this);

            // link task to computational node
            cGate *srcIn, *srcOut, *destIn, *destOut;
            cMod->getOrCreateFirstUnconnectedGatePair("taskPort", false, true, srcIn, srcOut);
            taskMod->getOrCreateFirstUnconnectedGatePair("taskPort", false, true, destIn, destOut);

            taskId2ChannelPair[nodeId][taskId] = ChannelPair(
                                createLink(taskId, srcOut, destIn),
                                createLink(taskId, destOut, srcIn));

            // read JSON
            int period = j0["tasks"][taskId]["period"];
            double wcet = (j0["tasks"][taskId])["wcet"][nodeId];
            int messageSize = (j0["tasks"][taskId])["messageSize"];
            std::string taskName = (j0["tasks"][taskId])["name"];

            // set up task node
            taskMod->setId(taskId);
            taskMod->setPeriod(period);
            taskMod->setWCET(wcet);
            taskMod->setMessageSize(messageSize);
            taskMod->setPortGate(destOut);
            taskMod->setName(taskName.c_str());

            // sate task nodes in lists
            nodeId2TaskMap[nodeId][taskId] = taskMod;
            nodeName2TaskMap[nodeId][taskName] = taskMod;
            if(nodeId == 0) {
                taskName2IdMap[taskName] = taskId;
                taskNameList[taskId] = taskName;
            }

            // set task port assignment
            cMod->setTaskPortAssignmentIn(taskId,srcIn);
            cMod->setTaskPortAssignmentOut(taskId,srcOut);
        }

        // set task connectivity and dependencies
        for(int depId = 0; depId < numTasks; depId++){
            Task *mainTask = (Task *) nodeId2TaskMap[nodeId][depId];

            for(int k = 0; k < j0["tasks"][depId]["connectivity"].size(); k++){
                int conId = j0["tasks"][depId]["connectivity"][k];
                mainTask->setConnectivity(conId);

                Task *depTask = (Task *) nodeId2TaskMap[nodeId][conId];
                depTask->setDependency(depId);
            }
        }

        // save comp and router nodes in lists
        compNodeId2Mod[nodeId] = cMod;
        routerNodeId2Mod[nodeId] = rMod;
        compNodeId2DispStr[nodeId] = std::string("cNode") + std::to_string(nodeId);
        routerNodeId2DispStr[nodeId] = std::string("rNode") + std::to_string(nodeId);
        compNodeId2NameStr[nodeId] = name;
        compNodeName2IdMap[name] = nodeId;

        compNodeList.push_back(cMod);
        routerNodeList.push_back(rMod);
        nodeNameList[nodeId] = name;

        // link router to computational node
        cGate *srcIn, *srcOut, *destIn, *destOut;
        cMod->getOrCreateFirstUnconnectedGatePair("port", false, true, srcIn, srcOut);
        rMod->getOrCreateFirstUnconnectedGatePair("port", false, true, destIn, destOut);

        double datarate = 100;

        linkId2ChannelPair[-nodeId-1] = ChannelPair(
                    createLink(datarate, -nodeId-1, srcOut, destIn),
                    createLink(datarate, -nodeId-1, destOut, srcIn));

        // register ports to internal list
        cMod->setPortAssignmentIn(-1, srcIn);
        cMod->setPortAssignmentOut(-1, srcOut);
        rMod->setPortIdIn(-1, destIn);
        rMod->setPortIdOut(-1, destOut);
    }

    // Create Links
    int numLinks = (j0["links"]).size();
    for(int i = 0; i < numLinks; i++){
        // get fields from json
        int linkId = i;
        int srcnodeid = (j0["links"])[i]["from"];
        int destnodeid = (j0["links"])[i]["to"];
        double datarate = (j0["links"])[i]["bandwidth"];

        NodeId2ModuleMap::iterator srcIt = routerNodeId2Mod.find(srcnodeid);
        NodeId2ModuleMap::iterator destIt = routerNodeId2Mod.find(destnodeid);

        // find and assign gates
        ASSERT(srcIt != routerNodeId2Mod.end());
        ASSERT(destIt != routerNodeId2Mod.end());
        RouterNode *srcmod = check_and_cast<RouterNode *>(srcIt->second);
        RouterNode *destmod = check_and_cast<RouterNode *>(destIt->second);

        cGate *srcIn, *srcOut, *destIn, *destOut;
        srcmod->getOrCreateFirstUnconnectedGatePair("port", false, true, srcIn, srcOut);
        destmod->getOrCreateFirstUnconnectedGatePair("port", false, true, destIn, destOut);
        linkId2ChannelPair[linkId] = ChannelPair(
                    createLink(datarate, linkId, srcOut, destIn),
                    createLink(datarate, linkId, destOut, srcIn));

        // register ports on each node
        srcmod->setPortIdIn(destnodeid, srcIn);
        srcmod->setPortIdOut(destnodeid, srcOut);
        destmod->setPortIdIn(srcnodeid, destIn);
        destmod->setPortIdOut(srcnodeid, destOut);
    }

    // load lists from other configurations
    int numConfigs = this->getParentModule()->par("numConfigs");
    for(int config = 0; config < numConfigs; config++){
        json ji = loadJSON(networkName, config, false);

        // set routings
        int numRoutings = ji["routing"].size();
        for(int j = 0; j < numRoutings; j++){
            int src = ji["routing"][j]["from"];
            int dest = ji["routing"][j]["to"];
            int pathLength = ji["routing"][j]["path"].size();

            // read path
            std::vector<int>* path = new std::vector<int>;
            for(int i = 0; i < pathLength; i++){
                path->push_back(ji["routing"][j]["path"][i]);
            }

            // assign it to the appropriate list
            ((CompNode *) compNodeId2Mod[src])->setRouting(config, dest, path);
        }

        // set availability
        std::map<std::string, bool> configAv;
        for(int j = 0; j < ji["nodes"].size(); j++){
            configAv[ji["nodes"][j]["name"]] = true;
        }
        for(int j = 0; j < numNodes; j++){
            for(int k = 0; k < numNodes; k++){
                bool available = false;
                if(configAv.find(j0["nodes"][k]["name"]) != configAv.end()){
                    available = true;
                }
                ((CompNode *) compNodeId2Mod[j])->setAvailability(config, k, available);
            }
        }

        // set task assignments
        for(int taskNo = 0; taskNo < ji["tasks"].size(); taskNo++){
            if(ji["tasks"][taskNo]["appropriateNodes"].size() < 1){
                continue;
            }

            int appropriateNode = ji["tasks"][taskNo]["appropriateNodes"][0];
            std::string nodeName = ji["nodes"][appropriateNode]["name"];
            std::string taskName = ji["tasks"][taskNo]["name"];

            int taskId = taskName2IdMap[taskName];
            int nodeId = compNodeName2IdMap[nodeName];

            for(int compNodeId = 0; compNodeId < numNodes; compNodeId++){
                CompNode *compMod = (CompNode *) compNodeId2Mod[compNodeId];

                if(compNodeId == nodeId) compMod->setMyTaskAssignment(config, taskId);
                compMod->setTaskAssignment(config, taskId, nodeId);
            }
        }
    }

    // initialize channels when need.
    if (!systemModuleInitialized) {
        // We are in the stage 0, the channel->callInitialize(0) already called on all channels generated by NED.
        // We must call the channel->callInitialize(0) for our dynamically generated channels.
        // The cMysqlNetBuilder has init stage 1, it's guarantee the calling of channel->callInitialize(1) for all channels.
        for (LinkId2ChannelPair::iterator it = linkId2ChannelPair.begin();
                it != linkId2ChannelPair.end(); ++it) {
//            std::cout << "Initializing channel no" << it->first << endl;
            if (it->second.first)
                it->second.first->callInitialize(0);
            if (it->second.second)
                it->second.second->callInitialize(0);
        }

        for(TaskId2ChannelPair::iterator it = taskId2ChannelPair.begin();
                it != taskId2ChannelPair.end(); it++){
            for(auto x : it->second){
                if (x.second.first)
                    x.second.first->callInitialize(0);
                if (x.second.second)
                    x.second.second->callInitialize(0);
            }
        }
    }

    // final touches: build inside, initialize()
    // initialize router nodes
    std::map<int, cModule *>::iterator it;
    for (it = routerNodeId2Mod.begin(); it != routerNodeId2Mod.end(); ++it) {
        cModule *mod = it->second;
        long nodeId = it->first;
        mod->finalizeParameters();
        mod->setDisplayString(routerNodeId2DispStr[nodeId].c_str());
        mod->getDisplayString().setTagArg("i", 0, "device/accesspoint");
        mod->buildInside();

        RouterNode *node = check_and_cast<RouterNode *>(mod);
        node->setNodeNames(compNodeId2NameStr);
    }
    for (it = routerNodeId2Mod.begin(); it != routerNodeId2Mod.end(); ++it) {
        cModule *mod = it->second;
        if (systemModuleInitialized)
            mod->callInitialize();
        else {
            // we are in system initialize stage 0
            #if 0
                // TODO call callInitialize(0) when need
                if (mod->getParentModule()->stage0Initialized()) {  // TODO mod->stage0Initialized() not existing, need implementing it
                    // parent module already called callInitialize(0) on its other static submodules, we should call it on dynamic submodules
                    if (mod->callInitialize(0)) {
                        // mod has more stages, reset FL_INITIALIZED on parents of mod
                        for ( ; ; ) {
                            mod = mod->getParentModule();
                            if (!mod)
                                break;
                            mod->setFlag(FL_INITIALIZED, false);  // TODO setFlag() is protected, FL_INITIALIZED is private
                        }
                    }
                }
            #endif
        }
    }

    // initialize computational nodes
    for (it = compNodeId2Mod.begin(); it != compNodeId2Mod.end(); ++it) {
        cModule *mod = it->second;
        long nodeId = it->first;
        mod->finalizeParameters();
        mod->setDisplayString(compNodeId2DispStr[nodeId].c_str());
        mod->getDisplayString().setTagArg("i", 0, "device/cpu");
        mod->buildInside();

        CompNode *node = check_and_cast<CompNode *>(mod);
        node->setNodeNames(compNodeId2NameStr);
    }
    for (it = compNodeId2Mod.begin(); it != compNodeId2Mod.end(); ++it) {
        cModule *mod = it->second;
        if (systemModuleInitialized)
            mod->callInitialize();
        else {
            // we are in system initialize stage 0
            #if 0
                // TODO call callInitialize(0) when need
                if (mod->getParentModule()->stage0Initialized()) {  // TODO mod->stage0Initialized() not existing, need implementing it
                    // parent module already called callInitialize(0) on its other static submodules, we should call it on dynamic submodules
                    if (mod->callInitialize(0)) {
                        // mod has more stages, reset FL_INITIALIZED on parents of mod
                        for ( ; ; ) {
                            mod = mod->getParentModule();
                            if (!mod)
                                break;
                            mod->setFlag(FL_INITIALIZED, false);  // TODO setFlag() is protected, FL_INITIALIZED is private
                        }
                    }
                }
            #endif
        }
    }

    // initialize task nodes
    for (NodeId2TaskMap::iterator itt = nodeId2TaskMap.begin();
            itt != nodeId2TaskMap.end(); ++itt) {

        for(it = itt->second.begin(); it != itt->second.end(); ++it){
            cModule *mod = it->second;

            mod->finalizeParameters();
            mod->setDisplayString("");
            mod->getDisplayString().setTagArg("n", 0, "");
            mod->buildInside();
        }
    }

    for (NodeId2TaskMap::iterator itt = nodeId2TaskMap.begin();
                itt != nodeId2TaskMap.end(); ++itt) {

        for (it = itt->second.begin(); it != itt->second.end(); ++it) {
            cModule *mod = it->second;
            if (systemModuleInitialized)
                mod->callInitialize();
            else {
                // we are in system initialize stage 0
                #if 0
                    // TODO call callInitialize(0) when need
                    if (mod->getParentModule()->stage0Initialized()) {  // TODO mod->stage0Initialized() not existing, need implementing it
                        // parent module already called callInitialize(0) on its other static submodules, we should call it on dynamic submodules
                        if (mod->callInitialize(0)) {
                            // mod has more stages, reset FL_INITIALIZED on parents of mod
                            for ( ; ; ) {
                                mod = mod->getParentModule();
                                if (!mod)
                                    break;
                                mod->setFlag(FL_INITIALIZED, false);  // TODO setFlag() is protected, FL_INITIALIZED is private
                            }
                        }
                    }
                #endif
            }
        }
    }

    double simDuration =  getParentModule()->par("simTimeout").doubleValue();
    scheduleAt(simTime()+simDuration, new SimMessage);
}

cChannel *NetworkBuilder::createLink(double datarate, int linkId, cGate *srcGate, cGate *destGate){
    cChannelType *channelType = cChannelType::getDatarateChannelType();
    cDatarateChannel *channel = channelType ? channelType->createDatarateChannel("channel") : nullptr;
    channel->setDatarate(datarate * 1e6);

    // create connection
    srcGate->connectTo(destGate, channel);
    return channel;
}

cChannel *NetworkBuilder::createLink(int linkId, cGate *srcGate, cGate *destGate){
    cChannelType *channelType = cChannelType::getDatarateChannelType();
    cDelayChannel *channel = channelType ? channelType->createDelayChannel("channel") : nullptr;
    channel->setDelay(0.0);

    // create connection
    srcGate->connectTo(destGate, channel);
    return channel;
}

void NetworkBuilder::handleMessage(cMessage *msg){
    if (msg->isSelfMessage()) {
        if(strcmp( msg->getClassName(), "SimMessage" ) == 0){
            //TERMINATING SIM

            // Save results from all nodes in the network
            // Create directory
            std::string networkName = this->getParentModule()->par("networkName").stringValue();
            std::string confStr = std::to_string(this->getParentModule()->par("configNum").intValue());
            std::string directory_name(this->outputDirStr + networkName + std::string("_") + confStr);

            if(_mkdir(directory_name.c_str())){
                std::cout << "Creating output directory: " << directory_name << endl;
            }

            // Compile and save results

            // -Delay results
            std::string delayResultsPath = directory_name + std::string("/delays.csv");
            std::ofstream delayFile(delayResultsPath);

            // --Print Headers
            delayFile << "SenderNodeId" << "," << "SenderName" << ","
                        << "DestinatioNodeId"  << "," << "DestinationName" << ","
                        << "PathLength" << "," << "Configuration"
                        << "," << "MessageSize" << "," << "SentTime"
                        << "," << "ReceptionTime" << ","
                        << "TransmissionDuration" << "," << "TaskId" << "," << "TaskName" << endl;

            // --Print Results
            for(cModule *mod : compNodeList){
                CompNode *cMod = check_and_cast<CompNode *>(mod);
                std::vector<DelayResult> results = cMod->getDelayResults();

                for(DelayResult result : results){
                    int sender = result.getSenderId();
                    int dest = result.getDestinationId();
                    int pathLength = result.getPathLength();
                    int config = result.getConfig();
                    int messageSize = result.getMessageSize();
                    double sentTime = result.getSentTime();
                    double receptionTime = result.getReceptionTime();
                    int taskId = result.getTaskId();

                    delayFile << sender << "," << nodeNameList[sender] << ","
                            << dest << "," << nodeNameList[dest] << ","
                            << pathLength << "," << config << "," << messageSize
                            << "," << sentTime << "," << receptionTime
                            << "," << receptionTime-sentTime << ","
                            << taskId << "," << this->taskNameList[taskId] << endl;
                }
            }

            delayFile.close();

            // -Task Triggering Results
            std::string taskResultsPath = directory_name + std::string("/task_trigger.csv");
            std::ofstream taskFile(taskResultsPath);

            // --Print Headers
            taskFile << "NodeId" << "," <<  "NodeName"  << "," << "TaskId" << ","
                    << "TaskName" << "," << "StartTime" << "," << "EndTime" << endl;

            // --Print Results
            for(cModule *mod : compNodeList){
                CompNode *cMod = check_and_cast<CompNode *>(mod);
                std::vector<TaskResult> results = cMod->getTaskResults();

                for(TaskResult result : results){
                    std::string nodeName = result.getNodeName();
                    int nodeId = result.getNodeId();
                    int taskId = result.getTaskId();
                    std::string taskName = taskNameList[taskId];
                    double startTime = result.getStartTime();
                    double endTime = result.getEndTime();

                    taskFile << nodeId << "," <<  nodeName  << "," << taskId << ","
                            << taskName << "," << startTime << "," << endTime << endl;
                }
            }

            taskFile.close();

            // -Port Assignment Results
            std::string portResultsPath = directory_name + std::string("/port_assignment.csv");
            std::ofstream portFile(portResultsPath);

            // --Print Headers
            portFile << "SenderNodeId" << "," << "SenderNodeName" << ","
                    << "DestinatioNodeId"  << "," << "DestinationNodeName" << ","
                    << "StarTime" << "," << "EndTime" << "," << "Duration" << endl;

            // --Print Results
            for(cModule *mod : routerNodeList){
                RouterNode *rMod = check_and_cast<RouterNode *>(mod);
                std::map<int, std::stack<PortAssignmentResult>> results = rMod->getPortAssignmentResults();

                for(std::map<int, std::stack<PortAssignmentResult>>::iterator it = results.begin();
                        it != results.end(); it++){
                    while(!it->second.empty()){
                        PortAssignmentResult result = it->second.top();
                        it->second.pop();
                        simtime_t startTime = result.getStartTime();
                        simtime_t endTime = result.getEndTime();
                        double duration = result.getDuration();
                        int senderNode = result.getSenderNode();
                        int destinationNode = result.getDestinationNode();

                        portFile << senderNode << "," << nodeNameList[senderNode] << ","
                                << destinationNode  << "," << nodeNameList[destinationNode] << ","
                                << startTime << "," << endTime << "," << duration << endl;
                    }
                }
            }

            portFile.close();

            // -Message Transmission Results
            std::string txResultsPath = directory_name + std::string("/transmission.csv");
            std::ofstream txFile(txResultsPath);

            // --Print Headers
            txFile << "SenderNodeId" << "," << "SenderNodeName" << ","
                    << "DestinatioNodeId"  << "," << "DestinationNodeName" << ","
                    << "StarTime" << "," << "EndTime" << "," << "Duration" << ","
                    << "TaskId" << "," << "TaskName" << "," << "BytesSent" << endl;

            // --Print Results
            for(cModule *mod : routerNodeList){
                RouterNode *rMod = check_and_cast<RouterNode *>(mod);
                std::map<int, std::stack<TransmissionResult>> results = rMod->getTransmissionResults();

                for(std::map<int, std::stack<TransmissionResult>>::iterator it = results.begin();
                        it != results.end(); it++){
                    while(!it->second.empty()){
                        TransmissionResult result = it->second.top();
                        it->second.pop();
                        simtime_t startTime = result.getStartTime();
                        simtime_t endTime = result.getEndTime();
                        double duration = result.getDuration();
                        int senderNode = result.getSenderNode();
                        int destinationNode = result.getDestinationNode();
                        int taskId = result.getTaskId();
                        int bytes = result.getBytesSent();

                        txFile << senderNode << "," << nodeNameList[senderNode] << ","
                                << destinationNode  << "," << nodeNameList[destinationNode] << ","
                                << startTime << "," << endTime << "," << duration << ","
                                << taskId << "," << taskNameList[taskId] << "," << bytes << endl;
                    }
                }
            }

            txFile.close();

            // End sim
            this->endSimulation();
        }
        else{
            delete msg;
            readAndBuild(this->getParentModule()->par("networkName").stringValue());
        }
    }
    else{
        error("I do not process external messages");
    }
}

json NetworkBuilder::loadJSON(const char *networkName, int config, bool print){
    try{
        std::string dirNameStr = networkName;
        std::string configNumStr = std::to_string(config);

        std::string fPath = inputDirStr + dirNameStr + configStr + configNumStr + jsonStr;

        if(print) {
            EV << "Reading file path: " << fPath << std::endl;
            std::cout << "Reading file path: " << fPath << std::endl;
        }

        std::ifstream jf;
        jf.open(fPath);
        if(!jf.is_open()){
            throw std::invalid_argument("Could not open file");
        }

        if(print) {EV << "File opened successfully!" << std::endl;}

        json j;
        jf >> j;
        return j;
    }
    catch(const std::invalid_argument& e){
        EV << "Could not open file... Terminating sim" << std::endl;
        EV << "Terminating, goodbye!" << std::endl;
        std::cout << e.what();
    }

    return NULL;
}
