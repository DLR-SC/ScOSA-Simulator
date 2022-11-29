#include "Task.h"

Define_Module(Task);

void Task::initialize(){
    this->getDisplayString().setTagArg("i", 0, "old/ball_vs");
}

void Task::handleMessage(cMessage *msg){
    if(strcmp( msg->getClassName(), "PacketMsg" ) == 0){
        if(!activated){
            dropAndDelete(msg);
        }
        else if(msg->isSelfMessage()){
            // if self-message, attempt retransmission
            bool sent = this->send_msg(msg);

            if(sent){
                // if sent, reset dependencies
                resetDepStatus();
            }
        }
        else{
            // check what task sent message
            PacketMsg* pkt = check_and_cast<PacketMsg *>(msg);
            int senderTaskId = pkt->getTaskId();

            // update dependency status
            for(int depId : dependencies){
                if(depId == senderTaskId){
                    depStatus[senderTaskId] = true;
                    break;
                }
            }

            // check if dependencies are met
            bool depsMet = true;
            for(int depId : dependencies){
                if(!depStatus[depId]){
                    depsMet = false;
                    break;
                }
            }

            // if so, send task ready message to parent module
            if(depsMet){
                TaskReady* rdy = new TaskReady;
                rdy->setTaskId(this->taskId);

                this->send_msg(rdy);
            }
            dropAndDelete(msg);
        }
    }
    else if(strcmp( msg->getClassName(), "TaskActivation" ) == 0){
        TaskActivation* act = check_and_cast<TaskActivation *>(msg);
        this->activated = act->getStatus();

        if(this->activated) this->getDisplayString().setTagArg("i", 0, "old/app");
        else this->getDisplayString().setTagArg("i", 0, "old/ball_vs");

        if(activated && periodic){
            bool depsMet = true;
            for(int depId : dependencies){
                if(!depStatus[depId]){
                    depsMet = false;
                    break;
                }
            }

            if(depsMet){
                TaskReady* rdy = new TaskReady;
                rdy->setTaskId(this->taskId);

                scheduleAt(simTime(), rdy);
            }
        }

        dropAndDelete(msg);
    }
    else if(strcmp( msg->getClassName(), "TaskReady" ) == 0){
        if(!activated){
            dropAndDelete(msg);
        }
        else if(msg->isSelfMessage() && periodic){
            // if self-message and periodic task,
            // send ready to parent task and schedule next periodic event
            bool sent = this->send_msg(msg);

            // if sent, schedule next ready message after task period
            if(sent){
                TaskReady* rdy = new TaskReady;
                rdy->setTaskId(this->taskId);

                scheduleAt(simTime()+this->period, rdy);
            }
        }
    }
    else if(strcmp( msg->getClassName(), "TaskTrigger" ) == 0){
        if(!activated){
            dropAndDelete(msg);
        }
        else if(msg->isSelfMessage()){
            // if self message, attempt retransmission
            this->send_msg(msg);
        }
        else{
            TaskDone* dn = new TaskDone;
            dn->setTaskId(this->taskId);
            dn->setMessageSize(this->messageSize);
            for(int dest : connectivity){
                dn->getDestIds().push_back(dest);
            }
            dn->setStartTime(simTime().dbl());
            dn->setEndTime(simTime().dbl()+this->wcet);

            scheduleAt(simTime()+this->wcet, dn);

            dropAndDelete(msg);
        }
    }
    else if(strcmp( msg->getClassName(), "TaskDone" ) == 0){
        if(msg->isSelfMessage()){
            // if self message, attempt retransmission
            this->send_msg(msg);
        }
    }
    else {
        dropAndDelete(msg);
    }
}

bool Task::send_msg(cMessage *msg){
    cChannel *tX = this->portGate->getChannel();
    cDelayChannel *tXX = check_and_cast<cDelayChannel *>(tX);
    simtime_t txfinishtime = tXX->getTransmissionFinishTime();

    EV <<  this->getName() << " - Sending message of Type '" << msg->getClassName() << "' to its Computational Node." << endl;
    if(txfinishtime<=simTime()){
        EV <<  this->getName() << " - Output port is available for transmission. Sending message..." << endl;
        send(msg,this->portGate);
        EV <<  this->getName() << " - " << msg->getClassName() << " message sent!" << endl;
        return true;
    }
    else{
        EV <<  this->getName() << " - Port is currently under use. Try again later at time " << txfinishtime << "..." << endl;
        scheduleAt(txfinishtime, msg);
        return false;
    }
}

void Task::resetDepStatus(){
    for(int depId : dependencies){
        depStatus[depId] = false;
    }
}

// -------------------
// Getters and Setters
// -------------------
void Task::setId(int id){
    this->taskId = id;
}
void Task::setPeriod(double period){
    this->periodic = true;
    this->period = period;
}
void Task::setWCET(double wcet){
    this->wcet = wcet;
}
void Task::setMessageSize(int messageSize){
    this->messageSize = messageSize;
}
void Task::setDependency(int depId){
    this->dependencies.push_back(depId);
    this->depStatus[depId] = false;
}
void Task::setConnectivity(int conId){
    this->connectivity.push_back(conId);
}
void Task::setPortGate(cGate *portGate){
    this->portGate = portGate;
}
