#ifndef TASK_H_
#define TASK_H_

#include <omnetpp.h>
#include <string>
#include <vector>
#include <map>

#include "SpacewireMessages_m.h"
#include "TaskMessages_m.h"
using namespace omnetpp;

class Task: public cSimpleModule {
private:
    int taskId = -1;
    bool periodic = false;
    double period = -1;
    double wcet;
    double messageSize;
    std::vector<int> dependencies;
    std::vector<int> connectivity;
    cGate *portGate;

    bool activated = false;
    std::map<int, bool> depStatus;

    virtual void handleMessage(cMessage *msg) override;
    bool send_msg(cMessage *msg);
    void resetDepStatus();

protected:
    virtual void initialize() override;

public:
    void setId(int id);
    void setPeriod(double period);
    void setWCET(double wcet);
    void setMessageSize(int messageSize);
    void setDependency(int depId);
    void setConnectivity(int conId);
    void setPortGate(cGate *portGate);
};

#endif /* TASK_H_ */
