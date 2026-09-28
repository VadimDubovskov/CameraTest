#ifndef MVCSTATE_H
#define MVCSTATE_H

#include <bitset>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>

using namespace std::string_view_literals;

class MVCState
{
public:
    MVCState() = default;
    ~MVCState() = default;

    // Имена параметров
    std::string_view pStartGrabbing = "StartGrabbing"sv;
    std::string_view pDeviceOpen = "DeviceOpen"sv;
    std::string_view pDeviceListNotEmpty = "DeviceListNotEmpty"sv;

    bool StartGrabbing = false;
    bool DeviceOpen = false;
    bool DeviceListNotEmpty = false;
    //
    bool InitSDKFail = false;
    bool EnumDevicesFail = false;
    bool CreateHandleFail = false;
    bool DeviceNotAccessible = false;
    bool OpenDeviceFail = false;
    bool CloseDeviceFail = false;
    bool SetPacketSizeFail = false;
    bool SetTriggerModeFail = false;
    bool GetPayloadSizeFail = false;
    bool StartGrabbingFail = false;
    bool StopGrabbingFail = false;
    bool SetPixelFormatFail = false;
    bool GetFrameFail = false;
    bool SetBayerQualityFail = false;

    // TRUE - если есть любая ошибка
    bool allError();
    // Получить индекс параметра по имени
    std::optional<uint8_t> getIndex(const std::string_view nameParametr);
    // Проверка значения параметра по индексу (проверка валидности индекса не выполняется)
    bool checkParametr(uint8_t index);
    // Проверка значения параметра по имени
    std::optional<bool> checkParametr(const std::string_view nameParametr);
    // Сброс ошибок, изменение состояния камеры на close/stop
    void resetError();

    private:
    void init();
    std::bitset<3> mData;
    std::bitset<14> mErrors;
    std::unordered_map<std::string, int> u = {
        {static_cast<std::string>(pStartGrabbing), 0},
        {static_cast<std::string>(pDeviceOpen), 1},
        {static_cast<std::string>(pDeviceListNotEmpty), 2}
    };

};

#endif // MVCSTATE_H
