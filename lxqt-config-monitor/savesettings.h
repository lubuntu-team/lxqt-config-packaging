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


#ifndef _SAVESETTINGS_H_
#define _SAVESETTINGS_H_

#include "ui_savesettings.h"
#include <LXQt/Settings>

// Monitor info
class SaveSettings : public QDialog {
  Q_OBJECT

public:
  SaveSettings(LxQt::Settings*applicationSettings, QWidget* parent = 0);

  Ui::SaveSettings ui;

public slots:
  /*! Load settings to QListWidgets.
      eids is hardware code to detect hardware compatible settings.
   */
  void loadSettings();

  void setHardwareIdentifier(QString hardwareIdentifier);

  void setSavedSettings(QListWidgetItem * item);

  void onDeleteItem();

  void onRenameItem();

private:
  LxQt::Settings*applicationSettings;
  QString hardwareIdentifier;

};

#endif // _SAVESETTINGS_H_
