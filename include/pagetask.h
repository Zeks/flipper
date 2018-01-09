#pragma once
#include <QString>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSharedPointer>
#include <QUuid>
class PageSubTask;
class PageTask;
class PageFailure;
class PageTaskAction;
typedef QSharedPointer<PageTaskAction> PageTaskActionPtr;
typedef QSharedPointer<PageFailure> PageFailurePtr;
// used to uniquely identify actions performed on page tasks and bind info to them
class PageTaskAction{
public:
    static PageTaskActionPtr CreateNewAction();
    //static PageTaskActionPtr CreateNewAction(QString str, int taskId, int subTaskId);
    PageTaskAction():id(QUuid::createUuid()){}
    PageTaskAction(QString str, int taskId, int subTaskId):id(QUuid(str)){
        this->taskId = taskId;
        this->subTaskId = subTaskId;
    }
    void SetData(QString str, int taskId, int subTaskId);
    QUuid id;
    int taskId = -1; // will use this to record which is this action operating on
    int subTaskId = -1; // will use this to record which is this action operating on
    bool isNewAction = true;
    QDateTime started;
    QDateTime finished;
    bool success = false;
    bool isValid = true;
    QList<PageFailurePtr> errors;
    bool NeedsInsertion(){return isNewAction;}
};

class PageFailure;

class BasePageTask{
public:
    bool isValid = false;
    // if the task had errors or not (need to implement a separate table for error codes)
    // errors are success = false;
    bool success = false;
    // created automatically for supertasks via autoincrement or proc to avoid potential problems in the future
    // creted in code for subtasks
    int id = -1;
    int retries = -1; // how much retries actually happened (subtask retries can be fetched separately)
    QDateTime created; // task creation timestamp
    QDateTime scheduledTo; // when it's supposed to be run
    QDateTime started; // when the task has been staretd
    QDateTime finished; // when it was finished (whether succesfully or not)
    QList<PageFailurePtr> errors;
    QHash<QUuid, QList<PageFailurePtr>> actionFailures;
    QList<PageTaskActionPtr> executedActions;
    QStringList ListFailures();
    bool NeedsInsertion(){return id == -1;}
};
typedef QSharedPointer<PageTask> PageTaskPtr;
typedef QSharedPointer<PageSubTask> SubTaskPtr;
class PageTask : public BasePageTask{
public:
    static PageTaskPtr CreateNewTask();
    bool ignoreCache = false; // if only data from the internet is to be used
    bool refreshIfNeeded = false; // ties in with page expiration mechanism. Passed to pagegetter to let it know if the page needs to be fetched ifit has expired
    int type = -1; // task type
    int parts = -1; // amount of subtasks the task is split into
    int entities = -1; // I don't remember what teh fuck that is. to be explained later
    int allowedRetries = -1; // how much retries a task can use
    int allowedSubtaskRetries = -1; // how much retries a task can use
    QString results; // why the fuck am I using a string for the results, looks retarded
    QString taskComment; // if necessary
    QList<SubTaskPtr> subTasks;
};


class PageSubTask: public BasePageTask{

public:
    static SubTaskPtr CreateNewSubTask();
    int parentId = -1; // id of the parent task
    QString content; // part of page content or url list to execute
};

typedef QList<PageFailurePtr> SubTaskErrors;
class PageFailure{
public:
    static PageFailurePtr CreateNewPageFailure();
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
    QString error;
    QDateTime attemptTimeStamp;
    QDateTime lastSeen;
    Action action;
};


