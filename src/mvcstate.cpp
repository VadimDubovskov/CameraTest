#include "mvcstate.h"


bool MVCState::allError()
{
    init();
    return mErrors.any();
}

std::optional<uint8_t> MVCState::getIndex(const std::string_view nameParametr)
{
    init();
    if (auto search = u.find(static_cast<std::string>(nameParametr)); search != u.end())
        return search->second;
    else
        return std::nullopt;
}

bool MVCState::checkParametr(uint8_t index)
{
    init();
    return mData[index];
}

std::optional<bool> MVCState::checkParametr(const std::string_view nameParametr)
{
    init();
    if (auto search = u.find(static_cast<std::string>(nameParametr)); search != u.end())
        return mData[search->second];
    else
        return std::nullopt;
}

void MVCState::resetError()
{
    InitSDKFail = false;
    EnumDevicesFail = false;
    CreateHandleFail = false;
    DeviceNotAccessible = false;
    OpenDeviceFail = false;
    CloseDeviceFail = false;
    SetPacketSizeFail = false;
    SetTriggerModeFail = false;
    GetPayloadSizeFail = false;
    StartGrabbingFail = false;
    StopGrabbingFail = false;
    SetPixelFormatFail = false;
    GetFrameFail = false;
    SetBayerQualityFail = false;
    //
    init();
}

void MVCState::init()
{
    mData[0] = StartGrabbing;
    mData[1] = DeviceOpen;
    mData[2] =  DeviceListNotEmpty;
    //
    mErrors[0] = InitSDKFail;
    mErrors[1] = EnumDevicesFail;
    mErrors[2] = CreateHandleFail;
    mErrors[3] = DeviceNotAccessible;
    mErrors[4] = OpenDeviceFail;
    mErrors[5] = CloseDeviceFail;
    mErrors[6] = SetPacketSizeFail;
    mErrors[7] = SetTriggerModeFail;
    mErrors[8] = GetPayloadSizeFail;
    mErrors[9] = StartGrabbingFail;
    mErrors[10] = StopGrabbingFail;
    mErrors[11] = SetPixelFormatFail;
    mErrors[12] = GetFrameFail;
    mErrors[13] = SetBayerQualityFail;
}
