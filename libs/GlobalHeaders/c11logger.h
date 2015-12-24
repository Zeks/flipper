#ifndef CXX11LOGGER_H
#define CXX11LOGGER_H
#include <memory>
#include <functional>
#include <QObject>
#include "GlobalHeaders/SingletonHolder.h"

namespace logger
{

template<class Manipulator, class Backend, bool Enable = true>
class log
{
private:
    An<Backend> b;
    Manipulator m;
    void prepend(...) {}
    template<typename M = Manipulator>
    auto prepend(M* m) -> decltype(b(m->before()), void())
    {
        b.getData()(m->before());
    }

    void append(...) {}
    template<typename M = Manipulator>
    auto append(M* m) -> decltype(b(m->after()), void())
    {
        b.getData()->operator()(m->after());
    }

    template<typename T, typename M = Manipulator>
    auto manipulate(T&& t, M* m) -> decltype(b->operator()(m->operator()(std::forward<T>(t))), void())
    {
        b.getData()->operator()(m->operator()(std::forward<T>(t)));
    }

    void process() {}
    template<typename T, typename ...P>
    void process(T&& t, P&&... p)
    {
        manipulate(std::forward<T>(t), &m);
        process(std::forward<P>(p)...);
    }
public:
    template<typename ...P> log(P&&... p)
    {
        (*this)(std::forward<P>(p)...);
    }
    template<typename ...P>
    void operator()(P&&... p)
    {
        prepend(&m);
        process(std::forward<P>(p)...);
        append(&m);
    }

    log(log&&) = delete;
    log(const log&) = delete;
    log& operator=(const log&) = delete;
};

template<class Manipulator, class Backend>
struct log<Manipulator, Backend, false>
{
    template<typename ...P> log(P&&... p) {}
    log(Backend&& b) {}
    log(Manipulator&& m) {}
    log(Manipulator&& m, Backend&& b) {}

    template<typename ...P> void operator()(P&&... p) {}

    log(log&&) = delete;
    log(const log&) = delete;
    log& operator=(const log&) = delete;
};

} // namespace logger

namespace logger
{
namespace manipulator
{

struct none {};

struct base
{
    template<typename T> T&& operator()(T&& t) { return std::forward<T>(t); }
    const char* after() { return "\n"; }
};

struct debug : public base
{
#ifdef DEBUG
    const char* before() { return "[DEBUG] "; }
#else
    template<typename T> void operator()(T&& t) {}
    void after() {}
#endif
};

struct info : public base
{
    const char* before() { return "[INFO] "; }
};

struct warning : public base
{
    const char* before() { return "[WARN] "; }
};

} // namespace manipulator
} // namespace logger
BIND_TO_SELF_SINGLE(logger::manipulator::info);
namespace logger
{
namespace backend
{

struct cerr
{
    template<typename T>
    void operator()(T&& t) { Q_UNUSED(t);/*std::cerr << t;*/ }
};

struct cout
{
    template<typename T>
    void operator()(T&& t) { Q_UNUSED(t); /*std::cout << t;*/ }
};

} // namespace backend
} // namespace logger


#endif // LUA_LOGGING_H
