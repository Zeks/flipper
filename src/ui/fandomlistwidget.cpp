#include "ui/fandomlistwidget.h"
#include "ui_fandomlistwidget.h"
#include "ui-models/include/custom_icon_delegate.h"
#include "ui-models/include/TreeItem.h"
#include "ui-models/include/treeviewtemplatefunctions.h"
#include "Interfaces/fandom_lists.h"
#include "GlobalHeaders/snippets_templates.h"
#include "environment.h"
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QInputDialog>
#include <QMessageBox>
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
                return QPixmap(":/icons/icons/cross_red.png");
            else
                return  QPixmap(":/icons/icons/ok.png");
});

    crossoverDelegate = createDelegate([](QModelIndex index){
            MEMBER_VALUE_OR_DEFAULT(crossoverInclusionMode);
            auto value = getter(basePtr);
            if(value == cim_select_all)
                return QPixmap(":/icons/icons/bit2.png");
            else if(value == cim_select_crossovers)
                return  QPixmap(":/icons/icons/shuffle_blue.png");
            else
            return  QPixmap(":/icons/icons/open_book.png");
});

    ui->tvFandomLists->setItemDelegateForColumn(0, dummyDelegate);
    ui->tvFandomLists->setItemDelegateForColumn(1, modeDelegate);
    ui->tvFandomLists->setItemDelegateForColumn(2, crossoverDelegate);
    CreateContextMenus();

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
    connect(ui->tvFandomLists, &QTreeView::customContextMenuRequested, this, &FandomListWidget::OnContextMenuRequested);


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
    ui->tvFandomLists->blockSignals(true);
    rootItem->addChild(newListItem);
    ui->tvFandomLists->blockSignals(false);
    treeModel->Refresh();
}

void FandomListWidget::DeleteListUnderCursor()
{
    QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, "QUestion", "Do you really want to delete this fandom list?",
                                    QMessageBox::Yes|QMessageBox::No);
      if (reply == QMessageBox::No)
            return;

      using namespace core::fandom_lists;
      auto pointer = static_cast<TreeItemInterface*>(clickedIndex.internalPointer());
      ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer());
      env->interfaces.fandomLists->RemoveFandomList(basePtr->id);
      pointer->removeChildren(pointer->row(), 1);
      treeModel->Refresh();
}

void FandomListWidget::RenameListUnderCursor()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Name selector"),
                                             tr("Choose a new name for your list:"), QLineEdit::Normal,
                                             "", &ok);
    if(name.length() == 0)
        return;

    using namespace core::fandom_lists;
    auto pointer = static_cast<TreeItemInterface*>(clickedIndex.internalPointer());
    ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer());
    basePtr->name = name;
    env->interfaces.fandomLists->EditListState(std::dynamic_pointer_cast<List>(basePtr->shared_from_this()));
    treeModel->Refresh();
}

void FandomListWidget::DeleteFandomUnderCursor()
{
    using namespace core::fandom_lists;
    auto pointer = static_cast<TreeItemInterface*>(clickedIndex.internalPointer());
    auto parentPtr = pointer->parent();
    ListBase* fandomPtr = static_cast<ListBase*>(pointer->InternalPointer());
    ListBase* listPtr = static_cast<ListBase*>(parentPtr->InternalPointer());
    env->interfaces.fandomLists->RemoveFandomFromList(listPtr->id, fandomPtr->id);
    parentPtr->removeChildren(pointer->row(), 1);
    treeModel->Refresh();
}

std::shared_ptr<TreeItemInterface> FandomListWidget::FetchAndConvertFandomLists()
{
    auto fandomListInterface = env->interfaces.fandomLists;
    auto lists = fandomListInterface->GetLoadedFandomLists();
    using namespace interfaces;
    using ListType = decltype(std::declval<FandomLists>().GetFandomList(std::declval<QString>()));
    std::vector<ListType> listVec;
    for(auto name : lists){
        listVec.push_back(fandomListInterface->GetFandomList(name));
    }
    std::sort(listVec.begin(),listVec.end(), [](const auto& i1,const auto& i2){
        return i1->uiIndex < i2->uiIndex;
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
            auto* ptr = static_cast<FandomStateInList*>(basePtr);
            if(index.column() == 1)
                ptr->inclusionMode = ptr->Rotate(ptr->inclusionMode);
            else
                ptr->crossoverInclusionMode = ptr->Rotate(ptr->crossoverInclusionMode);
        }
    }
    ui->tvFandomLists->blockSignals(false);
    treeModel->Refresh();
}

void FandomListWidget::OnTreeItemChecked(const QModelIndex &index)
{
    using namespace core::fandom_lists;
    auto pointer = static_cast<TreeItemInterface*>(index.internalPointer());
    ListBase* basePtr = static_cast<ListBase*>(pointer->InternalPointer());
    basePtr->isEnabled = pointer->checkState() == Qt::Checked;
    if(basePtr->type == et_list){
        env->interfaces.fandomLists->EditListState(std::dynamic_pointer_cast<List>(basePtr->shared_from_this()));
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


