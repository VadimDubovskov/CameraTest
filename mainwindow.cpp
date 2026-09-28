#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHBoxLayout>
#include <QMessageBox>

using namespace Qt::StringLiterals;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Hikvision Robotics
    setWindowTitle(u"Hikvision Robotics camera test"_s);

    connect(this, &MainWindow::sShowFrame, this, &MainWindow::onShowFrame, Qt::QueuedConnection);
    //
    auto xInfoCameras = mCamera->SATEGetCameraList();
    if(xInfoCameras){
        uint8_t index = 0;
        uint8_t indexCamera = 0;
        for(const auto &xx : *xInfoCameras){
            ui->cbxCameras->addItem(QString::fromStdString(xx.UserDefinedName));
            // Если встречается камера с UserDefinedName == Loader задаем использовать ее
            if (xx.UserDefinedName == "Loader"){
                indexCamera = index;
            }
            index++;
        }
        ui->cbxCameras->setCurrentIndex(indexCamera);
        ui->btnStartCamera->setEnabled(true);
        ui->btnOpenCamera->setEnabled(true);
    }
    //
    connect(ui->btnOpenCamera, &QPushButton::clicked, this, &MainWindow::onOpenCamera, Qt::DirectConnection);
    connect(ui->btnStartCamera, &QPushButton::clicked, this, &MainWindow::onCameraStart, Qt::DirectConnection);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onShowFrame(const cv::Mat &nImg)
{
    if(nImg.type() == CV_8UC3){
        QImage image(nImg.data, nImg.cols, nImg.rows, static_cast<int>(nImg.step), QImage::Format_RGB888);
        // OpenCV uses BGR, Qt uses RGB
        QPixmap pixmap = QPixmap::fromImage(image.rgbSwapped());
        if (!pixmap.isNull())
            ui->showImage->setPixmap(pixmap.scaled(ui->showImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void MainWindow::onOpenCamera(){
    auto xInfoCameras = mCamera->SATEGetCameraList();
    if (xInfoCameras){
        auto index = ui->cbxCameras->currentIndex();
        auto xStateMVS = mCamera->getStatus();
        if (xStateMVS->DeviceOpen){
            if (xStateMVS->StartGrabbing){
                // Отключаем камеру
                if (tCamera.get_stop_token().stop_requested() == false){
                    tCamera.request_stop();
                    if (tCamera.joinable()) {
                        tCamera.join();
                    }
                }
                // Камера подключена, отключаем
                mCamera->SATECameraStop();
                ui->btnStartCamera->setText("Старт");
                ui->showImage->clear();
            }
            // Камера подключена или в состоянии ошибки, отключаем
            mCamera->SATECameraClose();
        } else {
            // Камера откл., подключаем
           mCamera->SATECameraOpen(index);
        }
        if (xStateMVS->allError()){
            QMessageBox msgBox;
            msgBox.setWindowTitle("Предупреждение");
            msgBox.setInformativeText("Ошибка камеры!");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();
        }
        if (xStateMVS->DeviceOpen){
            ui->btnOpenCamera->setText("Отключить");
            ui->btnStartCamera->setEnabled(true);
            ui->cbxCameras->setEnabled(false);
            ui->cbxCameras->setEnabled(false);
        } else {
            ui->btnOpenCamera->setText("Подключить");
            ui->btnStartCamera->setEnabled(false);
            ui->cbxCameras->setEnabled(true);
        }
    }
}

void MainWindow::onCameraStart()
{
    auto xStateMVS = mCamera->getStatus();
    if(xStateMVS->DeviceOpen){
        if (xStateMVS->StartGrabbing){
            // Отключаем камеру
            if (tCamera.get_stop_token().stop_requested() == false){
                tCamera.request_stop();
                if (tCamera.joinable()) {
                    tCamera.join();
                }
            }
            // Камера подключена, отключаем
            mCamera->SATECameraStop();
            ui->btnStartCamera->setText("Старт");
            ui->showImage->clear();
        } else {
            // Камера выкл., подключаем
            ui->btnStartCamera->setText("Стоп");
            mCamera->SATECameraStart();
            if (xStateMVS->StartGrabbing){
                auto nCameraSettings = CameraSettings();
                // Запускаем поток
                tCamera = std::jthread([this](std::stop_token st) {
                    this->workerCamera(st, 20);
                });
            }
        }
        if (xStateMVS->allError()){
            QMessageBox msgBox;
            msgBox.setWindowTitle("Предупреждение");
            msgBox.setInformativeText("Ошибка камеры!");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();
        }
    }
}


void MainWindow::workerCamera(std::stop_token stoken, float nFrameRateFPS) {
    // Запускаем поток чтения кадров с камеры
    frameUpdate.store(false);
    // Перевод FPS в период в мсек (опрашиваем каждые 90% времени обновления кадра)
    uint64_t framerateTime = std::ceil((1000/nFrameRateFPS) * 0.9);
    auto start = std::chrono::steady_clock::now();
    while (!stoken.stop_requested()) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= framerateTime) {
            auto state = mCamera->SATECameraGetFrame(mCurrentFrameCamera, framerateTime * 10);
            if (state.allError())
                break;
            else {
                frameUpdate.store(true);
                onShowFrame(mCurrentFrameCamera);
            }
            start = std::chrono::steady_clock::now();
        }
    }
}
