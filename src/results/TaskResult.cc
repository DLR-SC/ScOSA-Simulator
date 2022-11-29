#include "TaskResult.h"

TaskResult::TaskResult(TaskDone* dn, std::string nodeName, int nodeId){
    this->nodeName = nodeName;
    this->nodeId = nodeId;
    this->taskId = dn->getTaskId();
    this->startTime = dn->getStartTime();
    this->endTime = dn->getEndTime();
}
std::string TaskResult::getNodeName(){
    return this->nodeName;
}
int TaskResult::getNodeId(){
    return this->nodeId;
}
int TaskResult::getTaskId(){
    return this->taskId;
}
double TaskResult::getStartTime(){
    return this->startTime;
}
double TaskResult::getEndTime(){
    return this->endTime;
}
