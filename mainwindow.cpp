#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->lineEdit_version->setText(QString((char*)BOXV3CHECKAPP_VERSION));
    slotDisplayInit(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slotDisplayInit(bool defFlag)
{
    Qt::CheckState checkState;
    if(defFlag)
    {
        checkState = Qt::Checked;
    }else
    {
        checkState = Qt::Unchecked;
    }

    ui->checkBox_deviceNode->setCheckState(checkState);
    ui->checkBox_ltemodule->setCheckState(checkState);
    ui->checkBox_SIMSlot->setCheckState(checkState);
    ui->checkBox_SIMDataService->setCheckState(checkState);
    ui->checkBox_SIMOperator->setCheckState(checkState);
    ui->checkBox_SIMDialing->setCheckState(checkState);
    ui->checkBox_SIMSignal->setCheckState(checkState);
    ui->checkBox_netAccess->setCheckState(checkState);

    ui->lineEdit_deviceNode->setText(NULL);
    ui->lineEdit_LTEmodule->setText(NULL);
    ui->lineEdit_SIMslot->setText(NULL);
    ui->lineEdit_service->setText(NULL);
    ui->lineEdit_operator->setText(NULL);
    ui->lineEdit_dialing->setText(NULL);
    ui->lineEdit_signal->setText(NULL);
    ui->lineEdit_netaccess->setText(NULL);
}

void MainWindow::slotDisplay(char stage, QString result)
{
    Qt::CheckState checkState;
    if(result.isEmpty())
    {
        checkState = Qt::Unchecked;
    }else
    {
        checkState = Qt::Checked;
    }

    switch(stage)
    {
    case STAGE_NODE:
    {
        slotDisplayInit(false);
        ui->checkBox_deviceNode->setCheckState(checkState);
        ui->lineEdit_deviceNode->setText(result);
        break;
    }
    case STAGE_MODULE:
    {
        ui->checkBox_ltemodule->setCheckState(checkState);
        ui->lineEdit_LTEmodule->setText(result);
        break;
    }
    case STAGE_SLOT:
    {
        ui->checkBox_SIMSlot->setCheckState(checkState);
        ui->lineEdit_SIMslot->setText(result);
        break;
    }
    case STAGE_SERVICE:
    {
        ui->checkBox_SIMDataService->setCheckState(checkState);
        ui->lineEdit_service->setText(result);
        break;
    }
    case STAGE_OPERATOR:
    {
        ui->checkBox_SIMOperator->setCheckState(checkState);
        ui->lineEdit_operator->setText(result);
        break;
    }
    case STAGE_TEMP:
    {
        ui->checkBox_temp->setCheckState(checkState);
        ui->lineEdit_temp->setText(result);
        break;
    }
    case STAGE_DIALE:
    {
        ui->checkBox_SIMDialing->setCheckState(checkState);
        ui->lineEdit_dialing->setText(result);
        break;
    }
    case STAGE_SIGNAL:
    {
        ui->checkBox_SIMSignal->setCheckState(checkState);
        ui->lineEdit_signal->setText(result);
        break;
    }
    case STAGE_NET:
    {
        ui->checkBox_netAccess->setCheckState(checkState);
        ui->lineEdit_netaccess->setText(result);
        break;
    }
    case STAGE_DISPLAY_NOTES:
    {
        ui->textEdit_info->append(result);
        break;
    }
    default:
    {
        DEBUG_PRINTF();
        break;
    }
    }
}
