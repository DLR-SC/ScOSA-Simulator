#include "DelayResult.h"

DelayResult::DelayResult(FlitMsg* flt, int pathLength, int config, int taskId){
    this->senderId = flt->getSender();
    this->destinationId = flt->getDestination();
    this->pathLength = pathLength;
    this->taskId = taskId;
    this->messageSize = flt->getOriginalSize();
    this->config = config;
    this->sentTime = flt->getSentTime();
    this->receptionTime = flt->getReceptionTime();
}

int DelayResult::getSenderId(){
    return this->senderId;
}
int DelayResult::getDestinationId(){
    return this->destinationId;
}
int DelayResult::getPathLength(){
    return this->pathLength;
}
int DelayResult::getConfig(){
    return this->config;
}
int DelayResult::getMessageSize(){
    return this->messageSize;
}
double DelayResult::getSentTime(){
    return this->sentTime;
}
double DelayResult::getReceptionTime(){
    return this->receptionTime;
}
int DelayResult::getTaskId(){
    return this->taskId;
}
