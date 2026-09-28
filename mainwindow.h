#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <opencv2/core/mat.hpp>

#include "satecameracontrol.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    //
    Ui::MainWindow *ui;

    // Объект камеры
    std::shared_ptr<SATECameraControl>  mCamera = std::make_shared<SATECameraControl>();
    // Поток чтения кадров
    std::jthread tCamera;
    // Отображение кадров
    void eventShowFrame(const cv::Mat &nImg);
    void workerCamera(std::stop_token stoken, float nFrameRateFPS);
    // Флаг чтения кадра в функции workerCamera
    std::atomic<bool> frameUpdate{false};
    // Текущий считанный кадр с камеры
    cv::Mat mCurrentFrameCamera;
signals:
    void sShowFrame(const cv::Mat&);

private slots:
    void onShowFrame(const cv::Mat &nImg);
    //
    void onOpenCamera();
    void onCameraStart();
};
#endif // MAINWINDOW_H
