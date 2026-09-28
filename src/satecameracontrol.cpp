#include "satecameracontrol.h"
#include "config_satecv.h"
#include "cvutils.h"

// OpenCV
#include <opencv2/opencv.hpp>

#if defined (_WIN32)|| defined(_WIN64)|| defined(__WIN32__) || defined(__WINDOWS__)
#include <Windows.h>
#include <process.h>
#include <conio.h>
#endif
// Linux, BSD and Solaris define "unix", OSX doesn't, even though it derives from BSD
#if defined(unix) || defined(__unix__) || defined(__unix)

#endif


SATECameraControl::SATECameraControl() :
    mDeviceList(new MV_CC_DEVICE_INFO_LIST),
    mState(std::make_shared<MVCState>()){
    mState->DeviceOpen = false;

    try {
        // Create a file rotating logger with 5 MB size max and 3 rotated files
        auto max_size = 1048576 * 5;
        auto max_files = 50;
        mLogger = spdlog::rotating_logger_mt("SATE_CV", FILE_LOG, max_size, max_files);
        mLogger->set_pattern(FORMAT_LOG);
        mLogger->set_level(LEVEL_LOG);
        mLogger->info("Start camera control");
    } catch (const spdlog::spdlog_ex &ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
    }

    // Initialize SDK
    if (int code = MV_CC_Initialize() != MV_OK) {
        initSDK = false;
        mLogger->error("Initialize SDK fail, ERR_CODE {:X}, in SATECameraControl()", code);
    } else {
        mLogger->trace("Initialize SDK Camera done");
        initSDK = true;
        SATECameraEnumeration();
    }
}

SATECameraControl::~SATECameraControl()
{
    mLogger->trace("Call ~SATECameraControl()");
    if (mHandle) {
        int code;
        try {
            mLogger->trace("Destroy object camera control");
            // Stop grab image
            if(mState->StartGrabbing){
                mLogger->trace("Call MV_CC_StopGrabbing()");
                code = MV_CC_StopGrabbing(mHandle);
                if (code != MV_OK)
                    mLogger->error("MV_CC_StopGrabbing fail, ERR_CODE {:X}, in ~SATECameraControl()", code);
            }
            // Close device
            if(mState->DeviceOpen && !mState->allError()){
                mLogger->trace("Call MV_CC_CloseDevice()");
                code = MV_CC_CloseDevice(mHandle);
                if (code != MV_OK)
                    mLogger->error("MV_CC_CloseDevice fail, ERR_CODE {:X}, in ~SATECameraControl()", code);
            }
            // Destroy handle
            if (mHandle != NULL){
                mLogger->trace("Call MV_CC_DestroyHandle()");
                code = MV_CC_DestroyHandle(mHandle);
                if (code != MV_OK)
                    mLogger->error("MV_CC_DestroyHandle fail, ERR_CODE {:X}, in ~SATECameraControl()", code);
            }
        } catch (...) {
            mLogger->error("Close object SATECameraControl fail");
            mLogger->flush();
        }
    }
    if(mDeviceList)
        delete mDeviceList;

    if (initSDK)
        MV_CC_Finalize();

    mLogger->flush();
    //spdlog::shutdown();
}

MVCState SATECameraControl::SATECameraEnumeration()
{
    mLogger->trace("Call SATECameraEnumeration()");
    if(!initSDK){
        mLogger->error("SDK not init, need restart app");
        mState->InitSDKFail = true;
    }
    // Enum device
    int code = MV_CC_EnumDevices(MV_GIGE_DEVICE , mDeviceList);
    if (code != MV_OK) {
        mState->EnumDevicesFail = true;
        mLogger->error("MV_CC_EnumDevices fail, ERR_CODE {:X}", code);
    }
    if (mDeviceList->nDeviceNum <= 0){
        mState->DeviceListNotEmpty = false;
        mLogger->error("SATECameraOpen({0}) fail. DeviceList empty");
        return *mState;
    }
    mState->DeviceListNotEmpty = true;
    return *mState;
}

MVCState SATECameraControl::SATECameraOpen(unsigned int nDeviceNum)
{
    bool xSts = mState->allError();
    if (mState->DeviceOpen || xSts || nDeviceNum >= mDeviceList->nDeviceNum){
        mLogger->error("SATECameraOpen({0}) fail, isOpen={1}, isError={2}", nDeviceNum, mState->DeviceOpen, xSts);
        return *mState;
    }

    if (!MV_CC_IsDeviceAccessible(mDeviceList->pDeviceInfo[nDeviceNum], MV_ACCESS_Exclusive)) {
        mState->DeviceNotAccessible = true;
        mLogger->error("SATECameraOpen({0}) fail. Device not accessible", nDeviceNum);
        return *mState;
    }
    // Create handle
    int code = MV_CC_CreateHandle(&mHandle, mDeviceList->pDeviceInfo[nDeviceNum]);
    if (code != MV_OK)
    {
        mLogger->error("SATECameraOpen({0}) fail. Create Handle fail, ERR_CODE {1:X}", nDeviceNum, code);
        mState->CreateHandleFail = true;
        return *mState;
    }
    // Open device
    code = MV_CC_OpenDevice(mHandle);
    if (code != MV_OK) {
        mLogger->error("SATECameraOpen({0}) fail. Open Device fail, ERR_CODE {1:X}", nDeviceNum, code);
        mState->OpenDeviceFail = true;
        return *mState;
    }
    // Set trigger mode as off
    if (MV_CC_SetEnumValue(mHandle, "TriggerMode", 0) != MV_OK) {
        mLogger->error("SATECameraOpen({0}) fail. Set Trigger Mode fail", nDeviceNum);
        mState->SetTriggerModeFail = true;
        return *mState;
    }
    // Detection network optimal package size(It only works for the GigE camera)
    if (MV_GIGE_DEVICE == mDeviceList->pDeviceInfo[nDeviceNum]->nTLayerType) {
        int nPacketSize = MV_CC_GetOptimalPacketSize(mHandle);
        if (nPacketSize > 0)
        {
            // Packet
            if (MV_CC_SetIntValue(mHandle, "GevSCPSPacketSize", nPacketSize) != MV_OK) {
                mLogger->error("SATECameraOpen({0}) fail. Set Packet Size fail", nDeviceNum);
                mState->SetPacketSizeFail = true;
                return *mState;
            }
        } else {
            mLogger->error("SATECameraOpen({0}) fail. Get Packet Size fail", nDeviceNum);
            mState->SetPacketSizeFail = true;
            return *mState;
        }
    }
    // Set pixel format
    // 0x02180014
    // при формате PixelType_Gvsp_BGR8_Packed - частота обновления кадров падает до 6
    // PixelType_Gvsp_BayerRG8 - формат по умолчанию, частота обновления кадров около 18
    if (MV_CC_SetEnumValue(mHandle, "PixelFormat", PixelType_Gvsp_BayerRG8) != MV_OK)
    {
        mLogger->error("SATECameraOpen({0}) fail. Set PixelFormat fail", nDeviceNum);
        mState->SetPixelFormatFail = true;
        return *mState;
    }
    mLogger->trace("SATECameraOpen({:X}) done. Set PixelFormat PixelType_Gvsp_BayerRG8", nDeviceNum);
    mLogger->flush();
    mState->DeviceOpen = true;
    return *mState;
}

MVCState SATECameraControl::SATECameraClose()
{
    int code = 0;
    mLogger->trace("SATECameraClose() call.");
    mState->resetError();
    if(mHandle == NULL){
        mState->DeviceOpen = false;
        mState->StartGrabbing = false;
        mLogger->error("SATECameraClose() fail, mHandle is NULL ");
        return *mState;
    }
    // Close device
    mLogger->trace("MV_CC_CloseDevice({0}) call.", (int64) mHandle);
    code = MV_CC_CloseDevice(mHandle);
    if (code != MV_OK) {
        mLogger->error("SATECameraClose() fail. Close Device fail, ERR_CODE {0:X}", code);
        mState->CloseDeviceFail = true;
        return *mState;
    }

    // Destroy handle
    mLogger->trace("MV_CC_DestroyHandle({0}) call.", (int64) mHandle);
    code = MV_CC_DestroyHandle(mHandle);
    mHandle = nullptr;
    if (code != MV_OK) {
        mLogger->error("SATECameraClose() fail. Destroy handle fail, ERR_CODE {0:X}", code);
    }
    mState->DeviceOpen = false;
    mState->StartGrabbing = false;
    mLogger->trace("SATECameraClose() done");
    mLogger->flush();
    return *mState;
}

MVCState SATECameraControl::SATECameraStart()
{
    if (mState->StartGrabbing == true){
        mLogger->trace("Call SATECameraStart(), already started");
        return *mState;
    }
    if(mHandle && mState->allError() == false){
        // Start grab image
        int code = MV_CC_StartGrabbing(mHandle);
        if (code != MV_OK) {
            mLogger->error("SATECameraStart() fail. Start Grabbing fail, ERR_CODE {:X}", code);
            mState->StartGrabbingFail = true;
            return *mState;
        }
        mState->StartGrabbingFail = false;
        mState->StartGrabbing = true;
        mLogger->trace("SATECameraStart() done");
    } else {
        mLogger->error("SATECameraStart() fail. Handle is NULL or error");
    }
    return *mState;
}

MVCState SATECameraControl::SATECameraStop()
{
    if(mHandle){
        // Start grab image
        int code = MV_CC_StopGrabbing(mHandle);
        if(code != MV_OK) {
            mLogger->error("SATECameraStop() fail. Stop Grabbing fail, ERR_CODE {:X}", code);
            mState->StopGrabbingFail = true;
            //mState->StartGrabbing = false;
            return *mState;
        }
        mState->StopGrabbingFail = false;
        mState->StartGrabbing = false;
        mLogger->trace("SATECameraStop() done");
    } else {
        mLogger->error("SATECameraStop() fail. Handle is NULL");
    }
    return *mState;
}

MVCState SATECameraControl::SATECameraGetFrame(cv::Mat &outFrame, uint32_t nTimeout)
{
    bool xSts = mState->allError();
    if(mHandle == NULL || xSts){
        mLogger->error("SATECameraGetFrame() fail, Error: xSts");
        return *mState;
    }

    MV_FRAME_OUT stOutFrame = {0};

    // Get frame from camera with timeout
    unsigned int code = MV_CC_GetImageBuffer(mHandle, &stOutFrame, nTimeout);
    if (code != MV_OK)
    {
        mState->GetFrameFail = true;
        //mState->StartGrabbing = false;
        mLogger->error("SATECameraGetFrame() fail. MV_CC_GetImageBuffer return error or timeout, ERR_CODE {:X}", code);
        return *mState;
    }

    // Set interpolation method to equilibrium
    /*nRet = MV_CC_SetBayerCvtQuality(mHandle.get(), 1);
    if (MV_OK != nRet)
    {
        std::cout << "Set interpolation method fail!" << std::endl;
        mLogger->AddLine("Set interpolation method fail!");
        mState = MVCState::SetBayerQualityFail;
        return mState;
    }*/

    if (PixelType_Gvsp_BGR8_Packed == stOutFrame.stFrameInfo.enPixelType )
    {
        // Mat BGR
        outFrame = cv::Mat(stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nWidth, CV_8UC3, stOutFrame.pBufAddr).clone();
        mLogger->trace("SATECameraGetFrame() done. PixelType_Gvsp_BGR8_Packed");
    } else if (PixelType_Gvsp_BayerRG8 == stOutFrame.stFrameInfo.enPixelType) {
        // Wrap raw pointer into a single-channel Mat (Zero-copy)
        cv::Mat bayerMat(stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nWidth, CV_8UC1, stOutFrame.pBufAddr);
        // Convert Bayer RG to BGR (Standard format for cv::imshow / cv::imwrite)
        cv::cvtColor(bayerMat, outFrame, cv::COLOR_BayerBG2BGR);
        mLogger->trace("SATECameraGetFrame() done. PixelType_Gvsp_BayerRG8");
    } else {
        mLogger->error("SATECameraGetFrame() fail. Frame format not supported");
        mState->GetFrameFail = true;
        //mState->StartGrabbing = false;
        return *mState;
    }
    code = MV_CC_FreeImageBuffer(mHandle, &stOutFrame);
    if (code != MV_OK){
        mLogger->error("SATECameraGetFrame() fail. MV_CC_FreeImageBuffer, ERR_CODE {:X}", code);
    }
    return *mState;
}

uint32_t SATECameraControl::SATEGetCameraNumbers()
{
    if (mDeviceList)
        return mDeviceList->nDeviceNum;

    return 0;
}

std::optional<std::vector<SATECameraInfo>> SATECameraControl::SATEGetCameraList()
{
    if (mDeviceList->nDeviceNum == 0)
        return std::nullopt;

    std::vector<SATECameraInfo> ret;
    for(auto i = 0; i < mDeviceList->nDeviceNum; i++){
        SATECameraInfo xInfo;
        xInfo.IP = int32ToIPString(mDeviceList->pDeviceInfo[i]->SpecialInfo.stGigEInfo.nCurrentIp);                                                    // NetWork IP Address
        xInfo.ManufacturerName = std::string(reinterpret_cast<const char*>(mDeviceList->pDeviceInfo[i]->SpecialInfo.stGigEInfo.chManufacturerName));   // Manufacturer Name
        xInfo.ModelName = std::string(reinterpret_cast<const char*>(mDeviceList->pDeviceInfo[i]->SpecialInfo.stGigEInfo.chModelName));                 // Device Version
        xInfo.SerialNumber = std::string(reinterpret_cast<const char*>(mDeviceList->pDeviceInfo[i]->SpecialInfo.stGigEInfo.chSerialNumber));           // Serial Number
        xInfo.UserDefinedName = std::string(reinterpret_cast<const char*>(mDeviceList->pDeviceInfo[i]->SpecialInfo.stGigEInfo.chUserDefinedName));     // User Defined Name
        ret.push_back(xInfo);
    }

    return ret;
}

std::optional<SATECameraInfo> SATECameraControl::SATEGetCameraInfo(uint32_t nDeviceNum)
{
    if(mDeviceList->nDeviceNum == 0 || nDeviceNum >= mDeviceList->nDeviceNum)
        return std::nullopt;

    SATECameraInfo ret;
    ret.IP = int32ToIPString(mDeviceList->pDeviceInfo[nDeviceNum]->SpecialInfo.stGigEInfo.nCurrentIp);                                                    // NetWork IP Address
    ret.ManufacturerName = std::string(reinterpret_cast<const char*>(mDeviceList->pDeviceInfo[nDeviceNum]->SpecialInfo.stGigEInfo.chManufacturerName));   // Manufacturer Name
    ret.ModelName = std::string(reinterpret_cast<const char*>(mDeviceList->pDeviceInfo[nDeviceNum]->SpecialInfo.stGigEInfo.chModelName));                 // Device Version
    ret.SerialNumber = std::string(reinterpret_cast<const char*>(mDeviceList->pDeviceInfo[nDeviceNum]->SpecialInfo.stGigEInfo.chSerialNumber));           // Serial Number
    ret.UserDefinedName = std::string(reinterpret_cast<const char*>(mDeviceList->pDeviceInfo[nDeviceNum]->SpecialInfo.stGigEInfo.chUserDefinedName));     // User Defined Name

    return ret;
}

std::optional<CameraSettings> SATECameraControl::getCameraSettings()
{
    if(mCameraSettings == nullptr)
        mCameraSettings = std::make_shared<CameraSettings>();

    if (auto ret = get<int>(CAP_PROP_EXPOSURE_AUTO))
        mCameraSettings->ExposureAuto = ret.value();
    else
        return std::nullopt;
    //
    if (auto ret = get<float>(CAP_PROP_EXPOSURE_TIME))
        mCameraSettings->ExposureTime = ret.value();
    else
        return std::nullopt;
    //
    if (auto ret = get<float>(CAP_PROP_FRAMERATE))
        mCameraSettings->FrameRateFPS = ret.value();
    else
        return std::nullopt;
    //
    if (auto ret = get<float>(CAP_PROP_GAIN))
        mCameraSettings->Gain = ret.value();
    else
        return std::nullopt;
    //
    return *mCameraSettings;
}

bool SATECameraControl::setCameraSettings(CameraSettings &nCameraSettings)
{
    if (getCameraSettings() == std::nullopt) return false;

    bool ret = false;
    // CAP_PROP_EXPOSURE_AUTO
    if(nCameraSettings.ExposureAuto != mCameraSettings->ExposureAuto){
        ret = set(CAP_PROP_EXPOSURE_AUTO, nCameraSettings.ExposureAuto);
        if (!ret) {
            mLogger->error("Set camera properties fail. ExposureAuto");
            return ret;
        }
    }
    // CAP_PROP_EXPOSURE_TIME
    if(nCameraSettings.ExposureTime != mCameraSettings->ExposureTime){
        ret = set(CAP_PROP_EXPOSURE_TIME, nCameraSettings.ExposureTime);
        if (!ret) {
            mLogger->error("Set camera properties fail. ExposureTime");
            return ret;
        }
    }
    // CAP_PROP_FRAMERATE
    //if(nCameraSettings.FrameRateFPS != mCameraSettings->FrameRateFPS){
    //    ret = set(CAP_PROP_FRAMERATE_ENABLE, true);
    //    if (!ret) {
    //        mLogger->error("Set camera properties fail. FrameRateEnable");
    //        return ret;
    //    }
    //    ret = set(CAP_PROP_FRAMERATE, nCameraSettings.FrameRateFPS);
    //    if (!ret) {
    //        mLogger->error("Set camera properties fail. FrameRate");
    //        return ret;
    //    }
    //}
    // CAP_PROP_GAIN
    if(nCameraSettings.Gain != mCameraSettings->Gain){
        bool ret = set(CAP_PROP_GAIN, nCameraSettings.Gain);
        if (!ret) {
            mLogger->error("Set camera properties fail. Gain");
            return ret;
        }
    }
    return true;
}

std::optional<int> SATECameraControl::exposureAuto(){
    auto ret = get<int>(CAP_PROP_EXPOSURE_AUTO);
    if (ret == std::nullopt) mLogger->error("Get camera properties fail. ExposureAuto");
    return ret;
}

bool SATECameraControl::setExposureAuto(int nExposureAuto){
    bool ret = set(CAP_PROP_EXPOSURE_AUTO, nExposureAuto);
    if (!ret) mLogger->error("Set camera properties fail. ExposureAuto");
    return ret;
}

std::optional<float> SATECameraControl::gain()
{
    auto ret = get<float>(CAP_PROP_GAIN);
    if (ret == std::nullopt) mLogger->error("Get camera properties fail. Gain");
    return ret;
}

bool SATECameraControl::setGain(float nGain)
{
    bool ret = set(CAP_PROP_GAIN, nGain);
    if (!ret) mLogger->error("Set camera properties fail. Gain");
    return ret;
}

std::optional<float> SATECameraControl::exposureTime()
{
    auto ret = get<float>(CAP_PROP_EXPOSURE_TIME);
    if (ret == std::nullopt) mLogger->error("Get camera properties fail. ExposureTime");
    return ret;
}

bool SATECameraControl::setExposureTime(float nExposureTime)
{
    bool ret = set(CAP_PROP_EXPOSURE_TIME, nExposureTime);
    if (!ret) mLogger->error("Set camera properties fail. ExposureTime");
    return ret;
}

std::optional<float> SATECameraControl::frameRate()
{
    auto ret = get<float>(CAP_PROP_FRAMERATE);
    if (ret == std::nullopt) mLogger->error("Get camera properties fail. FrameRate");
    return ret;
}

bool SATECameraControl::setFrameRate(float nFrameRate)
{
    bool ret = set(CAP_PROP_FRAMERATE, nFrameRate);
    if (!ret) mLogger->error("Set camera properties fail. FrameRate");
    return ret;
}

std::optional<bool> SATECameraControl::frameRateEnable()
{
    auto ret = get<bool>(CAP_PROP_FRAMERATE_ENABLE);
    if (ret == std::nullopt) mLogger->error("Get camera properties fail. FrameRateEnable");
    return ret;
}

bool SATECameraControl::setFrameRateEnable(bool nFrameRateEnable)
{
    bool ret = set(CAP_PROP_FRAMERATE_ENABLE, nFrameRateEnable);
    if (!ret) mLogger->error("Set camera properties fail. FrameRateEnable");
    return ret;
}

std::optional<int> SATECameraControl::offset_y()
{
    auto ret = get<int>(CAP_PROP_OFFSETY);
    if (ret == std::nullopt) mLogger->error("Get camera properties fail. Offset_y");
    return ret;
}

bool SATECameraControl::setOffset_y(int nOffset_y)
{
    bool ret = set(CAP_PROP_OFFSETY, nOffset_y);
    if (!ret) mLogger->error("Set camera properties fail. Offset_y");
    return ret;
}

std::optional<int> SATECameraControl::offset_x()
{
    auto ret = get<int>(CAP_PROP_OFFSETX);
    if (ret == std::nullopt) mLogger->error("Get camera properties fail. Offset_x");
    return ret;
}

bool SATECameraControl::setOffset_x(int nOffset_x)
{
    bool ret = set(CAP_PROP_OFFSETX, nOffset_x);
    if (!ret) mLogger->error("Set camera properties fail. Offset_x");
    return ret;
}

std::optional<int> SATECameraControl::height()
{
    auto ret = get<int>(CAP_PROP_HEIGHT);
    if (ret == std::nullopt) mLogger->error("Get camera properties fail. Height");
    return ret;
}

bool SATECameraControl::setHeight(int nHeight)
{
    bool ret = set(CAP_PROP_HEIGHT, nHeight);
    if (!ret) mLogger->error("Set camera properties fail. Height");
    return ret;
}

std::optional<int> SATECameraControl::width()
{
    auto ret = get<int>(CAP_PROP_WIDTH);
    if (ret == std::nullopt) mLogger->error("Get camera properties fail. Width");
    return ret;
}

bool SATECameraControl::setWidth(int nWidth)
{
    bool ret = set(CAP_PROP_WIDTH, nWidth);
    if (!ret) mLogger->error("Set camera properties fail. Width");
    return ret;
}
