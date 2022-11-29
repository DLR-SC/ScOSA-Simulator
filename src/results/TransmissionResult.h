#ifndef RESULTS_TRANSMISSIONRESULT_H_
#define RESULTS_TRANSMISSIONRESULT_H_

#include <omnetpp.h>

using namespace omnetpp;
class TransmissionResult{
private:
    simtime_t startTime;
    simtime_t endTime;
    double duration;
    int senderNode;
    int destinationNode;
    int taskId;
    int bytesSent = 0;
public:
    TransmissionResult(simtime_t startTime, simtime_t endTime, int senderNode, int destinationNode, int taskId);
    simtime_t getStartTime();
    simtime_t getEndTime();
    double getDuration();
    int getSenderNode();
    int getDestinationNode();
    int getTaskId();
    void setEndTime(simtime_t endTime);
    int getBytesSent();
};


#endif /* RESULTS_TRANSMISSIONRESULT_H_ */
