#ifndef NETWORKBUILDER_H_
#define NETWORKBUILDER_H_

#include <omnetpp.h>
#include <fstream>
#include <cstdlib>

#include <winbase.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>

#include "CompNode.h"
#include "RouterNode.h"
#include "Task.h"
#include "./results/DelayResult.h"
#include "./results/TaskResult.h"
#include "SimMessage_m.h"
#include "./nlohmann-json/single_include/nlohmann/json.hpp"

using namespace omnetpp;
using json = nlohmann::json;

class NetworkBuilder: public cSimpleModule {
  private:
    // Constants
    std::string inputDirStr = "../input/";
    std::string outputDirStr = "../output/";
    std::string configStr = "/config";
    std::string jsonStr = ".json";

    std::vector<cModule *> compNodeList;
    std::vector<cModule *> routerNodeList;
    std::map<int, std::string> taskNameList;
    std::map<int, std::string> nodeNameList;

  public:
    NetworkBuilder() : cSimpleModule() {};

  protected:
    void readAndBuild(const char *networkName);
    cChannel *createLink(double datarate, int linkId, cGate *srcGate, cGate *destGate);
    cChannel *createLink(int linkId, cGate *srcGate, cGate *destGate);

    virtual void initialize();
    virtual int numInitStages() const { return 2; }
    virtual void handleMessage(cMessage *msg);
    json loadJSON(const char *networkName, int config, bool print);
};

#endif /* NETWORKBUILDER_H_ */
