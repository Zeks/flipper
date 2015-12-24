#include "include/treeviewfunctions.h"
#include <QTreeView>
#include <functional>
namespace TreeFunctions
{
void ApplyNodePathExpandState(QStringList & nodes,
                      QTreeView * view,
                      QAbstractItemModel * model,
                      const QModelIndex startIndex,
                      QString path)
{
    // Bouml preserved body begin 00201FAA
    path+=QString::number(startIndex.row()) + QString::number(startIndex.column());
    for(int i(0); i < model->rowCount(startIndex); ++i)
    {
        QModelIndex nextIndex = model->index(i, 0, startIndex);
        QString nextPath = path + QString::number(nextIndex.row()) + QString::number(nextIndex.column());
        if(!nodes.contains(nextPath))
            continue;
        ApplyNodePathExpandState(nodes, view, model, nextIndex, path);
    }
    if(nodes.contains(path))
        view->setExpanded( startIndex.sibling(startIndex.row(), 0), true );
    // Bouml preserved body end 00201FAA
}

void StoreNodePathExpandState(QStringList & nodes,
                      QTreeView * view,
                      QAbstractItemModel * model,
                      const QModelIndex startIndex,
                      QString path)
{
    // Bouml preserved body begin 002020AA
    path+=QString::number(startIndex.row()) + QString::number(startIndex.column());
    for(int i(0); i < model->rowCount(startIndex); ++i)
    {
        if(!view->isExpanded(model->index(i, 0, startIndex)))
            continue;
        StoreNodePathExpandState(nodes, view, model, model->index(i, 0, startIndex), path);
    }

    if(view->isExpanded(startIndex))
        nodes << path;
    // Bouml preserved body end 002020AA
}


}
