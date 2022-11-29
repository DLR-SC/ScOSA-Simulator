#include "CompNode.h"

Define_Module(CompNode);

void CompNode::initialize(){
    EV << this->getName() << " is being initialized..." << endl;
    this->timeOut = this->par("timeOut");

    if(myTaskAssignment.empty()){
        // no task assignment given, performing default test
        if(this->id < 2){
            this->generateRandomMessages();
            this->sendPortRequest();
        }
    }
    else{ // task assignment found in JSON files, executing simulation
        // activate every task assigned to this node in the current configuration
        for(int taskId : this->myTaskAssignment[this->currentConfig]){
            TaskActivation* act = new TaskActivation;
            act->setStatus(true);
            act->setTaskId(taskId);

            this->send_task_msg(act);
        }
    }
}

void CompNode::handleMessage(cMessage *msg){
    if(this->checkIfRouterMessage(msg)){
        this->handleRouterMessage(msg);
    }
    else if(this->checkIfTaskMessage(msg)){
        this->handleTaskMessage(msg);
    }
    else{
        dropAndDelete(msg);
    }
}

void CompNode::handleRouterMessage(cMessage* msg){
    if(strcmp( msg->getClassName(), "FlitMsg" ) == 0){
        if( msg->isSelfMessage() ){
            // if from self, attempt transmission
            FlitMsg* flt_sent = (FlitMsg *) msg;
            if(flt_sent->getHeader()) lastSentTime = simTime().dbl();
            flt_sent->setSentTime(lastSentTime);

            bool sent = this->send_msg(flt_sent);

            // check if flit was successfully sent, if the channel is available for transmission,
            // and if there are more flits available for sending
            if(sent && this->canTransmitt && !flitQueue.isEmpty()){
                // send next flit from packet
                FlitMsg* flt = (FlitMsg *) flitQueue.front();

                if(flt->getHeader()){
                    // if next flit is the header file for a new packet, send a channel request
                    EV << this->getName() << " - Finished sending packet. Sending next in the queue...";

                    this->canTransmitt = false;
                    this->sendPortRequest();

                    // create watch-dog timer
                    WatchdogTimer* tmr = new WatchdogTimer;
                    tmr->setSender(flt->getSender());
                    tmr->setTaskId(flt->getTaskId());
                    tmr->setSrcGate(-1);

                    scheduleAt(simTime()+timeOut, tmr);
                }
                else{
                    // else continue sending current packet
                    EV << this->getName() << " - Forwarding next flit from the current packet...";

                    this->flitQueue.pop();
                    scheduleAt(simTime(), flt);
                }
                if(flt->getEop()){
//                    this->canTransmitt = true;
                }
            }
            else if(sent && flitQueue.isEmpty()){
                this->busy = false;

                if (!packetQueue.isEmpty()){
                    // if other packets are awaiting transmission, send them
                    PacketMsg* pkt = (PacketMsg *) packetQueue.front();
                    packetQueue.pop();

                    scheduleAt(simTime(),pkt);
                }
                else if(!readyQueue.isEmpty()){
                    // if not busy and queue empty, send trigger
                    TaskReady* rdy = check_and_cast<TaskReady *>(readyQueue.front());
                    readyQueue.pop();

                    TaskTrigger* tgr = new TaskTrigger;
                    tgr->setTaskId(rdy->getTaskId());

                    this->send_task_msg(tgr);
                    this->busy = true;
                }
            }
        }
        else{
            // if from another node, add to received packets
            FlitMsg* flt = (FlitMsg *) msg;
            int sender = flt->getSender();

            std::string out = std::string("Flit Received (") + std::string(std::to_string(flt->getFltId()+1)) + std::string("/") + std::string(std::to_string(flt->getOriginalSize())) + std::string(") - ID: ") + std::to_string(flt->getTaskId());
            bubble(out.c_str());

            EV << this->getName() << " - Received Flit with Packet ID: " << flt->getTaskId() << " (" << flt->getFltId()+1 << "/" << flt->getOriginalSize() << ") from " << nodeNames[sender] << "!" << endl;

            if( flt->getHeader() ){
                // Header file received, new packet reception is initiated
                EV << this->getName() << " - Header file received!" << endl;

                this->receivingTaskId = flt->getTaskId();
            }
            // received flit from packet currently in transmission
            if(this->receivingTaskId == flt->getTaskId()){
                this->receivingFlitCounter++;

                if(flt->getEop()){
                    // flit is EOP for packet
                    if(this->receivingFlitCounter == flt->getOriginalSize()){
                        EV << this->getName() << " - Successfully received EOP flit from " << nodeNames[sender] << "!" << endl;

                        if(!myTaskAssignment.empty()){
                            PacketMsg* pkt = new PacketMsg;
                            pkt->setTaskId(flt->getTaskId());
                            pkt->setOriginalSize(flt->getOriginalSize());
                            for(int taskDest : flt->getTaskDestination()){
                                pkt->getTaskDestination().push_back(taskDest);
                            }

                            this->send_task_msg(pkt);
                        }

                        flt->setReceptionTime( simTime().dbl() );

                        this->saveDelayResult(flt);

                        this->receivingFlitCounter = 0;
                    }
                }
                else if( flt->getEep() ){
                    // EEP of last packet was received
                    EV << this->getName() << " - Received EEP flit from " << nodeNames[sender] << "!" << endl;

                    std::string rec = "Partial Packet Received (";
                    rec += std::to_string(this->receivingFlitCounter) + std::string("/") + std::string(std::to_string(flt->getOriginalSize())) + std::string(")");
                    bubble(rec.c_str());

                    this->receivingFlitCounter = 0;
                }
            }

            dropAndDelete(flt);
        }
    }
    else if(strcmp( msg->getClassName(), "ChannelRequest" ) == 0){
        // request received, send confirmation
        ChannelRequest* req = (ChannelRequest *) msg;
        int sender = req->getSender();

        // process request
        if(msg->isSelfMessage()){
            // if from self, attempt transmission
            this->send_msg(msg);
        }
        else{
            // create confirmation
            EV << this->getName() << " - Channel request received from Router[" << sender << "]! Sending confirmation..." << endl;

            ChannelConfirmation* conf = new ChannelConfirmation;
            conf->setSender(-1);
            conf->setDestination(sender);

            this->send_msg(conf);

            // delete processed request
            dropAndDelete(req);
        }
    }
    else if(strcmp( msg->getClassName(), "ChannelConfirmation" ) == 0){
        // port path confirmation received
        ChannelConfirmation* conf = check_and_cast<ChannelConfirmation *>(msg);

        std::string out = std::string("Confirmation received. Starting Packet (ID: ") + std::to_string(((FlitMsg *) flitQueue.front())->getTaskId()) + std::string(") transmission...");
        bubble(out.c_str());

        if(msg->isSelfMessage()){
            // if from self, attempt transmission
            this->send_msg(msg);
        }
        else if( !this->flitQueue.isEmpty() ){
            // set indicator to true
            this->canTransmitt = true;

            // send packet
            FlitMsg* flt = (FlitMsg *) flitQueue.front();
            this->flitQueue.pop();
            scheduleAt(simTime(), flt);

            // delete confirmation message
            dropAndDelete(conf);
        }
    }
    else if(strcmp( msg->getClassName(), "ChannelStop" ) == 0){
        // halt command received, stopping transmission
        std::string out = std::string("Stop command received. Halting Packet (ID: ")
            + std::to_string(((FlitMsg *) flitQueue.front())->getTaskId())
            + std::string(") transmission...");
        bubble(out.c_str());

        EV << this->getName() << " - Received a Channel Stop from Port[" << -1 << "]. Halting transmission..." << endl;
        this->canTransmitt = false;

        dropAndDelete(msg);

        sendPortRequest();
    }
    else if(strcmp( msg->getClassName(), "WatchdogTimer" ) == 0){
        // check if top element in the buffer is still waiting to be transmitted
        WatchdogTimer* tmr = check_and_cast<WatchdogTimer *>(msg);

        if(!flitQueue.isEmpty()){
            FlitMsg* flt = check_and_cast<FlitMsg *>(flitQueue.front());

            if(flt->getTaskId() == tmr->getTaskId()){
                std::string out = std::string("Timeout reached for Packet (ID: ")
                    + std::to_string(((FlitMsg *) flitQueue.front())->getTaskId())
                    + std::string("). Dumping packet...");
                bubble(out.c_str());

                EV <<  this->getName() << " -  Timeout for Packet ID: " << flt->getTaskId() << " reached." << endl;

                // send EEP to next node
                EV <<  this->getName() << "Sending EEP..." << endl;
                FlitMsg* eep = new FlitMsg;
                eep->setEep(true);

                this->send_msg(eep);

                // discard all flits of this packet contained in the buffer
                EV <<  this->getName() << "Discarding packet flits remaining in buffer..." << endl;
                bool eopFound = false;
                while(!eopFound && !flitQueue.isEmpty()){
                    FlitMsg* fltTop = check_and_cast<FlitMsg *>(flitQueue.front());
                    flitQueue.pop();

                    eopFound = fltTop->getEop();

                    dropAndDelete(fltTop);
                }

                // send next packet
                EV <<  this->getName() << "Sending next packet in the queue..." << endl;
                this->sendPortRequest();
            }
        }
        dropAndDelete(msg);
    }
    else{
        EV <<  this->getName() << " - Unknown message type. Dropping Packet..." << endl;
        dropAndDelete(msg);
    }
}
void CompNode::handleTaskMessage(cMessage* msg){
    if(strcmp( msg->getClassName(), "PacketMsg" ) == 0){
        this->busy = true;

        // create flits and store in queue
        PacketMsg* pkt = check_and_cast<PacketMsg *>(msg);
        int size = pkt->getOriginalSize();
        int taskId = pkt->getTaskId();
        int sender = this->id;
        std::vector<int> taskDestination = pkt->getTaskDestination();
        int dest = this->taskAssignment[this->currentConfig][taskDestination.at(0)];

        if(isTaskAssigned(taskDestination.at(0))){
            //  if destination task is assigned to self, send packet to task
            for(int taskDest : taskDestination){
                PacketMsg* pkt_new = new PacketMsg;
                pkt_new->setOriginalSize(size);
                pkt_new->setTaskId(taskId);
                pkt_new->getTaskDestination().push_back(taskDest);

                this->send_task_msg(pkt_new);
            }
            this->busy = false;

            if(!packetQueue.isEmpty()){
                PacketMsg* pkt = (PacketMsg *) packetQueue.front();
                packetQueue.pop();

                scheduleAt(simTime(),pkt);
            }
            else if(!readyQueue.isEmpty()){
                // if not busy and ready queue is not empty, send trigger to next ready task
                TaskReady* rdy = check_and_cast<TaskReady *>(readyQueue.front());
                readyQueue.pop();

                TaskTrigger* tgr = new TaskTrigger;
                tgr->setTaskId(rdy->getTaskId());

                this->send_task_msg(tgr);
                this->busy = true;
            }
        }
        else{
            //  else, send packet to destination node
            std::vector<int> *route = routing[this->currentConfig][dest];
            for(int i = 0; i < size; i++){
                FlitMsg* flit = new FlitMsg;
                flit->setSender(sender);
                flit->setDestination(dest);
                flit->setOriginalSize(size);
                flit->setByteLength(1);
                flit->setTaskId(taskId);
                flit->setFltId(i);

                for(int taskDest : pkt->getTaskDestination()){
                    flit->getTaskDestination().push_back(taskDest);
                }

                for(int x : *route){
                    (flit->getPath()).push(x);
                }

                if(i == 0){
                    flit->setHeader(true);
                    EV <<  this->getName() << " - Creating message from Node[" << sender << "] to Node[" << dest << "] through path: ";
                    for(int x : *route){
                        EV << x << " ";
                    }
                    EV << endl;
                }

                if(i == size-1 || size == 1) flit->setEop(true);

                (this->flitQueue).insert(flit);
            }

            // start sending flits
            this->sendPortRequest();

            dropAndDelete(pkt);
        }
    }
    else if(strcmp( msg->getClassName(), "TaskReady" ) == 0){
        TaskReady* rdy = check_and_cast<TaskReady *>(msg);
        if(msg->isSelfMessage()){
            // if self message, attempt retransmission
            this->send_task_msg(rdy);
        }
        else{
            if(!busy && readyQueue.isEmpty()){
                // if not busy and queue empty, send trigger
                TaskTrigger* tgr = new TaskTrigger;
                tgr->setTaskId(rdy->getTaskId());

                this->send_task_msg(tgr);
                this->busy = true;

                dropAndDelete(msg);
            }
            else{
                // else, add to queue
                readyQueue.insert(msg);
            }
        }
    }
    else if(strcmp( msg->getClassName(), "TaskTrigger" ) == 0){
        if(msg->isSelfMessage()){
            // if self message, attempt retransmission
            this->send_task_msg(msg);
        }
        else{
            dropAndDelete(msg);
        }
    }
    else if(strcmp( msg->getClassName(), "TaskDone" ) == 0){
        // create packets from done task
        TaskDone* dn = check_and_cast<TaskDone *>(msg);
        int taskId = dn->getTaskId();

        // count the total number of node destinations;
        std::vector<int> destNodes;
        std::map<int, std::vector<int>> nodes2tasks;
        for(int destTask : dn->getDestIds()){
            int destNode = this->taskAssignment[this->currentConfig][destTask];
            nodes2tasks[destNode].push_back(destTask);
        }

        // for each destination, generate a packet
        for(std::map<int, std::vector<int>>::iterator it = nodes2tasks.begin();
                it != nodes2tasks.end(); it++){
            PacketMsg* pkt = new PacketMsg;
            pkt->setTaskId(taskId);
            pkt->setOriginalSize(dn->getMessageSize());

            for(int destTask : it->second){
                pkt->getTaskDestination().push_back(destTask);
            }

            packetQueue.insert(pkt);
        }

//        for(int dest : dn->getDestIds()){
//            PacketMsg* pkt = new PacketMsg;
//            pkt->setTaskId(taskId);
//            pkt->setOriginalSize(dn->getMessageSize());
//            pkt->setTaskDestination(dest);
//
//            packetQueue.insert(pkt);
//        }

        if(dn->getDestIds().size() < 1){
            // if no packets are generated, do next task in the queue
            this->busy = false;

            if(!readyQueue.isEmpty()){
                // if not busy and queue empty, send trigger to next ready task
                TaskReady* rdy = check_and_cast<TaskReady *>(readyQueue.front());
                readyQueue.pop();

                TaskTrigger* tgr = new TaskTrigger;
                tgr->setTaskId(rdy->getTaskId());

                this->send_task_msg(tgr);
                this->busy = true;
            }
        }
        else{
            // if packets were generated, start transmission
            PacketMsg* pkt = (PacketMsg *) packetQueue.front();
            packetQueue.pop();

            scheduleAt(simTime(),pkt);
        }

        // Save Task Done Result
        this->saveTaskResult(dn);

        dropAndDelete(msg);
    }
    else {
        // unknown message type. Discard message.
        dropAndDelete(msg);
    }
}

bool CompNode::isTaskAssigned(int taskId){
    for(int id : this->myTaskAssignment[this->currentConfig]){
        if(id == taskId){
            return true;
        }
    }

    return false;
}

bool CompNode::send_msg(cMessage *msg){
    cChannel *tX = portAssignmentsOut[-1]->getChannel();
    cDatarateChannel *tXX = check_and_cast<cDatarateChannel *>(tX);
    simtime_t txfinishtime = tXX->getTransmissionFinishTime();

    EV <<  this->getName() << " - Sending message of Type '" << msg->getClassName() << "' to its router." << endl;
    if(txfinishtime<=simTime()){
        EV <<  this->getName() << " - Output port is available for transmission. Sending message..." << endl;
        send(msg,"port$o",0);
        EV <<  this->getName() << " - " << msg->getClassName() << " message sent!" << endl;
        return true;
    }
    else{
        EV <<  this->getName() << " - Port is currently under use. Try again later at time " << txfinishtime << "..." << endl;
        scheduleAt(txfinishtime, msg);
        return false;
    }
}

bool CompNode::send_task_msg(cMessage *msg){
    int taskId = -1;
    if(strcmp(msg->getClassName(), "PacketMsg" ) == 0){
        taskId = ((PacketMsg *) msg)->getTaskDestination().at(0);
    }
    else if(strcmp(msg->getClassName(), "TaskReady" ) == 0
        || strcmp(msg->getClassName(), "TaskTrigger" ) == 0
        || strcmp(msg->getClassName(), "TaskActivation" ) == 0
        || strcmp(msg->getClassName(), "TaskDone" ) == 0){
        taskId = ((TaskMsg *) msg)->getTaskId();
    }

    if(taskId == -1){
        return false;
    }

    cChannel *tX = taskPortAssignmentsOut[taskId]->getChannel();
    cDelayChannel *tXX = check_and_cast<cDelayChannel *>(tX);
    simtime_t txfinishtime = tXX->getTransmissionFinishTime();

    EV <<  this->getName() << " - Sending message of Type '" << msg->getClassName() << "' to Task[" << taskId << "]." << endl;
    if(txfinishtime<=simTime()){
        EV <<  this->getName() << " - Output port is available for transmission. Sending message..." << endl;
        send(msg,taskPortAssignmentsOut[taskId]);
        EV <<  this->getName() << " - " << msg->getClassName() << " message sent!" << endl;
        return true;
    }
    else{
        EV <<  this->getName() << " - Port is currently under use. Try again later at time " << txfinishtime << "..." << endl;
        scheduleAt(txfinishtime, msg);
        return false;
    }
}

void CompNode::generateRandomMessages(){
    int size = 30;
    int sender = this->id;
    int dest = -1;
    if(id == 0){
        dest = 2;
    }
    else if(id == 1){
        dest = 2;
    }

    std::vector<int> *route = routing[this->currentConfig][dest];
    for(int i = 0; i < size; i++){
        FlitMsg* flit = new FlitMsg;
        flit->setSender(sender);
        flit->setDestination(dest);
        flit->setOriginalSize(size);
        flit->setByteLength(1);
        flit->setTaskId(this->id);
        flit->setFltId(i);

        for(int x : *route){
            (flit->getPath()).push(x);
        }

        if(i == 0){
            flit->setHeader(true);
            EV << "Creating message from Node[" << sender << "] to Node[" << dest << "] through path: ";
            for(int x : *route){
                EV << x << " ";
            }
            EV << endl;
        }
        else if(i == size-1) flit->setEop(true);

        (this->flitQueue).insert(flit);
    }
}

void CompNode::sendPortRequest(){
    if(!this->flitQueue.isEmpty()){
        ChannelRequest* req = new ChannelRequest;
        req->setSender(-1);
        req->setDestination(this->id);
        req->getPath().push(this->id);

        this->send_msg(req);
    }
}

bool CompNode::checkIfRouterMessage(cMessage* msg){
    if(strcmp(msg->getClassName(), "FlitMsg") == 0
            || strcmp(msg->getClassName(), "ChannelRequest" ) == 0
            || strcmp(msg->getClassName(), "ChannelConfirmation" ) == 0
            || strcmp(msg->getClassName(), "ChannelStop" ) == 0
            || strcmp(msg->getClassName(), "WatchdogTimer" ) == 0){
        return true;
    }

    return false;
}

bool CompNode::checkIfTaskMessage(cMessage* msg){
    if(strcmp(msg->getClassName(), "PacketMsg" ) == 0
        || strcmp(msg->getClassName(), "TaskReady" ) == 0
        || strcmp(msg->getClassName(), "TaskTrigger" ) == 0
        || strcmp(msg->getClassName(), "TaskDone" ) == 0){
        return true;
    }

    return false;
}

/**
 * RESULTS
 */
void CompNode::saveDelayResult(FlitMsg* flt){
    int pathLength = this->routing[this->currentConfig][flt->getSender()]->size();
    int taskId = flt->getTaskId();

    DelayResult result(flt, pathLength, this->currentConfig, taskId);

    this->delayResults.push_back(result);
}
std::vector<DelayResult> CompNode::getDelayResults(){
    return this->delayResults;
}

void CompNode::saveTaskResult(TaskDone* dn){
    TaskResult result(dn, this->getName(), this->id);
    this->taskResults.push_back(result);
}
std::vector<TaskResult> CompNode::getTaskResults(){
    return this->taskResults;
}

/**
 * GETTERS AND SETTERS
 */

int CompNode::getId(){
    return this->id;
}
void CompNode::setId(int id){
    this->id = id;
}
double CompNode::getUtilLimit(){
    return utilLimit;
}
std::map<int, std::map<int, std::vector<int> *>> CompNode::getRoutings(){
    return routing;
}

void CompNode::setUtilLimit(double utilLimit){
    this->utilLimit = utilLimit;
}
void CompNode::setCurrentConfig(int config){
    this->currentConfig = config;
}

void CompNode::setPortAssignmentIn(int id, cGate *gate){
    this->portAssignmentsIn[id] = gate;
}
void CompNode::setPortAssignmentOut(int id, cGate *gate){
    this->portAssignmentsOut[id] = gate;
}
void CompNode::setRouting(int config, int dest, std::vector<int> *path){
    this->routing[config][dest] = path;
}
void CompNode::setAvailability(int config, int nodeId, bool available){
    this->online[config][nodeId] = available;
}
void CompNode::setNodeNames(std::map<int, std::string> nodeNames){
    std::map<int, std::string>::iterator it;
    for(it = nodeNames.begin(); it != nodeNames.end(); ++it){
        int id = it->first;
        std::string name = it->second;

        this->nodeNames[id] = name;
    }
}
void CompNode::setTaskPortAssignmentIn(int taskId, cGate *gate){
    this->taskPortAssignmentsIn[taskId] = gate;
}
void CompNode::setTaskPortAssignmentOut(int taskId, cGate *gate){
    this->taskPortAssignmentsOut[taskId] = gate;
}
void CompNode::setMyTaskAssignment(int config, int taskId){
    this->myTaskAssignment[config].push_back(taskId);
}
void CompNode::setTaskAssignment(int config, int taskId, int nodeId){
    this->taskAssignment[config][taskId] = nodeId;
}
