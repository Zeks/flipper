#ifndef RUNONCE_H_
#define RUNONCE_H_
#include <functional>
#include "logger/QsLog.h"
#include <QString>
class once
{
	public:
    once(std::function<void()> func){func();}
};


class FuncCleanup
{
	public:
    FuncCleanup(){}
    FuncCleanup(std::function<void()> _func) { func = _func;}
    ~FuncCleanup()
    {
        try
        {
            if(func) func();
        }
        catch(const std::exception& e)
        {
            QLOG_FATAL() << "exception while doing function cleanup:" + QString(e.what());
        }
        catch(...)
        {
            QLOG_FATAL() << "exception while doing function cleanup";
        }
    }
    void dismiss(){func = std::function<void()>();}
	std::function<void()> func;

};

class BeginEnd
{
    public:
    BeginEnd(std::function<void()> _beginFunc, std::function<void()> _endFunc, std::function<void()> _actionFunc): endFunc(_endFunc) { _beginFunc(); _actionFunc();}
    ~BeginEnd(){if(endFunc) endFunc();}
    std::function<void()> endFunc;

};

#endif
