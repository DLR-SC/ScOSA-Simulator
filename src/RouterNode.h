#ifndef ROUTERNODE_H_
#define ROUTERNODE_H_

#include <string.h>
#include <omnetpp.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <stack>
#include "SpacewireMessages_m.h"
#include "./results/PortAssignmentResult.h"
#include "./results/TransmissionResult.h"

using namespace omnetpp;

class RouterNode: public cSimpleModule {
private:
    int id;
    int messagecount;
    int bufferSize;
    double timeOut;
    std::map<int, cGate *> portIdsIn;
    std::map<int, cGate *> portIdsOut;

    std::map<cGate *, cGate *> portAssignment;
    std::map<cGate *, cQueue> buffer;
    std::map<cGate *, cQueue> channelRequestQueue;
    std::map<cGate *, bool> canTransmitt;
    std::map<cGate *, cQueue> portRequestQueue;
    std::map<cGate *, bool> dumpMessages;
    std::map<int, std::string> nodeNames;

    std::map<int, std::stack<PortAssignmentResult>> portAssignmentResults;
    std::map<int, std::stack<TransmissionResult>> transmissionResults;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    bool send_msg(cMessage *msg, cGate *gate, bool resend);
    int getArrivalPort( cMessage *msg );
    int findInPortId(cGate *gate);
    int findOutPortId(cGate *gate);

public:
    void setId(int id);
    void setPortIdIn(int id, cGate *gate);
    void setPortIdOut(int id, cGate *gate);
    void setNodeNames(std::map<int, std::string> nodeNames);

    std::map<int, std::stack<PortAssignmentResult>> getPortAssignmentResults();
    std::map<int, std::stack<TransmissionResult>> getTransmissionResults();
};


#endif /* ROUTERNODE_H_ */
