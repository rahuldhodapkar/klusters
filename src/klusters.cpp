/***************************************************************************
                          klusters.cpp  -  description
                             -------------------
    begin                : Mon Sep  8 12:06:21 EDT 2003
    copyright            : (C) 2003 by Lynn Hazan
    email                : lynn.hazan@myrealbox.com
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
#include <qdir.h>
#include <q3vbox.h>
#include <q3whatsthis.h>
#include <qtooltip.h>
#include <qtoolbutton.h>
#include <qstring.h>
#include <qimage.h>
#include <qicon.h>  
#include <qcursor.h>
#include <qfileinfo.h> 
#include <qapplication.h>
#include <qinputdialog.h>
#include <QPrinter>
//Added by qt3to4:
#include <QLabel>
#include <QPixmap>
#include <Q3ValueList>
#include <QEvent>
#include <Q3Frame>
#include <QCustomEvent>
#include <QDebug>
#include <QStatusBar>
#include <QProcess>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QKeySequence>
#include <QFileDialog>
// application specific includes
#include "klusters.h"
#include "klustersdoc.h"
#include "clusterPalette.h"
#include "savethread.h"
#include "prefdialog.h"
#include "configuration.h"  // class Configuration
#include "processwidget.h"

extern int nbUndo;

const QString KlustersApp::INITIAL_WAVEFORM_TIME_WINDOW = "30";
const long KlustersApp::DEFAULT_NB_SPIKES_DISPLAYED = 100;
const QString KlustersApp::INITIAL_CORRELOGRAMS_HALF_TIME_FRAME = "30";
const QString KlustersApp::DEFAULT_BIN_SIZE = "1";


KlustersApp::KlustersApp()
    : QMainWindow(0,"Klusters"), displayCount(0),mainDock(0),
      clusterPanel(0),clusterPalette(0),tabsParent(0),dimensionX(0L),dimensionY(0L),isInit(true),
      currentNbUndo(0),currentNbRedo(0),timeWindow(INITIAL_WAVEFORM_TIME_WINDOW.toLong()),validator(this),spikesTodisplayStep(20),
      correlogramTimeFrame(INITIAL_CORRELOGRAMS_HALF_TIME_FRAME.toInt() * 2 + 1),binSize(DEFAULT_BIN_SIZE.toInt()),binSizeValidator(this),
      correlogramsHalfTimeFrameValidator(this),prefDialog(0L),processWidget(0L),processFinished(true),processOutputDock(0L),
      processOutputsFinished(true),processKilled(false),errorMatrixExists(false),filePath("")
{

    //Gets the configuration object of the application throught the static reference to the application kapp
    printer = new QPrinter;

    //Apply the user settings.
    initializePreferences();

    ///////////////////////////////////////////////////////////////////
    // call inits to invoke all other construction parts
    initStatusBar();
    initClusterPanel();

    //Create a KlustersDoc which will hold the document manipulated by the application.
    doc = new KlustersDoc(this,*clusterPalette,configuration().isCrashRecovery(),configuration().crashRecoveryInterval());
    //create the thread which will be used to save the cluster file
    saveThread = new SaveThread(this);

    //Prepare the actions
    initActions();

    createMenus();

    //Prepare the spineboxes and line edit
    initSelectionBoxes();

    setMinimumSize(QSize(600,400));

    // Apply the saved mainwindow settings, if any, and ask the mainwindow
    // to automatically save settings if changed: window size, toolbar
    // position, icon size, etc.
    //KDAB_PENDING setAutoSaveSettings();

    slotUpdateParameterBar();

    // initialize the recent file list
    //KDAB_PENDING fileOpenRecent->loadEntries(config);

    //Disable some actions at startup (see the klustersui.rc file)
    //KDAB_PENDING slotStateChanged("initState");
}

KlustersApp::~KlustersApp()
{
    //Clear the memory by deleting all the pointers
    delete printer;
    delete doc;
    delete saveThread;
    if(processWidget != 0L){
        delete processWidget;
        processWidget = 0L;
    }
    if(processOutputDock != 0L){
        delete processOutputDock;
        processOutputDock = 0L;
    }
}

void KlustersApp::createMenus()
{
    //File Menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    mOpenAction = fileMenu->addAction(tr("&Open..."));
    mOpenAction->setShortcut(QKeySequence::Open);
    connect(mOpenAction, SIGNAL(triggered()), this, SLOT(slotFileOpen()));

    //KDAB_TODO fileOpenRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const QString&)), actionCollection());

    mSaveAction = fileMenu->addAction(tr("Save..."));
    mSaveAction->setShortcut(QKeySequence::Save);
    connect(mSaveAction, SIGNAL(triggered()), this, SLOT(slotFileSave()));

    mSaveAsAction = fileMenu->addAction(tr("&Save As..."));
    mSaveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(mSaveAsAction, SIGNAL(triggered()), this, SLOT(slotFileSaveAs()));

    mCloseAction = fileMenu->addAction(tr("Close"));
    mCloseAction->setShortcut(QKeySequence::Close);
    connect(mCloseAction, SIGNAL(triggered()), this, SLOT(slotFileClose()));

    mPrintAction = fileMenu->addAction(tr("Print"));
    mPrintAction->setShortcut(QKeySequence::Print);
    connect(mPrintAction, SIGNAL(triggered()), this, SLOT(slotFilePrint()));

    mImportFile = fileMenu->addAction(tr("&Import File"));
    mImportFile->setShortcut(Qt::CTRL + Qt::Key_I);
    connect(mImportFile,SIGNAL(triggered()), this,SLOT(slotFileImport()));

    mQuitAction = fileMenu->addAction(tr("Quit"));
    mQuitAction->setShortcut(QKeySequence::Print);
    connect(mQuitAction, SIGNAL(triggered()), this, SLOT(close()));

    mRenumberAndSave = fileMenu->addAction(tr("Re&number and Save"));
    mRenumberAndSave->setIcon(QIcon(QPixmap("filesave.png")));
    mRenumberAndSave->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_S);
    connect(mRenumberAndSave,SIGNAL(triggered()), this,SLOT(slotFileRenumberAndSave()));


#if 0
    viewMainToolBar = KStdAction::showToolbar(this, SLOT(slotViewMainToolBar()), actionCollection());
    viewStatusBar = KStdAction::showStatusbar(this, SLOT(slotViewStatusBar()), actionCollection());

#endif


    //Edit Menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    mSelectAllAction = editMenu->addAction(tr("Select &All"));
    mSelectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(mSelectAllAction, SIGNAL(triggered()), this, SLOT(slotSelectAll()));

    mSelectAllExceptAction = editMenu->addAction(tr("Select &All"));
    mSelectAllExceptAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_A);
    connect(mSelectAllAction, SIGNAL(triggered()), this, SLOT(slotSelectAllWO01()));


    mUndo = editMenu->addAction(tr("Undo"));
    mUndo->setShortcut(QKeySequence::Undo);
    connect(mUndo, SIGNAL(triggered()), this, SLOT(slotUndo()));

    mRedo = editMenu->addAction(tr("Redo"));
    mRedo->setShortcut(QKeySequence::Redo);
    connect(mRedo, SIGNAL(triggered()), this, SLOT(slotRedo());


    mSelectAllExceptAction = editMenu->addAction(tr("Select &All"));
    mSelectAllExceptAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_A);
    connect(mSelectAllAction, SIGNAL(triggered()), this, SLOT(slotSelectAllWO01()));

    //Displays menu
    QMenu *displayMenu = menuBar()->addMenu(tr("&Displays"));
    //viewMenu = new QActionMenu(tr("&Window"), actionCollection(), "window_menu");
    newClusterDisplay = displayMenu->addAction(tr("New C&luster Display"));
    connect(newClusterDisplay,SIGNAL(triggered()), this,SLOT(slotWindowNewClusterDisplay()));

    newWaveformDisplay = displayMenu->addAction(tr("New &Waveform Display"));
    connect(newWaveformDisplay,SIGNAL(triggered()), this,SLOT(slotWindowNewWaveformDisplay()));

    newCrosscorrelationDisplay = displayMenu->addAction(tr("New C&orrelation Display"));
    connect(newCrosscorrelationDisplay,SIGNAL(triggered()), this,SLOT(slotWindowNewCrosscorrelationDisplay()));

    newOverViewDisplay = displayMenu->addAction(tr("New &Overview Display"));
    connect(newOverViewDisplay,SIGNAL(triggered()), this,SLOT(slotWindowNewOverViewDisplay()));

    newGroupingAssistantDisplay = displayMenu->addAction(tr("New &Grouping Assistant Display"));
    connect(newGroupingAssistantDisplay,SIGNAL(triggered()), this,SLOT(slotWindowNewGroupingAssistantDisplay()));

    mRenameActiveDisplay = displayMenu->addAction(tr("&Rename Active Display"));
    mRenameActiveDisplay->setShortcut(Qt::CTRL + Qt::Key_R);
    connect(mRenameActiveDisplay,SIGNAL(triggered()), this,SLOT(renameActiveDisplay()));

    mCloseActiveDisplay = displayMenu->addAction(tr("&Close Active Display"));
    mCloseActiveDisplay->setShortcut(Qt::CTRL + Qt::Key_W);
    connect(mCloseActiveDisplay,SIGNAL(triggered()), this,SLOT(slotDisplayClose()));

    mNewTraceDisplay = displayMenu->addAction(tr("New &Trace Display"));
    connect(mNewTraceDisplay,SIGNAL(triggered()), this,SLOT(slotNewTraceDisplay()));

    //Actions menu
    QMenu *actionMenu = menuBar()->addMenu(tr("&Actions"));
    mDeleteArtifact = actionMenu->addAction(tr("Delete &Artifact Cluster(s)"));
    mDeleteArtifact->setIcon(QIcon(":/icons/delete_artefact"));
    mDeleteArtifact->setShortcut(Qt::SHIFT + Qt::Key_Delete);
    connect(mDeleteArtifact,SIGNAL(triggered()), clusterPalette,SLOT(moveClustersToArtefact()));

    mDeleteNoisy = actionMenu->addAction(tr("Delete &Noisy Cluster(s)"));
    mDeleteNoisy->setIcon(QIcon(":/icons/delete_noise"));
    mDeleteNoisy->setShortcut(Qt::Key_Delete);
    connect(mDeleteNoisy,SIGNAL(triggered()), clusterPalette,SLOT(moveClustersToNoise()));

    mGroupeClusters = actionMenu->addAction(tr("&Group Clusters"));
    mGroupeClusters->setIcon(QIcon(":/icons/group"));
    mGroupeClusters->setShortcut(Qt::Key_G);
    connect(mGroupeClusters,SIGNAL(triggered()), clusterPalette,SLOT(groupClusters()));

    mUpdateDisplay = actionMenu->addAction(tr("&Update Display"));
    mUpdateDisplay->setIcon(QIcon(":/icons/update"));
    connect(mUpdateDisplay,SIGNAL(triggered()), clusterPalette,SLOT(updateClusters()));

    mRenumberClusters = actionMenu->addAction(tr("&Renumber Clusters"));
    mRenumberClusters->setShortcut(Qt::Key_R);
    connect(mRenumberClusters,SIGNAL(triggered()), doc,SLOT(renumberClusters()));

    mUpdateErrorMatrix = actionMenu->addAction(tr("&Update Error Matrix"));
    mUpdateErrorMatrix->setShortcut(Qt::Key_U);
    connect(mUpdateErrorMatrix,SIGNAL(triggered()), doc,SLOT(slotUpdateErrorMatrix()));
    mReCluster = actionMenu->addAction(tr("Re&cluster"));
    mReCluster->setShortcut(Qt::SHIFT  + Qt::Key_R);
    connect(mReCluster,SIGNAL(triggered()), this,SLOT(slotRecluster()));

    mAbortReclustering = actionMenu->addAction(tr("&Abort Reclustering"));
    connect(mAbortReclustering,SIGNAL(triggered()), this,SLOT(slotStopRecluster()));

    //Tools menu
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    mZoomAction = toolsMenu->addAction(tr("Zoom"));
    mZoomAction->setIcon(QIcon(":/icons/zoom_tool"));
    mZoomAction->setShortcut(Qt::Key_Z);
    connect(mZoomAction,SIGNAL(triggered()), this,SLOT(slotZoom()));

    mNewCluster = toolsMenu->addAction(tr("New Cluster"));
    mNewCluster->setIcon(QIcon(":/icons/new_cluster"));
    mNewCluster->setShortcut(Qt::Key_C);
    connect(mNewCluster,SIGNAL(triggered()), this,SLOT(slotSingleNew()));

    mSplitClusters = toolsMenu->addAction(tr("&Split Clusters"));
    mSplitClusters->setIcon(QIcon(":/icons/new_clusters"));
    mSplitClusters->setShortcut(Qt::Key_S);
    connect(mSplitClusters,SIGNAL(triggered()), this,SLOT(slotMultipleNew()));

    mDeleteArtifactSpikes = toolsMenu->addAction(tr("Delete &Artifact Spikes"));
    mDeleteArtifactSpikes->setIcon(QIcon(":/icons/delete_artefact_tool"));
    mDeleteArtifactSpikes->setShortcut(Qt::Key_A);
    connect(mDeleteArtifactSpikes,SIGNAL(triggered()), this,SLOT(slotDeleteArtefact()));

    mDeleteNoisySpikes = toolsMenu->addAction(tr("Delete &Noisy Spikes"));
    mDeleteNoisySpikes->setIcon(QIcon(":/icons/delete_noise_tool"));
    mDeleteNoisySpikes->setShortcut(Qt::Key_N);
    connect(mDeleteNoisySpikes,SIGNAL(triggered()), this,SLOT(slotDeleteNoise()));

    mSelectTime = toolsMenu->addAction(tr("Select Time"));
    mSelectTime->setIcon(QIcon(":/icons/time_tool"));
    mSelectTime->setShortcut(Qt::Key_W);
    connect(mSelectTime,SIGNAL(triggered()), this,SLOT(slotSelectTime()));

    //Waveforms menu
    QMenu *waveFormsMenu = menuBar()->addMenu(tr("&Waveforms"));
    timeFrameMode = waveFormsMenu->addAction(tr("&Time Frame"));
    timeFrameMode->setShortcut(Qt::Key_T);
    timeFrameMode->setCheckable(true);
    connect(timeFrameMode,SIGNAL(triggered()), this,SLOT(slotTimeFrameMode()));

    overlayPresentation = waveFormsMenu->addAction(tr("&Overlay"));
    overlayPresentation->setShortcut(Qt::Key_O);
    overlayPresentation->setCheckable(true);
    connect(overlayPresentation,SIGNAL(triggered()), this,SLOT(setOverLayPresentation()));

    meanPresentation = waveFormsMenu->addAction(tr("&Mean and Standard Deviation"));
    meanPresentation->setShortcut(Qt::Key_M);
    meanPresentation->setCheckable(true);
    connect(meanPresentation,SIGNAL(triggered()), this,SLOT(slotMeanPresentation()));


    mIncreaseAmplitude = waveFormsMenu->addAction(tr("&Increase Amplitude"));
    mIncreaseAmplitude->setShortcut(Qt::Key_I);
    connect(mIncreaseAmplitude,SIGNAL(triggered()), this,SLOT(slotIncreaseAmplitude()));

    mDecreaseAmplitude = waveFormsMenu->addAction(tr("&Decrease Amplitude"));
    mDecreaseAmplitude->setShortcut(Qt::Key_D);
    connect(mDecreaseAmplitude,SIGNAL(triggered()), this,SLOT(slotDecreaseAmplitude()));

    timeFrameMode->setChecked(false);
    overlayPresentation->setChecked(false);
    meanPresentation->setChecked(false);

    //Correlations menu
    QMenu *correlationsMenu = menuBar()->addMenu(tr("&Correlations"));
    scaleByMax = correlationsMenu->addAction(tr("Scale by &Maximum"));

    QActionGroup *grp = new QActionGroup(this);
    grp->addAction(scaleByMax);
    scaleByMax->setShortcut(Qt::SHIFT + Qt::Key_M);
    scaleByMax->setCheckable(true);
    connect(scaleByMax,SIGNAL(triggered()), this,SLOT(slotScaleByMax()));

    scaleByShouler = correlationsMenu->addAction(tr("Scale by &Asymptote"));
    grp->addAction(scaleByShouler);
    scaleByShouler->setShortcut(Qt::SHIFT + Qt::Key_A);
    scaleByShouler->setCheckable(true);
    connect(scaleByShouler,SIGNAL(triggered()), this,SLOT(slotScaleByShouler()));

    noScale = correlationsMenu->addAction(tr("&Uniform Scale"));
    grp->addAction(noScale);
    noScale->setShortcut(Qt::SHIFT + Qt::Key_U);
    noScale->setCheckable(true);
    connect(noScale,SIGNAL(triggered()), this,SLOT(slotNoScale()));

    //Initialize the presentation mode to scale by maximum.
    scaleByMax->setChecked(true);
    mIncreaseAmplitudeCorrelation = correlationsMenu->addAction(tr("&Increase Amplitude"));
    mIncreaseAmplitudeCorrelation->setShortcut(Qt::SHIFT + Qt::Key_I);
    connect(mIncreaseAmplitudeCorrelation,SIGNAL(triggered()), this,SLOT(slotIncreaseCorrelogramsAmplitude()));

    mDecreaseAmplitudeCorrelation = correlationsMenu->addAction(tr("&Decrease Amplitude"));
    mDecreaseAmplitudeCorrelation->setShortcut(Qt::SHIFT +  Qt::Key_D);
    connect(mDecreaseAmplitudeCorrelation,SIGNAL(triggered()), this,SLOT(slotDecreaseCorrelogramsAmplitude()));

    shoulderLine = correlationsMenu->addAction(tr("Asymptote &Line"));
    shoulderLine->setShortcut(Qt::Key_L);
    shoulderLine->setCheckable(true);
    connect(shoulderLine,SIGNAL(triggered()), this,SLOT(slotShoulderLine()));




    //Traces menu
    QMenu *traceMenu = menuBar()->addMenu(tr("T&races"));
    mIncreaseChannelAmplitudes = traceMenu->addAction(tr("&Increase Channel Amplitudes"));
    mIncreaseChannelAmplitudes->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_I);
    connect(mIncreaseChannelAmplitudes,SIGNAL(triggered()), this,SLOT(slotIncreaseAllChannelsAmplitude()));

    mDecreaseChannelAmplitudes = traceMenu->addAction(tr("&Decrease Channel Amplitudes"));
    mDecreaseChannelAmplitudes->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_D);
    connect(mDecreaseChannelAmplitudes,SIGNAL(triggered()), this,SLOT(slotDecreaseAllChannelsAmplitude()));

    showHideLabels = traceMenu->addAction(tr("Show &Labels"));
    showHideLabels->setShortcut(Qt::CTRL + Qt::Key_L);
    showHideLabels->setCheckable(true);
    connect(showHideLabels,SIGNAL(triggered()), this,SLOT(slotShowLabels()));

    showHideLabels->setChecked(false);
    mNextSpike = traceMenu->addAction(tr("&Next Spike"));
    mNextSpike->setIcon(QIcon(":/icons/forwardCluster"));
    mNextSpike->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_F);
    connect(mNextSpike,SIGNAL(triggered()), this,SLOT(slotShowNextCluster()));

    mPreviousSpike = traceMenu->addAction(tr("&Previous Spike"));
    mPreviousSpike->setIcon(QIcon(":/icons/backCluster"));
    mPreviousSpike->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_B);
    connect(mPreviousSpike,SIGNAL(triggered()), this,SLOT(slotShowPreviousCluster()));


    //Settings menu
    QMenu *settingsMenu = menuBar()->addMenu(tr("Settings"));
    viewActionBar = settingsMenu->addAction(tr("Show Actions"));
    viewActionBar->setCheckable(true);
    connect(viewActionBar,SIGNAL(triggered()), this,SLOT(slotViewActionBar()));

    viewActionBar->setChecked(true);
    viewToolBar = settingsMenu->addAction(tr("Show Tools"));
    viewToolBar->setCheckable(true);
    connect(viewToolBar,SIGNAL(triggered()), this,SLOT(slotViewToolBar()));

    viewToolBar->setChecked(true);
    viewParameterBar = settingsMenu->addAction(tr("Show Parameters"));
    viewParameterBar->setCheckable(true);
    connect(viewParameterBar,SIGNAL(triggered()), this,SLOT(slotViewParameterBar()));

    viewParameterBar->setChecked(true);
    mImmediateSelection = settingsMenu->addAction(tr("Immediate Update"));
    grp = new QActionGroup(this);
    grp->addAction(mImmediateSelection);
    mImmediateSelection->setCheckable(true);
    connect(mImmediateSelection,SIGNAL(triggered()), this,SLOT(slotImmediateSelection()));

    mDelaySelection = settingsMenu->addAction(tr("Delayed Update"));
    grp->addAction(mDelaySelection);
    mDelaySelection->setCheckable(true);
    connect(mDelaySelection,SIGNAL(triggered()), this,SLOT(slotDelaySelection()));

    viewClusterInfo = settingsMenu->addAction(tr("Show Cluster Info"));
    viewClusterInfo->setCheckable(true);
    connect(viewClusterInfo,SIGNAL(triggered()), this,SLOT(slotViewClusterInfo()));

    viewClusterInfo->setChecked(false);

#if KDAB_PENDING
    //Initialize the update mode
    immediateSelection->setChecked(true);

    KStdAction::preferences(this,SLOT(executePreferencesDlg()), actionCollection());

    fileOpenRecent->setStatusText(tr("Opens a recently used file"));
    actionCollection()->action(KStdAction::stdName(KStdAction::Save))->setStatusText(tr("Saves the actual document"));
    actionCollection()->action(KStdAction::stdName(KStdAction::SaveAs))->setStatusText(tr("Saves the actual document as..."));
    actionCollection()->action(KStdAction::stdName(KStdAction::Close))->setStatusText(tr("Closes the actual document"));
    actionCollection()->action(KStdAction::stdName(KStdAction::Print))->setStatusText(tr("Prints out the actual document"));
    actionCollection()->action(KStdAction::stdName(KStdAction::Quit))->setStatusText(tr("Quits the application"));
    viewMainToolBar->setStatusText(tr("Enables/disables the main tool bar"));
    viewStatusBar->setStatusText(tr("Enables/disables the status bar"));
#endif
    //Custom connections
    connect(clusterPalette, SIGNAL(singleChangeColor(int)), this, SLOT(slotSingleColorUpdate(int)));
    connect(clusterPalette, SIGNAL(updateShownClusters(Q3ValueList<int>)), this, SLOT(slotUpdateShownClusters(Q3ValueList<int>)));
    connect(clusterPalette, SIGNAL(groupClusters(Q3ValueList<int>)), this, SLOT(slotGroupClusters(Q3ValueList<int>)));
    connect(clusterPalette, SIGNAL(moveClustersToNoise(Q3ValueList<int>)), this, SLOT(slotMoveClustersToNoise(Q3ValueList<int>)));
    connect(clusterPalette, SIGNAL(moveClustersToArtefact(Q3ValueList<int>)), this, SLOT(slotMoveClustersToArtefact(Q3ValueList<int>)));
    connect(clusterPalette, SIGNAL(clusterInformationModified()), this, SLOT(slotClusterInformationModified()));
    connect(doc, SIGNAL(updateUndoNb(int)), this, SLOT(slotUpdateUndoNb(int)));
    connect(doc, SIGNAL(updateRedoNb(int)), this, SLOT(slotUpdateRedoNb(int)));
    connect(doc, SIGNAL(spikesDeleted()), this, SLOT(slotSpikesDeleted()));


}

void KlustersApp::initActions()
{

}

void KlustersApp::initSelectionBoxes(){
    QFont font("Helvetica",9);

    QToolBar *topBar = new QToolBar(tr("Tools"));
    addToolBar(Qt::TopToolBarArea, topBar);

    paramBar = addToolBar(tr("Parameters"));

    //Create and initialize the spin boxes for the dimensions
    dimensionX = new QSpinBox(1,1,1,paramBar,"dimensionX");
    dimensionY = new QSpinBox(1,1,1,paramBar,"dimensionY");
    //Enable to step the value from the highest value to the lowest value and vice versa
    dimensionX->setWrapping(true);
    dimensionY->setWrapping(true);
    featureXLabel = new QLabel("Features (x,y) ",paramBar);
    featureXLabel->setFrameStyle(QFrame::StyledPanel|Q3Frame::Plain);
    featureXLabel->setFont(font);
    //Insert the spine boxes in the main tool bar and make the connections
    paramBar->addWidget(featureXLabel);
    paramBar->addWidget(dimensionX);
    paramBar->addWidget(dimensionY);
    connect(dimensionX, SIGNAL(valueChanged(int)),this, SLOT(slotUpdateDimensionX(int)));
    connect(dimensionY, SIGNAL(valueChanged(int)),this, SLOT(slotUpdateDimensionY(int)));

    //Create and initialize the spin boxe and lineEdit for the waveforms time frame mode.
    start = new QSpinBox(1,1,timeWindow,paramBar,"start");
    //Enable to step the value from the highest value to the lowest value and vice versa
    start->setWrapping(true);
    duration = new QLineEdit(paramBar,"INITIAL_WAVEFORM_TIME_WINDOW");
    duration->setMaxLength(5);
    //duration will only accept integers between 0 and a max equal
    //to maximum of time for the current document (set when the document will be opened)
    duration->setValidator(&validator);
    durationLabel = new QLabel("  Duration (s)",paramBar);
    durationLabel->setFrameStyle(QFrame::StyledPanel|Q3Frame::Plain);
    durationLabel->setFont(font);
    startLabel = new QLabel("  Start time (s)",paramBar);
    startLabel->setFrameStyle(QFrame::StyledPanel|Q3Frame::Plain);
    startLabel->setFont(font);
    paramBar->addWidget(startLabel);
    start->setMinimumSize(70,start->minimumHeight());
    start->setMaximumSize(70,start->maximumHeight());
    paramBar->addWidget(start);
    paramBar->addWidget(durationLabel);
    duration->setMinimumSize(70,duration->minimumHeight());
    duration->setMaximumSize(70,duration->maximumHeight());
    paramBar->addWidget(duration);
    connect(start, SIGNAL(valueChanged(int)),this, SLOT(slotUpdateStartTime(int)));
    connect(duration, SIGNAL(returnPressed()),this, SLOT(slotUpdateDuration()));

    //Create and initialize the spin boxe for the waveforms sample mode.
    spikesTodisplay = new QSpinBox(1,1,spikesTodisplayStep,paramBar,"spikesTodisplay");
    //Enable to step the value from the highest value to the lowest value and vice versa
    spikesTodisplay->setWrapping(true);
    spikesTodisplayLabel = new QLabel("  Waveforms",paramBar);
    spikesTodisplayLabel->setFrameStyle(QFrame::StyledPanel|Q3Frame::Plain);
    spikesTodisplayLabel->setFont(font);
    paramBar->addWidget(spikesTodisplayLabel);
    spikesTodisplay->setMinimumSize(70,spikesTodisplay->minimumHeight());
    spikesTodisplay->setMaximumSize(70,spikesTodisplay->maximumHeight());
    paramBar->addWidget(spikesTodisplay);
    connect(spikesTodisplay, SIGNAL(valueChanged(int)),this, SLOT(slotSpikesTodisplay(int)));

    //Create and initialize the lineEdit for the correlations.
    binSizeBox = new QLineEdit(paramBar,"DEFAULT_BIN_SIZE");
    binSizeBox->setMaxLength(10);
    //binSizeBox will only accept integers between 1 and a max equal
    //to maximum of time for the current document in miliseconds (set when the document will be opened)
    binSizeBox->setValidator(&binSizeValidator);
    binSizeLabel = new QLabel("  Bin size (ms)",paramBar);
    binSizeLabel->setFrameStyle(QFrame::StyledPanel|Q3Frame::Plain);
    binSizeLabel->setFont(font);
    paramBar->addWidget(binSizeLabel);
    binSizeBox->setMinimumSize(30,binSizeBox->minimumHeight());
    binSizeBox->setMaximumSize(30,binSizeBox->maximumHeight());
    paramBar->addWidget(binSizeBox);
    connect(binSizeBox, SIGNAL(returnPressed()),this, SLOT(slotUpdateBinSize()));

    correlogramsHalfDuration = new QLineEdit(paramBar,"INITIAL_CORRELOGRAMS_HALF_TIME_FRAME");
    correlogramsHalfDuration->setMaxLength(12);
    //correlogramsHalfDuration will only accept integers between 1 and a max equal
    //to half the maximum of time for the current document in miliseconds (set when the document will be opened)
    correlogramsHalfDuration->setValidator(&correlogramsHalfTimeFrameValidator);
    correlogramsHalfDurationLabel = new QLabel("  Duration (ms)",paramBar);
    correlogramsHalfDurationLabel->setFrameStyle(QFrame::StyledPanel|Q3Frame::Plain);
    correlogramsHalfDurationLabel->setFont(font);
    paramBar->addWidget(correlogramsHalfDurationLabel);
    correlogramsHalfDuration->setMinimumSize(70,correlogramsHalfDuration->minimumHeight());
    correlogramsHalfDuration->setMaximumSize(70,correlogramsHalfDuration->maximumHeight());
    paramBar->addWidget(correlogramsHalfDuration);
    connect(correlogramsHalfDuration, SIGNAL(returnPressed()),this, SLOT(slotUpdateCorrelogramsHalfDuration()));

    //Connect the move function of the parameterBar to slotUpdateParameterBar to always correctly show its contents.
    connect(paramBar, SIGNAL(placeChanged(Q3DockWindow::Place)), this, SLOT(slotUpdateParameterBar()));
    connect(paramBar, SIGNAL(moved(BarPosition)), this, SLOT(slotUpdateParameterBar()));
}

void KlustersApp::executePreferencesDlg(){
    if(prefDialog == 0L){
        if(mainDock) prefDialog = new PrefDialog(this,doc->nbOfchannels());  // create dialog on demand
        else prefDialog = new PrefDialog(this);
        // connect to the "settingsChanged" signal
        connect(prefDialog,SIGNAL(settingsChanged()),this,SLOT(applyPreferences()));
    }
    else{
        //If the dialog has been open the first time before any document has been open
        //the number of channel is zero. If now a document is open, update the number of channels.
        if(configuration().getNbChannels() == 0 && mainDock){
            prefDialog->resetChannelList(doc->nbOfchannels());
        }
    }

    // update the dialog widgets.
    prefDialog->updateDialog();

    if(prefDialog->exec() == QDialog::Accepted){  // execute the dialog
        //if the user did not click the applyButton, save the new settings.
        if(prefDialog->isApplyEnable()){
            prefDialog->updateConfiguration();        // store settings in config object
            applyPreferences();                      // let settings take effect
        }
    }
}

void KlustersApp::applyPreferences() {  
    configuration().write();
    int newNbUndo = configuration().getNbUndo();
    if(nbUndo != newNbUndo){
        if(mainDock)doc->nbUndoChangedCleaning(newNbUndo);
        nbUndo = newNbUndo;
    }

    if(backgroundColor != configuration().getBackgroundColor()){
        backgroundColor = configuration().getBackgroundColor();
        if(mainDock)doc->setBackgroundColor(backgroundColor);
        clusterPalette->changeBackgroundColor(backgroundColor);
    }

    if(waveformsGain != configuration().getGain()){
        waveformsGain = configuration().getGain();
        if(mainDock)doc->setGain(waveformsGain);
    }

    if(displayTimeInterval != configuration().getTimeInterval()){
        displayTimeInterval = configuration().getTimeInterval();
        if(mainDock)doc->setTimeStepInSecond(displayTimeInterval);
    }

    if(configuration().isCrashRecovery()){
        if(mainDock)doc->updateAutoSavingInterval(configuration().crashRecoveryInterval());
        else doc->setAutoSaving(configuration().crashRecoveryInterval());
    }
    else doc->stopAutoSaving();

    if(configuration().getNbChannels() != 0 && channelPositions != (*configuration().getChannelPositions())){
        Q3ValueList<int>* positions = configuration().getChannelPositions();
        channelPositions.clear();
        for(int i = 0; i < static_cast<int>(positions->size()); ++i)
            channelPositions.append((*positions)[i]);
        if(mainDock)doc->setChannelPositions(channelPositions);
    }

    if(reclusteringExecutable != configuration().getReclusteringExecutable())
        reclusteringExecutable = configuration().getReclusteringExecutable();

    if(reclusteringArgs != configuration().getReclusteringArguments())
        reclusteringArgs = configuration().getReclusteringArguments();
}

void KlustersApp::initializePreferences(){
    nbUndo = configuration().getNbUndo();
    waveformsGain = configuration().getGain();
    displayTimeInterval = configuration().getTimeInterval();
    backgroundColor =  configuration().getBackgroundColor();
    reclusteringExecutable =  configuration().getReclusteringExecutable();
    reclusteringArgs = configuration().getReclusteringArguments();
}

void KlustersApp::initStatusBar()
{
    ///////////////////////////////////////////////////////////////////
    // STATUSBAR
    statusBar()->showMessage(tr("Ready."),1);
}


bool KlustersApp::eventFilter(QObject* object,QEvent* event){
    if(object == paramBar && event->type() == 71){//filter the removal of items from the paramBar
        return true;
    }
    return QWidget::eventFilter(object,event);    // standard event processing
}


void KlustersApp::initClusterPanel()
{
    //Creation of the left panel containing the clusters
    clusterPanel = new QDockWidget(tr("The cluster list"),0L);
    clusterPanel->setWindowIcon(QIcon("classnew"));
    //Initialisation of the cluster palette containing the cluster list
    clusterPalette = new ClusterPalette(backgroundColor,clusterPanel,statusBar(),"ClusterPalette");
    //Place the clusterPalette frame in the clusterPanel (the view)
    clusterPanel->setWidget(clusterPalette);
}

void KlustersApp::initDisplay(){
    isInit = true; //prevent the spine boxes or the lineedit and the editline to trigger during initialisation
    timeFrameMode->setChecked(false);
    overlayPresentation->setChecked(false);
    meanPresentation->setChecked(false);
    scaleByMax->setChecked(true);
    dimensionX->show();
    dimensionY->show();
    featureXLabel->show();
    spikesTodisplay->show();
    spikesTodisplayLabel->show();
    correlogramsHalfDuration->setText(INITIAL_CORRELOGRAMS_HALF_TIME_FRAME);
    correlogramsHalfDuration->show();
    correlogramsHalfDurationLabel->show();
    binSizeBox->setText(DEFAULT_BIN_SIZE);
    binSizeBox->show();
    binSizeLabel->show();
    shoulderLine->setChecked(true);

    //Set the range value of the spine boxes
    dimensionX->setRange(1,doc->nbDimensions());
    dimensionY->setRange(1,doc->nbDimensions());
    maximumTime = doc->maxTime();
    start->setRange(0,maximumTime);
    validator.setRange(0,maximumTime);
    long totalNbSpikes = doc->totalNbOfSpikes();
    spikesTodisplay->setRange(1,totalNbSpikes);
    maximumTime *= 1000;
    correlogramsHalfTimeFrameValidator.setRange(0,static_cast<int>((maximumTime - 1) / 2));
    binSizeValidator.setRange(0,maximumTime);

    //Create the mainDock (first view)
    mainDock = new QDockWidget( tr(doc->documentName().toLatin1()),0);

            //KDAB_PENDING look at last element createDockWidget( "1", QPixmap(), 0L, tr(doc->documentName().toLatin1()), "Overview Display");
    mainDock->setDockWindowTransient(this,true);

    //If the setting dialog exists (has already be open once), enable the settings for the channels.
    if(prefDialog != 0L) prefDialog->enableChannelSettings(true);

    //No clusters are shown by default.
    Q3ValueList<int>* clusterList = new Q3ValueList<int>();
    
    //Update the dimension and start spine boxes
    dimensionX->setValue(1);
    dimensionY->setValue(2);

    isInit = false; //now a change in a spine box or the lineedit will trigger an update of the display

    //If 2 documents, opened one after the other, do not have the same number of channels
    //discard any settings concerning the positions of the channels.
    if(configuration().getNbChannels() != 0 && configuration().getNbChannels() != doc->nbOfchannels()) channelPositions.clear();

    KlustersView* view = new KlustersView(*this,*doc,backgroundColor,1,2,clusterList,KlustersView::OVERVIEW,mainDock,0,Qt::WDestructiveClose,statusBar(),
                                          displayTimeInterval,waveformsGain,channelPositions,false,0,timeWindow,DEFAULT_NB_SPIKES_DISPLAYED,
                                          false,false,DEFAULT_BIN_SIZE.toInt(),INITIAL_CORRELOGRAMS_HALF_TIME_FRAME.toInt() * 2 + 1,Data::MAX);

    view->installEventFilter(this);

    //Keep track of the number of displays
    displayCount ++;

    //Update the document's list of view
    doc->addView(view);

    mainDock->setWidget(view);
    //allow dock on the left side only
    mainDock->setDockSite(QDockWidget::DockLeft);
    setView(mainDock); // central widget in a KDE mainwindow <=> setMainWidget
    setMainDockWidget(mainDock);
    //disable docking abilities of mainDock itself
    mainDock->setEnableDocking(QDockWidget::DockNone);

    //Initialize and dock the clusterpanel
    //Create the cluster list and select the clusters which will be drawn
    clusterPalette->createClusterList(doc);
    clusterPalette->selectItems(*clusterList);

    //allow dock on the right side only
    clusterPanel->setDockSite(QDockWidget::DockRight);

    //Dock the clusterPanel on the left
    clusterPanel->setEnableDocking(QDockWidget::DockFullSite);
    clusterPanel->manualDock(mainDock,QDockWidget::DockLeft,0);  // relation target/this (in percent)

    //forbit docking abilities of clusterPanel itself
    clusterPanel->setEnableDocking(QDockWidget::DockNone);

    //Update the Time frame and sample related widgets
    spikesTodisplay->setValue(DEFAULT_NB_SPIKES_DISPLAYED);
    start->setValue(0);
    start->setLineStep(timeWindow);
    duration->setText(INITIAL_WAVEFORM_TIME_WINDOW);
    if(timeFrameMode->isChecked()){
        duration->show();
        durationLabel->show();
        start->show();
        startLabel->show();
        spikesTodisplay->hide();
        spikesTodisplayLabel->hide();
    }
    else{
        duration->hide();
        durationLabel->hide();
        start->hide();
        startLabel->hide();
        spikesTodisplay->show();
        spikesTodisplayLabel->show();
    }

    //show all the encapsulated widgets of all controlled dockwidgets
    dockManager->activate();

    //Enable some actions now that a document is open (see the klustersui.rc file)
    //KDAB_PENDING slotStateChanged("documentState");
}

void KlustersApp::createDisplay(KlustersView::DisplayType type)
{
    if(mainDock){
        QDockWidget* display;
        QString displayName = (doc->documentName()).append(type);
        QString displayType = KlustersView::DisplayTypeNames[type];

        display = new QDockWidget(tr(displayName.toLatin1()),0L);
        display->setWindowIcon(QIcon("classnew"));
                //KDAB_PENDING createDockWidget(QString(QChar(displayCount)),QImage("classnew") , 0L, tr(displayName.toLatin1()), displayType);

        //Check if the active display contains a ProcessWidget
        bool isProcessWidget = doesActiveDisplayContainProcessWidget();

        //Present the clusters of the current display in the new display (if it was not a processing display).
        Q3ValueList<int>* clusterList = new Q3ValueList<int>();
        if(!isProcessWidget){
            const Q3ValueList<int>& currentClusters = activeView()->clusters();

            Q3ValueList<int>::const_iterator shownClustersIterator;
            for(shownClustersIterator = currentClusters.begin(); shownClustersIterator != currentClusters.end(); ++shownClustersIterator )
                clusterList->append(*shownClustersIterator);
        }

        //Use the current dimensions for the new display
        int XDimension = dimensionX->value();
        int YDimension = dimensionY->value();

        //Use the same scale, bin size, time frame and shoulder line for the new correlation view
        //as the existing one if it exists.
        Data::ScaleMode scaleMode = Data::MAX;
        int sizeOfBin = DEFAULT_BIN_SIZE.toInt();
        int correlogramTimeWindow = INITIAL_CORRELOGRAMS_HALF_TIME_FRAME.toInt() * 2 + 1;
        bool line = true;
        if(!isProcessWidget && activeView()->containsCorrelationView()){
            scaleMode = activeView()->scaleMode();
            sizeOfBin = binSize;
            correlogramTimeWindow = correlogramTimeFrame;
            line = activeView()->isShoulderLine();
        }

        //Use the same waveform presentation mode for the new display (will be apply only if
        //the new display contains a waveform view). The values of the associated widgets
        //(start and duration or spikesTodisplay) are the same as the activeDisplay ones.
        //The default for a new display is sample mode without mean and overlay.
        bool overLay = false;
        bool mean = false;
        bool inTimeFrameMode = false;
        long startingTime = 0;
        long timeFrameWidth = INITIAL_WAVEFORM_TIME_WINDOW.toLong();
        long nbSpkToDisplay = DEFAULT_NB_SPIKES_DISPLAYED;
        if(!isProcessWidget && activeView()->containsWaveformView()){
            overLay = activeView()->isOverLayPresentation();
            mean = activeView()->isMeanPresentation();
            if(activeView()->isInTimeFrameMode()){
                inTimeFrameMode = true;
                startingTime = startTime;
                timeFrameWidth = timeWindow;
            }
            else nbSpkToDisplay = spikesTodisplay->value();
        }

        KlustersView* view;

        if(!isProcessWidget) view = new KlustersView(*this,*doc,backgroundColor,XDimension,YDimension,clusterList,type,display,0,Qt::WDestructiveClose,statusBar(),
                                                     displayTimeInterval,waveformsGain,channelPositions,inTimeFrameMode,startingTime,timeFrameWidth,
                                                     nbSpkToDisplay,overLay,mean,sizeOfBin,correlogramTimeWindow,scaleMode,line,activeView()->getStartingTime(),activeView()->getDuration(),showHideLabels->isChecked(),activeView()->getUndoList(),activeView()->getRedoList());

        else view = new KlustersView(*this,*doc,backgroundColor,XDimension,YDimension,clusterList,type,display,0,Qt::WDestructiveClose,statusBar(),
                                     displayTimeInterval,waveformsGain,channelPositions,inTimeFrameMode,startingTime,timeFrameWidth,
                                     nbSpkToDisplay,overLay,mean,sizeOfBin,correlogramTimeWindow,scaleMode,line,activeView()->getStartingTime(),activeView()->getDuration(),showHideLabels->isChecked());


        view->installEventFilter(this);

        //Update the document's list of view
        doc->addView(view);
        //install the new view in the display so it can be see in the future tab.
        display->setWidget(view);

        //Temporarily allow addition of a new dockWidget in the center
        mainDock->setDockSite(QDockWidget::DockCenter);
        //Add the new display as a tab and get a new DockWidget, grandParent of the target (mainDock)
        //and the new display.
        QDockWidget* grandParent = display->manualDock(mainDock,QDockWidget::DockCenter);

        //Disconnect the previous connection
        if(tabsParent != NULL) disconnect(tabsParent,0,0,0);

        //The grandParent's widget is the QTabWidget regrouping all the tabs
        tabsParent = static_cast<QTabWidget*>(grandParent->widget());

        //Connect the change tab signal to slotTabChange(QWidget* widget) to trigger updates when
        //the active display change.
        connect(tabsParent, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotTabChange(QWidget*)));

        //KDAB_PENDING slotStateChanged("tabState");

        //Back to enable dock to the left side only
        mainDock->setDockSite(QDockWidget::DockLeft);

        // forbit docking abilities of display itself
        display->setEnableDocking(QDockWidget::DockNone);
        // allow others to dock to the left side only
        display->setDockSite(QDockWidget::DockLeft);

        //Keep track of the number of displays
        displayCount ++;

        //show all the encapsulated widgets of all controlled dockwidgets
        dockManager->activate();
    }
}

void KlustersApp::openDocumentFile(const QString& url)
{    
    slotStatusMsg(tr("Opening file..."));

    filePath = url;
    QFileInfo file(filePath);
    if(url.protocol() == "file"){
        if((fileOpenRecent->items().contains(url.prettyURL())) && !file.exists()){
            QString title = "File not found: ";
            title.append(filePath);
            int answer = KMessageBox::questionYesNo(this,tr("The selected file no longer exists, do you want to remove it from the list?"), tr(title.toLatin1()));
            if(answer == QMessageBox::Yes) {
                //KDAB_PENDING fileOpenRecent->removeURL(url);
            }
            else  {
                //KDAB_PENDING fileOpenRecent->addURL(url); //hack, unselect the item
            }
            filePath = "";
            slotStatusMsg(tr("Ready."));
            return;
        }
    }
    //Do not handle remote files
    else{
        KMessageBox::sorry(this,tr("Sorry, Klusters does not handle remote files."),tr("Remote file handling"));
        //KDAB_PENDING fileOpenRecent->removeURL(url);
        filePath = "";
        slotStatusMsg(tr("Ready."));
        return;
    }

    //Check if the file exists
    if(!file.exists()){
        QMessageBox::error (this,tr("The selected file does not exist."), tr("Error!"));
        return;
        slotStatusMsg(tr("Ready."));
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    
    //If no document is open already open the document asked.
    if(!mainDock){
        displayCount = 0;
        currentNbUndo = 0;
        currentNbRedo = 0;

        //KDAB_PENDING fileOpenRecent->addURL(url);

        // Open the file (that will also initialize the doc)
        QString errorInformation = "";
        int returnStatus = doc->openDocument(url,errorInformation);
        if(returnStatus == KlustersDoc::INCORRECT_FILE)
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this, tr("Error!"), tr("The selected file is invalid, it has to be of the form baseName.clu.n or baseName.fet.n or baseName.par.n"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //close the document
            doc->closeDocument();

            resetState();
            QApplication::restoreOverrideCursor();
            slotStatusMsg(tr("Ready."));
            return;
        }
        if(returnStatus == KlustersDoc::DOWNLOAD_ERROR)
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this,tr("Error!"),tr("Could not get the cluster file (base.clu.n)") );
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //close the document
            doc->closeDocument();
            resetState();
            QApplication::restoreOverrideCursor();
            slotStatusMsg(tr("Ready."));
            return;
        }

        if(returnStatus == KlustersDoc::SPK_DOWNLOAD_ERROR)
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this,tr("Error!"),tr("Could not get the spike file (base.spk.n)"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //close the document
            doc->closeDocument();
            resetState();
            QApplication::restoreOverrideCursor();
            slotStatusMsg(tr("Ready."));
            return;
        }
        if(returnStatus == KlustersDoc::FET_DOWNLOAD_ERROR)
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this,tr("Error!"),tr("Could not get the feature file (base.fet.n)"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //close the document
            doc->closeDocument();
            resetState();
            QApplication::restoreOverrideCursor();
            slotStatusMsg(tr("Ready."));
            return;
        }
        if(returnStatus == KlustersDoc::PAR_DOWNLOAD_ERROR)
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this,tr("Error!"),tr("Could not get the general parameter file (base.par)"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //close the document
            doc->closeDocument();
            resetState();
            QApplication::restoreOverrideCursor();
            slotStatusMsg(tr("Ready."));
            return;
        }
        if(returnStatus == KlustersDoc::PARX_DOWNLOAD_ERROR)
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this,tr("Error!"), tr("Could not get the specific parameter file (base.par.n)") );
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //close the document
            doc->closeDocument();
            resetState();
            QApplication::restoreOverrideCursor();
            slotStatusMsg(tr("Ready."));
            return;
        }

        if(returnStatus == KlustersDoc::PARXML_DOWNLOAD_ERROR)
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this,tr("Error!"),tr("Could not get the parameter file (base.xml)"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //close the document
            doc->closeDocument();
            resetState();
            QApplication::restoreOverrideCursor();
            slotStatusMsg(tr("Ready."));
            return;
        }

        if(returnStatus == KlustersDoc::OPEN_ERROR)
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this,tr("Error!"),tr("Could not open the files") );
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //close the document
            doc->closeDocument();
            resetState();
            QApplication::restoreOverrideCursor();
            slotStatusMsg(tr("Ready."));
            return;
        }
        if(returnStatus == KlustersDoc::INCORRECT_CONTENT)
        {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical (this,tr("Error!"),errorInformation);
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //close the document
            doc->closeDocument();
            resetState();
            QApplication::restoreOverrideCursor();
            slotStatusMsg(tr("Ready."));
            return;
        }

        //Save the recent file list
        fileOpenRecent->saveEntries(config);

        setCaption(doc->documentName());
        initDisplay();

        //A traceView is possible only if the variables it needs are available (provided in the new parameter file) and
        //the .dat file exists.
        if(doc->areTraceDataAvailable() && doc->isTraceViewVariablesAvailable()) {
            //KDAB_PENDING slotStateChanged("traceDisplayState");
        }

        QApplication::restoreOverrideCursor();
    }
    // check, if this document is already open. If yes, do not do anything
    else{
        QString docName = doc->documentName();
        QStringList fileParts = QStringList::split(".", url.fileName());
        QString electrodNb;
        if(fileParts.count() < 3) electrodNb = "";
        else electrodNb = fileParts[fileParts.count()-1];;
        QString baseName = fileParts[0];
        for(uint i = 1;i < fileParts.count()-2; ++i) baseName += "." + fileParts[i];
        QString name = url.directory() + "/" + baseName + "-" + electrodNb;

        if(docName == name){
            //KDAB_PENDING fileOpenRecent->addURL(url); //hack, unselect the item
            QApplication::restoreOverrideCursor();
            slotStatusMsg(tr("Ready."));
            return;
        }
        //If the document asked is not the already open. Open a new instance of the application with it.
        else{
            //KDAB_PENDING fileOpenRecent->addURL(url);
            filePath = doc->url();


            QProcess::startDetached("klusters", QStringList()<<command);


            QApplication::restoreOverrideCursor();
        }
    }

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::importDocumentFile(const QString& url)
{
    slotStatusMsg(tr("Importing file..."));

    //If no document is open already open the document ask.
    if(!mainDock){
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        // Open the file (that will also initialize the doc)
        if(!doc->importDocument(url))
        {
            QMessageBox::critical (this,tr("Error !"),tr("Could not import document !"));
            //close the document
            doc->closeDocument();
            QApplication::restoreOverrideCursor();
            return;
        }
        //KDAB_PENDING fileOpenRecent->addURL(url);
        initDisplay();
        QApplication::restoreOverrideCursor();
    }
    // check, if this document is already open. If yes, do not do anything
    else if(doc->url()==url){
        QApplication::restoreOverrideCursor();
        return;
    }

    //If the document asked is not the already open. Open a new instance of the application with it.
    //Only one document at the time is allowed.
    else{
        //TODO
        QApplication::restoreOverrideCursor();
    }

    slotStatusMsg(tr("Ready."));
}

bool KlustersApp::doesActiveDisplayContainProcessWidget(){
    QDockWidget* current;

    //Get the active tab
    if(tabsParent) current = static_cast<QDockWidget*>(tabsParent->currentPage());
    //or the active window if there is only one display (which can only be the mainDock)
    else current = mainDock;

    return (current->widget())->isA("ProcessWidget");
}

KlustersView* KlustersApp::activeView(){
    QDockWidget* current;

    //Get the active tab
    if(tabsParent) current = static_cast<QDockWidget*>(tabsParent->currentPage());
    //or the active window if there is only one display (which can only be the mainDock)
    else current = mainDock;

    return static_cast<KlustersView*>(current->widget());
}

void KlustersApp::saveProperties()
{
#if KDAB_PENDING
    // the 'config' object points to the session managed
    // config file.  anything you write here will be available
    // later when this app is restored

    //Save the recent file list
    fileOpenRecent->saveEntries(config);
    config->writePathEntry("openFile",filePath);
#endif
}

void KlustersApp::readProperties()
{
#if KDAB_PENDING
    // the 'config' object points to the session managed
    // config file.  this function is automatically called whenever
    // the app is being restored.  read in here whatever you wrote
    // in 'saveProperties'

    // initialize the recent file list
    fileOpenRecent->loadEntries(config);
    filePath = config->readPathEntry("openFile");
    QString url;
    url.setPath(filePath);
    openDocumentFile(url);
#endif
}

//TO implement , see documentation
bool KlustersApp::queryClose()
{  
    //Save the recent file list
    fileOpenRecent->saveEntries(config);
    
    //call when the kDockMainWindow will be close
    //implement to ask the user to save if necessary before closing
    if(doc == 0) return true;
    else{
        if(doc->canCloseView()){
            //Set a waiting cursor in case there is some delay to the ending of the running threads.
            QApplication::restoreOverrideCursor();
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            if(doc->canCloseDocument(this,"queryClose")){
                if(processWidget != 0L && processWidget->isRunning()){
                    processWidget->killJob();
                    processKilled = true;
                }
                if(!(processFinished && processOutputsFinished)){
                    QTimer::singleShot(2000,this, SLOT(close()));
                    return false;
                }
                else if(processWidget != 0L){
                    delete processWidget;
                    processWidget = 0L ;
                }
                doc->closeDocument();
                QApplication::restoreOverrideCursor();
                return true;
            }
            else return false;
        }
        else return false;
    }
}

//TO implement , see documentation
bool KlustersApp::queryExit()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    //If the saveThread has not finish, wait until id done
    while(!saveThread->wait()){qDebug()<<"in queryExit";};
    QApplication::restoreOverrideCursor();

    return true;
}

void KlustersApp::customEvent (QCustomEvent* event){
    //Event sent by the SaveThread
    if(event->type() == QEvent::User + 100){
        slotStatusMsg(tr("Save file done."));
        SaveThread::SaveDoneEvent* saveEvent = (SaveThread::SaveDoneEvent*) event;
        if(saveEvent->isSaveOk()){
            //The cluster data have been saved to a local temporary file if the original file was a remote one
            //(otherwise the so call temporary file is the orignal one).
            //If the requested save location is remote, the temporary file needs now
            //to be uploaded. The save process is made in a thread and it seams that
            //the KDE upload can not be call asynchronously, so the upload is call after the end of the thread.
            if(!KIO::NetAccess::upload(saveEvent->temporaryFile(),doc->url())){
                QMessageBox::critical (0,tr("Could not save the current document !"), tr("I/O Error !"));
            }
            if(saveEvent->isItSaveAs()){
                //KDAB_PENDING fileOpenRecent->addURL(doc->url());
                setCaption(doc->documentName());
            }
        }
        else
            QMessageBox::critical (0,tr("Could not save the current document !"), tr("I/O Error !"));

        slotStatusMsg(tr("Ready."));
        //KDAB_PENDING slotStateChanged("SavingDoneState");
    }
    //Event sent by klusterDoc to advice that there is some threads still running.
    if(event->type() == QEvent::User + 400){
        KlustersDoc::CloseDocumentEvent* closeEvent = (KlustersDoc::CloseDocumentEvent*) event;
        QString origin = closeEvent->methodOfOrigin();

        //Try to close the document again
        if(doc->canCloseDocument(this,origin)){
            doc->closeDocument();

            //Execute what is need it after the close depending on the callingMethod
            if(origin == "queryClose"){
                QApplication::restoreOverrideCursor();
                close();
            }
            else if(origin == "fileClose" || origin == "displayClose"){
                slotFileClose();
                QApplication::restoreOverrideCursor();
                slotStatusMsg(tr("Ready."));
            }
        }
    }

    //Event sent by the parameterBar to advice that it has been resized.
    if(event->type() == QEvent::User + 1000){
        slotUpdateParameterBar();
    }
}

/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////


void KlustersApp::slotFileOpen()
{
    slotStatusMsg(tr("Opening file..."));

    QString url=QFileDialog::getOpenFileName(this, tr("Open File..."),QString(),
                                             tr("*.fet.*|Feature File (*.fet.n)\n*.clu.*|Cluster File (*.clu.n)\n*.spk.*|Spike File (*.spk.n)\n*.par.*|Specific Parameter File (*.par.n)\n*|All files"));
    if(!url.isEmpty())
    {
        openDocumentFile(url);
    }

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotFileClose(){
    if(doc != 0){
        while(!saveThread->wait()){};
        if(doc->canCloseView()){
            QApplication::restoreOverrideCursor();
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //try to close the document
            if(doc->canCloseDocument(this,"fileClose")){
                //remove all the displays
                if(tabsParent){
                    //Remove the display from the group of tabs
                    while(true){
                        int nbOfTabs = tabsParent->count();
                        QDockWidget* current = static_cast<QDockWidget*>(tabsParent->page(0));
                        if(current == mainDock){
                            current = static_cast<QDockWidget*>(tabsParent->page(1));
                        }

                        if((current->widget())->isA("KlustersView")){
                            tabsParent->removePage(current);
                            delete current;
                        }
                        else{//give the time to the process to finish.
                            if(processWidget->isRunning()){
                                processWidget->killJob();
                                processKilled = true;
                            }
                            if(processFinished && processOutputsFinished){
                                tabsParent->removePage(current);
                                delete current;
                                processWidget = 0L;
                            }
                            else{
                                QTimer::singleShot(2000,this, SLOT(slotFileClose()));
                                return;
                            }
                        }

                        if(nbOfTabs == 2) break; //the reminding one is the mainDock
                    }

                    tabsParent = 0L;
                }
                //reset the cluster palette and hide the cluster panel
                clusterPalette->reset();
                clusterPanel->hide();
                clusterPanel->undock();
                //The last display is the mainDock
                if((mainDock->widget())->isA("KlustersView")) delete mainDock;
                else{
                    if(processWidget->isRunning()){
                        processWidget->killJob();
                        processKilled = true;
                    }
                    if(processFinished && processOutputsFinished){
                        delete mainDock;
                        processWidget = 0L;
                    }
                    else{
                        mainDock->hide();
                        QTimer::singleShot(2000,this, SLOT(slotFileClose()));
                        return;
                    }
                }
                mainDock = 0L;
                doc->closeDocument();
                resetState();
                QApplication::restoreOverrideCursor();
            }
        }
    }
}

void KlustersApp::slotFileImport(){
    slotStatusMsg(tr("Importing file..."));

    QString url=QFileDialog::getOpenFileName(this, tr("Import File..."),QString(),
                                             tr("*|All files"));
    if(!url.isEmpty())
    {
        importDocumentFile(url);
    }

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotFileOpenRecent(const QString& url)
{
    slotStatusMsg(tr("Opening file..."));

    openDocumentFile(url);

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotFileSave()
{
    slotStatusMsg(tr("Saving file..."));

    //KDAB_PENDING slotStateChanged("SavingState");
    saveThread->save(doc->url(),doc,false);
}

void KlustersApp::slotFileRenumberAndSave(){
    slotStatusMsg(tr("Renumbering and saving..."));
    //KDAB_PENDING slotStateChanged("SavingState");
    doc->renumberClusters();
    slotFileSave();
}

void KlustersApp::slotFileSaveAs()
{
    slotStatusMsg(tr("Saving file with a new filename..."));

    QString url=KFileDialog::getSaveURL(QDir::currentPath(),
                                        tr("*|All files"), this, tr("Save as..."));
    if(!url.isEmpty()){
        //KDAB_PENDING slotStateChanged("SavingState");
        saveThread->save(url,doc,true);
    }
}

void KlustersApp::slotDisplayClose()
{
    QDockWidget* current;

    slotStatusMsg(tr("Closing display..."));

    //Get the active tab
    if(tabsParent){
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        int nbOfTabs = tabsParent->count();
        current = static_cast<QDockWidget*>(tabsParent->currentPage());
        //If the active display is the mainDock, assign the mainDock status to an other display (take the first one available)
        if(current == mainDock){
            if(tabsParent->currentPageIndex() == 0){
                mainDock = static_cast<QDockWidget*>(tabsParent->page(1));
                setMainDockWidget(mainDock);
            }
            else setMainDockWidget(static_cast<QDockWidget*>(tabsParent->page(0)));
        }
        //Remove the display from the group of tabs
        tabsParent->removePage(current);
        displayCount --;
        //If there is only one display left, the group of tabs will be deleted so we set tabsParent to null
        if(nbOfTabs == 2){
            //KDAB_PENDING slotStateChanged("noTabState");
            tabsParent = 0L;
        }

        //Remove the view from the document list if need it
        if((current->widget())->isA("KlustersView")){
            KlustersView* view = dynamic_cast<KlustersView*>(current->widget());
            doc->removeView(view);

            //Update the Displays menu if the current display is a grouping assistant.
            if(view->containsErrorMatrixView()){
                //KDAB_PENDING slotStateChanged("groupingAssistantDisplayNotExists");
                errorMatrixExists = false;
            }

            //Delete the view
            delete current;
        }
        else{
            if(processFinished && processOutputsFinished){
                delete current;
                processWidget = 0L;
            }
            else{
                processOutputDock = current;
                processWidget->hideWidget();
            }
        }

        QApplication::restoreOverrideCursor();
    }
    //or the active window if there is only one display (which can only be the mainDock)
    else{
        //If a save is already in process, wait until it is done
        if(saveThread->running()){
            QApplication::restoreOverrideCursor();
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            while(!saveThread->wait()){};
            //reset the cluster palette and hide the cluster panel
            clusterPalette->reset();
            clusterPanel->hide();
            clusterPanel->undock();
            //try to close the document
            if(doc->canCloseDocument(this,"displayClose")){
                doc->closeDocument();
                //Delete the view
                if((mainDock->widget())->isA("KlustersView")){
                    if(processWidget != 0L){
                        delete processWidget;
                        processWidget = 0L;
                    }
                    delete mainDock;
                }
                else{
                    if(processWidget->isRunning()){
                        processWidget->killJob();
                        processKilled = true;
                    }
                    if(processFinished && processOutputsFinished){
                        delete mainDock;
                        processWidget = 0L;
                    }
                    else{
                        mainDock->hide();
                        processWidget->hideWidget();
                        QTimer::singleShot(2000,this, SLOT(slotDisplayClose()));
                        return;
                    }
                }
                mainDock = 0L;
                QApplication::restoreOverrideCursor();
            }
        }
        //Ask the user if wants to save the document if need it before closing and confirm the closing
        else if(doc->canCloseView()){
            QApplication::restoreOverrideCursor();
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            //reset the cluster palette and hide the cluster panel
            clusterPalette->reset();
            clusterPanel->hide();
            clusterPanel->undock();
            //try to close the document
            if(doc->canCloseDocument(this,"displayClose")){
                doc->closeDocument();
                //Delete the view
                if((mainDock->widget())->isA("KlustersView")){
                    if(processWidget != 0L){
                        delete processWidget;
                        processWidget = 0L;
                    }
                    delete mainDock;
                }
                else{
                    if(processWidget->isRunning()){
                        processWidget->killJob();
                        processKilled = true;
                    }
                    if(processFinished && processOutputsFinished){
                        delete mainDock;
                        processWidget = 0L;
                    }
                    else{
                        mainDock->hide();
                        processWidget->hideWidget();
                        QTimer::singleShot(2000,this, SLOT(slotDisplayClose()));
                        return;
                    }
                }
                mainDock = 0L;
                QApplication::restoreOverrideCursor();
            }
            resetState();
        }
    }
    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotFilePrint()
{
    slotStatusMsg(tr("Printing..."));

    printer->setOrientation(QPrinter::Landscape);
    printer->setColorMode(QPrinter::Color);
    //REmove it
    //printer->addDialogPage(new printDialogPage(this));

    if (printer->setup(this))
    {
        //retrieve the backgroundColor setting from KPrinter object, //1 <=> white background
        int backgroundColor = printer->option("kde-klusters-backgroundColor").toInt();
        if(!doesActiveDisplayContainProcessWidget()){
            KlustersView* view = activeView();
            if(backgroundColor == 1) view->print(printer,filePath,true);
            else view->print(printer,filePath,false);
        }
        else{
            QDockWidget* dock = static_cast<QDockWidget*>(tabsParent->currentPage());
            ProcessWidget* view = static_cast<ProcessWidget*>(dock->widget());
            view->print(printer,filePath);
        }
    }

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotFileQuit()
{
    slotStatusMsg(tr("Exiting..."));
    /*
  // close the first window, the list makes the next one the first again.
  // This ensures that queryClose() is called on each window to ask for closing
  KMainWindow* w;
  if(memberList) //List of members of KMainWindow class
  {
    for(w=memberList->first(); w!=0; w=memberList->first())
    {
      // only close the window if the closeEvent is accepted. If the user presses Cancel on the saveModified() dialog,
      // the window and the application stay open.
      if(!w->close())
        break;
    }
  }*/

    close();

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotUndo()
{  
    slotStatusMsg(tr("Reverting last action..."));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    doc->undo();
    QApplication::restoreOverrideCursor();

    //Update the browsing possibility of the traceView
    KlustersView* view = activeView();
    if(view->containsTraceView() && view->clusters().size() != 0) {
        //KDAB_PENDING slotStateChanged("traceViewBrowsingState");
    }
    else
    {
        //KDAB_PENDING slotStateChanged("noTraceViewBrowsingState");
    }

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotRedo()
{
    slotStatusMsg(tr("Reverting last undo action..."));
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    doc->redo();
    QApplication::restoreOverrideCursor();

    //Update the browsing possibility of the traceView
    KlustersView* view = activeView();
    if(view->containsTraceView() && view->clusters().size() != 0)
    {
        //KDAB_PENDING slotStateChanged("traceViewBrowsingState");
    }
    else  {
        //KDAB_PENDING slotStateChanged("noTraceViewBrowsingState");
    }

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotUpdateUndoNb(int undoNb){
    currentNbUndo = undoNb;
    if(currentNbUndo > 0) {
        //KDAB_PENDING slotStateChanged("undoState");
    }
    else {
        //KDAB_PENDING slotStateChanged("emptyUndoState");
    }
}

void KlustersApp::slotUpdateRedoNb(int redoNb){
    currentNbRedo = redoNb;
    if(currentNbRedo == 0) {
        //KDAB_PENDING slotStateChanged("emptyRedoState");
    }
}

void KlustersApp::slotViewMainToolBar()
{
    slotStatusMsg(tr("Toggle the main toolbar..."));

    // turn Toolbar on or off
    if(!viewMainToolBar->isChecked())
    {
        toolBar("mainToolBar")->hide();
    }
    else
    {
        toolBar("mainToolBar")->show();
    }

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotViewActionBar(){
    slotStatusMsg(tr("Toggle the action..."));

    // turn Toolbar on or off
    if(!viewActionBar->isChecked())
    {
        toolBar("actionBar")->hide();
    }
    else
    {
        toolBar("actionBar")->show();
    }
    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotViewParameterBar(){
    slotStatusMsg(tr("Toggle the parameters..."));

    // turn Toolbar on or off
    if(!viewParameterBar->isChecked())
    {
        paramBar->hide();
    }
    else
    {
        paramBar->show();
        slotUpdateParameterBar();
    }
    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotViewToolBar()
{
    slotStatusMsg(tr("Toggle the tools..."));

    // turn Toolbar on or off
    if(!viewToolBar->isChecked())
    {
        toolBar("toolBar")->hide();
    }
    else
    {
        toolBar("toolBar")->show();
    }

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotViewStatusBar()
{
    slotStatusMsg(tr("Toggle the statusbar..."));
    ///////////////////////////////////////////////////////////////////
    //turn Statusbar on or off
    if(!viewStatusBar->isChecked())
    {
        statusBar()->hide();
    }
    else
    {
        statusBar()->show();
    }

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotViewClusterInfo(){
    slotStatusMsg(tr("Toggle the presentation of the cluster information in the cluster palette..."));

    // turn the user cluster information on or off
    if(!viewClusterInfo->isChecked())
    {
        clusterPalette->hideUserClusterInformation();
    }
    else
    {
        doc->showUserClusterInformation();
    }

    slotStatusMsg(tr("Ready."));

}


void KlustersApp::slotWindowNewClusterDisplay()
{
    slotStatusMsg(tr("Opening a new cluster view..."));

    createDisplay(KlustersView::CLUSTERS);
    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotWindowNewWaveformDisplay()
{
    slotStatusMsg(tr("Opening a new waveform view..."));

    createDisplay(KlustersView::WAVEFORMS);
    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotWindowNewCrosscorrelationDisplay()
{
    slotStatusMsg(tr("Opening a new correlation view..."));

    createDisplay(KlustersView::CORRELATIONS);
    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotWindowNewOverViewDisplay()
{
    slotStatusMsg(tr("Opening a new over view..."));

    createDisplay(KlustersView::OVERVIEW);
    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotWindowNewGroupingAssistantDisplay()
{
    slotStatusMsg(tr("Opening a new grouping assistant view..."));

    createDisplay(KlustersView::GROUPING_ASSISTANT_VIEW);
    //KDAB_PENDING slotStateChanged("groupingAssistantDisplayExists");
    errorMatrixExists = true;

    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotNewTraceDisplay(){
    slotStatusMsg(tr("Opening a new grouping assistant view..."));

    createDisplay(KlustersView::TRACES);

    slotStatusMsg(tr("Ready."));
}


void KlustersApp::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->showMessage(text);
}


void KlustersApp::viewMenuActivated( int id )
{
    /* QWidget* w = pWorkspace->windowList().at( id );
  if ( w )
    w->setFocus();*/
}


/*Slots for the actions menu*/
/**Creates a single cluster by selecting an area*/
void KlustersApp::slotSingleNew(){
    slotStatusMsg(tr("Create new cluster..."));

    //If we are in delay mode, update the display, if need it, before triggering the tool change
    if(mDelaySelection->isChecked()){
        clusterPalette->updateClusters();
    }

    KlustersView* view = activeView();
    view->setMode(ViewWidget::NEW_CLUSTER);
    slotStatusMsg(tr("Ready."));
}
/**Creates a multiple clusters by selecting an area*/
void KlustersApp::slotMultipleNew(){
    slotStatusMsg(tr("Split clusters..."));

    //If we are in delay mode, update the display, if need it, before triggering the tool change
    if(mDelaySelection->isChecked()){
        clusterPalette->updateClusters();
    }

    KlustersView* view = activeView();
    view->setMode(ViewWidget::NEW_CLUSTERS);
    slotStatusMsg(tr("Ready."));
}
/**Deletes spikes from a cluster and move them to the cluster (number 1) containing the poorly isolated cells*/
void KlustersApp::slotDeleteNoise(){
    slotStatusMsg(tr("Delete noise..."));

    //If we are in delay mode, update the display, if need it, before triggering the tool change
    if(mDelaySelection->isChecked()){
        clusterPalette->updateClusters();
    }

    KlustersView* view = activeView();
    view->setMode(ViewWidget::DELETE_NOISE);
    slotStatusMsg(tr("Ready."));
}
/**Deletes spikes from a cluster and move them to the cluster (number 0) containing the artifacts*/
void KlustersApp::slotDeleteArtefact(){
    slotStatusMsg(tr("Delete artifact..."));

    //If we are in delay mode, update the display, if need it, before triggering the tool change
    if(mDelaySelection->isChecked()){
        clusterPalette->updateClusters();
    }

    KlustersView* view = activeView();
    view->setMode(ViewWidget::DELETE_ARTEFACT);
    slotStatusMsg(tr("Ready."));
}
/**Zooms*/
void KlustersApp::slotZoom(){
    slotStatusMsg(tr("Zooming..."));

    //If we are in delay mode, update the display, if need it, before triggering the tool change
    if(mDelaySelection->isChecked()){
        clusterPalette->updateClusters();
    }

    KlustersView* view = activeView();
    view->setMode(BaseFrame::ZOOM);
    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotSelectTime(){
    slotStatusMsg(tr("Selecting time..."));

    //If we are in delay mode, update the display, if need it, before triggering the tool change
    if(mDelaySelection->isChecked()){
        clusterPalette->updateClusters();
    }

    KlustersView* view = activeView();
    view->setMode(ViewWidget::SELECT_TIME);

    slotStatusMsg(tr("Ready."));
}


void KlustersApp::slotSingleColorUpdate(int clusterId){
    //Trigger the action only if the active display does not contain a ProcessWidget
    if(!doesActiveDisplayContainProcessWidget()){
        KlustersView* view = activeView();
        doc->singleColorUpdate(clusterId,*view);
    }
}

void KlustersApp::slotUpdateShownClusters(Q3ValueList<int> selectedClusters){  
    //Trigger ths action only if the active display does not contain a ProcessWidget
    if(!doesActiveDisplayContainProcessWidget()){

        //Update the browsing possibility of the traceView
        if(activeView()->containsTraceView() && selectedClusters.size() != 0) {
            //KDAB_PENDING slotStateChanged("traceViewBrowsingState");
        }
        else{

            //KDAB_PENDING slotStateChanged("noTraceViewBrowsingState");
        }

        KlustersView* view = activeView();
        doc->shownClustersUpdate(selectedClusters,*view);
    }
}

void KlustersApp::slotGroupClusters(Q3ValueList<int> selectedClusters){
    slotStatusMsg(tr("Grouping clusters..."));
    KlustersView* view = activeView();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    doc->groupClusters(selectedClusters,*view);
    QApplication::restoreOverrideCursor();
    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotMoveClustersToNoise(Q3ValueList<int> selectedClusters){
    slotStatusMsg(tr("Delete &noisy cluster(s)..."));
    KlustersView* view = activeView();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    doc->deleteClusters(selectedClusters,*view,1);

    //Update the browsing possibility of the traceView
    if(view->containsTraceView() && view->clusters().size() != 0) {
        //KDAB_PENDING slotStateChanged("traceViewBrowsingState");
    }
    else{

        //KDAB_PENDING slotStateChanged("noTraceViewBrowsingState");
    }

    QApplication::restoreOverrideCursor();
    slotStatusMsg(tr("Ready."));
}

void KlustersApp::slotMoveClustersToArtefact(Q3ValueList<int> selectedClusters){
    slotStatusMsg(tr("Delete &artifact cluster(s)..."));
    KlustersView* view = activeView();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    doc->deleteClusters(selectedClusters,*view,0);

    //Update the browsing possibility of the traceView
    if(view->containsTraceView() && view->clusters().size() != 0) {
        //KDAB_PENDING slotStateChanged("traceViewBrowsingState");
    }
    else {
        //KDAB_PENDING slotStateChanged("noTraceViewBrowsingState");
    }

    QApplication::restoreOverrideCursor();
    slotStatusMsg(tr("Ready."));
}


void KlustersApp::slotImmediateSelection(){
    //Disable the update action (see the klustersui.rc file)
    //KDAB_PENDING slotStateChanged("immediateSelectionState");
    clusterPalette->setImmediateMode();
    clusterPalette->updateClusters();
}
void KlustersApp::slotDelaySelection(){
    //Enable the update action (see the klustersui.rc file)
    //KDAB_PENDING slotStateChanged("delaySelectionState");
    clusterPalette->setDelayMode();
}

void KlustersApp::slotTabChange(QWidget* widget){
    QDockWidget* display = dynamic_cast<QDockWidget*>(widget);
    if((display->widget())->isA("KlustersView")){
        KlustersView* activeView = dynamic_cast<KlustersView*>(display->widget());

        //Update the content of the view contains in active display.
        activeView->updateViewContents();

        isInit = true; //prevent the spine boxes or the lineedit and the editline to trigger during initialisation

        //The select time tool is useful only if both a clusterView and a traceView are present
        if(activeView->containsClusterView() && activeView->containsTraceView()) {
            //KDAB_PENDING slotStateChanged("traceViewClusterViewState");
        }

        if(activeView->containsClusterView()){
            //Update the dimension spine boxes
            int x =  activeView->abscissaDimension();
            int y =  activeView->ordinateDimension();
            dimensionX->setValue(x);
            dimensionY->setValue(y);
            //KDAB_PENDING slotStateChanged("clusterViewState");
            dimensionX->show();
            dimensionY->show();
            featureXLabel->show();
        }
        else{
            //KDAB_PENDING slotStateChanged("noClusterViewState");
            dimensionX->hide();
            dimensionY->hide();
            featureXLabel->hide();
        }

        if(activeView->containsWaveformView()){
            //KDAB_PENDING slotStateChanged("waveformsViewState");
            if(activeView->isOverLayPresentation()) overlayPresentation->setChecked(true);
            else overlayPresentation->setChecked(false);
            if(activeView->isMeanPresentation()) meanPresentation->setChecked(true);
            else meanPresentation->setChecked(false);

            if(activeView->isInTimeFrameMode()){
                timeFrameMode->setChecked(true);
                timeWindow = activeView->timeFrameWidth();
                startTime =  activeView->timeFrameStart();
                start->setValue(startTime);
                start->setLineStep(timeWindow);
                duration->setText(QString("%1").arg(timeWindow));
                duration->show();
                durationLabel->show();
                start->show();
                startLabel->show();
                spikesTodisplay->hide();
                spikesTodisplayLabel->hide();
            }
            else{
                timeFrameMode->setChecked(false);
                duration->hide();
                durationLabel->hide();
                start->hide();
                startLabel->hide();
                spikesTodisplay->setValue(activeView->displayedNbSpikes());
                spikesTodisplay->show();
                spikesTodisplayLabel->show();
            }
        }
        else{
            timeFrameMode->setChecked(false);
            overlayPresentation->setChecked(false);
            meanPresentation->setChecked(false);
            //KDAB_PENDING slotStateChanged("noWaveformsViewState");
            duration->hide();
            durationLabel->hide();
            start->hide();
            startLabel->hide();
            spikesTodisplay->hide();
            spikesTodisplayLabel->hide();
        }

        if(activeView->containsCorrelationView()){
            //KDAB_PENDING slotStateChanged("correlationViewState");
            Data::ScaleMode correlationScale = activeView->scaleMode();
            switch(correlationScale){
            case Data::RAW :
                noScale->setChecked(true);
                break;
            case Data::MAX :
                scaleByMax->setChecked(true);
                break;
            case Data::SHOULDER :
                scaleByShouler->setChecked(true);
                break;
            }

            //Update the lineEdit
            correlogramTimeFrame = activeView->correlationTimeFrameWidth();
            binSize = activeView->sizeOfBin();
            correlogramsHalfDuration->setText(QString("%1").arg(correlogramTimeFrame / 2));
            binSizeBox->setText(QString("%1").arg(binSize));
            correlogramsHalfDuration->show();
            correlogramsHalfDurationLabel->show();
            binSizeBox->show();
            binSizeLabel->show();
            //Update the shoulder line menu entry
            shoulderLine->setChecked(activeView->isShoulderLine());
        }
        else{
            //KDAB_PENDING slotStateChanged("noCorrelationViewState");
            correlogramsHalfDuration->hide();
            correlogramsHalfDurationLabel->hide();
            binSizeBox->hide();
            binSizeLabel->hide();
        }

        if(activeView->containsErrorMatrixView()){
            //KDAB_PENDING slotStateChanged("errorMatrixViewState");
        }
        else{

            //KDAB_PENDING slotStateChanged("noErrorMatrixViewState");
        }

        if(activeView->containsTraceView()){
            showHideLabels->setChecked(activeView->getLabelStatus());
            activeView->updateClustersProvider();
            //KDAB_PENDING slotStateChanged("traceViewState");

            //Update the browsing possibility of the traceView
            if(activeView->clusters().size() != 0) {
                //KDAB_PENDING slotStateChanged("traceViewBrowsingState");
            }
            else{
                //KDAB_PENDING slotStateChanged("noTraceViewBrowsingState");
            }
        }
        else{

            //KDAB_PENDING slotStateChanged("noTraceViewState");
        }

        isInit = false; //now a change in a spine box  or the lineedit
        //will trigger an update of the display

        //Update the cluster palette
        clusterPalette->selectItems(activeView->clusters());


        //Check if a reclustering process is working in order to correctly set up the menus
        if(processFinished){
            //KDAB_PENDING slotStateChanged("noReclusterState");
            updateUndoRedoDisplay();
        }
        else{
            //KDAB_PENDING slotStateChanged("reclusterState");
        }
    }
    else{// a ProcessWidget
        dimensionX->hide();
        dimensionY->hide();
        featureXLabel->hide();
        timeFrameMode->setChecked(false);
        overlayPresentation->setChecked(false);
        meanPresentation->setChecked(false);
        duration->hide();
        durationLabel->hide();
        start->hide();
        startLabel->hide();
        spikesTodisplay->hide();
        spikesTodisplayLabel->hide();
        correlogramsHalfDuration->hide();
        correlogramsHalfDurationLabel->hide();
        binSizeBox->hide();
        binSizeLabel->hide();

        //Update the palette of clusters
        if(!processFinished) clusterPalette->selectItems(clustersToRecluster);
        else{
            Q3ValueList<int> emptyList;
            clusterPalette->selectItems(emptyList);
        }

        //KDAB_PENDING slotStateChanged("reclusterViewState");
    }
}

void KlustersApp::slotUpdateDimensionX(int dimensionXValue){
    if(!isInit)updateDimensions(dimensionXValue,dimensionY->value());
}

void KlustersApp::slotUpdateDimensionY(int dimensionYValue){
    if(!isInit)updateDimensions(dimensionX->value(),dimensionYValue);
}

void KlustersApp::updateDimensions(int dimensionXValue, int dimensionYValue){
    activeView()->updateDimensions(dimensionXValue,dimensionYValue);
    activeView()->showAllWidgets();
}

void KlustersApp::slotUpdateDuration(){
    if(!isInit){
        timeWindow =  (duration->displayText()).toLong();
        start->setLineStep(timeWindow);
        activeView()->updateTimeFrame(startTime,timeWindow);
    }
}

void KlustersApp::slotTimeFrameMode(){
    if(!isInit){
        if(timeFrameMode->isChecked()){
            timeWindow = activeView()->timeFrameWidth();
            startTime =  activeView()->timeFrameStart();
            start->setValue(startTime);
            start->setLineStep(timeWindow);
            duration->setText(QString("%1").arg(timeWindow));
            duration->show();
            durationLabel->show();
            start->show();
            startLabel->show();
            spikesTodisplay->hide();
            spikesTodisplayLabel->hide();
            activeView()->setTimeFrameMode();
        }
        else{
            spikesTodisplay->setValue(activeView()->displayedNbSpikes());
            spikesTodisplay->show();
            spikesTodisplayLabel->show();
            duration->hide();
            durationLabel->hide();
            start->hide();
            startLabel->hide();
            activeView()->setSampleMode();
        }
    }
}

void KlustersApp::slotClusterInformationModified(){
    doc->clusterInformationModified();
}

void KlustersApp::resetState(){
    isInit = true; //prevent the spine boxes or the lineedit and the editline to trigger during initialisation
    timeFrameMode->setChecked(false);
    duration->hide();
    durationLabel->hide();
    start->hide();
    startLabel->hide();
    dimensionX->hide();
    dimensionY->hide();
    featureXLabel->hide();
    spikesTodisplay->hide();
    spikesTodisplayLabel->hide();
    correlogramsHalfDuration->hide();
    correlogramsHalfDurationLabel->hide();
    binSizeBox->hide();
    binSizeLabel->hide();
    shoulderLine->setChecked(true);
    binSize = DEFAULT_BIN_SIZE.toInt();
    correlogramTimeFrame = INITIAL_CORRELOGRAMS_HALF_TIME_FRAME.toInt() * 2 + 1;
    startTime = 0;
    timeWindow = INITIAL_WAVEFORM_TIME_WINDOW.toLong();
    showHideLabels->setChecked(false);

    isInit = false; //now a change in a spine box  or the lineedit
    //will trigger an update of the view contain in the display.

    processOutputDock = 0L;
    displayCount = 0;
    errorMatrixExists = false;
    filePath = "";

    //Disable some actions when no document is open (see the klustersui.rc file)
    //KDAB_PENDING slotStateChanged("initState");
    //A traceView is possible only if the variables it needs are available (provided in the new parameter file) and
    //the .dat file exists. Therefore disable the menu entry by default.
    //KDAB_PENDING slotStateChanged("noTraceDisplayState");

    setCaption("");

    //If the a setting dialog exists (has already be open once), disable the settings for the channels.
    if(prefDialog != 0L) prefDialog->enableChannelSettings(false);
}

void KlustersApp::slotUpdateCorrelogramsHalfDuration(){
    if(!isInit){
        int halfTimeFrame = (correlogramsHalfDuration->displayText()).toInt();
        if(halfTimeFrame > (maximumTime - binSize) / 2){
            correlogramsHalfDuration->setText(QString("%1").arg((correlogramTimeFrame - binSize) / 2));
            return;
        }

        float x = (2*static_cast<float>(halfTimeFrame)
                   /static_cast<float>(binSize)-1) * 0.5;
        int k = static_cast<int>(x + 0.5);

        correlogramTimeFrame = (2*k+1)*binSize;
        if(k != x){
            correlogramsHalfDuration->setText(QString("%1").arg(static_cast<int>(correlogramTimeFrame / 2)));
        }
        activeView()->updateBinSizeAndTimeFrame(binSize,correlogramTimeFrame);
    }
}

void KlustersApp::slotUpdateBinSize(){
    if(!isInit){
        binSize = (binSizeBox->displayText()).toInt();
        activeView()->updateBinSizeAndTimeFrame(binSize,correlogramTimeFrame);
    }
}

void KlustersApp::slotUpdateParameterBar(){  
    duration->hide();
    durationLabel->hide();
    start->hide();
    startLabel->hide();
    dimensionX->hide();
    dimensionY->hide();
    featureXLabel->hide();
    spikesTodisplay->hide();
    spikesTodisplayLabel->hide();
    correlogramsHalfDuration->hide();
    correlogramsHalfDurationLabel->hide();
    binSizeBox->hide();
    binSizeLabel->hide();

    if(mainDock != 0L){
        KlustersView* currentView = activeView();

        if(currentView->containsClusterView()){
            dimensionX->show();
            dimensionY->show();
            featureXLabel->show();
        }

        if(currentView->containsWaveformView()){
            if(currentView->isInTimeFrameMode()){
                duration->show();
                durationLabel->show();
                start->show();
                startLabel->show();
            }
            else{
                spikesTodisplay->show();
                spikesTodisplayLabel->show();
            }
        }

        if(currentView->containsCorrelationView()){
            correlogramsHalfDuration->show();
            correlogramsHalfDurationLabel->show();
            binSizeBox->show();
            binSizeLabel->show();
        }
    }
}

void KlustersApp::slotUpdateErrorMatrix(){
    KlustersView* view = activeView();
    view->updateErrorMatrix();
}

void KlustersApp::slotSelectAll(){
    //Trigger the action only if the active display does not contain a ProcessWidget
    if(!doesActiveDisplayContainProcessWidget()){
        Q3ValueList<int> clustersToHide;
        doc->showAllClustersExcept(clustersToHide);
    }
}

void KlustersApp::slotSelectAllWO01(){
    //Trigger the action only if the active display does not contain a ProcessWidget
    if(!doesActiveDisplayContainProcessWidget()){
        Q3ValueList<int> clustersToHide;
        clustersToHide.append(0);
        clustersToHide.append(1);
        doc->showAllClustersExcept(clustersToHide);
    }
}

void KlustersApp::slotRecluster(){
    if(processOutputDock != 0L){
        if(processFinished && processOutputsFinished){
            delete processOutputDock;
            processOutputDock = 0L;
            processWidget = 0L;
            processKilled = false;
        }
        else{
            QTimer::singleShot(2000,this, SLOT(slotRecluster()()));
            return;
        }
    }

    //Get the clusters to recluster (those selected in the active display)
    const Q3ValueList<int>& currentClusters = activeView()->clusters();
    if(currentClusters.size() == 0){
        QMessageBox::critical (this,tr("Error !"),tr("No clusters have been selected to be reclustered."));
        return;
    }

    clustersToRecluster.clear();
    Q3ValueList<int>::const_iterator shownClustersIterator;
    for(shownClustersIterator = currentClusters.begin(); shownClustersIterator != currentClusters.end(); ++shownClustersIterator)
        clustersToRecluster.append(*shownClustersIterator);
    //KDAB_PENDING
    //qSort(clustersToRecluster);

    //Build the command line to launch the reclustering

    //Create the fet file name: baseName + -- + -clusterIDs -- + timestamp + .fet + .electrodeGroupID
    QString fileBaseName = doc->documentBaseName();
    fileBaseName.append("--");

    int max = clustersToRecluster.size() - 1;
    int i = 0;
    for(; i < max; ++i){
        fileBaseName.append(QString::number(clustersToRecluster[i]));
        fileBaseName.append("-");
    }
    fileBaseName.append(QString::number(clustersToRecluster[i]));

    QString date = QDate::currentDate().toString("--MM.dd.yyyy-");
    QString time = QTime::currentTime().toString("hh.mm");
    fileBaseName.append(date);
    fileBaseName.append(time);

    QString command(reclusteringExecutable);
    command.append(" ");
    command.append(reclusteringArgs);

    QString electrodeGroupID = doc->currentElectrodeGroupID();
    if(electrodeGroupID == "") electrodeGroupID = "1";

    //The default arguments are: "%fileBaseName %electrodeGroupID -MinClusters 2 -MaxClusters 12"
    //The fet file name will be docPath/%fileBaseName.fet.%electrodeNb
    if(command.contains("%electrodeGroupID")) command.replace("%electrodeGroupID",electrodeGroupID);
    command.replace("%fileBaseName",fileBaseName);
    reclusteringFetFileName = (doc->documentDirectory() + "/");
    reclusteringFetFileName.append(fileBaseName);
    reclusteringFetFileName.append(".fet.");
    reclusteringFetFileName.append(electrodeGroupID);

    //Subset of the input features to use. The default is to use all the PCA and the time feature.
    //If other features exist there are not used that is zeros are added for them.
    //NB: the time is always the last feature.
    if(command.contains("%features")){
        int totalNbOfPCAs = doc->totalNbOfPCAs();
        int nbDimensions = doc->nbDimensions();
        int nbExtraFeatures = nbDimensions - totalNbOfPCAs - 1;
        QString features;
        for(int i = 0; i < totalNbOfPCAs; ++i) features.append("1");
        for(int i = 0; i < nbExtraFeatures; ++i) features.append("0");
        features.append("1"); // the time feature.
        command.replace("%features",features);
    }

    //Create the feature file for the selected clusters and get its name.
    int returnStatus = doc->createFeatureFile(clustersToRecluster,reclusteringFetFileName);
    if(returnStatus == KlustersDoc::OPEN_ERROR){
        QMessageBox::critical (this,tr("Error !"),tr("The reclustering feature file cannot be created (possibly because of insufficient file access permissions).\n Reclustering can not be done."));
        return;
    }
    if(returnStatus == KlustersDoc::CREATION_ERROR){
        QMessageBox::critical (this,tr("IO Error !"),tr("An error happened while creating the reclustering feature file.\n Reclustering can not be done."));
        return;
    }


    if(processWidget == 0L){
        QDockWidget* display;
        display = new QDockWidget(tr("Recluster output"),0);
                //KDAB_PENDING createDockWidget(QString(QChar(displayCount)),0, 0L, tr("Recluster output"), tr("Recluster output"));

        processWidget = new ProcessWidget(display);
        connect(processWidget,SIGNAL(processExited(QProcess*)), this, SLOT(slotProcessExited(QProcess*)));
        connect(processWidget,SIGNAL(processOutputsFinished()), this, SLOT(slotOutputTreatmentOver()));

        //install the new view in the display so it can be see in the future tab.
        display->setWidget(processWidget);

        //Temporarily allow addition of a new dockWidget in the center
        mainDock->setDockSite(QDockWidget::DockCenter);
        //Add the new display as a tab and get a new DockWidget, grandParent of the target (mainDock)
        //and the new display.
        QDockWidget* grandParent = display->manualDock(mainDock,QDockWidget::DockCenter);

        //The grandParent's widget is the QTabWidget regrouping all the tabs
        tabsParent = static_cast<QTabWidget*>(grandParent->widget());

        //Connect the change tab signal to slotTabChange(QWidget* widget) to trigger updates when
        //the active display changes.
        connect(tabsParent, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotTabChange(QWidget*)));

        //Back to enable dock to the left side only
        mainDock->setDockSite(QDockWidget::DockLeft);

        // forbit docking abilities of display itself
        display->setEnableDocking(QDockWidget::DockNone);

        // allow others to dock to the left side only
        display->setDockSite(QDockWidget::DockLeft);

        //Keep track of the number of displays
        displayCount ++;

        //show all the encapsulated widgets of all controlled dockwidgets
        dockManager->activate();
    }

    //Rest the different variables.
    clustersFromReclustering.clear();
    processFinished = false;
    processOutputsFinished = false;
    processKilled = false;
    //KDAB_PENDING slotStateChanged("reclusterState");

    //Start the process
    bool status;
    status = processWidget->startJob(doc->documentDirectory(),command);

    if(!status){
        QMessageBox::critical (this,tr("Error !"),tr("The reclustering program could not be started.\n"
                                                     "One possible reason is that the automatic reclustering program could not be found."));
        processFinished = true;
        processKilled = false;
        //KDAB_PENDING slotStateChanged("noReclusterState");
        updateUndoRedoDisplay();
    }
}

void KlustersApp::slotProcessExited(QProcess* process){
    //Check if the process has exited "voluntarily" and if so if it was successful
    if(!process->normalExit() || (process->normalExit() && process->exitStatus())){
        if(process->normalExit() || (!process->normalExit() && !processKilled))
            QMessageBox::critical (this,tr("Error !"),tr("The reclustering program did not finished normaly.\n"
                                                         "Check the output log for more information."));

        if(!QFile::remove(reclusteringFetFileName))
            QMessageBox::critical(0,tr("Warning !"),tr("Could not delete the temporary feature file used by the reclustering program."));
        processFinished = true;
        processOutputsFinished = true;
        processKilled = false;
        //KDAB_PENDING slotStateChanged("noReclusterState");
        if(!doesActiveDisplayContainProcessWidget()) updateUndoRedoDisplay();
        else{

            //KDAB_PENDING slotStateChanged("reclusterViewState");
        }
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    int returnStatus = doc->integrateReclusteredClusters(clustersToRecluster,clustersFromReclustering,reclusteringFetFileName);

    switch(returnStatus){
    case KlustersDoc::DOWNLOAD_ERROR:
        QApplication::restoreOverrideCursor();
        QMessageBox::critical(this,tr("Error !"),tr("Could not download the temporary file containing the new clusters."));
        processFinished = true;
        processOutputsFinished = true;
        processKilled = false;
        //KDAB_PENDING slotStateChanged("noReclusterState");
        if(!doesActiveDisplayContainProcessWidget()) updateUndoRedoDisplay();
        else{

            //KDAB_PENDING slotStateChanged("reclusterViewState");
        }
        return;
    case KlustersDoc::OPEN_ERROR:
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (this,tr("Error !"),tr("Could not open the temporary file containing the new clusters."));
        processFinished = true;
        processOutputsFinished = true;
        processKilled = false;
        //KDAB_PENDING slotStateChanged("noReclusterState");
        if(!doesActiveDisplayContainProcessWidget()) updateUndoRedoDisplay();
        else{

            //KDAB_PENDING slotStateChanged("reclusterViewState");
        }
        return;
    case KlustersDoc::INCORRECT_CONTENT:
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (this,tr("Error !"),tr("The temporary file containing the new clusters contains incorrect data."));
        processFinished = true;
        processOutputsFinished = true;
        processKilled = false;
        //KDAB_PENDING slotStateChanged("noReclusterState");
        if(!doesActiveDisplayContainProcessWidget()) updateUndoRedoDisplay();
        else{

            //KDAB_PENDING slotStateChanged("reclusterViewState");
        }
        return;
    case KlustersDoc::OK:
        break;
    }

    QString info = "The automatic reclustering of ";
    if(clustersToRecluster.size() > 1) info.append("clusters ");
    else info.append("cluster ");
    Q3ValueList<int>::iterator iterator;
    for(iterator = clustersToRecluster.begin(); iterator != clustersToRecluster.end(); ++iterator ){
        info.append(QString::number(*iterator));
        info.append(" ");
    }

    if(clustersFromReclustering.size() > 1)
        info.append("is finished.\nThe following new clusters have been created: ");
    else info.append("is finished.\nThe following new cluster has been created: ");
    for(iterator = clustersFromReclustering.begin(); iterator != clustersFromReclustering.end(); ++iterator ){
        info.append(QString::number(*iterator));
        info.append(" ");
    }
    info.append(".\nThe cluster list will now be updated.");

    QApplication::restoreOverrideCursor();
    QMessageBox::information(this, tr("Automatic Reclustering !"),info);
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    doc->reclusteringUpdate(clustersToRecluster,clustersFromReclustering);

    processFinished = true;
    processKilled = false;
    //KDAB_PENDING slotStateChanged("noReclusterState");
    if(!doesActiveDisplayContainProcessWidget()) updateUndoRedoDisplay();
    else{

        //KDAB_PENDING slotStateChanged("reclusterViewState");
    }
    QApplication::restoreOverrideCursor();
}

void KlustersApp::slotStopRecluster(){
    processWidget->killJob();
    processKilled = true;
    //KDAB_PENDING slotStateChanged("stoppedReclusterState");
}

void KlustersApp::slotOutputTreatmentOver(){
    processOutputsFinished = true;
    //KDAB_PENDING slotStateChanged("noRclusterState");
    if(!doesActiveDisplayContainProcessWidget()) updateUndoRedoDisplay();
    else{

        //KDAB_PENDING slotStateChanged("reclusterViewState");
    }
}

void KlustersApp::updateUndoRedoDisplay(){
    if(currentNbUndo > 0) {
        //KDAB_PENDING slotStateChanged("undoState");
    }
    else{

        //KDAB_PENDING slotStateChanged("emptyUndoState");
    }
    if(currentNbRedo == 0) {
        //KDAB_PENDING slotStateChanged("emptyRedoState");
    }
}

void KlustersApp::widgetAddToDisplay(KlustersView::DisplayType displayType,QDockWidget* docWidget){
    KlustersView* view = activeView();
    bool newWidgetType = view->addView(docWidget,displayType,backgroundColor,statusBar(),displayTimeInterval,waveformsGain,channelPositions);

    isInit = true; //prevent the spine boxes or the lineedit and the editline to trigger during initialisation.

    if(newWidgetType)
        switch(displayType){
        case KlustersView::CLUSTERS:
            //Update the dimension spine boxes with the initial values.
            dimensionX->setValue(1);
            dimensionY->setValue(2);
            //KDAB_PENDING slotStateChanged("clusterViewState");
            dimensionX->show();
            dimensionY->show();
            featureXLabel->show();
            break;
        case KlustersView::WAVEFORMS:
            //KDAB_PENDING slotStateChanged("waveformsViewState");
            overlayPresentation->setChecked(false);
            meanPresentation->setChecked(false);
            timeFrameMode->setChecked(false);
            duration->hide();
            durationLabel->hide();
            start->hide();
            startLabel->hide();
            spikesTodisplay->setValue(DEFAULT_NB_SPIKES_DISPLAYED);
            spikesTodisplay->show();
            spikesTodisplayLabel->show();
            break;
        case KlustersView::CORRELATIONS:
            //KDAB_PENDING slotStateChanged("correlationViewState");
            //Update the lineEdit
            scaleByMax->setChecked(true);
            correlogramTimeFrame = INITIAL_CORRELOGRAMS_HALF_TIME_FRAME.toInt() * 2 + 1;
            binSize = DEFAULT_BIN_SIZE.toInt();
            correlogramsHalfDuration->setText(QString("%1").arg(correlogramTimeFrame / 2));
            binSizeBox->setText(QString("%1").arg(binSize));
            correlogramsHalfDuration->show();
            correlogramsHalfDurationLabel->show();
            binSizeBox->show();
            binSizeLabel->show();
            //Update the shoulder line menu entry
            shoulderLine->setChecked(true);
            break;
        case KlustersView::ERROR_MATRIX :
            //KDAB_PENDING slotStateChanged("errorMatrixViewState");
            //KDAB_PENDING slotStateChanged("groupingAssistantDisplayExists");
            errorMatrixExists = true;
            break;
        case KlustersView::OVERVIEW:
            break;
        case KlustersView::GROUPING_ASSISTANT_VIEW:
            break;
        case KlustersView::TRACES:
            //KDAB_PENDING slotStateChanged("traceViewState");
            showHideLabels->setChecked(view->getLabelStatus());
            //Update the browsing possibility of the traceView
            if(view->clusters().size() != 0) {
                //KDAB_PENDING slotStateChanged("traceViewBrowsingState");
            }
            break;
        }

    isInit = false; //now a change in a spine box  or the lineedit
    //will trigger an update of the view contains in the acative display.

    //The select time tool is useful only if both a clusterView and a traceView are present
    if(view->containsClusterView() && view->containsTraceView()){
        //KDAB_PENDING slotStateChanged("traceViewClusterViewState");
    }
}

void KlustersApp::widgetRemovedFromDisplay(KlustersView::DisplayType displayType){
    switch(displayType){
    case KlustersView::CLUSTERS:
        //KDAB_PENDING slotStateChanged("noClusterViewState");
        dimensionX->hide();
        dimensionY->hide();
        featureXLabel->hide();
        break;
    case KlustersView::WAVEFORMS:
        timeFrameMode->setChecked(false);
        overlayPresentation->setChecked(false);
        meanPresentation->setChecked(false);
        //KDAB_PENDING slotStateChanged("noWaveformsViewState");
        duration->hide();
        durationLabel->hide();
        start->hide();
        startLabel->hide();
        spikesTodisplay->hide();
        spikesTodisplayLabel->hide();
        break;
    case KlustersView::CORRELATIONS:
        //KDAB_PENDING slotStateChanged("noCorrelationViewState");
        correlogramsHalfDuration->hide();
        correlogramsHalfDurationLabel->hide();
        binSizeBox->hide();
        binSizeLabel->hide();
        break;
    case KlustersView::ERROR_MATRIX :
        //KDAB_PENDING slotStateChanged("noErrorMatrixViewState");
        //KDAB_PENDING slotStateChanged("groupingAssistantDisplayNotExists");
        errorMatrixExists = false;
        break;
    case KlustersView::OVERVIEW:
        break;
    case KlustersView::GROUPING_ASSISTANT_VIEW:
        break;
    case KlustersView::TRACES:
        //KDAB_PENDING slotStateChanged("noTraceViewState");
        break;
    }
}

void KlustersApp::updateDimensionSpinBoxes(int dimensionX, int dimensionY){
    isInit = true; //prevent the spine boxes or the lineedit and the editline to trigger during initialisation

    //Update the dimension spine boxes
    this->dimensionX->setValue(dimensionX);
    this->dimensionY->setValue(dimensionY);
    this->dimensionX->show();
    this->dimensionY->show();

    isInit = false; //now a change in a spine box  or the lineedit
    //will trigger an update of the view contains in the active display.
}

void KlustersApp::renameActiveDisplay(){
    QDockWidget* current;

    //Get the active tab
    current = static_cast<QDockWidget*>(tabsParent->currentPage());

    bool ok;
    QString newLabel = QInputDialog::getText(tr("New Display label"),tr("Type in the new display label"),QLineEdit::Normal, QString(), &ok, this, current->tabPageLabel());
    if(!newLabel.isEmpty() && ok){
        tabsParent->setTabLabel(current,newLabel);
        current->setTabPageLabel(newLabel);
    }
}

void KlustersApp::slotShowLabels(){
    KlustersView* currentView = activeView();
    if(currentView->containsTraceView()) currentView->showLabelsUpdate(showHideLabels->isChecked());
}

void KlustersApp::updateCorrelogramViewVariables(int binSize,int timeWindow,bool isShoulderLine, Data::ScaleMode correlationScale){
    isInit = true; //prevent the boxes or menu entry to trigger during initialisation

    //Update the boxes
    this->binSize = binSize;
    binSizeBox->setText(QString("%1").arg(binSize));
    //timeWindow is the time frame use to compute the correlograms, the lineedit correlogramsHalfDuration contains half of it.
    correlogramTimeFrame = timeWindow;
    //int k = ((static_cast<float>(correlogramTimeFrame) / static_cast<float>(binSize)) - 1) / 2;
    //correlogramsHalfDuration->setText(QString("%1").arg(k * binSize));
    correlogramsHalfDuration->setText(QString("%1").arg(static_cast<int>(correlogramTimeFrame / 2)));
    shoulderLine->setChecked(isShoulderLine);

    switch(correlationScale){
    case Data::RAW :
        noScale->setChecked(true);
        break;
    case Data::MAX :
        scaleByMax->setChecked(true);
        break;
    case Data::SHOULDER :
        scaleByShouler->setChecked(true);
        break;
    }

    isInit = false; //now a change in the boxes or menu entry
    //will trigger an update of the view contains in the active display.
}

void KlustersApp::slotShowNextCluster(){
    //Trigger ths action only if the active display does not contain a ProcessWidget
    if(!doesActiveDisplayContainProcessWidget()){
        KlustersView* view = activeView();
        view->showNextCluster();
    }
}

void KlustersApp::slotShowPreviousCluster(){
    //Trigger ths action only if the active display does not contain a ProcessWidget
    if(!doesActiveDisplayContainProcessWidget()){
        KlustersView* view = activeView();
        view->showPreviousCluster();
    }
}

void KlustersApp::slotSpikesDeleted(){
    //Update the browsing possibility of the traceView
    KlustersView* view = activeView();
    if(view->containsTraceView() && view->clusters().size() != 0) {
        //KDAB_PENDING slotStateChanged("traceViewBrowsingState");
    }
    else{

        //KDAB_PENDING slotStateChanged("noTraceViewBrowsingState");
    }
}

#include "klusters.moc"
