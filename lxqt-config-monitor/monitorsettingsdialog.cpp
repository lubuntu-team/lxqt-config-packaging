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

#include "monitorwidget.h"
#include "monitor.h"
#include "timeoutdialog.h"
#include "monitorpicture.h"
#include "settingsdialog.h"
#include "fastmenu.h"

#include <KScreen/Output>
#include <QJsonObject>
#include <QJsonArray>
#include <LXQt/Settings>
#include <QJsonDocument>
#include <KScreen/EDID>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QDateTime>
#include <lxqtautostartentry.h>

MonitorSettingsDialog::MonitorSettingsDialog() :
    QDialog(nullptr, 0)
{
    ui.setupUi(this);

    KScreen::GetConfigOperation *operation = new KScreen::GetConfigOperation();
    connect(operation, &KScreen::GetConfigOperation::finished, [this, operation] (KScreen::ConfigOperation *op) {
        KScreen::GetConfigOperation *configOp = qobject_cast<KScreen::GetConfigOperation *>(op);
        qDebug() << "Connecting to KScreen...";
        if (configOp && configOp->config() && configOp->config()->screen())
        {
            mOldConfig = configOp->config()->clone();
            loadConfiguration(configOp->config());
            operation->deleteLater();
        }
        else if(configOp && !configOp->config())
        {
            qDebug() << "Error: Config is invalid, probably backend couldn't load";
            exit(1);
        }
        else if(configOp && configOp->config() && !configOp->config()->screen())
        {
            qDebug() << "Error: No screen in the configuration, broken backend";
            exit(2);
        }
        else
        {
            qDebug() << "Error: Connect to KScreen is not possible";
            exit(3);
        }
    });

    connect(ui.buttonBox, &QDialogButtonBox::clicked, [&] (QAbstractButton *button) {
        if (ui.buttonBox->standardButton(button) == QDialogButtonBox::Apply)
            applyConfiguration(false);
        if (ui.buttonBox->standardButton(button) == QDialogButtonBox::Save)
        {
            applyConfiguration(true);
        }

    });

    connect(ui.settingsButton, SIGNAL(clicked()), this, SLOT(showSettingsDialog()));
}

MonitorSettingsDialog::~MonitorSettingsDialog()
{
}

void MonitorSettingsDialog::loadConfiguration(KScreen::ConfigPtr config)
{
    if (mConfig == config)
        return;

    mConfig = config;

    MonitorPictureDialog *monitorPicture = nullptr;
    FastMenu *fastMenu = nullptr;
    KScreen::OutputList outputs = mConfig->outputs();

    int nMonitors = 0;
    for (const KScreen::OutputPtr &output : outputs)
    {
        if (output->isConnected())
            nMonitors++;

        if (nMonitors > 1)
        {
            fastMenu = new FastMenu(config, this);
            ui.monitorList->addItem(tr("Fast Menu"));
            ui.stackedWidget->addWidget(fastMenu);
            
            monitorPicture = new MonitorPictureDialog(config, this);
            ui.monitorList->addItem(tr("Set position"));
            ui.stackedWidget->addWidget(monitorPicture);
            break;
        }
    }

    QList<MonitorWidget*> monitors;

    for (const KScreen::OutputPtr &output : outputs)
    {
        if (output->isConnected())
        {
            MonitorWidget *monitor = new MonitorWidget(output, mConfig, this);
            ui.monitorList->addItem(output->name());
            ui.stackedWidget->addWidget(monitor);
            monitors.append(monitor);
        }
    }
    
    Q_FOREACH(MonitorWidget *monitor1, monitors)
    {
        Q_FOREACH(MonitorWidget *monitor, monitors)
        {
            if(monitor != monitor1)
                connect(monitor, SIGNAL(primaryOutputChanged(MonitorWidget *)), monitor1, SLOT(onPrimaryOutputChanged(MonitorWidget *)));
        }
    }

    if (monitorPicture)
        monitorPicture->setScene(monitors);

    ui.monitorList->setCurrentRow(0);
    adjustSize();
}

void MonitorSettingsDialog::applyConfiguration(bool saveConfigOk)
{
    if (mConfig && KScreen::Config::canBeApplied(mConfig))
    {
        KScreen::SetConfigOperation(mConfig).exec();

        TimeoutDialog mTimeoutDialog;
        if (mTimeoutDialog.exec() == QDialog::Rejected)
            KScreen::SetConfigOperation(mOldConfig).exec();
        else
        {
            mOldConfig = mConfig->clone();
            if (saveConfigOk)
                saveConfiguration(mConfig);
        }
    }
}

void MonitorSettingsDialog::accept()
{
    //applyConfiguration(true);
    QDialog::accept();
}

void MonitorSettingsDialog::reject()
{
    QDialog::reject();
}

void MonitorSettingsDialog::saveConfiguration(KScreen::ConfigPtr config)
{
    
    QList<MonitorSettings> currentSettings;
    KScreen::OutputList outputs = config->outputs();
    for (const KScreen::OutputPtr &output : outputs)
    {
        MonitorSettings monitor;
        monitor.name = output->name();
        KScreen::Edid* edid = output->edid();
        if (edid && edid->isValid())
            monitor.hash = edid->hash();
        monitor.connected = output->isConnected();
        if ( output->isConnected() )
        {
            monitor.enabled = output->isEnabled();
            monitor.primary = output->isPrimary();
            monitor.xPos = output->pos().x();
            monitor.yPos = output->pos().y();
            monitor.currentMode = output->currentModeId();
            monitor.currentModeWidth = output->currentMode()->size().width();
            monitor.currentModeHeight = output->currentMode()->size().height();
            monitor.currentModeRate = output->currentMode()->refreshRate();
            monitor.rotation = output->rotation();
        }
        currentSettings.append(monitor);
    }
    
    LXQt::Settings settings("lxqt-config-monitor");
    settings.beginGroup("currentConfig");
    saveMonitorSettings(settings, currentSettings);
    settings.endGroup();
    
    QList<MonitorSavedSettings> monitors;
    settings.beginGroup("SavedConfigs");
    loadMonitorSettings(settings, monitors);
    qDebug() << "[ MonitorSettingsDialog::saveConfiguration] # monitors Read:" << monitors.size();
    MonitorSavedSettings monitor;
    monitor.name = QDateTime::currentDateTime().toString();
    monitor.date = QDateTime::currentDateTime().toString(Qt::ISODate);
    monitor.monitors = currentSettings;
    monitors.append(monitor);
    saveMonitorSettings(settings, monitors);
    qDebug() << "[ MonitorSettingsDialog::saveConfiguration] # monitors Write:" << monitors.size();
    settings.endGroup();

    LXQt::AutostartEntry autoStart("lxqt-config-monitor-autostart.desktop");
    XdgDesktopFile desktopFile(XdgDesktopFile::ApplicationType, "lxqt-config-monitor-autostart", "lxqt-config-monitor -l");
    //desktopFile.setValue("OnlyShowIn", QString(qgetenv("XDG_CURRENT_DESKTOP")));
    desktopFile.setValue("OnlyShowIn", "LXQt");
    desktopFile.setValue("Comment", "Autostart monitor settings for LXQt-config-monitor");
    autoStart.setFile(desktopFile);
    autoStart.commit();
}


void MonitorSettingsDialog::showSettingsDialog()
{
    QByteArray configName = qgetenv("LXQT_SESSION_CONFIG");

    if (configName.isEmpty())
        configName = "MonitorSettings";

    LXQt::Settings settings(configName);

    SettingsDialog settingsDialog(tr("Advanced settings"), &settings, mConfig);
    settingsDialog.exec();
}
