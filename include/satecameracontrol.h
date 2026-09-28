#ifndef SATECAMERACONTROL_H
#define SATECAMERACONTROL_H

// spdlog
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "satecvdatatypes.h"
// MVC
#include "MVS/CameraParams.h"
#include "MVS/MvCameraControl.h"
//
#include "mvcstate.h"
//
#include <memory>
#include <optional>
#include <vector>

namespace cv {
    class Mat;
}


class SATECameraControl
{
public:
    SATECameraControl();
    ~SATECameraControl();

    // Работа с MVC
    MVCState SATECameraEnumeration();
    MVCState SATECameraOpen(uint32_t nDeviceNum);
    MVCState SATECameraClose();
    MVCState SATECameraStart();
    MVCState SATECameraStop();
    MVCState SATECameraGetFrame(cv::Mat &outFrame, uint32_t mTimeout);
    uint32_t SATEGetCameraNumbers();
    std::optional<std::vector<SATECameraInfo> > SATEGetCameraList();
    std::optional<SATECameraInfo> SATEGetCameraInfo(uint32_t nDeviceNum);
    std::shared_ptr<MVCState> getStatus(){return mState;}

    std::optional<int> width();
    bool setWidth(int nWidth);

    std::optional<int> height();
    bool setHeight(int nHeight);

    std::optional<int> offset_x();
    bool setOffset_x(int nOffset_x);

    std::optional<int> offset_y();
    bool setOffset_y(int nOffset_y);

    std::optional<bool> frameRateEnable();
    bool setFrameRateEnable(bool nFrameRateEnable);

    std::optional<float> frameRate();
    bool setFrameRate(float nFrameRate);

    std::optional<float> exposureTime();
    bool setExposureTime(float nExposureTime);

    std::optional<float> gain();
    bool setGain(float nGain);

    // 0 - "Off", 1 - "Once", 2 - "Continuous"
    std::optional<int> exposureAuto();
    bool setExposureAuto(int nExposureAuto);

    std::optional<CameraSettings> getCameraSettings();
    bool setCameraSettings(CameraSettings &nCameraSettings);

private:
    template <typename T>
    bool set(CameraProperties type, T value) {
        if (mHandle == NULL) {
            return false;
        }

        switch (type)
        {
        case CAP_PROP_FRAMERATE_ENABLE:
        {
            if (MV_CC_SetBoolValue(mHandle, "AcquisitionFrameRateEnable", value) != MV_OK){
                return false;
            }
            break;
        }
        case CAP_PROP_FRAMERATE:
        {
            if (MV_CC_SetFloatValue(mHandle, "AcquisitionFrameRate", value) != MV_OK){
                return false;
            }
            break;
        }
        case CAP_PROP_HEIGHT:
        {
            if (MV_CC_SetIntValueEx(mHandle, "Height", value) != MV_OK){
                return false;
            }
            break;
        }
        case CAP_PROP_WIDTH:
        {
            if (MV_CC_SetIntValueEx(mHandle, "Width", value) != MV_OK){
                return false;
            }
            break;
        }
        case CAP_PROP_OFFSETX:
        {
            if (MV_CC_SetIntValueEx(mHandle, "OffsetX", value) != MV_OK){
                return false;
            }
            break;
        }
        case CAP_PROP_OFFSETY:
        {
            if (MV_CC_SetIntValueEx(mHandle, "OffsetY", value) != MV_OK){
                return false;
            }
            break;
        }
        case CAP_PROP_EXPOSURE_TIME:
        {
            if (MV_CC_SetFloatValue(mHandle, "ExposureTime", value) != MV_OK){
                return false;
            }
            break;
        }
        case CAP_PROP_GAIN:
        {
            if (MV_CC_SetFloatValue(mHandle, "Gain", value) != MV_OK){
                return false;
            }
            break;
        }
        case CAP_PROP_EXPOSURE_AUTO:
        {
            if (MV_CC_SetEnumValue(mHandle, "ExposureAuto", value) != MV_OK){
                return false;
            }
            break;
        }
        default:
            return false;
        }
        return true;
    }

    template <typename T>
    std::optional<T> get(CameraProperties type){
        if (mHandle == NULL)
            return std::nullopt;

        switch (type)
        {
        case CAP_PROP_FRAMERATE_ENABLE:
        {
            bool boolValue;
            if (MV_CC_GetBoolValue(mHandle, "AcquisitionFrameRateEnable", &boolValue) != MV_OK){
                return std::nullopt;
            }
            return static_cast<T>(boolValue);
            break;
        }
        case CAP_PROP_FRAMERATE:
        {
            MVCC_FLOATVALUE tempValue;
            if (MV_CC_GetFloatValue(mHandle, "ResultingFrameRate", &tempValue) != MV_OK){
                return std::nullopt;
            }
            return static_cast<T>(tempValue.fCurValue);
            break;
        }
        case CAP_PROP_HEIGHT:
        {
            MVCC_INTVALUE tempValue;
            if (MV_CC_GetIntValue(mHandle, "Height", &tempValue) != MV_OK){
                return std::nullopt;
            }
            return static_cast<T>(tempValue.nCurValue);
            break;
        }
        case CAP_PROP_WIDTH:
        {
            MVCC_INTVALUE tempValue;
            if (MV_CC_GetIntValue(mHandle, "Width", &tempValue) != MV_OK){
                return std::nullopt;
            }
            return static_cast<T>(tempValue.nCurValue);
            break;
        }
        case CAP_PROP_OFFSETX:
        {
            MVCC_INTVALUE tempValue = {0};
            if (MV_CC_GetIntValue(mHandle, "OffsetX", &tempValue) != MV_OK){
                return std::nullopt;
            }
            return static_cast<T>(tempValue.nCurValue);
            break;
        }
        case CAP_PROP_OFFSETY:
        {
            MVCC_INTVALUE tempValue = {0};
            if (MV_CC_GetIntValue(mHandle, "OffsetY", &tempValue) != MV_OK){
                return std::nullopt;
            }
            return static_cast<T>(tempValue.nCurValue);
            break;
        }
        case CAP_PROP_EXPOSURE_TIME:
        {
            MVCC_FLOATVALUE tempValue;
            if (MV_CC_GetFloatValue(mHandle, "ExposureTime", &tempValue) != MV_OK){
                return std::nullopt;
            }
            return static_cast<T>(tempValue.fCurValue);
            break;
        }
        case CAP_PROP_GAIN:
        {
            MVCC_FLOATVALUE tempValue;
            if (MV_CC_GetFloatValue(mHandle, "Gain", &tempValue) != MV_OK){
                return std::nullopt;
            }
            return static_cast<T>(tempValue.fCurValue);
            break;
        }
        case CAP_PROP_EXPOSURE_AUTO:
        {
            MVCC_ENUMVALUE tempValue = { 0 };
            if (MV_CC_GetEnumValue(mHandle, "ExposureAuto", &tempValue) != MV_OK){
                return std::nullopt;
            }
            return static_cast<T>(tempValue.nCurValue);
            break;
        }
        default:
            return std::nullopt;
        }
        return true;
    }

private:
    // Флаг инициализации SDK камеры
    bool initSDK = false;
    // Список доступных устройств (камер)
    MV_CC_DEVICE_INFO_LIST *mDeviceList = nullptr;
    // Состояние камеры
    std::shared_ptr<MVCState> mState;
    //
    void* mHandle = NULL;
    // Текущие настроки камеры
    std::shared_ptr<CameraSettings> mCameraSettings = nullptr;
    // Логгер
    std::shared_ptr<spdlog::logger> mLogger;
};

#endif // SATECAMERACONTROL_H
