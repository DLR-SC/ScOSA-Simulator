#ifndef RESULTS_PORTASSIGNMENTRESULT_H_
#define RESULTS_PORTASSIGNMENTRESULT_H_

#include <omnetpp.h>

using namespace omnetpp;
class PortAssignmentResult{
private:
    simtime_t startTime;
    simtime_t endTime;
    double duration;
    int senderNode;
    int destinationNode;
public:
    PortAssignmentResult(simtime_t startTime, int senderNode, int destinationNode);
    void setEndTime(simtime_t endTime);
    simtime_t getStartTime();
    simtime_t getEndTime();
    double getDuration();
    int getSenderNode();
    int getDestinationNode();
};


#endif /* RESULTS_PORTASSIGNMENTRESULT_H_ */
