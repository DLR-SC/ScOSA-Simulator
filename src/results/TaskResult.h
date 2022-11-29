#ifndef RESULTS_TASKRESULT_H_
#define RESULTS_TASKRESULT_H_

#include <string>
#include "../TaskMessages_m.h"

class TaskResult{
private:
    std::string nodeName;
    int nodeId;
    std::string taskName;
    int taskId;
    double startTime;
    double endTime;

public:
    TaskResult(TaskDone* dn, std::string nodeName, int nodeId);
    std::string getNodeName();
    int getNodeId();
    int getTaskId();
    double getStartTime();
    double getEndTime();
};



#endif /* RESULTS_TASKRESULT_H_ */
