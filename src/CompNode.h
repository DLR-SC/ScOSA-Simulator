#ifndef COMPNODE_H_
#define COMPNODE_H_

#include <winsock2.h>
#include <windows.h>
#include <omnetpp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>

#include "SpacewireMessages_m.h"
#include "TaskMessages_m.h"
#include "results/DelayResult.h"
#include "results/TaskResult.h"
#include "./nlohmann-json/single_include/nlohmann/json.hpp"

using namespace omnetpp;
using json = nlohmann::json;

class CompNode: public cSimpleModule {
private:
    // Constants and lists
    int id = -1;
    double utilLimit = -1;
    double timeOut;
    std::map<int, cGate *> portAssignmentsIn;
    std::map<int, cGate *> portAssignmentsOut;
    std::map<int, cGate *> taskPortAssignmentsIn;
    std::map<int, cGate *> taskPortAssignmentsOut;
    std::map<int, std::map<int, std::vector<int> *>> routing;           // routing to other nodes: f(config, destination) -> route[]
    std::map<int, std::map<int, bool>> online;                          // computational node availability: f(config, destination) -> availability
    std::map<int, std::map<int, int>> taskAssignment;                   // assignment of tasks for all nodes: f(config, task) -> compNodeIndex
    std::map<int, std::vector<int>> myTaskAssignment;                   // assignment of tasks for this node: f(config) -> taskNodesIndex[]
    std::map<int, std::string> nodeNames;

    // Simulation status
    int currentConfig = -1;
    double lastSentTime;
//    double util = 0;

    cQueue flitQueue;
    cQueue packetQueue;
    cQueue readyQueue;
    bool canTransmitt = false;
    bool busy = false;

    int receivingFlitCounter = 0;
    int receivingTaskId = -1;

    // Methods
    void generateRandomMessages();
    void sendPortRequest();
    bool send_msg(cMessage *msg);
    bool send_task_msg(cMessage *msg);
    bool isTaskAssigned(int taskId);

    bool checkIfRouterMessage(cMessage* msg);
    bool checkIfTaskMessage(cMessage* msg);
    void handleRouterMessage(cMessage* msg);
    void handleTaskMessage(cMessage* msg);
    virtual void handleMessage(cMessage *msg) override;

    // Results
    std::vector<DelayResult> delayResults;
    std::vector<TaskResult> taskResults;
    void saveDelayResult(FlitMsg* flt);
    void saveTaskResult(TaskDone* dn);

protected:
    virtual void initialize() override;

public:
    // getters and setters
    int getId();
    void setId(int id);
    double getUtilLimit();
    std::map<int, std::map<int, std::vector<int> *>> getRoutings();

    void setUtilLimit(double utilLimit);
    void setCurrentConfig(int config);
    void setPortAssignmentIn(int id, cGate *gate);
    void setPortAssignmentOut(int id, cGate *gate);
    void setTaskPortAssignmentIn(int id, cGate *gate);
    void setTaskPortAssignmentOut(int id, cGate *gate);

    void setRouting(int config, int dest, std::vector<int> *path);
    void setAvailability(int config, int nodeId, bool available);
    void setNodeNames(std::map<int, std::string> nodeNames);
    void setMyTaskAssignment(int config, int taskId);
    void setTaskAssignment(int config, int nodeId, int taskId);

    // Results
    std::vector<DelayResult> getDelayResults();
    std::vector<TaskResult> getTaskResults();
};

#endif /* COMPNODE_H_ */
