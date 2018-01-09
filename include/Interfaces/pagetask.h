#pragma once
#include <QString>
#include <QDateTime>
#include <QSharedPointer>
#include <QUuid>
namespace database{
class PageSubtask;
class PageTask;

// used to uniquely identify actions performed on page tasks and bind info to them
class Action{
public:
    Action():id(QUuid::createUuid()){}
    QUuid id;
    int taskId = -1; // will use this to record which is this action operating on
    QDateTime performed;
};

class PageFailure;

class BasePageTask{
public:
    // if the task had errors or not (need to implement a separate table for error codes)
    // errors are success = false;
    bool success = false;
    bool ignoreCache = false; // if only data from the internet is to be used
    bool refreshIfNeeded = false; // ties in with page expiration mechanism. Passed to pagegetter to let it know if the page needs to be fetched ifit has expired
    int id = -1; // created automatically via autoincrement or proc to avoid potential problems in the future
    int allowedRetries = -1; // how much retries a task can use
    QDateTime created; // task creation timestamp
    QDateTime scheduledTo; // when it's supposed to be run
    QDateTime started; // when the task has been staretd
    QDateTime finished; // when it was finished (whether succesfully or not)
    QList<PageFailure> errors;
};
typedef QSharedPointer<PageTask> PageTaskPtr;
class PageTask : public BasePageTask{
public:
    static PageTaskPtr CreateNewTask();
    //{return PageTaskPtr(new PageTask());}
    int type = -1; // task type
    int parts = -1; // amount of subtasks the task is split into
    int entities = -1; // I don't remember what teh fuck that is. to be explained later
    int retries = -1; // how much retries actually happened (subtask retries can be fetched separately)
    QString results; // why the fuck am I using a string for the results, looks retarded
    QString taskComment; // if necessary
    QList<PageSubtask> subTasks;

};


class PageSubtask: public BasePageTask{
public:
    int parentId = -1; // id of the parent task
    QString content; // part of page content or url list to execute
    QList<Action> executedActions;

};


class PageFailure{
public:
    enum class EFailureReason{
        none = 0,
        page_absent = 1,
        unexpected_content = 2
    };
    enum class EErrorLevel{
        none = 0,
        warning = 1,
        error = 2
    };
    EErrorLevel errorlevel = EErrorLevel::none;
    EFailureReason errorCode = EFailureReason::none;
    QString url;
    QDateTime attemptTimeStamp;
    QDateTime lastSeen;
    Action actionId;
};




}
