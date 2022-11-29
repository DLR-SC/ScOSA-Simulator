#include "PortAssignmentResult.h"

PortAssignmentResult::PortAssignmentResult(simtime_t startTime, int senderNode, int destinationNode){
    this->startTime = startTime;
    this->senderNode = senderNode;
    this->destinationNode = destinationNode;
}
void PortAssignmentResult::setEndTime(simtime_t endTime){
    this->endTime = endTime;
    this->duration = this->endTime.dbl()-this->startTime.dbl();
}
simtime_t PortAssignmentResult::getStartTime(){
    return this->startTime;
}
simtime_t PortAssignmentResult::getEndTime(){
    return this->endTime;
}
double PortAssignmentResult::getDuration(){
    return this->duration;
}
int PortAssignmentResult::getSenderNode(){
    return this->senderNode;
}
int PortAssignmentResult::getDestinationNode(){
    return this->destinationNode;
}
