#pragma once

#include <QWidget>
#include "ui-models/include/TreeModel.h"
#include "ui-models/include/ItemController.h"
#include "core/fandom_list.h"
#include <QMenu>

namespace Ui {
class FandomListWidget;
}

class CustomIconDelegate;
class CoreEnvironment;

class FandomListWidget : public QWidget
{
    Q_OBJECT
    using ListControllerType = ItemController<core::fandom_lists::List>;
    using FandomControllerType = ItemController<core::fandom_lists::FandomStateInList>;
public:
    explicit FandomListWidget(QWidget *parent = nullptr);
    ~FandomListWidget();
    void SetupItemControllers();
    void InitTree();
    void CreateContextMenus();
    // context menu controls
    void AddNewList();
    void DeleteListUnderCursor();
    void RenameListUnderCursor();
    void DeleteFandomUnderCursor();


    QSharedPointer<CoreEnvironment> env;
private:
    std::shared_ptr<TreeItemInterface> FetchAndConvertFandomLists();
    Ui::FandomListWidget *ui;

    TreeModel* treeModel = nullptr;

    std::unique_ptr<QMenu> noItemMenu;
    std::unique_ptr<QMenu> listItemMenu;
    std::unique_ptr<QMenu> fandomItemMenu;

    std::shared_ptr<TreeItemInterface> rootItem;
    std::shared_ptr<ListControllerType> listItemController;
    std::shared_ptr<FandomControllerType> fandomItemController;
    CustomIconDelegate* modeDelegate;
    CustomIconDelegate* crossoverDelegate;
    CustomIconDelegate* dummyDelegate;

    QModelIndex clickedIndex;


private slots:
    void OnTreeItemDoubleClicked(const QModelIndex& index);
    void OnTreeItemChecked(const QModelIndex& index);
    void OnContextMenuRequested(const QPoint &pos);
};


