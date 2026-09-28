#ifndef SATECVDATATYPES_H
#define SATECVDATATYPES_H

#include <cstdint>
#include <string>
//

enum CameraProperties
{
    CAP_PROP_FRAMERATE_ENABLE,      //
    CAP_PROP_FRAMERATE,             //
    CAP_PROP_HEIGHT,                //
    CAP_PROP_WIDTH,                 //
    CAP_PROP_EXPOSURE_TIME,         //
    CAP_PROP_GAIN,                  //
    CAP_PROP_OFFSETX,               //
    CAP_PROP_OFFSETY,                //
    CAP_PROP_EXPOSURE_AUTO          //
};

enum CameraIlluminationChannel : uint8_t
{
    RED = 5,
    BLUE = 10
};


struct SATECameraInfo
{
    std::string     IP;                   //Current Ip
    std::string     ManufacturerName;     // Manufacturer Name
    std::string     ModelName;            // Device Version
    std::string     SerialNumber;         // Serial Number
    std::string     UserDefinedName;      // User Defined Name

    bool operator==(const SATECameraInfo &other) const {
        return IP == other.IP &&
               ManufacturerName == other.ManufacturerName &&
               ModelName == other.ModelName &&
               SerialNumber == other.SerialNumber &&
               UserDefinedName == other.UserDefinedName;
    }

    bool operator!=(const SATECameraInfo& other) const {
        return !(*this == other);
    }

};

// Настройки камеры
struct CameraSettings{
    // Канал подсветки
    CameraIlluminationChannel СhannelIllumination =
        CameraIlluminationChannel::BLUE;
    // Уровень подсветки в процентах
    uint8_t IlluminationValue = 90;
    //
    float FrameRateFPS = 20; // fps
    // ExposureTime можно задать если значение ExposureAuto равно 0 т.е. "Off"
    float ExposureTime = 5000;
    // ExposureAuto: 0 - "Off", 1 - "Once", 2 - "Continuous"
    int ExposureAuto = 0;
    float Gain = 0.0;

    bool operator==(const CameraSettings &other) const {
        return FrameRateFPS == other.FrameRateFPS &&
               ExposureTime == other.ExposureTime &&
               ExposureAuto == other.ExposureAuto &&
               Gain == other.Gain &&
               СhannelIllumination == other.СhannelIllumination;
    }

    bool operator!=(const CameraSettings& other) const {
        return !(*this == other);
    }

    CameraSettings() = default;
    CameraSettings(const CameraSettings &) = default;
    CameraSettings(CameraSettings &&) = default;
    CameraSettings &operator=(const CameraSettings &) = default;
    CameraSettings &operator=(CameraSettings &&) = default;
};

#endif // SATECVDATATYPES_H
