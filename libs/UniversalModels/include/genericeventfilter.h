#ifndef GENERICEVENTFILTER_H
#define GENERICEVENTFILTER_H
#include <QObject>
#include <QString>
#include <QEvent>
#include <functional>
#include "l_tree_controller_global.h"
class L_TREE_CONTROLLER_EXPORT GenericEventFilter: public QObject
{
Q_OBJECT
private:
    std::function<bool(QObject *, QEvent *)> eventProcessor;
public:
    GenericEventFilter(QObject* parent = 0);
    virtual ~GenericEventFilter();
    void SetEventProcessor(std::function<bool(QObject *, QEvent *)>);
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};
#endif // GENERICEVENTFILTER_H
