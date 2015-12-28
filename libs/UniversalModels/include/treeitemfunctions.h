// *************************************************************************
//
// Copyright 2012-2013 Nikolai Marchenko.
//
// This file is part of the Douml Uml Toolkit.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License Version 3.0 as published by
// the Free Software Foundation and appearing in the file LICENSE.GPL included in the
//  packaging of this file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License Version 3.0 for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// e-mail : doumleditor@gmail.com
//
// *************************************************************************
#ifndef TREEITEMFUNCTIONS_H
#define TREEITEMFUNCTIONS_H
#include "include/TreeItemInterface.h"
namespace TreeFunctions
{
template<typename K>
using RootChecks = QList<std::function<bool(K*)> >;
template<typename K>
using ModifierFunction = std::function<void(K*)>;
template<typename K>
using ModifierFunctions = QList<ModifierFunction<K> >;


/* Отфильтровывает из дерева ноды которые удовлетворяют условиям проверок
 * @rootItem верхний уровень дерева от которого производится фильтрация
 * @filterFunctions список проверок которым должна удовлетворять нода
 * @forcePick говорит о том, что ноду нужно зацепить в любом случае(когда нода-папка удовлетворяет условию)
 **/
template<typename InterfaceType, typename ConcreteItemType>
QSharedPointer<InterfaceType> RecursiveGetSubset(InterfaceType* rootItem, RootChecks<InterfaceType> filterFunctions, bool forcePick = false)
{
    QList<QSharedPointer<InterfaceType>> newChildren;
    bool nodeValid = true;
    if(!forcePick)
    {
        for(std::function<bool(InterfaceType*)> filter: filterFunctions)
        {
            if(!filter(rootItem))
            {
                nodeValid = false;
                break;
            }
        }
    }
    if(!rootItem->GetChildren().isEmpty())
    {
        for(QSharedPointer<InterfaceType> child: rootItem->GetChildren())
        {
            QSharedPointer<InterfaceType> newChild = RecursiveGetSubset<InterfaceType,ConcreteItemType>(child.data(), filterFunctions, nodeValid);
            if(!newChild.isNull())
                newChildren += newChild;
        }
    }
    QSharedPointer<InterfaceType> newItem;
    bool hasValidDescendants = !newChildren.isEmpty();
    if(nodeValid || hasValidDescendants || forcePick)
    {
        newItem = QSharedPointer<InterfaceType>(new ConcreteItemType(*dynamic_cast<ConcreteItemType*>(rootItem)));
        newItem->removeChildren(0, newItem->childCount());
        for(QSharedPointer<InterfaceType> child: newChildren)
        {
            child->SetParent(newItem);
        }
        newItem->AddChildren(newChildren);
    }
    return newItem;
}


template<typename InterfaceType>
QList<InterfaceType*> FlattenHierarchy(InterfaceType* rootItem, RootChecks<InterfaceType> filterFunctions, bool forcePick = false)
{
    QList<InterfaceType* > result;

    QList<QSharedPointer<InterfaceType>> newChildren;
    bool nodeValid = true;
    for(std::function<bool(InterfaceType*)> filter: filterFunctions)
    {
       if(filter(rootItem))
       {
           nodeValid = false;
           break;
       }
    }
    if(!rootItem->GetChildren().isEmpty())
    {
        for(QSharedPointer<InterfaceType> child: rootItem->GetChildren())
        {
            if(!child.isNull())
                result.append(FlattenHierarchy<InterfaceType>(child.data(), filterFunctions, nodeValid));
        }
    }
    if(rootItem != 0 && (nodeValid || forcePick))
    {
        result+=rootItem;
    }
    return result;
}


template<typename InterfaceType>
bool RecursiveSearch(InterfaceType* rootItem, RootChecks<InterfaceType> filterFunctions)
{
    bool result = true;
    for(std::function<bool(InterfaceType*)> filter: filterFunctions)
    {
       if(!filter(rootItem))
       {
           result = false;
           break;
       }
    }
    if(result)
        return true;
    if(!rootItem->GetChildren().isEmpty())
    {
        for(QSharedPointer<InterfaceType> child: rootItem->GetChildren())
        {
            bool result = RecursiveSearch<InterfaceType>(child.data(), filterFunctions);
            if(result)
                return true;
        }
    }
    return false;
}

template<typename InterfaceType>
void ModifyNode(InterfaceType* node, ModifierFunctions<InterfaceType> modifierFunctions)
{
    if(!node)
        return;
    for(ModifierFunction<InterfaceType> func : modifierFunctions)
    {
        func(node);
    }

}

template<typename InterfaceType>
bool RecursiveModify(InterfaceType* rootItem, RootChecks<InterfaceType> filterFunctions, ModifierFunctions<InterfaceType> modifierFunctions,
                     bool parentsFirst = true, bool stopAtFirstMatch = true )
{
    bool result = false;
    bool nodeValid = true;
    if(!rootItem)
        return false;
    for(std::function<bool(InterfaceType*)> filter: filterFunctions)
    {
       if(!filter(rootItem))
       {
           nodeValid = false;
           break;
       }
    }
    if(nodeValid && parentsFirst)
    {
        result = true;
        ModifyNode<InterfaceType>(rootItem,modifierFunctions);
        if(stopAtFirstMatch)
            return result;
    }

    if(!rootItem->GetChildren().isEmpty())
    {
        for(QSharedPointer<InterfaceType> child: rootItem->GetChildren())
        {
            result = RecursiveModify<InterfaceType>(child.data(), filterFunctions, modifierFunctions, parentsFirst, stopAtFirstMatch);
            if(stopAtFirstMatch)
                return result;
        }
    }
    if(nodeValid && !parentsFirst)
    {
        result = true;
        ModifyNode<InterfaceType>(rootItem,modifierFunctions);
        if(stopAtFirstMatch)
            return result;
    }
    return result;
}




/* Отфильтровывает из дерева список свойств по переданному аксессору
 * @rootItem верхний уровень дерева от которого производится фильтрация
 * @accessor функция для доступа к конкретному члену структуры который надо выцепить
 **/
template<typename K, typename T>
QList<K> RecursiveGet(T* rootItem, std::function<K(T*)> accessor)
{
    QList<K> result;
    if(!rootItem)
        return result;
    if(!rootItem->GetChildren().isEmpty())
    {
        for(QSharedPointer<T> child: rootItem->GetChildren())
        {
            result += RecursiveGet(child.data(), accessor);
        }
    }
    result.append(accessor(rootItem));
    return result;

}
/* Отфильтровывает из дерева ноды которые удовлетворяют условиям проверок
 * и помещает их в интерфейс для последующего запихивания в дерево
 *
 * @rootItem верхний уровень дерева от которого производится фильтрация
 * @filterFunctions список проверок которым должна удовлетворять нода
 **/
template<typename InterfaceType, template <typename> class ItemType, typename DataType>
QSharedPointer<InterfaceType > FilterSubset(QSharedPointer<InterfaceType> rootItem, QList<std::function<bool(InterfaceType*)> > filterFunctions)
{
    QSharedPointer<InterfaceType > item;
    ItemType<DataType>* newRoot = new ItemType<DataType>();
    item = QSharedPointer<InterfaceType >(newRoot);

    item = TreeFunctions::RecursiveGetSubset<InterfaceType, ItemType<DataType>>(rootItem.data(), filterFunctions);
    return item;
}

};
#endif // TREEITEMFUNCTIONS_H
