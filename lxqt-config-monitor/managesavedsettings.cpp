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


#include "managesavedsettings.h"
#include "loadsettings.h"
#include "configure.h"
#include "monitor.h"
#include <QDebug>
#include <QInputDialog>
#include <QDateTime>

Q_DECLARE_METATYPE(MonitorSavedSettings)


ManageSavedSettings::ManageSavedSettings(LXQt::Settings * applicationSettings,  KScreen::ConfigPtr config, QWidget * parent):
QDialog(parent)
{

    this->applicationSettings = applicationSettings;
    this->config = config;

    ui.setupUi(this);

    connect(ui.allConfigs, SIGNAL(itemActivated(QListWidgetItem *)), SLOT(showSelectedConfig(QListWidgetItem *)));
    connect(ui.deletePushButton, SIGNAL(clicked()), SLOT(onDeleteItem()));
    connect(ui.renamePushButton, SIGNAL(clicked()), SLOT(onRenameItem()));
    connect(ui.applyPushButton, SIGNAL(clicked()), SLOT(onApplyItem()));

    loadSettings();
}


void ManageSavedSettings::showSelectedConfig(QListWidgetItem * item)
{
    MonitorSavedSettings o = item->data(Qt::UserRole).value<MonitorSavedSettings>();
    QString text;
    for(int i=0; i < o.monitors.size(); i++)
    {
        MonitorSettings setting = o.monitors[i];
        if(! setting.connected )
            continue;
        text += QString("<b>%1:</b><br/>").arg(setting.name);
        text += QString("&nbsp;Mode: %1x%2<br/>").arg(setting.currentModeWidth).arg(setting.currentModeHeight);
        text += QString("&nbsp;Rate: %1 Hz<br/>").arg(setting.currentModeRate);
        switch(setting.rotation)
        {
            case KScreen::Output::Rotation::None:
                text += QString("&nbsp;Rotation: %1<br/>").arg(tr("None"));
                break;
            case KScreen::Output::Rotation::Left:
                text += QString("&nbsp;Rotation: %1<br/>").arg(tr("Left"));
                break;
            case KScreen::Output::Rotation::Inverted:
                text += QString("&nbsp;Rotation: %1<br/>").arg(tr("Inverted"));
                break;
            case KScreen::Output::Rotation::Right:
                text += QString("&nbsp;Rotation: %1<br/>").arg(tr("Right"));
                break;
        }
        text += QString("&nbsp;Position: %1x%2<br/>").arg(setting.xPos).arg(setting.yPos);
        text += QString("&nbsp;Primary: %1<br/>").arg(setting.primary?tr("True"):tr("False"));
        text += QString("&nbsp;Enabled: %1<br/>").arg(setting.enabled?tr("True"):tr("False"));
    }
    text += "<br/>";
    ui.selectedSettingsTextEdit->setText(text);
    ui.applyPushButton->setEnabled(isHardwareCompatible(o));
}


bool ManageSavedSettings::isHardwareCompatible(MonitorSavedSettings &settings)
{
    KScreen::OutputList outputs = config->outputs();
    for (const KScreen::OutputPtr &output : outputs)
    {
        bool ok = false;
        for (int i=0; i < settings.monitors.size(); i++)
        {
            MonitorSettings o = settings.monitors[i];
            if(o.name != output->name())
                continue;
            KScreen::Edid *edid = output->edid();
            if(edid && edid->isValid())
                if(o.hash != output->edid()->hash())
                    return false;
            ok = true;
            break;
        }
        if(!ok)
            return false;
    }
    return true;
}

void ManageSavedSettings::onDeleteItem()
{
    if (ui.allConfigs->currentItem() == NULL)
        return;
    MonitorSavedSettings obj = ui.allConfigs->currentItem()->data(Qt::UserRole).value<MonitorSavedSettings>();

    LXQt::Settings settings("lxqt-config-monitor");
    QList<MonitorSavedSettings> monitors;
    settings.beginGroup("SavedConfigs");
    loadMonitorSettings(settings, monitors);
    for (int i = 0; i < monitors.size(); i++) {
        MonitorSavedSettings o = monitors[i];
        if (o == obj) {
            monitors.removeAt(i);
            break;
        }
    }
    saveMonitorSettings(settings, monitors);

    settings.endGroup();

    loadSettings();
}

void ManageSavedSettings::onRenameItem()
{
    if (ui.allConfigs->currentItem() == NULL)
        return;
    MonitorSavedSettings obj = ui.allConfigs->currentItem()->data(Qt::UserRole).value<MonitorSavedSettings>();
    bool ok;
    QString configName = QInputDialog::getText(this, tr("Name"), tr("Name:"),
                                               QLineEdit::Normal,
                                               obj.name, &ok);
    if (!ok || configName.isEmpty())
        return;

    LXQt::Settings settings("lxqt-config-monitor");
    QList<MonitorSavedSettings> monitors;
    settings.beginGroup("SavedConfigs");
    loadMonitorSettings(settings, monitors);
    for (int i = 0; i < monitors.size(); i++) {
        MonitorSavedSettings o = monitors[i];
        if (o == obj) {
            monitors.removeAt(i);
            obj.name = configName;
            monitors.append(obj);
            break;
        }
    }
    saveMonitorSettings(settings, monitors);

    settings.endGroup();

    loadSettings();
}

void ManageSavedSettings::onApplyItem()
{
    if (ui.allConfigs->currentItem() == NULL)
        return;
    MonitorSavedSettings settings = ui.allConfigs->currentItem()->data(Qt::UserRole).value<MonitorSavedSettings>();
    applySettings(config, settings.monitors);
}



void ManageSavedSettings::loadSettings()
{
    ui.allConfigs->clear();
    //ui.hardwareCompatibleConfigs->clear();
    LXQt::Settings settings("lxqt-config-monitor");
    QList<MonitorSavedSettings> monitors;
    settings.beginGroup("SavedConfigs");
    loadMonitorSettings(settings, monitors);
    settings.endGroup();
    foreach(MonitorSavedSettings o, monitors) {
        QListWidgetItem *item = new QListWidgetItem(o.name+" - "+o.date, ui.allConfigs);
        QVariant var;
        var.setValue(o);
        item->setData(Qt::UserRole, var);
        if (isHardwareCompatible(o)) {
            QFont font = ui.allConfigs->font();
            font.setBold(true);
            item->setData(Qt::FontRole, font);
        }
    }
}
