#include "include/genericeventfilter.h"
GenericEventFilter::GenericEventFilter(QObject* parent):QObject(parent)
{

}
GenericEventFilter::~GenericEventFilter()
{}

bool GenericEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if(eventProcessor != 0)
    {
        if(eventProcessor(obj, event))
        {
            event->accept();
            return true;
        }
        else return false;
    }
    else
        return QObject::eventFilter(obj, event);
}
void GenericEventFilter::SetEventProcessor(std::function<bool(QObject *, QEvent *)> processor)
{
    eventProcessor = processor;
}
