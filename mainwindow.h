#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
