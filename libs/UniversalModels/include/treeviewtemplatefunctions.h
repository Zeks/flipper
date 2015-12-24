#ifndef TREEVIEWTEMPLATEFUNCTIONS_H
#define TREEVIEWTEMPLATEFUNCTIONS_H
#include <QAbstractItemView>
#include <QTreeView>
#include "include/TreeModel.h"
#include "include/TreeItemInterface.h"
#include "include/treeviewfunctions.h"
#include "include/treeitemfunctions.h"
#include "include/l_tree_controller_global.h"
#include <type_traits>
//class QAbstractItemView;
template <typename T>
struct has_interface_pointer
{
    static const bool value = true;
};
namespace TreeFunctions
{
template <typename DataType, typename InterfaceType>
typename std::enable_if<has_interface_pointer<DataType>::value, void>::type AssignInterfacePointer(DataType& data, InterfaceType* interface)
{
    data.SetInterfacePointer(interface);
}
template <typename DataType, typename InterfaceType>
typename std::enable_if<!has_interface_pointer<DataType>::value, void>::type AssignInterfacePointer(DataType& , InterfaceType* )
{
    //data.SetInterfacePointer(interface);
}


/* Создаёт структуру интерфейса дерева из предварительно созданного объекта данных
 * @parentItem объект к которому следует прицепить данный интерфейс как потомка
 * @data готовая к обработке структура с данными
 * @controller указатель на структуру доступа к данным из модели
 **/
template<typename DataType, typename InterfaceType, template <typename> class ItemType, template <typename> class ControllerType>
QSharedPointer<InterfaceType>  CreateInterfaceFromData(QSharedPointer<InterfaceType> parentItem, DataType& data,
                                                       QSharedPointer<ControllerType<DataType> > controller)
{
    ItemType<DataType>* pointer = new ItemType<DataType>();
    QSharedPointer<InterfaceType > newItem(pointer);
    pointer->SetInternalData(data);
    pointer->SetController(controller);
    pointer->SetParent(parentItem);

    AssignInterfacePointer(data, newItem.data());

    //data.SetInterfacePointer(newItem.data());
    return newItem;
}
/* Возвращет из отображения указатель на текущий интерфейс
 * @view указатель на отображение из которого извлекается интерфейс
 **/
template<typename InterfaceType>
InterfaceType* GetCurrentInterface(QAbstractItemView* view)
{
    auto currentIndex = view->selectionModel()->currentIndex();
    if(!currentIndex.isValid())
        return nullptr;
    auto pointer = static_cast<InterfaceType*>(currentIndex.internalPointer());
    return pointer;
}
/* Возвращет из отображения указатель на текущие данные
 * @view указатель на отображение из которого извлекаются данные
 **/
template<template<typename> class TreeItemType, typename DataType, typename InterfaceType>
DataType* GetCurrentDataPointer(QAbstractItemView* view)
{
    TreeItemType<DataType> *pointer = dynamic_cast<TreeItemType<DataType>* >(GetCurrentInterface<InterfaceType>(view));
    DataType* data = static_cast<DataType*>(pointer->InternalPointer());
    return data;
}



/* Возвращает родителя для создаваемого потомка на основании режима вставки
 * @insertionMode Информация о том как именно вставляется потомок. Нодой или папкой
 * @current указатель на интерфейс являющийся текущим в модели
 * @root указатель на корневую ноду (для сличения с корнем дерева)
 **/
template<template<typename> class TreeItemType, typename DataType, typename InterfaceType>
QSharedPointer<InterfaceType> GetParent(ENodeInsertionMode insertionMode, InterfaceType* current, QSharedPointer<InterfaceType> sharedOfRoot)
{
    InterfaceType* parent = current->parent();
    bool parentIsRoot = parent == sharedOfRoot.data();
    QSharedPointer<InterfaceType> sharedOfParent;
    if(insertionMode == ENodeInsertionMode::sibling)
    {
        if(parentIsRoot)
            return sharedOfRoot;
        else
        {
            TreeItemType<DataType>* parentOfParent = dynamic_cast<TreeItemType<DataType>*>(parent->parent());
            sharedOfParent = parent->parent()->GetChildren().at(parentOfParent->IndexOf(parent));
        }
    }
    else
    {
        sharedOfParent = parent->GetChildren().at(parent->IndexOf(current));
    }
    return sharedOfParent;

}

/* Возвращает индекс строки для создания нового потомка в модели
 * @view указатель на отображение из которого извлекаются данные
 * @insertionMode Информация о том как именно вставляется потомок. Нодой или папкой
 * @addBelow указывает вставить потомка перед текущим индексом или после него
 **/
template<typename InterfaceType>
static inline int GetChildInsertionIndex(InterfaceType* iface, ENodeInsertionMode insertionMode, bool addBelow)
{
    int rowCount;
    if(insertionMode == ENodeInsertionMode::child)
    {
        rowCount = iface->childCount();
        return rowCount - 1;
    }
    if(!iface->parent())
        return 0;
    rowCount = iface->parent()->childCount();
    int indexOfCurrentNode = iface->parent()->GetIndexOfChild(iface);
    int indexToInsert;
    if(addBelow)
    {
        if(indexOfCurrentNode + 1 < rowCount)
            indexToInsert = indexOfCurrentNode + 1;
        else
            indexToInsert = indexOfCurrentNode;

    }
    else
        indexToInsert = indexOfCurrentNode;
    return indexToInsert;
}

/* Создаёт потомка или сиблинга для текущей ноды в дереве (берется из view)
 * @dataGenerator генератор для создания данных для помещения в интерфейс
 * @view указатель на отображение
 * @root указатель на корневую ноду (для сличения с корнем дерева)
 * @insertionMode Информация о том как именно вставляется потомок. Нодой или папкой
 * @addBelow указывает вставить потомка перед текущим индексом или после него
 **/
template<template<typename> class TreeItemType, typename DataType, typename InterfaceType>
TreeItemType<DataType>* AddItem(std::function<DataType(DataType*)> dataGenerator, QAbstractItemView* view,
                                QSharedPointer<InterfaceType> sharedOfRoot,
                                InterfaceType* root, ENodeInsertionMode insertionMode, bool addBelow = true)
{
    bool parentIsRoot = false;
    auto current = GetCurrentInterface<InterfaceType>(view);
    QSharedPointer<InterfaceType> parent;
    DataType* parentData;
    if(!current)
    {
        current = root;
        parentIsRoot = true;
        root = sharedOfRoot.data();
        parent = sharedOfRoot;
    }
    else
    {
        parent = GetParent<TreeItemType, DataType, InterfaceType>
            (insertionMode, current, sharedOfRoot);
        parentIsRoot = parent.data() == root;

        TreeItemType<DataType>* parentItem = dynamic_cast<TreeItemType<DataType>*>(parent.data());
        parentData = static_cast<DataType*>(parentItem->InternalPointer());
    }

    DataType data = dataGenerator(parentIsRoot ? nullptr : parentData);


    TreeItemType<DataType>* newTemplateItem = new TreeItemType<DataType>();
    QSharedPointer<InterfaceType > newItem(newTemplateItem);
    newTemplateItem->SetInternalData(data);
    newTemplateItem->SetController(dynamic_cast<TreeItemType<DataType>*>(parent.data())->GetController());
    newTemplateItem->SetParent(parent);

    data.SetInterfacePointer(newItem.data());

    int indexToInsert = GetChildInsertionIndex(root, insertionMode, addBelow);;

    if(indexToInsert == -1)
        indexToInsert = 0;
    parent->AddChildren(QList<QSharedPointer<InterfaceType> >() << newItem, indexToInsert);

    TreeModel* model = dynamic_cast<TreeModel*>(view->model());
    //model->InsertRootItem(root);
    model->Refresh();
    return dynamic_cast<TreeItemType<DataType>*>(newItem.data());
}


/* Производит фильтрацию в дереве и устанавливает результат в качестве корневой ноды
 * @dataGenerator генератор для создания данных для помещения в интерфейс
 * @treeView указатель на дерево-отображение
 * @rootItem указатель на корневую ноду (для сличения с корнем дерева)
 * @createCheckListFunc фуннкция создающая необходимые для фильтрации проверки
 * @expandedNodes ссылка на хранилице открытых нод дерева
 * @isFilteredTree ссылка на хранилище состояние фильтрованности дерева
 **/

template<typename InterfaceType, template <typename> class ItemType, typename DataType>
bool PerformFiltering(TreeModel* treeModel,
                      QSharedPointer<InterfaceType > rootItem,
                      std::function<QList<std::function<bool (InterfaceType *)> > ()> createCheckListFunc)
{
    QSharedPointer<InterfaceType > item;
    QList<std::function<bool (InterfaceType *)> > checkList = createCheckListFunc();
    if(checkList.isEmpty())
        item = rootItem;
    else
        item = TreeFunctions::FilterSubset<InterfaceType,ItemType,DataType>(rootItem, checkList);
    treeModel->InsertRootItem(item);
    if(item == rootItem)
        return false;
    return true;
}

template<typename InterfaceType, template <typename> class ItemType, typename DataType>
static inline QString GetPathFromIndex(std::function<QVariant(DataType*)> dataAccessor, InterfaceType* pointer)
{
    QString path;
    if(pointer)
    {
        DataType* data = static_cast<DataType*>(pointer->InternalPointer());
        //dataPointer = dynamic_cast<ItemType<DataType>* >(pointer);
        if(!data)
            return "";

        QString newPath = dataAccessor(data).toString() + QObject::tr(",");
        path+=newPath;
    }
    else
    {
        path+=QObject::tr("-11111,");
    }
    return path;
}

template<typename InterfaceType, template <typename> class ItemType, typename DataType>
static inline QString GetFullPathFromIndex(std::function<QVariant(DataType*)> dataAccessor, QModelIndex startIndex)
{
    QString result;
    if(!startIndex.isValid())
        return QString();
    if(startIndex.parent().isValid())
        result.append(GetFullPathFromIndex<InterfaceType,ItemType,DataType>(dataAccessor, startIndex.parent()));
    return result + GetPathFromIndex<InterfaceType,ItemType,DataType>(dataAccessor, startIndex);
}



template<typename InterfaceType>
bool ExpandAllSatisfying(std::function<bool(InterfaceType*)> check,
                              QTreeView * view,
                              QAbstractItemModel * model,
                              const QModelIndex startIndex)
{
    bool returnResult = false;
    for(int i(0); i < model->rowCount(startIndex); ++i)
    {
        QModelIndex child = model->index(i, 0, startIndex);
        if(ExpandAllSatisfying<InterfaceType>
                (check, view, model, child))
        {
            view->setExpanded( startIndex.sibling(startIndex.row(), 0), true );
            returnResult = true;
        }
    }
    if(returnResult || check(static_cast<InterfaceType*>(startIndex.internalPointer())))
        returnResult = true;
    return returnResult;
}


/* Рекурсивно сохраняет состояние нод дерева
 * @nodes список открытых нод. Заполняется как последовательная строка строка-колонка...
 * @view отображение в котором требуется сохранить состояние
 * @model указатель на модель
 * @startIndex индекс для детей которого сохраняется состояние
 * @path текущий путь вверх по дереву
 **/
template<typename InterfaceType, template <typename> class ItemType, typename DataType>
void StoreNodePathExpandState(std::function<QVariant(DataType*)> dataAccessor,
                              QStringList & nodes,
                              QTreeView * view,
                              QAbstractItemModel * model,
                              const QModelIndex& startIndex,
                              QString path = QString())
{
    // Bouml preserved body begin 002020AA

    path+=GetPathFromIndex<InterfaceType,ItemType,DataType>(dataAccessor,
                                                            startIndex.isValid()
                                                            ? static_cast<InterfaceType*>(startIndex.internalPointer()) : nullptr);
    //qDebug() << path;
    for(int i(0); i < model->rowCount(startIndex); ++i)
    {
        QModelIndex child = model->index(i, 0, startIndex);
        bool childValid = child.isValid();
        if(!childValid)
            continue;
        //InterfaceType* data = static_cast<InterfaceType*>(child.internalPointer());
//        if(!view->isExpanded(child))
//        {
//            QString nextPath = path + GetPathFromIndex<InterfaceType,ItemType,DataType>(dataAccessor, data);
//            nodes.removeAll(nextPath);
//            continue;
//        }
        StoreNodePathExpandState<InterfaceType,ItemType, DataType>
                (dataAccessor,nodes, view, model, model->index(i, 0, startIndex), path);
    }
    //QLOG_DEBUG() << view->isExpanded(startIndex);
    if(view->isExpanded(startIndex))
    {
        if(!nodes.contains(path))
            nodes << path;
    }
    // Bouml preserved body end 002020AA
}

/* Рекурсивно восстанавливает состояние нод дерева
 * @nodes список открытых нод. Заполняется как послеовательная строка строка-колонка...
 * @view отображение в котором требуется восстановить состояние
 * @model указатель на модель
 * @startIndex индекс для детей которого восстанавливается состояние
 * @path текущий путь вверх по дереву
 **/
//template<template<typename> class TreeItemType, typename DataType, typename InterfaceType>
//DataType* GetCurrentDataPointer(QAbstractItemView* view)

template<typename InterfaceType, template <typename> class ItemType, typename DataType>
void ApplyNodePathExpandState(std::function<QVariant(DataType*)> dataAccessor,
                              QStringList & nodes,
                              QTreeView * view,
                              QAbstractItemModel * model,
                              const QModelIndex& startIndex,
                              QString path = QString())
{
    path+=GetPathFromIndex<InterfaceType,ItemType,DataType>(dataAccessor, startIndex.isValid()
                                                            ? static_cast<InterfaceType*>(startIndex.internalPointer()) : nullptr);
    for(int i(0); i < model->rowCount(startIndex); ++i)
    {
        QModelIndex child = model->index(i, 0, startIndex);
        QString nextPath = path + GetPathFromIndex<InterfaceType,ItemType,DataType>(dataAccessor,child.isValid()
                                                                                    ? static_cast<InterfaceType*>(child.internalPointer()) : nullptr);
        if(!nodes.contains(nextPath))
            continue;
        ApplyNodePathExpandState<InterfaceType,ItemType, DataType>
                (dataAccessor, nodes, view, model, child, path);
    }
    if(nodes.contains(path))
        view->setExpanded( startIndex.sibling(startIndex.row(), 0), true );
}


template<typename InterfaceType, template <typename> class ItemType, typename DataType>
void FilterTreeAndRestoreNodes(std::function<QVariant(DataType*)> dataAccessor,
                               std::function<QList<std::function<bool (InterfaceType *)> > ()> createCheckListFunc,
                               QStringList & nodes,
                               QTreeView * view,
                               TreeModel * model,
                               QSharedPointer<InterfaceType > startIndex)
{
    TreeFunctions::StoreNodePathExpandState<InterfaceType,ItemType,DataType>(dataAccessor, nodes, view, model, QModelIndex());
    PerformFiltering<InterfaceType,ItemType,DataType>(model,startIndex,createCheckListFunc);
    TreeFunctions::ApplyNodePathExpandState<InterfaceType,ItemType,DataType>(dataAccessor, nodes, view, model, QModelIndex());
}


/* Рекурсивно сохраняет состояние нод дерева
 * @nodes список открытых нод. Заполняется как последовательная строка строка-колонка...
 * @view отображение в котором требуется сохранить состояние
 * @model указатель на модель
 * @startIndex индекс для детей которого сохраняется состояние
 * @path текущий путь вверх по дереву
 **/
template<typename InterfaceType, template <typename> class ItemType, typename DataType>
void StoreNodePathState(std::function<QVariant(DataType*)> dataAccessor,
                        std::function<bool(InterfaceType* )> stateChecker,
                              QStringList & nodes,
                              QTreeView * view,
                              QAbstractItemModel * model,
                              const QModelIndex startIndex,
                              QString path = QString())
{
    // Bouml preserved body begin 002020AA
    path+=GetPathFromIndex<InterfaceType,ItemType,DataType>(dataAccessor,
                                                            startIndex.isValid()
                                                            ? static_cast<InterfaceType*>(startIndex.internalPointer()) : nullptr);
    //qDebug() << path;
    int rowCount = model->rowCount(startIndex);
    for(int i(0); i < rowCount; ++i)
    {
        QModelIndex child = model->index(i, 0, startIndex);

        if(!child.isValid())
            continue;
        StoreNodePathState<InterfaceType,ItemType, DataType>
                (dataAccessor,stateChecker, nodes, view, model, model->index(i, 0, startIndex), path);
    }
    InterfaceType* pointer = static_cast<InterfaceType*>(startIndex.internalPointer());
    if(startIndex.isValid() && stateChecker(pointer))
    {
        if(!nodes.contains(path))
            nodes << path;
    }
    // Bouml preserved body end 002020AA
}

/* Рекурсивно восстанавливает состояние нод дерева
 * @nodes список открытых нод. Заполняется как послеовательная строка строка-колонка...
 * @view отображение в котором требуется восстановить состояние
 * @model указатель на модель
 * @startIndex индекс для детей которого восстанавливается состояние
 * @path текущий путь вверх по дереву
 **/
//template<template<typename> class TreeItemType, typename DataType, typename InterfaceType>
//DataType* GetCurrentDataPointer(QAbstractItemView* view)

template<typename InterfaceType, template <typename> class ItemType, typename DataType>
void ApplyNodePathState(std::function<QVariant(DataType*)> dataAccessor,
                        std::function<void(const QModelIndex&)> stateSetter,
                              QStringList nodes,
                              QTreeView * view,
                              QAbstractItemModel * model,
                              const QModelIndex startIndex,
                              QString path = QString())
{
    path+=GetPathFromIndex<InterfaceType,ItemType,DataType>(dataAccessor,
                                                            startIndex.isValid()
                                                            ? static_cast<InterfaceType*>(startIndex.internalPointer()) : nullptr);
    for(int i(0); i < model->rowCount(startIndex); ++i)
    {
        QModelIndex child = model->index(i, 0, startIndex);
        InterfaceType* childPointer = static_cast<InterfaceType*>(child.internalPointer());
        QString nextPath = path + GetPathFromIndex<InterfaceType,ItemType,DataType>(dataAccessor,childPointer);
        bool hasPath = false;
        for(auto path : nodes)
        {
            if(path.contains(nextPath))
                hasPath = true;
        }
        if(!hasPath)
            continue;
        ApplyNodePathState<InterfaceType,ItemType, DataType>
                (dataAccessor, stateSetter, nodes, view, model, child, path);
    }
    if(nodes.contains(path))
        stateSetter(startIndex);
}


}
#endif // TREEVIEWTEMPLATEFUNCTIONS_H
