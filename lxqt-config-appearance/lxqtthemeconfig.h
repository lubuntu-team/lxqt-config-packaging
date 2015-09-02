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

#ifndef LXQTTHEMECONFIG_H
#define LXQTTHEMECONFIG_H

#include <QWidget>
#include <LXQt/Settings>

class QTreeWidgetItem;

namespace Ui {
    class LXQtThemeConfig;
}

class LXQtThemeConfig : public QWidget
{
    Q_OBJECT

public:
    explicit LXQtThemeConfig(LXQt::Settings *settings, QWidget *parent = 0);
    ~LXQtThemeConfig();

public slots:
    void initControls();

private slots:
    void lxqtThemeSelected(QTreeWidgetItem* item, int column);

private:
    Ui::LXQtThemeConfig *ui;
    LXQt::Settings *mSettings;
};

#endif // LXQTTHEMECONFIG_H
