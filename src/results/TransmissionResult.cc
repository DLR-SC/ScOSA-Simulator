#include "TransmissionResult.h"

TransmissionResult::TransmissionResult(simtime_t startTime, simtime_t endTime, int senderNode, int destinationNode, int taskId){
    this->startTime = startTime;
    this->endTime = endTime;
    this->duration = this->endTime.dbl() - this->startTime.dbl();
    this->senderNode = senderNode;
    this->destinationNode = destinationNode;
    this->taskId = taskId;
    this->bytesSent++;
}
simtime_t TransmissionResult::getStartTime(){
    return this->startTime;
}
simtime_t TransmissionResult::getEndTime(){
    return this->endTime;
}
double TransmissionResult::getDuration(){
    return this->duration;
}
int TransmissionResult::getSenderNode(){
    return this->senderNode;
}
int TransmissionResult::getDestinationNode(){
    return this->destinationNode;
}
int TransmissionResult::getTaskId(){
    return this->taskId;
}
void TransmissionResult::setEndTime(simtime_t endTime){
    this->endTime = endTime;
    this->duration = this->endTime.dbl() - this->startTime.dbl();
    this->bytesSent++;
}
int TransmissionResult::getBytesSent(){
    return this->bytesSent;
}
