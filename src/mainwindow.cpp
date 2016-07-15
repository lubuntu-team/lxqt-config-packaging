/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * http://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include <QDirIterator>
#include <QLineEdit>
#include <QTimer>

#include "mainwindow.h"
#include <QtDebug>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QShortcut>
#include <QKeySequence>

#include <XdgDesktopFile>
#include <XdgIcon>
#include <XdgMenu>
#include <XmlHelper>

#include "qcategorizedview.h"
#include "qcategorydrawer.h"
#include "qcategorizedsortfilterproxymodel.h"

namespace LXQtConfig {

struct ConfigPaneData: public QSharedData
{
    QString id;
    QString category;
    XdgDesktopFile xdg;
};

class ConfigPane
{
public:
    ConfigPane(): d(new ConfigPaneData) { }
    ConfigPane(const ConfigPane &other): d(other.d) { }

    inline QString &id() const { return d->id; }
    inline XdgDesktopFile xdg() const { return d->xdg; }
    inline void setXdg(XdgDesktopFile xdg) { d->xdg = xdg; }
    inline QString &category() const { return d->category; }

    bool operator==(const ConfigPane &other)
    {
        return d->id == other.id();
    }

private:
    QExplicitlySharedDataPointer<ConfigPaneData> d;
};


class ConfigPaneModel: public QAbstractListModel
{
public:
    ConfigPaneModel(): QAbstractListModel()
    {
        QString menuFile = XdgMenu::getMenuFileName("config.menu");
        XdgMenu xdgMenu;
        xdgMenu.setEnvironments(QStringList() << "X-LXQT" << "LXQt" << "LXDE");
        bool res = xdgMenu.read(menuFile);
        if (!res)
        {
            QMessageBox::warning(0, "Parse error", xdgMenu.errorString());
            return;
        }

        DomElementIterator it(xdgMenu.xml().documentElement() , "Menu");
        while(it.hasNext())
        {
            this->buildGroup(it.next());
        }
    }

    void buildGroup(const QDomElement& xml)
    {
        QString category;
        if (! xml.attribute("title").isEmpty())
            category = xml.attribute("title");
        else
            category = xml.attribute("name");

        DomElementIterator it(xml , "AppLink");
        while(it.hasNext())
        {
            QDomElement x = it.next();

            XdgDesktopFile xdg;
            xdg.load(x.attribute("desktopFile"));

            ConfigPane pane;
            pane.id() = xdg.value("Icon").toString();
            pane.category() = category;
            pane.setXdg(xdg);
            m_list.append(pane);
        }
    }

    void activateItem(const QModelIndex &index)
    {
        if (!index.isValid())
            return;
        m_list[index.row()].xdg().startDetached();
    }

    ~ConfigPaneModel() { }

    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        return m_list.count();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
    {
        return false;
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
            return m_list[index.row()].xdg().name();
        if (role == QCategorizedSortFilterProxyModel::CategoryDisplayRole)
            return m_list[index.row()].category();
        if (role == QCategorizedSortFilterProxyModel::CategorySortRole)
            return m_list[index.row()].category();
        if (role == Qt::UserRole)
            return m_list[index.row()].id();
        if (role == Qt::DecorationRole)
        {
            return m_list[index.row()].xdg().icon(XdgIcon::defaultApplicationIcon());
        }
        return QVariant();
    }

private:
    QList<ConfigPane> m_list;
};

}


class ConfigItemDelegate : public QStyledItemDelegate
{
public:
    ConfigItemDelegate(QCategorizedView* view) : mView(view) { }
    ~ConfigItemDelegate() { }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        /* We let Qt calculate the real cell size but consider the 4-px margin
           around each cell and try to add a 2-px margin around the selection
           rectangle for styles that, unlike Fusion, highlight the whole item. */
        QStyleOptionViewItemV4 opt = option;
        int delta = opt.rect.width() - (mView->gridSize().width() - 8);
        if (delta > 0)
          opt.rect.adjust(delta/2, 0 , -delta/2, 0);
        QSize defaultSize = QStyledItemDelegate::sizeHint(opt, index);
        return QSize(qMin(defaultSize.width() + 4, mView->gridSize().width() - 8),
                     qMin(defaultSize.height() + 4, mView->gridSize().height() - 8));
    }

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyleOptionViewItemV4 opt = option;
        initStyleOption(&opt, index);

        const QSize & iconSize = option.decorationSize;
        QSize size(mView->gridSize().width() - 8, // 4-px margin around each cell
                   iconSize.height());
        // for having sharp non-scalable icons with HDPI
        int dpr = qApp->devicePixelRatio();
        if (dpr < 1) dpr = 1;
        QPixmap pixmap = opt.icon.pixmap(iconSize / dpr); // -> Qt doc -> QIcon::pixmap()
        if (dpr > 1 && pixmap.size() == iconSize) // exceptional (scalable or not from icon set)
            pixmap = opt.icon.pixmap(iconSize);
        opt.icon = QIcon(pixmap.copy(QRect(QPoint(0, 0), size * dpr)));
        opt.decorationSize = size;

        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter);
    }

private:
    QCategorizedView *mView;
};


LXQtConfig::MainWindow::MainWindow() : QMainWindow()
{
    setupUi(this);

    /* To always have the intended layout on startup,
       the listview should be shown after it's fully formed. */
    view->hide();

    model = new ConfigPaneModel();

    view->setViewMode(QListView::IconMode);
    view->setWordWrap(true);
    view->setUniformItemSizes(true);
    view->setCategoryDrawer(new QCategoryDrawerV3(view));

    connect(view, &QAbstractItemView::activated, this, &MainWindow::activateItem);
    view->setFocus();

    QTimer::singleShot(0, [this] { setSizing(); });
    QTimer::singleShot(1, this, SLOT(load()));
    new QShortcut{QKeySequence{Qt::CTRL + Qt::Key_Q}, this, SLOT(close())};
}

void LXQtConfig::MainWindow::load()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    proxyModel = new QCategorizedSortFilterProxyModel();
    proxyModel->setCategorizedModel(true);
    proxyModel->setSourceModel(model);

    view->setModel(proxyModel);
    view->setItemDelegate(new ConfigItemDelegate(view));

    view->show();

    QApplication::restoreOverrideCursor();
}

void LXQtConfig::MainWindow::activateItem(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QModelIndex orig = proxyModel->mapToSource(index);
    model->activateItem(orig);
}

void LXQtConfig::MainWindow::setSizing()
{
    // consult the style to know the icon size
    int iconSize = qBound(16, view->decorationSize().height(), 256);
    // DPR is automatically taken into account in setIconSize()
    // but wee need to consider it explicitly below
    int dpr = qApp->devicePixelRatio();
    if (dpr < 1) dpr = 1;
    iconSize *= dpr;
    /* To have an appropriate grid size, we suppose that
     *
     * (1) The text has 3 lines and each line has 16 chars (for languages like German), at most;
     * (2) The selection rect has a margin of 2 px, at most;
     * (3) There is, at most, a 3-px spacing between text and icon; and
     * (4) There is a 4-px margin around each cell.
     */
    QFontMetrics fm = fontMetrics();
    int textWidth = fm.averageCharWidth() * 16;
    int textHeight = fm.lineSpacing() * 3;
    QSize grid;
    grid.setWidth(qMax(iconSize, textWidth) + 4);
    grid.setHeight(iconSize + textHeight + 4 + 3);
    view->setGridSize(grid + QSize(8, 8));
}

bool LXQtConfig::MainWindow::event(QEvent * event)
{
    if (QEvent::StyleChange == event->type())
        setSizing();
    return QMainWindow::event(event);
}
