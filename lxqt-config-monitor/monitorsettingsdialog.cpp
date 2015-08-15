/*
    Copyright (C) 2014  P.L. Lucas <selairi@gmail.com>
    Copyright (C) 2013  <copyright holder> <email>

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


#include "monitorsettingsdialog.h"
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QComboBox>
#include <QProcess>
#include <QGroupBox>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QProgressBar>
#include <QInputDialog>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "monitorwidget.h"
#include "timeoutdialog.h"
#include "xrandr.h"
#include "monitorpicture.h"

MonitorSettingsDialog::MonitorSettingsDialog(MonitorSettingsBackend* backend, LxQt::Settings *applicationSettings):
  QDialog(NULL, 0),
  LVDS(NULL) {
  timeoutDialog = NULL;
  timer = NULL;
  this->applicationSettings = applicationSettings;
  this->backend = backend;
  backend->setParent(this);
  setupUi();
}


MonitorSettingsDialog::~MonitorSettingsDialog() {
}


void MonitorSettingsDialog::deleteTimeoutData() {
  timeoutDialog = NULL;
  Q_FOREACH(MonitorInfo * monitorInfo, timeoutSettings) {
    delete monitorInfo;
  }
  timeoutSettings.clear();
}

void MonitorSettingsDialog::onCancelSettings() {
  // restore the old settings
  QList<MonitorSettings*> settings;
  Q_FOREACH(MonitorInfo * monitorInfo, timeoutSettings) {
    settings.append((MonitorSettings*)monitorInfo);
  }
  backend->setMonitorsSettings(settings);
  deleteTimeoutData();
}

QList<MonitorSettings*> MonitorSettingsDialog::getMonitorsSettings() {
  // Build list of monitor and their settings
  QList<MonitorSettings*> settings;
  Q_FOREACH(MonitorWidget * monitor, monitors) {
    MonitorSettings* s = monitor->getSettings();
    settings.append(s);
    if(ui.primaryCombo->currentText() == monitor->monitorInfo->name)
      s->primaryOk = true;
  }
  if(ui.unify->isChecked()) {
    Q_FOREACH(MonitorSettings * s, settings) {
      s->position = MonitorSettings::None;
    }
  }
  return settings;
}

void MonitorSettingsDialog::setMonitorsConfig() {
  deleteTimeoutData();
  timeoutSettings = backend->getMonitorsInfo();
  // Show timeout dialog
  timeoutDialog = new TimeoutDialog(this);
  connect(timeoutDialog, SIGNAL(rejected()), this, SLOT(onCancelSettings()));
  connect(timeoutDialog, SIGNAL(finished(int)), timeoutDialog, SLOT(deleteLater()));
  // Build list of monitor and their settings
  QList<MonitorSettings*> settings = getMonitorsSettings();
  backend->setMonitorsSettings(settings);
  Q_FOREACH(MonitorSettings * s, settings) {
    delete s;
  }
  timeoutDialog->show();
}

// turn on both laptop LCD and the external monitor
void MonitorSettingsDialog::onUseBoth() {
  if(monitors.length() == 0)
    return;
  ui.unify->setChecked(true);
  MonitorWidget* monitor = monitors[0];
  bool ok;
  QString mode;
  for(int i = 0; i < monitor->monitorInfo->modes.length(); i++) {
    mode = monitor->monitorInfo->modes[i];
    ok = true;
    Q_FOREACH(MonitorWidget * monitor2, monitors) {
      ok = ok && monitor2->monitorInfo->modes.contains(mode);
    }
    if(ok)
      break;
  }
  qDebug() << "Mode selected" << mode << ok;
  Q_FOREACH(MonitorWidget * monitor2, monitors) {
    int index = monitor2->monitorInfo->modes.indexOf(mode) + 1;
    if(monitor2->ui.resolutionCombo->count() > index)
      monitor2->ui.resolutionCombo->setCurrentIndex(index);
    else
      monitor2->chooseMaxResolution();
    monitor2->enableMonitor(true);
    qDebug() << "Mode selected index" << index << "Mode" << monitor->ui.resolutionCombo->currentText();
  }
  setMonitorsConfig();
}

// external monitor only
void MonitorSettingsDialog::onExternalOnly() {
  Q_FOREACH(MonitorWidget * monitor, monitors) {
    monitor->chooseMaxResolution();
    monitor->enableMonitor(monitor != LVDS);
  }
  setMonitorsConfig();
}

// laptop panel - LVDS only
void MonitorSettingsDialog::onLaptopOnly() {
  Q_FOREACH(MonitorWidget * monitor, monitors) {
    monitor->chooseMaxResolution();
    monitor->enableMonitor(monitor == LVDS);
  }
  setMonitorsConfig();
}

void MonitorSettingsDialog::onExtended() {
  ui.unify->setChecked(false);
  int virtualWidth = 0;
  Q_FOREACH(MonitorWidget * monitor, monitors) {
    monitor->chooseMaxResolution();
    monitor->enableMonitor(true);
    monitor->disablePositionOption(false);
    QString modeName = monitor->ui.resolutionCombo->currentText();
    int modeWidth = monitor->monitorInfo->monitorModes[modeName]->width;
    monitor->ui.xPosSpinBox->setValue(virtualWidth);
    monitor->ui.yPosSpinBox->setValue(0);
    virtualWidth+=modeWidth;
  }
  setMonitorsConfig();
}

void MonitorSettingsDialog::setupUi() {
  ui.setupUi(this);
  connect(ui.positionPushButton, SIGNAL(clicked()), SLOT(onPositionButtonClicked()));

  // Get monitors information
  QList<MonitorInfo*> monitorsInfo = backend->getMonitorsInfo();

  // Search if LVSD monitor is connected
  hardwareIdentifier = "";
  Q_FOREACH(MonitorInfo * monitorInfo, monitorsInfo) {
    hardwareIdentifier+=monitorInfo->edid;
    if(! LVDS && (monitorInfo->name.startsWith("LVDS") || monitorInfo->name.startsWith("PANEL"))) {
      MonitorInfo::LVDS_Ok = true;
      break;
    }
  }

  int i = 0;
  connect(ui.unify, SIGNAL(toggled(bool)), this, SLOT(disablePositionOption(bool)));
  Q_FOREACH(MonitorInfo * monitorInfo, monitorsInfo) {
    ui.primaryCombo->addItem(monitorInfo->name);
    if(monitorInfo->primaryOk)
      ui.primaryCombo->setCurrentIndex(ui.primaryCombo->findText(monitorInfo->name));

    qDebug() << "Monitor" << monitorInfo->name;
    MonitorWidget* monitor = new MonitorWidget(monitorInfo, monitorsInfo, this);
    QString title = QString("Monitor %1: %2 (%3) %4")
                    .arg(i + 1)
                    .arg(monitor->monitorInfo->name)
                    .arg(monitor->monitorInfo->humanReadableName())
                    .arg(monitor->monitorInfo->vendor);
    qDebug() << "Monitor" << title;
    monitor->setTitle(title);

    connect(ui.unify, SIGNAL(toggled(bool)), monitor, SLOT(disablePositionOption(bool)));
    monitors.append(monitor);
    if(! LVDS && (monitorInfo->name.startsWith("LVDS") || monitorInfo->name.startsWith("PANEL"))) {
      LVDS = monitor;
    }
    ui.stackedWidget->addWidget(monitor);
    ui.monitorList->addItem(monitor->monitorInfo->name);
    ++i;
  }
  ui.monitorList->setCurrentRow(0);
  // set the max width of the list widget to the maximal width of its rows + the width of a vertical scrollbar.
  ui.monitorList->setMaximumWidth(ui.monitorList->sizeHintForColumn(0) + style()->pixelMetric(QStyle::PM_ScrollBarExtent) + 40);

  // are the monitors unified?
  if(monitorsInfo.length() > 1)
    ui.unify->setChecked(backend->isUnified(monitorsInfo));
  else {// disable the option if we only have one monitor
    ui.unify->setEnabled(false);
    ui.positionPushButton->setEnabled(false);
  }

  // If this is a laptop and there is an external monitor, offer quick options
  if(monitors.length() == 2) {
    // If there is only two monitors,offer quick options
    if(! LVDS) {
      LVDS = monitors[0];
    }
  }

  adjustSize();
}

void MonitorSettingsDialog::accept() {
  setMonitorsConfig();
  QDialog::accept();
}

void MonitorSettingsDialog::disablePositionOption(bool disable) {
  ui.positionPushButton->setEnabled(!disable);
}

void MonitorSettingsDialog::onPositionButtonClicked() {
  MonitorPictureDialog *dialog = new MonitorPictureDialog(this);
  dialog->setScene(monitors);
  dialog->exec();
  dialog->updateMonitorWidgets(ui.primaryCombo->currentText());
  delete dialog;
}

void MonitorSettingsDialog::applySettings() {
    setMonitorsConfig();
}

void MonitorSettingsDialog::saveSettings() {
    // Save config and exit
    QMessageBox msgBox;
    msgBox.setText(tr("Do you want to save changes?"));
    msgBox.setInformativeText(tr("Please, check the settings before saving."));
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    if( ret == QMessageBox::Cancel )
      return;
    bool ok;
    QString configName = QInputDialog::getText(this, tr("Name"),
                                         tr("Name:"), QLineEdit::Normal,
                                         tr("Actual"), &ok);
    if (!ok || configName.isEmpty())
        return;
    QList<MonitorSettings*> settings = getMonitorsSettings();
    QString cmd = backend->getCommand(settings);
    Q_FOREACH(MonitorSettings * s, settings) {
      delete s;
    }
    QString desktop = QString("[Desktop Entry]\n"
                              "Type=Application\n"
                              "Name=LXQt-config-monitor autostart\n"
                              "Comment=Autostart monitor settings for LXQt-config-monitor\n"
                              "Exec=%1\n"
                              "OnlyShowIn=LXQt\n").arg(cmd);
    // Check if ~/.config/autostart/ exists
    ok = true;
    QFileInfo fileInfo(QDir::homePath() + "/.config/autostart/");
    if( ! fileInfo.exists() )
      ok = QDir::root().mkpath(QDir::homePath() + "/.config/autostart/");
    QFile file(QDir::homePath() + "/.config/autostart/lxqt-config-monitor-autostart.desktop");
    if(ok)
            ok = file.open(QIODevice::WriteOnly | QIODevice::Text);
    if(!ok) {
      QMessageBox::critical(this, tr("Error"), tr("Config can not be saved"));
      return;
    }
    QTextStream out(&file);
    out << desktop;
    out.flush();
    file.close();

    // Save config in applicationSettings
    applicationSettings->beginGroup("configMonitor");
    QJsonArray  savedConfigs = QJsonDocument::fromJson(applicationSettings->value("saved").toByteArray()).array();
    QJsonObject monitorConfig;
    monitorConfig["hardwareIdentifier"] = hardwareIdentifier;
    monitorConfig["command"] = cmd;
    monitorConfig["name"] = configName;
    savedConfigs.append(monitorConfig);
    applicationSettings->setValue("saved", QVariant(QJsonDocument(savedConfigs).toJson()));
    applicationSettings->endGroup();
    emit(settingsSaved());
}

#include <QDialogButtonBox>

void MonitorSettingsDialog::processClickedFromDialog(QDialogButtonBox::StandardButton button)
{
    qDebug() << "[MonitorSettingsDialog::processClickedFromDialog]";
    if(button == QDialogButtonBox::Apply)
	setMonitorsConfig();
}

QString MonitorSettingsDialog::getHardwareIdentifier()
{
  return hardwareIdentifier;
}
