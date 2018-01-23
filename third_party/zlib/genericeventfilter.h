#ifndef GENERICEVENTFILTER_H
#define GENERICEVENTFILTER_H
#include <functional>
#include <QWidget>

class  GenericEventFilter: public QObject
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

