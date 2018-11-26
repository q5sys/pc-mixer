#include "MixerGUI.h"
#include "ui_MixerGUI.h"

MixerGUI::MixerGUI(QSettings *set) : QMainWindow(), ui(new Ui::MixerGUI){
  ui->setupUi(this); //load the designer file
  settings = set; //save this settings access for later
  closing = false;
  loadPulseDisabled();

  //connect the signals/slots
  connect(ui->actionClose_Mixer, SIGNAL(triggered()), this, SLOT(hideGUI()) );
  connect(ui->actionClose_Mixer_and_Tray, SIGNAL(triggered()), this, SLOT(closeApplication()) );
  connect(ui->menuConfiguration, SIGNAL(triggered(QAction*)), this, SLOT(startExternalApp(QAction*)) );
  connect(ui->tool_test, SIGNAL(clicked()), this, SLOT(TestSound()) );
  connect(ui->actionRestart_PulseAudio, SIGNAL(triggered()), this, SLOT(RestartPulseAudio()) );
  connect(ui->check_disablepulse, SIGNAL(toggled(bool)), this, SLOT(setPulseDisabled(bool)) );
  connect(ui->tool_set_default, SIGNAL(clicked()), this, SLOT(saveOutputDevice()) );
}

MixerGUI::~MixerGUI(){
	
}

void MixerGUI::updateGUI(){
  //Load the list of available devices
  QStringList devList = Mixer::getDevices();
  //devList.sort();
  //Clear the UI
  ui->combo_default->disconnect();
  ui->combo_default->clear();
  ui->combo_record->disconnect();
  ui->combo_record->clear();
  ui->combo_outdevice->disconnect();
  ui->combo_outdevice->clear();
  
  if(ui->scrollArea->widget() != nullptr)
  {	  
     delete ui->scrollArea->widget(); //delete the widget and all children
     ui->scrollArea->setWidget(nullptr);
  }

  ui->scrollArea->setWidget( new QWidget() ); //create a new widget in the scroll area
  ui->scrollArea->widget()->setContentsMargins(0,0,0,0);
  QHBoxLayout *layout = new QHBoxLayout;
  //Now Fill the UI with the devices
  QString cdefault ="none";
  QString rdefault = Mixer::getRecDevice();
  if(settings!=0){
    cdefault = settings->value("tray-device", "vol").toString();
  }
  for(int i=0; i<devList.length(); i++){
    //Get the individual pieces
    QString dev = devList[i].section(":",0,0);
    int Lval = devList[i].section(":",1,1).toInt();
    int Rval = devList[i].section(":",2,2).toInt();
    //Now create the device widget
    DeviceWidget *device = new DeviceWidget(this);
      device->setupDevice(dev, Lval, Rval);
      layout->addWidget(device);
      connect(device, SIGNAL(deviceChanged(QString)), this, SLOT(itemChanged(QString)) );
    //Now add the device to the default/record lists
    ui->combo_default->addItem(dev);
    ui->combo_record->addItem(dev);
    if(dev == cdefault){
      ui->combo_default->setCurrentIndex(i);
    }
    if(dev == rdefault){
      ui->combo_record->setCurrentIndex(i);
    }
  }
  layout->addStretch(); //add spacer to the end
  layout->setContentsMargins(2,2,2,2);
  layout->setSpacing(4);
  ui->scrollArea->widget()->setLayout(layout);
  ui->scrollArea->setMinimumHeight(ui->scrollArea->widget()->minimumSizeHint().height()+ui->scrollArea->horizontalScrollBar()->height());
  //Now rebuild the output device list
  QStringList outdevs =runShellCommand("cat /dev/sndstat");
  qDebug() << outdevs.length() << " of Output Devices Found:" << outdevs;
  for(int i=0; i<outdevs.length(); i++){
    if(outdevs[i].startsWith("pcm")){
      ui->combo_outdevice->addItem(outdevs[i].section(" default",0,0), outdevs[i].section(":",0,0) );
      if(outdevs[i].contains(" default")){ ui->combo_outdevice->setCurrentIndex( ui->combo_outdevice->count()-1); }
    }
  }
  ui->combo_outdevice->setEnabled(ui->combo_outdevice->count() > 0);
  //re-connect combobox signal
  connect(ui->combo_default, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeDefaultTrayDevice(QString)) );
  connect(ui->combo_record, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeRecordingDevice(QString)) );
  connect(ui->combo_outdevice, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeOutputDevice()) );
  //Hide the tray icon controls if necessary
  ui->combo_default->setVisible(settings!=0);
  ui->label_tray->setVisible(settings!=0);
  ui->actionClose_Mixer_and_Tray->setVisible(settings!=0);
  //Enable/Disable the pulseaudio shortcuts if they are not installed
  ui->actionRestart_PulseAudio->setEnabled( QFile::exists("/usr/local/bin/pulseaudio") );
  ui->action_PulseAudio_Mixer->setEnabled( QFile::exists("/usr/local/bin/pavucontrol") );
  ui->actionPulseAudio_Settings->setEnabled( QFile::exists("/usr/local/bin/paprefs") );
}

void MixerGUI::loadPulseDisabled(){
  bool disabled = false;
  //Find the current config directory
  QString confdir = getenv("XDG_CONFIG_HOME");
  if(confdir.isEmpty()){confdir =  QDir::homePath()+"/.config"; }
  //Find the client config file
  QString client_conf = confdir+"/pulse/client.conf";
  if(QFile::exists(client_conf) ){
    QStringList lines = runShellCommand("grep autospawn \""+client_conf+"\"");
    for(int i=0; i<lines.length(); i++){
      lines[i] = lines[i].simplified();
      if(lines[i].startsWith(";") || lines[i].startsWith("#")){ continue; } //commented out
      if(lines[i].contains("=")){
        disabled = lines[i].section("=",1,1).simplified().toLower()=="no";
      }
    }
  }
  //Now check the status of the auto-start on login (both must be disabled as a unit)
  if(disabled){
    QString auto_conf = confdir+"/autostart/pulseaudio.desktop";
    if(QFile::exists(auto_conf)){
      QStringList lines = runShellCommand("grep Hidden \""+auto_conf+"\"");
      for(int i=0; i<lines.length(); i++){
        lines[i] = lines[i].simplified();
        if(lines[i].contains("=")){
          disabled = lines[i].section("=",1,1).simplified().toLower()=="true";
          break;
        }
      }
    }else{
      disabled = false; //not disabled from auto-start on login for this user
    }
  }
  ui->menuConfiguration->setEnabled(!disabled);
  ui->check_disablepulse->setChecked(disabled);
}

void MixerGUI::setPulseDisabled(bool disable){
  //Find the current config directory
  QString confdir = getenv("XDG_CONFIG_HOME");
  if(confdir.isEmpty()){confdir =  QDir::homePath()+"/.config"; }
  //Adjust the client config file
  QString client_conf = confdir+"/pulse/client.conf";
  QStringList contents;
  if(QFile::exists(client_conf) ){
    QStringList lines = readFile(client_conf);
    for(int i=0; i<lines.length(); i++){
      if(lines[i].simplified().startsWith(";") || lines[i].simplified().startsWith("#")){ continue; } //commented out
      if(lines[i].contains("=") && lines[i].contains("autospawn") ){
	lines[i] = QString("autospawn = ")+(disable ? "no" : "yes");
      }
    }
    contents = lines;
  }else{
    contents << "#PulseAudio client configuration overrides";
    contents << "# - automatically generated by pc-mixer";
    contents << "# For all options please look at /usr/local/etc/pulse/client.conf";
    contents << QString("autospawn = ")+(disable ? "no" : "yes");
  }
  if( !writeFile(client_conf, contents) ){
    //Could not save client settings
    QMessageBox::warning(this, tr("ERROR"), tr("Could not save PulseAudio client configuration file"));
    ui->check_disablepulse->setChecked(!disable); //reset the check state back to the previous value
    return;
  }
  //Now adjust the autolaunch registration
  QString auto_conf = confdir+"/autostart/pulseaudio.desktop";
  contents.clear(); //re-use this string list for the next file
  if(QFile::exists(auto_conf)){
    contents = readFile(auto_conf);
    bool found = false;
    for(int i=0; i<contents.length() && !found; i++){
      if(contents[i].contains("Hidden=")){
        contents[i]=QString("Hidden=")+(disable ? "true" : "false");
        found = true;
      }
    }
    if(!found){ contents << QString("Hidden=")+(disable ? "true" : "false"); }
  }else{
    contents << "[Desktop Entry]";
    contents << "Type=Application";
    contents << "Hidden="+QString(disable ? "true" : "false"); 
  }
  if( !writeFile(auto_conf, contents) ){
    //Could not save client settings
    QMessageBox::warning(this, tr("ERROR"), tr("Could not save PulseAudio auto-start file\nYou may need to adjust the autostart registration through your DE itself"));
  }
  ui->menuConfiguration->setEnabled(!disable);
}

void MixerGUI::changeDefaultTrayDevice(QString device){
  if(settings!=0){ settings->setValue("tray-device", device); }
  emit updateTray();
}

void MixerGUI::changeRecordingDevice(QString device){
  Mixer::setRecDevice(device);
}

void MixerGUI::changeOutputDevice(){
  QString dev = ui->combo_outdevice->currentData().toString().section("pcm",1,-1); //should be just a number
  if(dev.isEmpty()){ return; }
  QProcess::execute("sysctl hw.snd.default_unit="+dev);
  updateGUI();
  emit outChanged();
}

void MixerGUI::saveOutputDevice(){
  QString dev = ui->combo_outdevice->currentData().toString().section("pcm",1,-1); //should be just a number
  if(dev.isEmpty()){ return; }
  QProcess::execute("qsudo sysrc -f /etc/sysctl.conf hw.snd.default_unit="+dev);
}

void MixerGUI::itemChanged(QString device){
  if(device == ui->combo_default->currentText()){
    emit updateTray();
  }
}
 
void MixerGUI::TestSound(){
  static QMediaPlayer *mediaobj = 0;
  //ui->tool_test->setEnabled(false);
  if(mediaobj==0){ 
    mediaobj = new QMediaPlayer();
    connect(mediaobj, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(TestStateChanged(QMediaPlayer::State)) );
  }else{
    mediaobj->stop();
    QApplication::processEvents();
  }
  mediaobj->setMedia( QUrl("qrc:/testsound.ogg"));
  mediaobj->setVolume(100);
  QApplication::processEvents();
  mediaobj->play();
}

void MixerGUI::TestStateChanged(QMediaPlayer::State state){
  ui->tool_test->setEnabled(state == QMediaPlayer::StoppedState);
}

void MixerGUI::RestartPulseAudio(){
  if(!QFile::exists("/usr/local/bin/pulseaudio")){ return; }
  QProcess::execute("pulseaudio --kill");
  QProcess::startDetached("start-pulseaudio-x11");
}
