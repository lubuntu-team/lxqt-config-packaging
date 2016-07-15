/*
    Copyright (C) 2015  P.L. Lucas <selairi@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __SETTINGSDIALOG_H__
#define __SETTINGSDIALOG_H__

#include <LXQt/ConfigDialog>
#include <LXQt/Settings>
#include <KScreen/GetConfigOperation>
#include <KScreen/SetConfigOperation>


class SettingsDialog : public LXQt::ConfigDialog
{
    Q_OBJECT

public:
    SettingsDialog(const QString &title, LXQt::Settings *settings, KScreen::ConfigPtr config, QWidget *parent = 0);

private Q_SLOTS:

private:

    // Configutarions
};

#endif // __SETTINGSDIALOG_H__

