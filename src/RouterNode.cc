#include "RouterNode.h"

Define_Module(RouterNode);

void RouterNode::initialize(){
    EV <<  this->getName() << " is being initialized..." << endl;
    this->messagecount = 0;
    this->bufferSize = this->par("bufferSize");
    this->timeOut = this->par("timeOut");
}

void RouterNode::handleMessage(cMessage *msg){
    if(strcmp( msg->getClassName(), "FlitMsg" ) == 0){
        FlitMsg* flt = check_and_cast<FlitMsg *>(msg);

        if( msg->isSelfMessage() ){
            // if from self, attempt transmission
            EV <<  this->getName() << " - Attempting Flit transmission..." << endl;
            int srcGateId = flt->getSrcGate();
            int outGateId;
            if(flt->getPath().size() < 1){
                outGateId = -1;
            }
            else{
                outGateId = flt->getPath().front();
            }

            cGate* srcGate = portIdsIn[srcGateId];
            cGate* outGate = portIdsOut[outGateId];

            if(canTransmitt[outGate]){
                //channel is available, send flit
                EV <<  this->getName() << " - Gate available! Sending Flit from " << nodeNames[flt->getSender()] << "..." << endl;

                bool sent = this->send_msg(flt, outGate, true);

                if(sent){
                    // if sent, check for end of file
                    if(flt->getEop()){
                        // if EOP, reset assignments and make the output channel unavailable
                        portAssignment[outGate] = NULL;
                        canTransmitt[outGate] = false;
                        EV << "Router [" << this->id << "] - EOP reached! Resetting Gate assignments..." << endl;

                        // proceed to transmit next buffer in the queue if available
                        if(!portRequestQueue[outGate].isEmpty()){
                            PortRequest* req =  check_and_cast<PortRequest *>(portRequestQueue[outGate].front());
                            portRequestQueue[outGate].pop();

                            scheduleAt(simTime(),req);
                        }

                        // Log end of port/link assignment time
                        cChannel *tX = outGate->getChannel();
                        cDatarateChannel *tXX = check_and_cast<cDatarateChannel *>(tX);
                        simtime_t txfinishtime = tXX->getTransmissionFinishTime();

                        portAssignmentResults[outGateId].top().setEndTime(txfinishtime);
                    }
                    else if(!buffer[srcGate].isEmpty()){
                        // there are other elements in the buffer, send next
                        FlitMsg* nxtFlt = check_and_cast<FlitMsg *>(buffer[srcGate].front());
                        buffer[srcGate].pop();

                        scheduleAt(simTime(),nxtFlt);
                    }

                    if(!channelRequestQueue[srcGate].isEmpty()){
                        ChannelRequest* req = check_and_cast<ChannelRequest *>(channelRequestQueue[srcGate].front());
                        channelRequestQueue[srcGate].pop();

                        scheduleAt(simTime(), req);
                    }
                }
            }
            else{
                // channel not available, try again later
                if(buffer[srcGate].isEmpty()){
                    buffer[srcGate].insert(flt);
                }
                else{
                    FlitMsg* topFlt = check_and_cast<FlitMsg *>(buffer[srcGate].front());
                    buffer[srcGate].insertBefore(topFlt, flt);
                }
            }
        }
        else{
            // external flit received, store in buffer and ask for permission to forward
            (flt->getPath()).pop();

            bool header = flt->getHeader();

            cGate* srcGate = flt->getArrivalGate();
            int srcGateId = findInPortId(srcGate);
            flt->setSrcGate( srcGateId );

            if(srcGateId == -1){
                EV <<  this->getName() << " - Flit Message with Packet ID: "<< flt->getTaskId() << " received from " << this->nodeNames[this->id] << "!" << endl;
            }
            else{
                EV <<  this->getName() << " - Flit Message with Packet ID: "<< flt->getTaskId() << " received from " << this->nodeNames[srcGateId] << "-Router!" << endl;
            }

            int outGateId;
            if(flt->getPath().size() < 1){ outGateId = -1; }
            else{ outGateId = flt->getPath().front(); }
            cGate* outGate = portIdsOut[outGateId];

            if(header){
                // if a header file, make a port request
                std::string out = "Header received. Assigning Port...";
                bubble(out.c_str());

                EV <<  this->getName() << " - Flit Message is a header file. Storing in buffer and creating port request..." << endl;
                PortRequest* req = new PortRequest;
                req->setSrcGate( srcGateId );
                req->setOutGate( outGateId );
                req->setHeader(flt->getHeader());
                req->setTaskId(flt->getTaskId());

                scheduleAt(simTime(), req);

                // store flit in buffer
                buffer[srcGate].insert(flt);

                // send watch-dog self-message to check packet transmission status later
                EV <<  this->getName() << " - Scheduling watch-dog timer to time " << simTime()+timeOut << "s..." << endl;
                WatchdogTimer* tmr = new WatchdogTimer;
                tmr->setTaskId(flt->getTaskId());
                tmr->setSrcGate(srcGateId);

                // reset message dump flag in case previous message was discarded;
                dumpMessages[srcGate] = false;

                scheduleAt(simTime()+timeOut, tmr);
            }
            else{ // not a header file
                if(dumpMessages[srcGate] || flt->getEep()){
                    // channel has been flagged for dumping incoming messages
                    // or incoming message is an Error packet
                    std::string out = "Discarding flit...";
                    bubble(out.c_str());

                    EV <<  this->getName() << " - EEP already received for this packet. Dumping Flit..." << endl;
                    dropAndDelete(flt);
                }
                else if(portAssignment[outGate] == srcGate
                        && canTransmitt[outGate]
                        && buffer[srcGate].isEmpty()){
                    // if destination port is assigned to input port then forward
                    std::string out = "Forwarding flit...";
                    bubble(out.c_str());

                    EV <<  this->getName() << " - Port is assigned and available. Forwarding Flit..." << endl;
                    scheduleAt(simTime(),flt);
                }
                else{
                    // else store in buffer
                    std::string out = "Storing in buffer...";
                    bubble(out.c_str());

                    EV <<  this->getName() << " - Port not assigned or available. Storing Flit in buffer..." << endl;
                    buffer[srcGate].insert(flt);
                }
            }


            // check buffer capacity and not discharging
            if(buffer[srcGate].getLength() == this->bufferSize-1
                    && !(portAssignment[outGate] == srcGate && canTransmitt[outGate])){
                // buffer has reached max capacity, send stop message to previous node
                ChannelStop* stp = new ChannelStop;
                int srcId = this->findInPortId(srcGate);
                cGate* returnGate = portIdsOut[srcId];
                stp->setSender(this->id);
                stp->setDestination(srcId);

                std::string out = std::string("Buffer[") + std::to_string(srcId) + std::string("] full.");
                bubble(out.c_str());

                EV <<  this->getName() << " - Buffer for Port[" << srcId << "] is full! Halting incoming transmission..." << endl;

                this->send_msg(stp, returnGate, true);
            }

        }
    }
    else if(strcmp( msg->getClassName(), "ChannelRequest" ) == 0){
        ChannelRequest* req = (ChannelRequest *) msg;

        if(!msg->isSelfMessage() || req->getSender() != this->id){
            cGate* srcGateIn = msg->getArrivalGate();

            int srcGateId = findInPortId(srcGateIn);
            if(srcGateId  == -1){
                EV <<  this->getName() << " - Channel request received from " << this->nodeNames[this->id] << "!" << endl;
            }
            else{
                EV <<  this->getName() << " - Channel request received from " << this->nodeNames[srcGateId] << "-Router!" << endl;
            }
            // if the input gate has never been used, has room in its buffer,
            // or is already assigned to an output gate, then allow for reception
            if( buffer.find(srcGateIn) == buffer.end()
                    || buffer[srcGateIn].getLength() < this->bufferSize-1
                    || portAssignment.find(srcGateIn) != portAssignment.end() ){

                // send confirmation
                int sender = req->getSender();

                EV <<  this->getName() << " - Channel request approved! Sending confirmation..." << endl;

                ChannelConfirmation* conf = new ChannelConfirmation;
                conf->setSender(this->id);
                conf->setDestination(sender);
                conf->setTaskId(req->getTaskId());

                cGate* srcGateOut = portIdsOut[sender];

                this->send_msg(conf, srcGateOut, true);

                dropAndDelete(req);
            }
            else{
                // buffer is full and cannot support more messages, attempt later after buffer has room
                EV <<  this->getName() << " - Channel request DENIED! Buffer is either full or not being depleted. Trying again when available..." << endl;
                req->setSrcGate(srcGateId);

                channelRequestQueue[srcGateIn].insert(req);
            }
        }
        else{
            // is self message, attempt retransmission
            int dest = req->getDestination();
            cGate* srcGateOut = portIdsOut[dest];

            this->send_msg(req, srcGateOut, true);
        }
    }
    else if(strcmp( msg->getClassName(), "ChannelConfirmation" ) == 0){
        // channel confirmation received
        ChannelConfirmation* conf = check_and_cast<ChannelConfirmation *>(msg);

        if(msg->isSelfMessage()){
            // self message, attempt retransmission
            cGate* outGate = portIdsOut[conf->getDestination()];
            this->send_msg(conf, outGate, true);
        }
        else{
            // external message
            int sender = conf->getSender();
            if(sender == -1){
                EV <<  this->getName() << " - Received a Channel Confirmation from " << this->nodeNames[this->id] << ". Starting transmission..." << endl;
            }
            else{
                EV <<  this->getName() << " - Received a Channel Confirmation from " << this->nodeNames[sender] << "-Router. Starting transmission..." << endl;
            }

            // update channel permissions
            cGate* outGate = portIdsOut[sender];
            cGate* srcGate = portAssignment[outGate];
            canTransmitt[outGate] = true;

            // start transmitting messages in that buffer
            if(!buffer[srcGate].isEmpty()){
                FlitMsg* flt = check_and_cast<FlitMsg *>(buffer[srcGate].front());
                buffer[srcGate].pop();

                scheduleAt(simTime(),flt);
            }

            dropAndDelete(conf);
        }
    }
    else if(strcmp( msg->getClassName(), "ChannelStop" ) == 0){
        ChannelStop* stp = check_and_cast<ChannelStop *>(msg);

        if(msg->isSelfMessage()){
            // halt received from self, attempt retransmission
            int dest = stp->getDestination();
            cGate* destGate = portIdsOut[dest];

            this->send_msg(stp, destGate, true);
        }
        else{
            int srcId = findInPortId(msg->getArrivalGate());
            std::string out = std::string("Stop command received. Halting transmission from Port[")
                        + std::to_string(srcId)
                        + std::string("]...");
            bubble(out.c_str());

            EV <<  this->getName() << " - Received a Channel Stop from Port[" << srcId << "]. Halting transmission..." << endl;

            // channel stop message received, halt any transmissions
            int dest = stp->getSender();
            cGate* outGate = portIdsOut[dest];
//            cGate* srcGate = portAssignment[outGate];

            canTransmitt[outGate] = false;

            // send a ChannelRequest to allow transmission in then next available opportunity
            EV <<  this->getName() << " - Sending a new Channel Request to Port[" << srcId << "]..." << endl;
            ChannelRequest* cReq = new ChannelRequest;
            cReq->setSender(this->id);
            cReq->setDestination(dest);
//            cReq->setTaskId( check_and_cast<FlitMsg *>(buffer[srcGate].front())->getTaskId() );

            cGate* srcGateOut = portIdsOut[dest];
            this->send_msg(cReq, srcGateOut, true);
        }
    }
    else if(strcmp( msg->getClassName(), "PortRequest" ) == 0){
        PortRequest* req = check_and_cast<PortRequest *>(msg);
        cGate* srcGate =  portIdsIn[req->getSrcGate()];
        cGate* outGate =  portIdsOut[req->getOutGate()];

        // check if the output gate is available or assigned to the input gate
        EV <<  this->getName() << " - Received Port Request (From Port[" << req->getSrcGate() << "] to Port[" << req->getOutGate() << "])." << endl;
        if(portAssignment[outGate] == srcGate
                || portAssignment[outGate] == NULL){

            // Log port/link assignment time
            if(portAssignment[outGate] == NULL){
                if(portAssignmentResults[req->getOutGate()].empty()){
                    PortAssignmentResult result(simTime(), this->id, req->getOutGate());
                    portAssignmentResults[req->getOutGate()].push(result);
                }
                else{
                    PortAssignmentResult lastResult = portAssignmentResults[req->getOutGate()].top();

                    if(lastResult.getEndTime() < simTime()){
                        PortAssignmentResult result(simTime(), this->id, req->getOutGate());
                        portAssignmentResults[req->getOutGate()].push(result);
                    }
                }
            }

            // assign port
            portAssignment[outGate] = srcGate;

            std::string out = std::string("Port[") + std::to_string(req->getSrcGate()) + std::string("] and Port[") + std::to_string(req->getOutGate()) + std::string("] assigned!");
            bubble(out.c_str());

            EV <<  this->getName() << " - Accepted Port Request! Port[" << req->getSrcGate() << "] is now assigned to Port[" << req->getOutGate() << "]." << endl;
            FlitMsg* flt = check_and_cast<FlitMsg *>(buffer[srcGate].front());

            // check for channel confirmation
            if(canTransmitt[outGate]){
                // if allowed, transmit flit
                EV <<  this->getName() << " - Transmission through channel on Port[" << req->getOutGate() << "] is available!" << endl;
                buffer[srcGate].pop();
                scheduleAt(simTime(),flt);

                dropAndDelete(req);

                return;
            }
            else if(!req->getCReqSent() && req->getHeader()){
                EV <<  this->getName() << " - Transmission through channel on Port[" << req->getOutGate() << "] is not yet confirmed. Sending a Channel Request..." << endl;

                ChannelRequest* cReq = new ChannelRequest;
                int dest;
                if(flt->getPath().size() < 1){dest = -1;}
                else{dest = flt->getPath().front();}

                cReq->setSender(this->id);
                cReq->setDestination(dest);
                cReq->setTaskId(req->getTaskId());

                cGate* srcGateOut = portIdsOut[dest];
                this->send_msg(cReq, srcGateOut, true);

                req->setCReqSent(true);

                dropAndDelete(req);
            }
            else{
                EV <<  this->getName() << " - Transmission through Port[" << req->getOutGate() << "] is still NOT available. Trying again later..." << endl;
            }
        }
        else{
            // if unavailable, try again later
            portRequestQueue[outGate].insert(req);
            EV <<  this->getName() << " - DENIED Port Request. Try again later..." << endl;
        }
    }
    else if(strcmp( msg->getClassName(), "WatchdogTimer" ) == 0){
        // check if top element in the buffer is still waiting to be transmitted
        WatchdogTimer* tmr = check_and_cast<WatchdogTimer *>(msg);
        cGate* srcGate =  portIdsIn[tmr->getSrcGate()];
        if(!buffer[srcGate].isEmpty()){
            FlitMsg* flt = check_and_cast<FlitMsg *>(buffer[srcGate].front());
            int pcktId = flt->getTaskId();

            if(pcktId == tmr->getTaskId()){
                // packet has exceeded timeout, dropping packet
                std::string out = std::string("Timeout reached for Packet (ID: ")
                    + std::to_string(flt->getTaskId())
                    + std::string("). Dumping packet...");
                bubble(out.c_str());

                EV <<  this->getName() << " -  Timeout for Packet ID: " << flt->getTaskId() << " reached." << endl;

                // send EEP to next node
                if(!flt->getHeader()){
                    EV <<  this->getName() << "Sending EEP..." << endl;
                    FlitMsg* eep = new FlitMsg;
                    eep->setEep(true);

                    if(!flt->getPath().empty()){
                        int outGateId = flt->getPath().front();
                        eep->getPath().push(outGateId);
                    }

                    scheduleAt(simTime(),eep);
                }

                // discard all flits of this packet contained in the buffer
                EV <<  this->getName() << "Discarding packet flits remaining in buffer..." << endl;
                bool eopFound = false;
                while(!eopFound && !buffer[srcGate].isEmpty()){
                    FlitMsg* fltTop = check_and_cast<FlitMsg *>(buffer[srcGate].front());
                    buffer[srcGate].pop();

                    eopFound = fltTop->getEop();

                    dropAndDelete(fltTop);
                }

                // discard all channel and port requests regarding this packet
                EV <<  this->getName() << "Discarding channel and port requests from discarded packet..." << endl;
                for (int i = 0; i < channelRequestQueue[srcGate].getLength(); i++){
                    ChannelRequest* req = check_and_cast<ChannelRequest *>(channelRequestQueue[srcGate].get(i));

                    if(tmr->getTaskId() == req->getTaskId()){
                        channelRequestQueue[srcGate].remove(req);
                        dropAndDelete(req);
                    }
                }

                for(const auto& x : portRequestQueue){
                    cQueue q = portRequestQueue[x.first];
                    for(int i = 0; i < q.getLength(); i++){
                        PortRequest* req = check_and_cast<PortRequest *> (portRequestQueue[x.first].get(i));

                        if(tmr->getTaskId() == req->getTaskId()){
                            portRequestQueue[x.first].remove(req);
                            dropAndDelete(req);
                        }
                    }
                }

                // if no eop was contained in buffer, set all incoming messages
                // from this port to be discarded until a new header file is received
                // else do not discard
                dumpMessages[srcGate] = !eopFound;

                if(eopFound){
                    EV <<  this->getName() << "All Flits with Packet ID: " << pcktId << " successfully discarded." << endl;
                }
                else{
                    EV <<  this->getName() << "Any incoming flits with Packet ID: " << pcktId << " will be discarded." << endl;
                }

                dropAndDelete(tmr);
            }
            else{
                // packet related to the timer has been sent, ignore timeout
                dropAndDelete(tmr);
            }
        }
    }
    else{
        EV <<  this->getName() << " - Unknown message type received. Dropping Flit..." << endl;
        dropAndDelete(msg);
    }
}

bool RouterNode::send_msg(cMessage *msg, cGate *gate, bool resend){
    SpacewireMsg* sw = (SpacewireMsg *) msg;
    int nextRouter = findOutPortId(gate);

    cChannel *tX = gate->getChannel();
    cDatarateChannel *tXX = check_and_cast<cDatarateChannel *>(tX);
    simtime_t txfinishtime = tXX->getTransmissionFinishTime();

    if(nextRouter == -1){
        EV <<  this->getName() << " - Sending message of Type '" << msg->getClassName() << "' towards " << nodeNames[this->id] << "..." << endl;
    }
    else{
        EV <<  this->getName() << " - Sending message of Type '" << msg->getClassName() << "' towards " << nodeNames[nextRouter] << "-Router..." << endl;
    }

    if(txfinishtime<=simTime()){
        if(nextRouter == -1){
            EV <<  this->getName() << " - Out channel to " << nodeNames[this->id] << " is available for transmission. Sending message..." << endl;
        }
        else{
            EV <<  this->getName() << " - Out channel to " << nodeNames[nextRouter] << "-Router is available for transmission. Sending message..." << endl;
        }
        send(sw,gate);

        // log transmission results
        if(strcmp( msg->getClassName(), "FlitMsg" ) == 0){
            tX = gate->getChannel();
            tXX = check_and_cast<cDatarateChannel *>(tX);
            txfinishtime = tXX->getTransmissionFinishTime();

            FlitMsg *flt =check_and_cast<FlitMsg *>(msg);
            int taskId = flt->getTaskId();

            if(transmissionResults[nextRouter].empty()){
                TransmissionResult result(simTime(), txfinishtime, this->id, nextRouter, taskId);
                transmissionResults[nextRouter].push(result);
            }
            else{
                TransmissionResult latestResult = transmissionResults[nextRouter].top();

                if( (simTime() - latestResult.getEndTime()) <= 1*8.00e-08 && latestResult.getTaskId() == taskId){
                    transmissionResults[nextRouter].top().setEndTime( txfinishtime );
                }
                else{
                    TransmissionResult result(simTime(), txfinishtime, this->id, nextRouter, taskId);
                    transmissionResults[nextRouter].push(result);
                }
            }
        }

        EV <<  this->getName() << " - "  << msg->getClassName() << " Message sent!" << endl;
        return true;
    }
    else{
        omnetpp::SimTime waitTime = txfinishtime;

        if(nextRouter == -1){
            EV <<  this->getName() << " - Out channel to " << nodeNames[this->id] << " is currently under use. Try again later at time " << waitTime << "s..." << endl;
        }
        else{
            EV <<  this->getName() << " - Out channel to " << nodeNames[nextRouter] << "-Router is currently under use. Try again later at time " << waitTime << "s..." << endl;
        }
        if(resend) scheduleAt(waitTime, msg);
        return false;
    }
}

int RouterNode::findInPortId(cGate *gate){
    for (std::map<int, cGate *>::iterator it = portIdsIn.begin(); it != portIdsIn.end(); ++it) {
        if(it->second == gate){
            return it->first;
        }
    }
    return -1;
}

int RouterNode::findOutPortId(cGate *gate){
    for (std::map<int, cGate *>::iterator it = portIdsOut.begin(); it != portIdsOut.end(); ++it) {
        if(it->second == gate){
            return it->first;
        }
    }
    return -1;
}

void RouterNode::setId(int id){
    this->id = id;
}

void RouterNode::setPortIdIn(int id, cGate *gate){
    this->portIdsIn[id] = gate;
}

void RouterNode::setPortIdOut(int id, cGate *gate){
    this->portIdsOut[id] = gate;
}
void RouterNode::setNodeNames(std::map<int, std::string> nodeNames){
    std::map<int, std::string>::iterator it;
    for(it = nodeNames.begin(); it != nodeNames.end(); ++it){
        int id = it->first;
        std::string name = it->second;

        this->nodeNames[id] = name;
    }
}

std::map<int, std::stack<PortAssignmentResult>> RouterNode::getPortAssignmentResults(){
    return this->portAssignmentResults;
}
std::map<int, std::stack<TransmissionResult>> RouterNode::getTransmissionResults(){
    return this->transmissionResults;
}
