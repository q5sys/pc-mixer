#ifndef _PC_MIXER_GUI_H
#define _PC_MIXER_GUI_H

#include <QMainWindow>
#include <QSettings>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QProcess>
#include <QAction>
#include <QMediaPlayer>

#include "DeviceWidget.h"
#include "MixerBackend.h"

namespace Ui{
	class MixerGUI;
};

class MixerGUI : public QMainWindow{
	Q_OBJECT
public:
	MixerGUI(QSettings* set = 0);
	~MixerGUI();

	void updateGUI(); //For the tray to call before the GUI becomes active

private:
	Ui::MixerGUI *ui;
	QSettings *settings;
	bool closing;

	QStringList runShellCommand(QString cmd){
	 //split the command string with individual commands seperated by a ";" (if any)
	   QProcess p;  
	   //Make sure we use the system environment to properly read system variables, etc.
	   p.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
	   //Merge the output channels to retrieve all output possible
	   p.setProcessChannelMode(QProcess::MergedChannels);   
	   p.start(cmd);
	   while(p.state()==QProcess::Starting || p.state() == QProcess::Running){
	     p.waitForFinished(200);
	     QCoreApplication::processEvents();
	   }
	   QString outstr = p.readAllStandardOutput();
	 if(outstr.endsWith("\n")){outstr.chop(1);} //remove the newline at the end 
	 return outstr.split("\n");
	}

private slots:
	void hideGUI(){
	  if(settings==0){ this->close(); } //no tray
	  else{ this->hide(); } //tray
	}
	void closeApplication(){
	  closing = true;
	  this->close();
	}
	
	void startExternalApp(QAction *act){
	  if(act->whatsThis().isEmpty()){ return; }
	  QProcess::startDetached(act->whatsThis());
	}
	
	void changeDefaultTrayDevice(QString device);
	void changeRecordingDevice(QString device);
	void changeOutputDevice();
	void itemChanged(QString device); //for individual device adjustments 
	void TestSound();
	void RestartPulseAudio();
	
	void slotSingleInstance(){
	  updateGUI();
	  this->show();
	}
	
protected:
	void closeEvent(QCloseEvent *event){
	  if(!closing && settings!=0){
	    //tray running - just hide it
	    event->ignore();
	    hideGUI();
	  }
	}
	
signals:
	void updateTray();
    void outChanged();
	
};

#endif
