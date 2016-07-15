/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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

#include "lxqtthemeconfig.h"
#include "ui_lxqtthemeconfig.h"
#include <QTreeWidget>
#include <QDebug>
#include <QProcess>
#include <QItemDelegate>
#include <QPainter>

/*!
 * \brief Simple delegate to draw system background color below decoration/icon
 * (needed by System theme, which uses widget background and therefore provides semi-transparent preview)
 */
class ThemeDecorator : public QItemDelegate
{
public:
    using QItemDelegate::QItemDelegate;
protected:
    virtual void drawDecoration(QPainter * painter, const QStyleOptionViewItem & option, const QRect & rect, const QPixmap & pixmap) const override
    {
        //Note: can't use QItemDelegate::drawDecoration, because it is ignoring pixmap,
        //if the icon is valid (and that is set in paint())
        if (pixmap.isNull() || !rect.isValid())
            return;

        QPoint p = QStyle::alignedRect(option.direction, option.decorationAlignment, pixmap.size(), rect).topLeft();
        painter->fillRect(QRect{p, pixmap.size()}, QApplication::palette().color(QPalette::Window));
        painter->drawPixmap(p, pixmap);
    }
};

LXQtThemeConfig::LXQtThemeConfig(LXQt::Settings *settings, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LXQtThemeConfig),
    mSettings(settings)
{
    ui->setupUi(this);
    {
        QScopedPointer<QAbstractItemDelegate> p{ui->lxqtThemeList->itemDelegate()};
        ui->lxqtThemeList->setItemDelegate(new ThemeDecorator{this});
    }

    connect(ui->lxqtThemeList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this, SLOT(lxqtThemeSelected(QTreeWidgetItem*,int)));


    QList<LXQt::LXQtTheme> themes = LXQt::LXQtTheme::allThemes();
    foreach(LXQt::LXQtTheme theme, themes)
    {
        QString themeName = theme.name();
        themeName[0] = themeName[0].toTitleCase();
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(themeName));
        if (!theme.previewImage().isEmpty())
        {
            item->setIcon(0, QIcon(theme.previewImage()));
        }
        item->setSizeHint(0, QSize(42,42)); // make icons non-cropped
        item->setData(0, Qt::UserRole, theme.name());
        ui->lxqtThemeList->addTopLevelItem(item);
    }

    initControls();
}


LXQtThemeConfig::~LXQtThemeConfig()
{
    delete ui;
}


void LXQtThemeConfig::initControls()
{
    QString currentTheme = mSettings->value("theme").toString();

    QTreeWidgetItemIterator it(ui->lxqtThemeList);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toString() == currentTheme)
        {
            ui->lxqtThemeList->setCurrentItem((*it));
            break;
        }
        ++it;
    }

    update();
}


void LXQtThemeConfig::lxqtThemeSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (!item)
        return;

    QVariant themeName = item->data(0, Qt::UserRole);
    mSettings->setValue("theme", themeName);

    LXQt::LXQtTheme theme(themeName.toString());
    if(theme.isValid()) {
		QString wallpaper = theme.desktopBackground();
		if(!wallpaper.isEmpty()) {
			// call pcmanfm-qt to update wallpaper
			QProcess process;
			QStringList args;
			args << "--set-wallpaper" << wallpaper;
			process.start("pcmanfm-qt", args, QIODevice::NotOpen);
			process.waitForFinished();
		}
	}
}
