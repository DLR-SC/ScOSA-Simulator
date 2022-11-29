#ifndef RESULTS_DELAYRESULT_H_
#define RESULTS_DELAYRESULT_H_

#include "../SpacewireMessages_m.h"

class DelayResult{
private:
    int senderId;
    int destinationId;
    int pathLength;
    int config;
    int taskId;
    int messageSize;
    double sentTime;
    double receptionTime;

public:
    DelayResult(FlitMsg* flt, int pathLength, int config, int taskId);
    int getSenderId();
    int getDestinationId();
    int getPathLength();
    int getConfig();
    int getTaskId();
    int getMessageSize();
    double getSentTime();
    double getReceptionTime();
};


#endif /* RESULTS_DELAYRESULT_H_ */
