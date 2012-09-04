/***************************************************************************
                          prefdialog.cpp  -  description
                             -------------------
    begin                : Thu Dec 12 2003
    copyright            : (C) 2003 by Lynn Hazan
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
// include files for QT
#include <qlayout.h>        // for QVBoxLayout
#include <qlabel.h>         // for QLabel
#include <q3frame.h>         // for QFrame
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <QMessageBox>

//include files for the application
#include "prefdialog.h"     // class PrefDialog

#include "configuration.h"          // class Configuration and Config()
#include "prefgeneral.h"            // class PrefGeneral
#include "prefwaveformview.h" // class PrefWaveformView
#include "prefclusterview.h"        // class PrefClusterView
#include "channellist.h"

//General C++ include files
#include <iostream>
using namespace std;

/**
  *@author Lynn Hazan
*/

PrefDialog::PrefDialog(QWidget *parent,int nbChannels, const char *name, Qt::WFlags f)
 : QPageDialog(parent)
{

    setButtons(Help | Default | Ok | Apply | Cancel);
    setDefaultButton(Ok);
    setFaceType(List);
    setCaption(tr("Preferences"));

    setHelp("settings","klusters");
    
    /*
    //adding page "General options"
    Q3Frame *frame = addPage(tr("General"), tr("Klusters General Configuration"),
        KGlobal::iconLoader()->loadIcon("kfm",KIcon::Panel,0,false) );
    */

    QWidget * w = new QWidget(this);
    QVBoxLayout*lay = new QVBoxLayout;
    w->setLayout(lay);
    prefGeneral = new PrefGeneral(w);
    addPage(w,tr("General"));



    //adding page "Cluster view configuration"
    /*
    frame = addPage(tr("Cluster view"), tr("Cluster View configuration"),
        QIcon(":icons/clusterview"));
    frameLayout = new Q3VBoxLayout( frame, 0, 0 );
    prefclusterView = new PrefClusterView(frame);
    frameLayout->addWidget(prefclusterView);
*/
    w = new QWidget(this);
    lay = new QVBoxLayout;
    prefclusterView = new PrefClusterView(w);
    addPage(w,tr("Cluster view"));



    //adding page "Waveform view configuration"
    /*
    frame = addPage(tr("Waveform view"), tr("Waveform View configuration"),
        QIcon(":icons/waveformview"));
    frameLayout = new Q3VBoxLayout( frame, 0, 0 );
    prefWaveformView = new PrefWaveformView(frame,nbChannels);
    frameLayout->addWidget(prefWaveformView);
*/
    w = new QWidget(this);
    lay = new QVBoxLayout;
    prefWaveformView = new PrefWaveformView(w,nbChannels);
    addPage(w,tr("Waveform view"));



    // connect interactive widgets and selfmade signals to the enableApply slotDefault
    connect(prefGeneral->crashRecoveryCheckBox,SIGNAL(clicked()),this,SLOT(enableApply()));
    connect(prefGeneral->crashRecoveryComboBox,SIGNAL(activated(int)),this,SLOT(enableApply()));
    connect(prefGeneral->undoSpinBox,SIGNAL(valueChanged(int)),this,SLOT(enableApply()));
    connect(prefGeneral->backgroundColorButton,SIGNAL(changed(const QColor&)),this,SLOT(enableApply()));
    connect(prefGeneral->reclusteringExecutableLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    //connect(prefGeneral,SIGNAL(reclusteringArgsUpdate()),this,SLOT(enableApply()));
    connect(prefGeneral->reclusteringArgsLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    
    connect(prefclusterView->intervalSpinBox,SIGNAL(valueChanged(int)),this,SLOT(enableApply()));
    connect(prefWaveformView->gainSpinBox,SIGNAL(valueChanged(int)),this,SLOT(enableApply()));
    connect(prefWaveformView,SIGNAL(positionsChanged()),this,SLOT(enableApply()));


    connect(this, SIGNAL(applyClicked()), SLOT(slotApply()));
    connect(this, SIGNAL(defaultClicked()), SLOT(slotDefault()));


    applyEnable = false;
}

void PrefDialog::updateDialog() {  
  prefGeneral->setCrashRecovery(configuration().isCrashRecovery());
  prefGeneral->setCrashRecoveryIndex(configuration().crashRecoveryIntervalIndex());
  prefGeneral->setNbUndo(configuration().getNbUndo());
  prefGeneral->setBackgroundColor(configuration().getBackgroundColor());
  prefGeneral->setReclusteringExecutable(configuration().getReclusteringExecutable());
  prefGeneral->setReclusteringArguments(configuration().getReclusteringArguments()); 
  prefclusterView->setTimeInterval(configuration().getTimeInterval());
  prefWaveformView->setGain(configuration().getGain());
  
  enableButtonApply(false);   // disable apply button
  applyEnable = false;
}
 

void PrefDialog::updateConfiguration(){
  configuration().setCrashRecovery(prefGeneral->isCrashRecovery());
  configuration().setCrashRecoveryIndex(prefGeneral->crashRecoveryIntervalIndex());
  configuration().setNbUndo(prefGeneral->getNbUndo());
  configuration().setBackgroundColor(prefGeneral->getBackgroundColor()); 
  configuration().setReclusteringExecutable(prefGeneral->getReclusteringExecutable());
  configuration().setReclusteringArguments(prefGeneral->getReclusteringArguments());
  configuration().setTimeInterval(prefclusterView->getTimeInterval());
  configuration().setGain(prefWaveformView->getGain());
  configuration().setNbChannels(prefWaveformView->getNbChannels());
  configuration().setChannelPositions(prefWaveformView->getChannelPositions()); 
      
  enableButtonApply(false);   // disable apply button
  applyEnable = false;
}


void PrefDialog::slotDefault() {
  if (QMessageBox::warning(this, tr("Set default options?"), tr("This will set the default options "
      "in ALL pages of the preferences dialog! Do you wish to continue?"),
      tr("Set defaults"))==QMessageBox::Ok){
        
   prefGeneral->setCrashRecovery(configuration().isCrashRecoveryDefault());
   prefGeneral->setCrashRecoveryIndex(configuration().crashRecoveryIntervalIndexDefault());
   prefGeneral->setNbUndo(configuration().getNbUndoDefault());
   prefGeneral->setBackgroundColor(configuration().getBackgroundColorDefault());
   prefGeneral->setReclusteringExecutable(configuration().getReclusteringExecutableDefault());
   prefGeneral->setReclusteringArguments(configuration().getReclusteringArgumentsDefault()); 
   prefclusterView->setTimeInterval(configuration().getTimeIntervalDefault());
   prefWaveformView->setGain(configuration().getGainDefault());
   prefWaveformView->resetChannelList(configuration().getNbChannels());
   
   enableApply();   // enable apply button
  }
}


void PrefDialog::slotApply() {
  updateConfiguration();      // transfer settings to configuration object
  emit settingsChanged();     // apply the preferences    
  enableButtonApply(false);   // disable apply button again
}


void PrefDialog::enableApply() {
    enableButtonApply(true);   // enable apply button
    applyEnable = true;
}

void PrefDialog::resetChannelList(int nbChannels){
  prefWaveformView->resetChannelList(nbChannels);
}

void PrefDialog::enableChannelSettings(bool state){
  prefWaveformView->enableChannelSettings(state);
}
    


#include "prefdialog.moc"
