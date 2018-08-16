#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include "global.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void slotDisplayInit(bool defFlag = false);
    void slotDisplay(char stage, QString result);
    void slotWatchDogHandler(void);
private:
    Ui::MainWindow *ui;
    QTimer timerWatchDog;
};

#endif // MAINWINDOW_H
