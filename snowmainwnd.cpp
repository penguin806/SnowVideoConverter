#include "snowmainwnd.h"
#include "ui_snowmainwnd.h"
#include <QFileDialog>
#include <QDebug>
#include <QProcess>
#include <QFileInfo>
#include <QMessageBox>
#include <QDateTime>

SnowMainWnd::SnowMainWnd(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SnowMainWnd)
{
    ui->setupUi(this);
    this->ui->lineEdit_InputPath->installEventFilter(this);
    this->ui->lineEdit_2_OutputPath->installEventFilter(this);

    this->ffmpegPath = qApp->applicationDirPath() + "/ffmpeg-4.1.3-win64-static/bin/ffmpeg.exe";
    this->logFile = new QFile(qApp->applicationDirPath() + "/log_" +
                              QDateTime::currentDateTime().toString().replace(' ','_').replace(':','_') + ".log");
    bool bResult = this->logFile->open(QFile::WriteOnly);

    qDebug() << this->logFile->fileName() << bResult;

    this->logFileStream = new QTextStream(this->logFile);
}

SnowMainWnd::~SnowMainWnd()
{
    delete this->logFileStream;
    delete this->logFile;
    delete ui;
}

bool SnowMainWnd::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::MouseButtonPress)
    {
        if(watched == this->ui->lineEdit_InputPath)
        {
            QStringList inputPathList;
            inputPathList = QFileDialog::getOpenFileNames(this,
                                                     "Select the video file to be converted",
                                                     "",
                                                     "AVI (*.avi)");
            this->ui->lineEdit_InputPath->setText(inputPathList.join(';'));
        }
        else if (watched == this->ui->lineEdit_2_OutputPath) {
            QString outputDir;
            outputDir = QFileDialog::getExistingDirectory(this,
                                                          "Select output folder",
                                                          "");
            this->ui->lineEdit_2_OutputPath->setText(outputDir);
        }
        return false;
    }
    return false;
}

void SnowMainWnd::on_toolButton_InputFileAdd_clicked()
{
    if(this->ui->lineEdit_InputPath->text().isEmpty())
    {
        return;
    }

    QStringList inputPathList;
    inputPathList = this->ui->lineEdit_InputPath->text().split(';');
//    qDebug() << inputPathList.length();
    if(inputPathList.isEmpty())
    {
        return;
    } else {
        int currentRowCount = this->ui->tableWidget_FileTable->rowCount();
        this->ui->tableWidget_FileTable->setRowCount(currentRowCount + inputPathList.length());

        for(int i=0; i < inputPathList.length(); i++)
        {
            this->ui->tableWidget_FileTable->setItem(currentRowCount + i,0,
                    new QTableWidgetItem("Ready"));
            this->ui->tableWidget_FileTable->setItem(currentRowCount + i,1,
                    new QTableWidgetItem(inputPathList.at(i)));
        }
    }
}

void SnowMainWnd::on_pushButton_StartConvert_clicked()
{
    if(this->ui->tableWidget_FileTable->rowCount() == 0)
    {
        return;
    }

    if(this->ui->lineEdit_2_OutputPath->text().isEmpty())
    {
        QMessageBox::critical(this,"Error","Output folder null");
        return;
    }


    this->ui->toolButton_InputFileAdd->setEnabled(false);
    this->ui->pushButton_StartConvert->setEnabled(false);
    this->outputDir = this->ui->lineEdit_2_OutputPath->text();

    this->ffmpegProcess = new QProcess(this);
    QObject::connect(this->ffmpegProcess, SIGNAL(started()), this, SLOT(ffmpegProcessStarted()));
    QObject::connect(this->ffmpegProcess, SIGNAL(readyReadStandardOutput()),this,SLOT(ffmpegReadyReadStandardOutput()));
    QObject::connect(this->ffmpegProcess, SIGNAL(readyReadStandardError()),this,SLOT(ffmpegReadyReadStandardError()));
    QObject::connect(this->ffmpegProcess, SIGNAL(finished(int)), this, SLOT(ffmpegFinished(int)));

    int tableRowCount = this->ui->tableWidget_FileTable->rowCount();

    this->filesToBeConvertedTotal = 0;
    this->currentConvertingFile = -1;
    for(int i=0; i<tableRowCount; i++)
    {
        if(this->ui->tableWidget_FileTable->item(i,0)->text() == QString("Ready"))
        {
            this->filesToBeConvertedTotal++;
        }
    }

    //qDebug() << this->ffmpegPath;
    *(this->logFileStream) << QString("this->ffmpegProcess->start(") + this->ffmpegPath + ")" << "\r\n";
    this->ffmpegProcess->start(this->ffmpegPath);
}

void SnowMainWnd::ffmpegProcessStarted()
{

}

void SnowMainWnd::ffmpegReadyReadStandardError()
{
    QString outputText = this->ffmpegProcess->readAllStandardError();
    this->ui->textEdit_Info->append(outputText);
    *(this->logFileStream) << outputText;
}

void SnowMainWnd::ffmpegReadyReadStandardOutput()
{
    QString outputText = this->ffmpegProcess->readAllStandardOutput();
    this->ui->textEdit_Info->append(outputText);
    *(this->logFileStream) << outputText;
}

void SnowMainWnd::ffmpegFinished(int exitCode)
{
    if(this->currentConvertingFile == -1)
    {
        this->currentConvertingFile = -2;
    } else {
        this->ui->tableWidget_FileTable->setItem(
                    this->currentConvertingFile, 0,
                    new QTableWidgetItem( exitCode == 0 ? QString("OK") : QString("ERROR") ));
    }

    if(this->filesToBeConvertedTotal == 0)
    {
        QMessageBox::information(this,"Tip","Finished");
        this->ui->toolButton_InputFileAdd->setEnabled(true);
        this->ui->pushButton_StartConvert->setEnabled(true);
        this->ffmpegProcess->deleteLater();
        return;
    }

    int tableRowCount = this->ui->tableWidget_FileTable->rowCount();
    for(int i=0; i<tableRowCount; i++)
    {
        if(this->ui->tableWidget_FileTable->itemAt(i,0)->text() == QString("Ready"))
        {
            this->filesToBeConvertedTotal--;
            this->currentConvertingFile = i;
            QString videoPathToBeConverted =
                    this->ui->tableWidget_FileTable->item(i,1)->text();
            QFileInfo videoFileInfo(videoPathToBeConverted);


            QStringList ffmpegArgs;
            ffmpegArgs << "-i" << videoFileInfo.absoluteFilePath()
                       << "-c:v" << "libx264" << "-preset" << "ultrafast"
                       << "-movflags" << "+faststart" << "-n"
                       << this->outputDir + '/' + videoFileInfo.baseName()+".mp4";

            //qDebug() << this->ffmpegPath << ffmpegArgs;
            *(this->logFileStream) << "this->ffmpegProcess->start(" << this->ffmpegPath << ", " << ffmpegArgs.join(' ') << ")"  << "\r\n";
            this->ffmpegProcess->start(this->ffmpegPath, ffmpegArgs);
            return;
        }
    }
}
