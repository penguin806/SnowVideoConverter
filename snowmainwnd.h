#ifndef SNOWMAINWND_H
#define SNOWMAINWND_H

#include <QMainWindow>
#include <QProcess>
#include <QFile>
#include <QTextStream>

namespace Ui {
class SnowMainWnd;
}

class SnowMainWnd : public QMainWindow
{
    Q_OBJECT

public:
    explicit SnowMainWnd(QWidget *parent = nullptr);
    ~SnowMainWnd();

private slots:
    void on_toolButton_InputFileAdd_clicked();
    void on_pushButton_StartConvert_clicked();
    void ffmpegProcessStarted();
    void ffmpegReadyReadStandardError();
    void ffmpegReadyReadStandardOutput();
    void ffmpegFinished(int exitCode);

private:
    Ui::SnowMainWnd *ui;
    bool eventFilter(QObject *watched, QEvent *event);
    QProcess *ffmpegProcess;
    int filesToBeConvertedTotal;
    int currentConvertingFile;
    QString ffmpegPath;

    QString outputDir;
    QFile *logFile;
    QTextStream *logFileStream;
};

#endif // SNOWMAINWND_H
