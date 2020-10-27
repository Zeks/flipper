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


private:
    Ui::FandomListWidget *ui;

    TreeModel* treeModel = nullptr;
    QMenu* treeItemMenu = nullptr;
    std::shared_ptr<TreeItemInterface> rootItem;
    std::shared_ptr<ListControllerType> listItemController;
    std::shared_ptr<FandomControllerType> fandomItemController;
    CustomIconDelegate* dummyDelegate;


private slots:
    void OnTreeItemDoubleClicked(const QModelIndex& index);
    void OnTreeItemChecked(const QModelIndex& index);
};


