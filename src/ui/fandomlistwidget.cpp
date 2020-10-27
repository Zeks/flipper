#include "ui/fandomlistwidget.h"
#include "ui_fandomlistwidget.h"
#include "ui-models/include/custom_icon_delegate.h"
#include "ui-models/include/TreeItem.h"
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QPainter>

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
static bool editorEvent(QEvent *event,
                        QAbstractItemModel *model,
                        const QStyleOptionViewItem &option,
                        const QModelIndex &index) {
    if ((event->type() == QEvent::MouseButtonRelease) ||
            (event->type() == QEvent::MouseButtonDblClick)) {
        QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);
        if (mouse_event->button() != Qt::LeftButton ||
                !CheckBoxRect(option).contains(mouse_event->pos())) {
            return false;
        }
        if (event->type() == QEvent::MouseButtonDblClick) {
            return true;
        }
    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space &&
                static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select) {
            return false;
        }
    } else {
        return false;
    }

    bool checked = index.model()->data(index, Qt::DisplayRole).toBool();
    model->setData(index, !checked, Qt::DisplayRole);
    return model->setData(index, !checked, Qt::EditRole);
}


static void paintCheckbox(const QStyledItemDelegate* delegate, QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    QVariant value = index.model()->data(index, Qt::DisplayRole);

    delegate->QStyledItemDelegate::paint(painter, option, index.parent().sibling(0,2));
    if(!value.isValid())
        return;

    bool checked = index.model()->data(index, Qt::DisplayRole).toBool();
    auto pixmapToDraw = checked ? QPixmap(":/icons/icons/heart_tag.png") :  QPixmap(":/icons/icons/heart_tag_gray.png");
    painter->drawPixmap(CheckBoxRect(option), pixmapToDraw);

}

FandomListWidget::FandomListWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FandomListWidget)
{
    ui->setupUi(this);
    dummyDelegate = new CustomIconDelegate();
    dummyDelegate->widgetCreator = [](QWidget * ) {return static_cast<QWidget*>(nullptr);};
    dummyDelegate->paintProcessor = paintCheckbox;
    dummyDelegate->editorEventProcessor = editorEvent;
    ui->tvFandomLists->setItemDelegateForColumn(1, dummyDelegate);
    InitTree();
    connect(ui->tvFandomLists, SIGNAL(itemCheckStatusChanged(const QModelIndex&)), this, SLOT(OnTreeItemChecked(const QModelIndex&)),Qt::UniqueConnection);
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
        return ptr->listName;
    });
    fandomItemController->AddGetter(3,displayRoles, [](const core::fandom_lists::FandomStateInList* ptr)->QVariant{
        return ptr->name;
    });


    // SETTERS
    listItemController->AddSetter(3,displayRoles, [](core::fandom_lists::List* data, QVariant value)->bool{
        data->listName = value.toString();
        return true;
    });
    fandomItemController->AddSetter(3,displayRoles, [](core::fandom_lists::FandomStateInList*data, QVariant value)->bool{
        data->name = value.toString();
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

    TreeItem<core::fandom_lists::List>* rootPointer = new TreeItem<core::fandom_lists::List>();
    rootItem = std::shared_ptr<TreeItemInterface>(rootPointer);
    rootPointer->SetController(listItemController);


    TreeItem<core::fandom_lists::List>* ignoresPointer = new TreeItem<core::fandom_lists::List>();
    auto ignoresItem = std::shared_ptr<TreeItemInterface>(ignoresPointer);
    ignoresPointer->SetController(listItemController);
    ignoresPointer->SetParent(rootItem);
    ignoresPointer->setData(3, "Ignores", 0);
    ignoresPointer->setData(3, "Ignores", 2);

    TreeItem<core::fandom_lists::List>* whitelistPointer = new TreeItem<core::fandom_lists::List>();
    auto whitelistItem = std::shared_ptr<TreeItemInterface>(whitelistPointer);
    whitelistPointer->SetController(listItemController);
    whitelistPointer->SetParent(rootItem);
    whitelistPointer->setData(3, "Whitelist", 0);

    TreeItem<core::fandom_lists::FandomStateInList>* ignoreNodePointer = new TreeItem<core::fandom_lists::FandomStateInList>();
    auto ignoreInnerItem = std::shared_ptr<TreeItemInterface>(ignoreNodePointer);
    ignoreNodePointer->SetController(fandomItemController);
    ignoreNodePointer->SetParent(ignoresItem);
    ignoreNodePointer->setData(3, "Fairy Tail", 0);

    listItemController->SetColumns(QStringList() << "dummy" << "inclusion" << "n" << "name");
    fandomItemController->SetColumns(QStringList()<< "dummy" << "inclusion" << "crossovers" << "name");

    rootPointer->addChildren({ignoresItem, whitelistItem});
    ignoresPointer->addChildren({ignoreInnerItem});

    treeModel->InsertRootItem(rootItem);
    ui->tvFandomLists->setModel(treeModel);
    ui->tvFandomLists->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tvFandomLists->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tvFandomLists->setColumnWidth(0, 230);
    ui->tvFandomLists->setRootIsDecorated(false);
    ui->tvFandomLists->setTreePosition(3);
    ui->tvFandomLists->setIndentation(5);
    ui->tvFandomLists->setExpandsOnDoubleClick(true);
    ui->tvFandomLists->header()->setMinimumSectionSize(2);


}

void FandomListWidget::OnTreeItemDoubleClicked(const QModelIndex &index)
{

}

void FandomListWidget::OnTreeItemChecked(const QModelIndex &index)
{
    TreeItemInterface* pointer = static_cast<TreeItemInterface*>(index.internalPointer());
}


