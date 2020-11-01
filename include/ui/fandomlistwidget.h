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
    enum EFilterRestorationMode{
        frm_full = 0,
        frm_token_state = 1,
    };
    explicit FandomListWidget(QWidget *parent = nullptr);
    ~FandomListWidget();
    void SetupItemControllers();
    void InitTree();
    void InitButtonConnections();
    void InitFandomList(QStringList);
    void CreateContextMenus();
    // context menu controls
    void AddNewList();
    bool ShowDeleteListConfirmation();
    void DeleteListUnderCursor();
    void DeleteCurrentListInCombobox();
    void RenameListUnderCursor();
    void DeleteFandomUnderCursor();
    void SetFandomToCombobox(const QString&);
    QString GetCurrentlySelectedFandom();
    std::shared_ptr<TreeItemInterface> CreateFandomListToken();

    void RestoreFandomListToken(std::shared_ptr<TreeItemInterface>, EFilterRestorationMode restoreMode = EFilterRestorationMode::frm_token_state);
    std::unordered_map<int,core::fandom_lists::FandomSearchStateToken> GetStateForSearches();
    static std::unordered_map<int,core::fandom_lists::FandomSearchStateToken> GetStateForSearchesFromTreeItem(std::shared_ptr<TreeItemInterface> item);

    QSharedPointer<CoreEnvironment> env;
private:
    std::shared_ptr<TreeItemInterface> FetchAndConvertFandomLists();
    void ScrollToFandom(std::shared_ptr<TreeItemInterface>, uint32_t id);
    bool IsFandomInList(std::shared_ptr<TreeItemInterface>, uint32_t);
    void AddFandomToList(std::shared_ptr<TreeItemInterface>, uint32_t, core::fandom_lists::EInclusionMode mode);
    QModelIndex FindIndexForPath(QStringList);
    void ReloadModel();
    void CollapseTree();

    Ui::FandomListWidget *ui;

    TreeModel* treeModel = nullptr;

    std::unique_ptr<QMenu> noItemMenu;
    std::unique_ptr<QMenu> listItemMenu;
    std::unique_ptr<QMenu> fandomItemMenu;

    std::shared_ptr<TreeItemInterface> rootItem;
    std::shared_ptr<TreeItemInterface> ignoresItem;
    std::shared_ptr<TreeItemInterface> whitelistItem;
    std::shared_ptr<ListControllerType> listItemController;
    std::shared_ptr<FandomControllerType> fandomItemController;
    CustomIconDelegate* modeDelegate;
    CustomIconDelegate* crossoverDelegate;

    QModelIndex clickedIndex;


private slots:
    void OnTreeItemDoubleClicked(const QModelIndex& index);
    void OnTreeItemSelectionChanged(const QModelIndex& oldIndex, const QModelIndex& newIndex);
    void OnTreeItemChecked(const QModelIndex& index);
    void OnContextMenuRequested(const QPoint &pos);
    // button functions
    void OnIgnoreCurrentFandom();
    void OnWhitelistCurrentFandom();
    void OnAddCurrentFandomToList();
    void OnCheckFandomsText(const QString&);
    //void OnFandomActivatedInCombobox(int);
    void OnCheckFandomListText(const QString&);
};


