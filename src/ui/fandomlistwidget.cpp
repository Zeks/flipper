#include "ui/fandomlistwidget.h"
#include "ui_fandomlistwidget.h"
#include "ui-models/include/custom_icon_delegate.h"
#include "ui-models/include/TreeItem.h"
#include "ui-models/include/treeitemfunctions.h"
#include "ui-models/include/treeviewtemplatefunctions.h"
#include "Interfaces/fandom_lists.h"
#include "Interfaces/fandoms.h"
#include "GlobalHeaders/snippets_templates.h"
#include "environment.h"
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QInputDialog>
#include <QMessageBox>
#include <QStringListModel>
#include <vector>

static QRect CheckBoxRect(const QStyleOptionViewItem &view_item_style_options) {
    QStyleOptionButton check_box_style_option;
    QRect check_box_rect = QApplication::style()->subElementRect(
                QStyle::SE_CheckBoxIndicator,
                &check_box_style_option);
    QPoint check_box_point(view_item_style_options.rect.x() +
                           view_item_style_options.rect.width() / 2 -
                           check_box_rect.width() / 2,
                           view_item_style_options.rect.y() +
                           view_item_style_options.rect.height() / 2 -
                           check_box_rect.height() / 2);
    return QRect(check_box_point, check_box_rect.size());
}


static const auto iconPainter = [](const QStyledItemDelegate* delegate, QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index, auto iconSelector)
{
    QVariant value = index.model()->data(index, Qt::DisplayRole);

    delegate->QStyledItemDelegate::paint(painter, option, index.parent().sibling(0,2));
    if(!value.isValid())
        return;

    auto pixmapToDraw = iconSelector(index);
    painter->drawPixmap(CheckBoxRect(option), pixmapToDraw);

};

#define MEMBER_GETTER(member) \
    using namespace core::fandom_lists; \
    auto pointer = static_cast<TreeItemInterface*>(index.internalPointer()); \
    ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer()); \
    auto getter = +[](ListBase* basePtr){return basePtr->member;};


#define MEMBER_VALUE_OR_DEFAULT(member) \
    using namespace core::fandom_lists; \
    auto pointer = static_cast<TreeItemInterface*>(index.internalPointer()); \
    ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer()); \
    using MemberType = decltype(std::declval<FandomStateInList>().member); \
    auto getter = basePtr->type == et_list ? +[](ListBase*)->MemberType{return MemberType();} : \
    +[](ListBase* basePtr){return static_cast<FandomStateInList*>(basePtr)->member;}



FandomListWidget::FandomListWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FandomListWidget)
{
    ui->setupUi(this);
    auto createDelegate = [](auto lambda){
        CustomIconDelegate* delegate = new CustomIconDelegate();
        delegate->widgetCreator = [](QWidget *) {return static_cast<QWidget*>(nullptr);};
        delegate->paintProcessor = std::bind(iconPainter,
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3,
                                             std::placeholders::_4,
                                             lambda);
        //delegate->editorEventProcessor = editorEvent;
        return delegate;
    };

    modeDelegate = createDelegate([](QModelIndex index){
            MEMBER_GETTER(inclusionMode);
            auto value = getter(basePtr);
            if(basePtr->type == et_list)
                return QPixmap(":/icons/icons/switch.png");
            if(value == im_exclude)
                return QPixmap(":/icons/icons/minus_darkened.png");
            else
                return QPixmap(":/icons/icons/plus_darkened.png");
});

    crossoverDelegate = createDelegate([](QModelIndex index){
            MEMBER_VALUE_OR_DEFAULT(crossoverInclusionMode);
            auto value = getter(basePtr);
            if(value == cim_select_all)
                return QPixmap(":/icons/icons/select_fic_crosses.png");
            else if(value == cim_select_crossovers)
                return QPixmap(":/icons/icons/shuffle_blue.png");
            else
            return QPixmap(":/icons/icons/select_fic.png");
});

    ui->tvFandomLists->setItemDelegateForColumn(0, dummyDelegate);
    ui->tvFandomLists->setItemDelegateForColumn(1, modeDelegate);
    ui->tvFandomLists->setItemDelegateForColumn(2, crossoverDelegate);
    ui->pbAddFandomToSelectedList->setEnabled(false);
    ui->pbIgnoreFandom->setEnabled(false);
    ui->pbWhitelistFandom->setEnabled(false);
    CreateContextMenus();
    InitButtonConnections();

}

FandomListWidget::~FandomListWidget()
{
    delete ui;
}

void FandomListWidget::SetupItemControllers()
{
    ListControllerType::SetDefaultTreeFlagFunctor();
    FandomControllerType::SetDefaultTreeFlagFunctor();
    listItemController.reset(new ListControllerType());
    fandomItemController.reset(new FandomControllerType());

    // first column will be inclusion mode
    // GETTERS
    static const std::vector<int> displayRoles = {0,2};
    listItemController->AddGetter(0,displayRoles, [](const core::fandom_lists::List*)->QVariant{
        return {};
    });
    fandomItemController->AddGetter(0,displayRoles, [](const core::fandom_lists::FandomStateInList*)->QVariant{
        return {};
    });
    listItemController->AddGetter(1,displayRoles, [](const core::fandom_lists::List* ptr)->QVariant{
        return static_cast<int>(ptr->inclusionMode);
    });
    fandomItemController->AddGetter(1,displayRoles, [](const core::fandom_lists::FandomStateInList* ptr)->QVariant{
        return static_cast<int>(ptr->inclusionMode);
    });
    listItemController->AddGetter(2,displayRoles, [](const core::fandom_lists::List* )->QVariant{
        return {};
    });
    fandomItemController->AddGetter(2,displayRoles, [](const core::fandom_lists::FandomStateInList* ptr)->QVariant{
        return static_cast<int>(ptr->crossoverInclusionMode);
    });
    listItemController->AddGetter(3,displayRoles, [](const core::fandom_lists::List* ptr)->QVariant{
        return ptr->name;
    });
    fandomItemController->AddGetter(3,displayRoles, [](const core::fandom_lists::FandomStateInList* ptr)->QVariant{
        return ptr->name;
    });

    listItemController->AddGetter(3, {Qt::FontRole}, [view = ui->tvFandomLists](const core::fandom_lists::List*)->QVariant{
        auto font = view->font();
        font.setWeight(60);
        return font;
    });
    fandomItemController->AddGetter(3, {Qt::FontRole}, [view = ui->tvFandomLists](const core::fandom_lists::FandomStateInList*)->QVariant{
        return view->font();
    });

    listItemController->AddGetter(1, {Qt::ToolTipRole}, [](const core::fandom_lists::List*)->QVariant{
        return "Double clicking here flips whole list between inclusion and exlusion mode.";
    });
    fandomItemController->AddGetter(1, {Qt::ToolTipRole}, [](const core::fandom_lists::FandomStateInList* fandom)->QVariant{
        QString tooltip = "Double clicking here changes inclusion/exclusion mode for this fandom.";
        if(fandom->inclusionMode == core::fandom_lists::EInclusionMode::im_include)
            tooltip +="\nCurrently active: include this fandom.";
        else
            tooltip +="\nCurrently active: exclude this fandom.";
        return tooltip;
    });
    fandomItemController->AddGetter(2, {Qt::ToolTipRole}, [](const core::fandom_lists::FandomStateInList* fandom)->QVariant{
        QString tooltip = "Double clicking here changes inclusion mode for crossovers between whole fandom, only crossovers and only non crossover.";
        if(fandom->crossoverInclusionMode == core::fandom_lists::ECrossoverInclusionMode::cim_select_all)
            tooltip +="\nCurrently active: select whole fandom including crossovers.";
        else if(fandom->crossoverInclusionMode == core::fandom_lists::ECrossoverInclusionMode::cim_select_crossovers)
            tooltip +="\nCurrently active: include only crossovers for this fandom.";
        else
            tooltip +="\nCurrently active: include only non crossovers for this fandom.";
        return tooltip;
    });


    // SETTERS
    listItemController->AddSetter(3,displayRoles, [](core::fandom_lists::List* data, QVariant value)->bool{
        data->name = value.toString();
        return true;
    });
    fandomItemController->AddSetter(3,displayRoles, [](core::fandom_lists::FandomStateInList*data, QVariant value)->bool{
        data->name = value.toString();
        return true;
    });
    listItemController->AddSetter(2,displayRoles, [](core::fandom_lists::List* data, QVariant value)->bool{
        data->inclusionMode = static_cast<core::fandom_lists::EInclusionMode>(value.toInt());
        return true;
    });
    fandomItemController->AddSetter(2,displayRoles, [](core::fandom_lists::FandomStateInList*data, QVariant value)->bool{
        data->inclusionMode = static_cast<core::fandom_lists::EInclusionMode>(value.toInt());
        return true;
    });



    fandomItemController->AddFlagsFunctor(
                    [](const QModelIndex& )
        {
            Qt::ItemFlags result;
            result |= Qt::ItemIsEnabled | Qt::ItemIsSelectable |  Qt::ItemIsUserCheckable;
            return result;
        });
    listItemController->AddFlagsFunctor(
                    [](const QModelIndex& )
        {
            Qt::ItemFlags result;
            result |= Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
            return result;
        });
}

void FandomListWidget::InitTree()
{
    treeModel = new TreeModel(this);
    SetupItemControllers();

    listItemController->SetColumns(QStringList() << "dummy" << "inclusion" << "n" << "name");
    fandomItemController->SetColumns(QStringList()<< "dummy" << "inclusion" << "crossovers" << "name");

    rootItem = FetchAndConvertFandomLists();
    ui->cbFandomLists->setCurrentText("Ignores");
    treeModel->InsertRootItem(rootItem);
    ui->tvFandomLists->setModel(treeModel);
    ui->tvFandomLists->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tvFandomLists->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tvFandomLists->setColumnWidth(0, 230);
    ui->tvFandomLists->setRootIsDecorated(true);
    ui->tvFandomLists->setTreePosition(3);
    ui->tvFandomLists->setIndentation(10);
    ui->tvFandomLists->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tvFandomLists->setExpandsOnDoubleClick(false);
    ui->tvFandomLists->header()->setMinimumSectionSize(2);
    static const int defaultSectionSize = 30;
    ui->tvFandomLists->header()->resizeSection(0, defaultSectionSize);
    ui->tvFandomLists->header()->resizeSection(1, defaultSectionSize);
    ui->tvFandomLists->header()->resizeSection(2, defaultSectionSize);
    connect(treeModel, &TreeModel::itemCheckStatusChanged, this, &FandomListWidget::OnTreeItemChecked);
    connect(ui->tvFandomLists, &QTreeView::doubleClicked, this, &FandomListWidget::OnTreeItemDoubleClicked);
    connect(ui->tvFandomLists->selectionModel(), &QItemSelectionModel::currentChanged, this, &FandomListWidget::OnTreeItemSelectionChanged);
    connect(ui->tvFandomLists, &QTreeView::customContextMenuRequested, this, &FandomListWidget::OnContextMenuRequested);
}

void FandomListWidget::InitButtonConnections()
{
    connect(ui->pbAddFandomToSelectedList, &QPushButton::clicked, this, &FandomListWidget::OnAddCurrentFandomToList);
    connect(ui->pbIgnoreFandom, &QPushButton::clicked, this, &FandomListWidget::OnIgnoreCurrentFandom);
    connect(ui->pbWhitelistFandom, &QPushButton::clicked, this, &FandomListWidget::OnWhitelistCurrentFandom);
    connect(ui->cbFandoms, &QComboBox::editTextChanged, this, &FandomListWidget::OnCheckFandomsText);
    //connect(ui->cbFandoms, qOverload<int>(&QComboBox::activated), this, &FandomListWidget::OnFandomActivatedInCombobox);
    connect(ui->cbFandomLists, &QComboBox::editTextChanged, this, &FandomListWidget::OnCheckFandomListText);

    connect(ui->pbAddNewFandomList, &QPushButton::clicked, [&]{AddNewList();});
    connect(ui->pbDeleteFandomList, &QPushButton::clicked, [&](){
        DeleteCurrentListInCombobox();
    });
}

void FandomListWidget::InitFandomList(QStringList fandomList)
{
    ui->cbFandoms->setModel(new QStringListModel(fandomList));
}

void FandomListWidget::CreateContextMenus()
{
    noItemMenu.reset(new QMenu());
    listItemMenu.reset(new QMenu());
    fandomItemMenu.reset(new QMenu());
    // setting up new item menu
    noItemMenu->addAction("New list", [&]{AddNewList();});
    listItemMenu->addAction("Rename list", [&]{RenameListUnderCursor();});
    listItemMenu->addAction("Delete list", [&]{DeleteListUnderCursor();});
    fandomItemMenu->addAction("Delete fandom", [&]{DeleteFandomUnderCursor();});

    noItemMenu->addAction("Collapse all", [&]{CollapseTree();});
    listItemMenu->addAction("Collapse all", [&]{CollapseTree();});
    fandomItemMenu->addAction("Collapse all", [&]{CollapseTree();});

}

void FandomListWidget::AddNewList()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Name selector"),
                                             tr("Choose a name for your list:"), QLineEdit::Normal,
                                             "", &ok);
    if(name.length() == 0)
        return;
    using namespace core::fandom_lists;
    auto result = env->interfaces.fandomLists->AddFandomList(name);
    if(result == -1)
        return;

    List::ListPtr list(new List());
    list->id = result;
    list->name = name;
    TreeItem<core::fandom_lists::List>* newListPointer = new TreeItem<core::fandom_lists::List>();
    newListPointer->SetInternalData(list.get());
    auto newListItem = std::shared_ptr<TreeItemInterface>(newListPointer);
    newListPointer->SetController(listItemController);
    rootItem->addChild(newListItem);
    newListPointer->SetParent(rootItem);
    ui->cbFandomLists->addItem(name);
    ui->cbFandomLists->setCurrentText(name);
    ReloadModel();
}

bool FandomListWidget::ShowDeleteListConfirmation()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "QUestion", "Do you really want to delete this fandom list?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No)
        return false;
    return true;
}

void FandomListWidget::DeleteListUnderCursor()
{
    if(!clickedIndex.isValid())
        return;

    using namespace core::fandom_lists;
    auto pointer = static_cast<TreeItemInterface*>(clickedIndex.internalPointer());
    auto parentPtr = pointer->parent();
    ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer());
    if(basePtr->name *in ("Ignores", "Whitelist")){
        QMessageBox::warning(nullptr, "Warning!", "Ignores and Whitelist cannot be deleted.");
        return;
    }
    if(!ShowDeleteListConfirmation())
        return;

    env->interfaces.fandomLists->RemoveFandomList(basePtr->id);
    parentPtr->removeChildren(pointer->row(), 1);
    ui->cbFandomLists->removeItem(ui->cbFandomLists->findText(basePtr->name));
    ReloadModel();
}

void FandomListWidget::DeleteCurrentListInCombobox()
{
    if(ui->cbFandomLists->currentText() *in ("Ignores", "Whitelist")){
        QMessageBox::warning(nullptr, "Warning!", "Ignores and Whitelist cannot be deleted.");
        return;
    }

    if(!ShowDeleteListConfirmation())
        return;

    // I need to find an index for the path
    auto index = FindIndexForPath({ui->cbFandomLists->currentText()});
    if(!index.isValid())
        return;

    using namespace core::fandom_lists;
    auto pointer = static_cast<TreeItemInterface*>(index.internalPointer());
    auto parentPtr = pointer->parent();
    ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer());
    env->interfaces.fandomLists->RemoveFandomList(basePtr->id);
    parentPtr->removeChildren(pointer->row(), 1);
    ui->cbFandomLists->removeItem(ui->cbFandomLists->findText(basePtr->name));
    ReloadModel();

}

void FandomListWidget::RenameListUnderCursor()
{
    if(!clickedIndex.isValid())
        return;

    using namespace core::fandom_lists;
    auto pointer = static_cast<TreeItemInterface*>(clickedIndex.internalPointer());
    ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer());
    if(basePtr->name *in ("Ignores", "Whitelist")){
        QMessageBox::warning(nullptr, "Warning!", "Ignores and Whitelist cannot be renamed.");
        return;
    }

    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Name selector"),
                                             tr("Choose a new name for your list:"), QLineEdit::Normal,
                                             "", &ok);
    if(name.length() == 0)
        return;

    basePtr->name = name;

    env->interfaces.fandomLists->EditListState(*static_cast<List*>(pointer->InternalPointer()));
    treeModel->Refresh();
}


//void StoreNodePathExpandState(std::function<QVariant(typename std::remove_pointer<DataType>::type *)> dataAccessor,
//                              QStringList & nodes,
//                              QTreeView * view,
//                              QAbstractItemModel * model,
//                              const QModelIndex& startIndex,
//                              QString path = QString())


void FandomListWidget::DeleteFandomUnderCursor()
{
    using namespace core::fandom_lists;
    auto pointer = static_cast<TreeItemInterface*>(clickedIndex.internalPointer());
    auto parentPtr = pointer->parent();
    ListBase* fandomPtr = static_cast<ListBase*>(pointer->InternalPointer());
    ListBase* listPtr = static_cast<ListBase*>(parentPtr->InternalPointer());
    env->interfaces.fandomLists->RemoveFandomFromList(listPtr->id, fandomPtr->id);
    parentPtr->removeChildren(pointer->row(), 1);
    ReloadModel();
}

void FandomListWidget::SetFandomToCombobox(const QString & text)
{
    ui->cbFandoms->setCurrentText(text);
}

QString FandomListWidget::GetCurrentlySelectedFandom()
{
    return ui->cbFandoms->currentText();
}

std::shared_ptr<TreeItemInterface> FandomListWidget::CreateFandomListToken()
{
    TreeItem<core::fandom_lists::List>* newRootPointer = new TreeItem<core::fandom_lists::List>();
    auto newRootItem = std::shared_ptr<TreeItemInterface>(newRootPointer);
    newRootPointer->SetController(listItemController);
    for(auto item: rootItem->GetChildren()){
        newRootItem->addChild(item->Clone(newRootItem));
    }
    return newRootItem;
}

void FandomListWidget::RestoreFandomListToken(std::shared_ptr<TreeItemInterface> token, EFilterRestorationMode restoreMode)
{
    if(restoreMode == EFilterRestorationMode::frm_full)
        rootItem = token;
    else{
        using namespace core::fandom_lists;
        using Flattened = std::unordered_map<QString, std::shared_ptr<TreeItemInterface>>;
        std::unordered_map<QString, std::shared_ptr<TreeItemInterface>> ownHash;
        std::unordered_map<QString, std::shared_ptr<TreeItemInterface>> tokenHash;
        auto flattener = [](std::shared_ptr<TreeItemInterface> topLevelItem)->Flattened{
            Flattened result;
            QString topLevelKey;
            for(auto listItem : topLevelItem->GetChildren())
            {
                ListBase* listPtr = static_cast<ListBase*>(listItem->InternalPointer());
                topLevelKey = listPtr->name;
                result[topLevelKey] = listItem;
                for(auto fandomItem : listItem->GetChildren()){
                    ListBase* fandomPtr = static_cast<ListBase*>(fandomItem->InternalPointer());
                    result[topLevelKey + fandomPtr->name] = fandomItem;
                }
            }
            return result;
        };
        auto flattenedOwnHash = flattener(rootItem);
        auto flattenedTokenHash = flattener(token);
        for(auto& [key, value]: flattenedTokenHash){
            auto it = flattenedOwnHash.find(key);
            if(it != flattenedOwnHash.end())
            {
                ListBase* basePtr = static_cast<ListBase*>(it->second->InternalPointer());
                if(basePtr->type == et_list){
                    List* ownListPtr = static_cast<List*>(it->second->InternalPointer());
                    List* tokenListPtr = static_cast<List*>(value->InternalPointer());

                    ownListPtr->isEnabled = tokenListPtr->isEnabled;
                    ownListPtr->isDefault = tokenListPtr->isDefault;
                    ownListPtr->id = tokenListPtr->id;
                    ownListPtr->name = tokenListPtr->name;
                    ownListPtr->uiIndex = tokenListPtr->uiIndex;
                    ownListPtr->type = tokenListPtr->type;
                    ownListPtr->inclusionMode = tokenListPtr->inclusionMode;
                    it->second->setCheckState(value->checkState());
                }
                else{
                    FandomStateInList* ownListPtr = static_cast<FandomStateInList*>(it->second->InternalPointer());
                    FandomStateInList* tokenListPtr = static_cast<FandomStateInList*>(value->InternalPointer());

                    ownListPtr->isEnabled = tokenListPtr->isEnabled;
                    ownListPtr->id = tokenListPtr->id;
                    ownListPtr->list_id = tokenListPtr->list_id;
                    ownListPtr->name = tokenListPtr->name;
                    ownListPtr->uiIndex = tokenListPtr->uiIndex;
                    ownListPtr->type = tokenListPtr->type;
                    ownListPtr->inclusionMode = tokenListPtr->inclusionMode;
                    ownListPtr->crossoverInclusionMode = tokenListPtr->crossoverInclusionMode;
                    it->second->setCheckState(value->checkState());
                }
            }
        }
    }
    ReloadModel();
}

std::unordered_map<int, core::fandom_lists::FandomSearchStateToken> FandomListWidget::GetStateForSearches()
{
    return GetStateForSearchesFromTreeItem(rootItem);
}

std::unordered_map<int,core::fandom_lists::FandomSearchStateToken> FandomListWidget::GetStateForSearchesFromTreeItem(std::shared_ptr<TreeItemInterface> item)
{
    // go from top to the bottom
    // lower records override upper ones (? indicate conflict)
    using namespace core::fandom_lists;
    std::unordered_map<int,core::fandom_lists::FandomSearchStateToken> result;
    for(auto list: item->GetChildren()){
        // in the list range
        if(list->checkState() == Qt::Unchecked)
            continue;
        for(auto fandom: list->GetChildren()){
            if(fandom->checkState() == Qt::Unchecked)
                continue;
            // in the fandom range
            FandomStateInList* fandomPtr = static_cast<FandomStateInList*>(fandom->InternalPointer());
            FandomSearchStateToken token;
            token.id = fandomPtr->id;
            token.inclusionMode = fandomPtr->inclusionMode;
            token.crossoverInclusionMode = fandomPtr->crossoverInclusionMode;
            result.insert_or_assign(fandomPtr->id, std::move(token));
        }
    }
    return result;
}

std::shared_ptr<TreeItemInterface> FandomListWidget::FetchAndConvertFandomLists()
{
    auto fandomListInterface = env->interfaces.fandomLists;
    auto lists = fandomListInterface->GetLoadedFandomLists();
    using namespace interfaces;
    //using ListType = decltype(std::declval<FandomLists>().GetFandomList(std::declval<QString>()));
    using namespace core::fandom_lists;
    std::vector<List::ListPtr> listVec;
    for(auto name : lists){
        auto list = fandomListInterface->GetFandomList(name);
        listVec.push_back(list);
    }
    std::sort(listVec.begin(),listVec.end(), [](const auto& i1,const auto& i2){
        return i1->id < i2->id;
    });

    TreeItem<core::fandom_lists::List>* rootPointer = new TreeItem<core::fandom_lists::List>();
    auto rootItem = std::shared_ptr<TreeItemInterface>(rootPointer);
    rootPointer->SetController(listItemController);

    for(auto list: listVec){
        auto item = TreeFunctions::CreateInterfaceFromData<core::fandom_lists::List, TreeItemInterface, TreeItem>
                (rootItem, list.get(), listItemController);
        rootPointer->addChild(item);
        if(list->isEnabled)
            item->setCheckState(Qt::Checked);
        if(list->name == "Ignores")
            ignoresItem = item;
        if(list->name == "Whitelist")
            whitelistItem = item;
        ui->cbFandomLists->addItem(list->name);

        auto fandomData = fandomListInterface->GetFandomStatesForList(list->name);
        std::sort(fandomData.begin(),fandomData.end(), [](const auto& i1,const auto& i2){
            return i1.name < i2.name;
        });
        for(auto fandomBit : fandomData)
        {
            auto fandomItem = TreeFunctions::CreateInterfaceFromData<core::fandom_lists::FandomStateInList, TreeItemInterface, TreeItem>
                    (item, fandomBit, fandomItemController);
            if(fandomBit.isEnabled)
                fandomItem->setCheckState(Qt::Checked);
            item->addChild(fandomItem);
        }
    }
    return rootItem;
}

void FandomListWidget::ScrollToFandom(std::shared_ptr<TreeItemInterface> nodeToScrollIn,uint32_t fandomId)
{
    using namespace core::fandom_lists;
    auto name = env->interfaces.fandoms->GetNameForID(fandomId);

    ListBase* basePtr = static_cast<ListBase*>(nodeToScrollIn->InternalPointer());
    auto index = FindIndexForPath({basePtr->name, name});
    if(!index.isValid())
        return;
    ui->tvFandomLists->scrollTo(index);
}

bool FandomListWidget::IsFandomInList(std::shared_ptr<TreeItemInterface> node, uint32_t fandomId)
{
    using namespace core::fandom_lists;
    for(auto child: node->GetChildren()){
        ListBase* basePtr = static_cast<ListBase*>(child->InternalPointer());
        if(basePtr->id == fandomId)
            return true;
    }
    return false;
}

void FandomListWidget::AddFandomToList(std::shared_ptr<TreeItemInterface> node, uint32_t fandomId, core::fandom_lists::EInclusionMode mode)
{
    using namespace core::fandom_lists;
    auto name = env->interfaces.fandoms->GetNameForID(fandomId);

    auto filter = [name](TreeItemInterface* interface){
        ListBase* basePtr = static_cast<ListBase*>(interface->InternalPointer());
        return basePtr->name == name;

    };
    auto listItem = TreeFunctions::FlattenHierarchy<TreeItemInterface>(node.get(),TreeFunctions::RootChecks<TreeItemInterface>{filter});
    for(auto item: listItem)
    {
        ListBase* basePtr = static_cast<ListBase*>(item->InternalPointer());
        if(basePtr->name == name){
            ScrollToFandom(node, fandomId);
            return;
        }
    }

    FandomStateInList newFandomState;
    newFandomState.id = fandomId;
    newFandomState.name = name;
    newFandomState.inclusionMode = mode;
    newFandomState.crossoverInclusionMode = ECrossoverInclusionMode::cim_select_all;

    ListBase* basePtr = static_cast<ListBase*>(node->InternalPointer());

    auto item = TreeFunctions::CreateInterfaceFromData<FandomStateInList, TreeItemInterface, TreeItem>
            (node, newFandomState, fandomItemController);
    item->setCheckState(Qt::Checked);
    node->setCheckState(Qt::Checked);

    node->addChild(item);
    auto children = node->GetChildren();
    std::sort(children.begin(),children.end(), [](const auto& i1,const auto& i2){
        return i1->data(3, Qt::DisplayRole).toString() < i2->data(3, Qt::DisplayRole).toString();
    });
    node->removeChildren();
    node->AddChildren(children);
    env->interfaces.fandomLists->AddFandomToList(basePtr->id, fandomId, name);
    env->interfaces.fandomLists->EditFandomStateForList(newFandomState);
    node->SetChildrenExclusive(Qt::Checked);
    ReloadModel();
    ScrollToFandom(node, fandomId);
}

QModelIndex FandomListWidget::FindIndexForPath(QStringList path)
{
    int currentDepth = 0;
    for(auto listIndex = 0; listIndex < treeModel->rowCount(QModelIndex()); listIndex++){
        QModelIndex currentListIndex = treeModel->index(listIndex, 3);
        //auto data = currentListIndex.data(Qt::DisplayRole).toString();
        QString sectionData = currentListIndex.data(Qt::DisplayRole).toString();
        if(currentListIndex.data(Qt::DisplayRole).toString() == path.at(currentDepth))
        {
            currentDepth++;
            if(path.size() == 1)
                return currentListIndex;
            for(auto fandomIndex = 0; fandomIndex < treeModel->rowCount(currentListIndex); fandomIndex++){
                auto currentFandomIndex = treeModel->index(fandomIndex, 3, currentListIndex);
                QString itemData = currentFandomIndex.data(Qt::DisplayRole).toString();
                if(currentFandomIndex.data(Qt::DisplayRole).toString() == path.at(currentDepth))
                    return currentFandomIndex;
            }
        }
    }
    return QModelIndex();
}

void FandomListWidget::ReloadModel()
{
    using namespace core::fandom_lists;
    QStringList expandedNodes;
    std::function<QVariant(ListBase*)> dataAccessFunc =
                [](ListBase* data){return QVariant(data->name);};
    TreeFunctions::StoreNodePathExpandState<TreeItemInterface, TreeItem, ListBase>(dataAccessFunc, expandedNodes, ui->tvFandomLists, treeModel, QModelIndex());
    treeModel->InsertRootItem(rootItem);
    TreeFunctions::ApplyNodePathExpandState<TreeItemInterface, TreeItem, ListBase>(dataAccessFunc, expandedNodes, ui->tvFandomLists, treeModel, QModelIndex());

}

void FandomListWidget::CollapseTree()
{
    QModelIndex startIndex;
    for(int i(0); i < treeModel->rowCount(QModelIndex()); ++i)
    {
        QModelIndex child = treeModel->index(i, 0, startIndex);
        ui->tvFandomLists->setExpanded(child, false);
    }
}

void FandomListWidget::OnTreeItemDoubleClicked(const QModelIndex &index)
{
    ui->tvFandomLists->blockSignals(true);
    if(index.column() *in(1,2)){
        using namespace core::fandom_lists;
        auto pointer = static_cast<TreeItemInterface*>(index.internalPointer());
        ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer());
        if(basePtr->type == et_list){
            TreeFunctions::Visit<TreeItemInterface>([](TreeItemInterface* item)->void{
                FandomStateInList* ptr = static_cast<FandomStateInList*>(item->InternalPointer());
                ptr->inclusionMode = ptr->Rotate(ptr->inclusionMode);
            }, treeModel, index);
            env->interfaces.fandomLists->FlipValuesForList(basePtr->id);
        }
        else{
            // preventing flip of individual nodes in ignorelist to avoid confusion
            if(pointer->parent()->data(3, Qt::DisplayRole).toString() == "Ignores" && index.column() == 1){
                QMessageBox::warning(nullptr, "Warning!", "Ignores is a special case so you can only flip mode for its contents but not individual fandoms.");
                return;
            }
            auto* ptr = static_cast<FandomStateInList*>(basePtr);
            if(index.column() == 1)
                ptr->inclusionMode = ptr->Rotate(ptr->inclusionMode);
            else
                ptr->crossoverInclusionMode = ptr->Rotate(ptr->crossoverInclusionMode);
        }
    }
    ui->tvFandomLists->blockSignals(false);
    ReloadModel();
}

void FandomListWidget::OnTreeItemSelectionChanged(const QModelIndex& newIndex, const QModelIndex&)
{
    if(!newIndex.isValid())
        return;
    using namespace core::fandom_lists;
    auto pointer = static_cast<TreeItemInterface*>(newIndex.internalPointer());
    ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer());
    if(basePtr->type == et_list){
        ui->cbFandomLists->setCurrentText(basePtr->name);
    }
    else{
        auto parentPtr = pointer->parent();
        ListBase* basePtr = static_cast<ListBase*>(parentPtr->InternalPointer());
        ui->cbFandomLists->setCurrentText(basePtr->name);
    }
}

void FandomListWidget::OnTreeItemChecked(const QModelIndex &index)
{
    using namespace core::fandom_lists;
    auto pointer = static_cast<TreeItemInterface*>(index.internalPointer());
    ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer());
    basePtr->isEnabled = pointer->checkState() == Qt::Checked;
    if(basePtr->type == et_list){
        env->interfaces.fandomLists->EditListState(*static_cast<List*>(basePtr));
    }
    else{
        env->interfaces.fandomLists->EditFandomStateForList(*static_cast<FandomStateInList*>(basePtr));
    }
}

void FandomListWidget::OnContextMenuRequested(const QPoint &pos)
{
    auto index = ui->tvFandomLists->indexAt(pos);
    clickedIndex = index;
    if(!index.isValid()){
        // request to add new list goes here
        noItemMenu->popup(ui->tvFandomLists->mapToGlobal(pos));
        return;
    }
    else{
        using namespace core::fandom_lists;
        auto pointer = static_cast<TreeItemInterface*>(index.internalPointer());
        ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer());
        if(basePtr->type == et_list)
            listItemMenu->popup(ui->tvFandomLists->mapToGlobal(pos));
        else
            fandomItemMenu->popup(ui->tvFandomLists->mapToGlobal(pos));
    }
}

void FandomListWidget::OnIgnoreCurrentFandom()
{
    QString fandom = ui->cbFandoms->currentText();
    auto id = env->interfaces.fandoms->GetIDForName(fandom);
    if(id == -1)
    {
        QMessageBox::warning(nullptr, "Warning!", QString("Fandom %1 doesn't exist").arg(fandom));
        return;
    }
    bool alreadyInList = IsFandomInList(ignoresItem, id);
    if(alreadyInList)
    {
        ScrollToFandom(ignoresItem, id);
        return;
    }
    AddFandomToList(ignoresItem, id, core::fandom_lists::EInclusionMode::im_exclude);
    ScrollToFandom(ignoresItem, id);
}

void FandomListWidget::OnWhitelistCurrentFandom()
{
    QString fandom = ui->cbFandoms->currentText();
    auto id = env->interfaces.fandoms->GetIDForName(fandom);
    if(id == -1)
    {
        QMessageBox::warning(nullptr, "Warning!", QString("Fandom %1 doesn't exist").arg(fandom));
        return;
    }
    bool alreadyInList = IsFandomInList(whitelistItem, id);
    if(alreadyInList)
    {
        ScrollToFandom(whitelistItem, id);
        return;
    }
    AddFandomToList(whitelistItem, id, core::fandom_lists::EInclusionMode::im_include);
    ScrollToFandom(whitelistItem, id);
}

void FandomListWidget::OnAddCurrentFandomToList()
{
    auto listName = ui->cbFandomLists->currentText();
    auto index = FindIndexForPath({listName});
    if(!index.isValid())
        return;

    using namespace core::fandom_lists;
    auto pointer = static_cast<TreeItemInterface*>(index.internalPointer());
    auto sharedList = pointer->shared_from_this();

    QString fandom = ui->cbFandoms->currentText();
    auto id = env->interfaces.fandoms->GetIDForName(fandom);
    auto mode = listName == "Ignores" ? core::fandom_lists::EInclusionMode::im_exclude : core::fandom_lists::EInclusionMode::im_include;
    AddFandomToList(sharedList, id, mode);
    ScrollToFandom(sharedList, id);
}

void FandomListWidget::OnCheckFandomsText(const QString & text)
{
    if(text.length() > 0){
        ui->pbAddFandomToSelectedList->setEnabled(true);
        ui->pbIgnoreFandom->setEnabled(true);
        ui->pbWhitelistFandom->setEnabled(true);
    }
    else{
        ui->pbAddFandomToSelectedList->setEnabled(false);
        ui->pbIgnoreFandom->setEnabled(false);
        ui->pbWhitelistFandom->setEnabled(false);
    }
}

//void FandomListWidget::OnFandomActivatedInCombobox(int)
//{
//    auto listName = ui->cbFandomLists->currentText();
//    auto index = FindIndexForPath({listName});
//    if(!index.isValid())
//        return;

//    using namespace core::fandom_lists;
//    auto pointer = static_cast<TreeItemInterface*>(index.internalPointer());
//    auto sharedList = pointer->shared_from_this();

//    QString fandom = ui->cbFandoms->currentText();
//    auto id = env->interfaces.fandoms->GetIDForName(fandom);
//    auto mode = listName == "Ignores" ? core::fandom_lists::EInclusionMode::im_exclude : core::fandom_lists::EInclusionMode::im_include;
//    AddFandomToList(sharedList, id, mode);
//}

void FandomListWidget::OnCheckFandomListText(const QString & text)
{
    if(text *in("Ignores" , "Whitelist")){
        ui->pbDeleteFandomList->setEnabled(false);
    }
    else{
        ui->pbDeleteFandomList->setEnabled(true);
    }
}


