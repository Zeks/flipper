#ifndef TREEFUNCTIONS_H
#define TREEFUNCTIONS_H
#include <QList>
#include <QModelIndex>
#include <QAbstractItemView>
#include "l_tree_controller_global.h"


class QTreeView;
namespace TreeFunctions
{
enum class ENodeInsertionMode
{
    sibling,
    child
};


/*
 * Информация о том как именно вставляется потомок. Нодой или папкой
 **/


/* Рекурсивно восстанавливает состояние нод дерева
 * @nodes список открытых нод. Заполняется как послеовательная строка строка-колонка...
 * @view отображение в котором требуется восстановить состояние
 * @model указатель на модель
 * @startIndex индекс для детей которого восстанавливается состояние
 * @path текущий путь вверх по дереву
 **/
L_TREE_CONTROLLER_EXPORT void ApplyNodePathExpandState(QStringList & nodes, QTreeView * view, QAbstractItemModel * model, const QModelIndex startIndex, QString path = QString());
/* Рекурсивно сохраняет состояние нод дерева
 * @nodes список открытых нод. Заполняется как последовательная строка строка-колонка...
 * @view отображение в котором требуется сохранить состояние
 * @model указатель на модель
 * @startIndex индекс для детей которого сохраняется состояние
 * @path текущий путь вверх по дереву
 **/
L_TREE_CONTROLLER_EXPORT void StoreNodePathExpandState(QStringList & nodes, QTreeView * view, QAbstractItemModel * model, const QModelIndex startIndex, QString path = QString());
}
#endif // TREEFUNCTIONS_H
