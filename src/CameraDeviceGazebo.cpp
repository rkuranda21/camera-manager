/*
 * This file is part of the Dronecode Camera Manager
 *
 * Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gazebo/common/Image.hh>
#include <gazebo/common/common.hh>
#include <gazebo/gazebo_client.hh>
#include <gazebo/msgs/msgs.hh>

#include <iostream>

#include "CameraDeviceGazebo.h"

const char CameraDeviceGazebo::PARAMETER_CUSTOM_UINT8[] = "custom-uint8";
const int CameraDeviceGazebo::ID_PARAMETER_CUSTOM_UINT8 = 101;
const char CameraDeviceGazebo::PARAMETER_CUSTOM_UINT32[] = "custom-uint32";
const int CameraDeviceGazebo::ID_PARAMETER_CUSTOM_UINT32 = 102;
const char CameraDeviceGazebo::PARAMETER_CUSTOM_INT32[] = "custom-int32";
const int CameraDeviceGazebo::ID_PARAMETER_CUSTOM_INT32 = 104;
const char CameraDeviceGazebo::PARAMETER_CUSTOM_REAL32[] = "custom-real32";
const int CameraDeviceGazebo::ID_PARAMETER_CUSTOM_REAL32 = 105;
const char CameraDeviceGazebo::PARAMETER_CUSTOM_ENUM[] = "custom-enum";
const int CameraDeviceGazebo::ID_PARAMETER_CUSTOM_ENUM = 106;

CameraDeviceGazebo::CameraDeviceGazebo(std::string device)
    : mDeviceId(device)
    , mMode(-1)
    , mWidth(640)
    , mHeight(360)
    , mPixelFormat(CameraParameters::ID_PIXEL_FORMAT_RGB24)
    , mOvText(device)
{
    log_info("%s path:%s", __func__, mDeviceId.c_str());
}

CameraDeviceGazebo::~CameraDeviceGazebo()
{
    stop();
    uninit();
}

std::string CameraDeviceGazebo::getDeviceId()
{
    return mDeviceId;
}

int CameraDeviceGazebo::getInfo(struct CameraInfo &camInfo)
{
    strcpy((char *)camInfo.vendorName, "Gazebo");
    strcpy((char *)camInfo.modelName, "SITL-Camera");
    camInfo.firmware_version = 1;
    camInfo.focal_length = 0;
    camInfo.sensor_size_h = 0;
    camInfo.sensor_size_v = 0;
    camInfo.resolution_h = 0;
    camInfo.resolution_v = 0;
    camInfo.lens_id = 0;
    camInfo.flags = ~0u;
    camInfo.cam_definition_version = 1;
    return 0;
}

bool CameraDeviceGazebo::isGstV4l2Src()
{
    return false;
}

int CameraDeviceGazebo::init(CameraParameters &camParam)
{
    camParam.setParameter(CameraParameters::CAMERA_MODE,
                          (uint32_t)CameraParameters::ID_CAMERA_MODE_VIDEO);
    camParam.setParameterIdType(PARAMETER_CUSTOM_UINT8, ID_PARAMETER_CUSTOM_UINT8,
                                CameraParameters::PARAM_TYPE_UINT8);
    camParam.setParameter(PARAMETER_CUSTOM_UINT8, (uint8_t)50);
    camParam.setParameterIdType(PARAMETER_CUSTOM_UINT32, ID_PARAMETER_CUSTOM_UINT32,
                                CameraParameters::PARAM_TYPE_UINT32);
    camParam.setParameter(PARAMETER_CUSTOM_UINT32, (uint32_t)50);
    camParam.setParameterIdType(PARAMETER_CUSTOM_INT32, ID_PARAMETER_CUSTOM_INT32,
                                CameraParameters::PARAM_TYPE_INT32);
    camParam.setParameter(PARAMETER_CUSTOM_INT32, (int32_t)-10);
    camParam.setParameterIdType(PARAMETER_CUSTOM_REAL32, ID_PARAMETER_CUSTOM_REAL32,
                                CameraParameters::PARAM_TYPE_REAL32);
    camParam.setParameter(PARAMETER_CUSTOM_REAL32, (float)0);
    camParam.setParameterIdType(PARAMETER_CUSTOM_ENUM, ID_PARAMETER_CUSTOM_ENUM,
                                CameraParameters::PARAM_TYPE_UINT32);
    camParam.setParameter(PARAMETER_CUSTOM_ENUM, (uint32_t)0);

    return 0;
}

int CameraDeviceGazebo::uninit()
{
    return 0;
}

int CameraDeviceGazebo::start()
{
    // Load Gazebo
    gazebo::client::setup();

    // Create our node for communication
    mNode.reset(new gazebo::transport::Node());
    mNode->Init();

    // TODO:: Get the number of buffers that gazebo camera has
    // TODO:: Initialize size of mFrameBuffer based on that

    // Listen to Gazebo <device> topic
    mSub = mNode->Subscribe(mDeviceId, &CameraDeviceGazebo::cbOnImages, this);
    mState = STATE_RUN;
    return 0;
}

int CameraDeviceGazebo::stop()
{
    std::lock_guard<std::mutex> locker(mLock);
    mState = STATE_INIT;
    // Make sure to shut everything down.
    gazebo::client::shutdown();
    return 0;
}

std::vector<uint8_t> CameraDeviceGazebo::read()
{
    std::lock_guard<std::mutex> locker(mLock);
    if (mState != STATE_RUN)
        return {};
    return mFrameBuffer;
}

int CameraDeviceGazebo::getSize(uint32_t &width, uint32_t &height)
{
    width = mWidth;
    height = mHeight;

    return 0;
}

int CameraDeviceGazebo::getPixelFormat(uint32_t &format)
{
    format = mPixelFormat;

    return 0;
}

int CameraDeviceGazebo::setMode(uint32_t mode)
{
    mMode = mode;
    return 0;
}

int CameraDeviceGazebo::getMode()
{
    return mMode;
}

int CameraDeviceGazebo::resetParams(CameraParameters &camParam)
{
    int ret = 0;

    // TODO :: The default params need to be stored in DS during init
    camParam.setParameter(CameraParameters::CAMERA_MODE,
                          (uint32_t)CameraParameters::ID_CAMERA_MODE_VIDEO);
    camParam.setParameter(PARAMETER_CUSTOM_UINT8, (uint8_t)50);
    camParam.setParameter(PARAMETER_CUSTOM_UINT32, (uint32_t)50);
    camParam.setParameter(PARAMETER_CUSTOM_INT32, (int32_t)-10);
    camParam.setParameter(PARAMETER_CUSTOM_REAL32, (float)0);
    camParam.setParameter(PARAMETER_CUSTOM_ENUM, (uint32_t)0);

    return ret;
}

int CameraDeviceGazebo::setParam(CameraParameters &camParam, std::string param,
                                 const char *param_value, size_t value_size, int param_type)

{
    int ret = 0;
    std::string ovValue;
    CameraParameters::cam_param_union_t u;
    memcpy(&u.param_float, param_value, sizeof(float));
    int paramId = camParam.getParameterID(param);
    switch (paramId) {
    case ID_PARAMETER_CUSTOM_UINT8:
        log_info("Parameter: %s Value: %d", param.c_str(), u.param_uint8);
        camParam.setParameter(param, u.param_uint8);
        ovValue = std::to_string(u.param_uint8);
        break;
    case ID_PARAMETER_CUSTOM_UINT32:
        log_info("Parameter: %s Value: %d", param.c_str(), u.param_uint32);
        camParam.setParameter(param, u.param_uint32);
        ovValue = std::to_string(u.param_uint32);
        break;
    case ID_PARAMETER_CUSTOM_INT32:
        log_info("Parameter: %s Value: %d", param.c_str(), u.param_int32);
        camParam.setParameter(param, u.param_int32);
        ovValue = std::to_string(u.param_int32);
        break;
    case ID_PARAMETER_CUSTOM_REAL32:
        log_info("Parameter: %s Value: %f", param.c_str(), u.param_float);
        camParam.setParameter(param, u.param_float);
        ovValue = std::to_string(u.param_float);
        break;
    case ID_PARAMETER_CUSTOM_ENUM:
        log_info("Parameter: %s Value: %d", param.c_str(), u.param_uint32);
        camParam.setParameter(param, u.param_uint32);
        ovValue = std::to_string(u.param_uint32);
        break;
    default:
        ret = -ENOTSUP;
        break;
    }

    if (!ret)
        setOverlayText(param + " = " + ovValue);

    return ret;
}

int CameraDeviceGazebo::setOverlayText(std::string text)
{
    log_debug("%s", __func__);

    mOvText = text;
    return 0;
}

std::string CameraDeviceGazebo::getOverlayText()
{
    return mOvText;
}

void CameraDeviceGazebo::cbOnImages(ConstImagesStampedPtr &_msg)
{
    std::lock_guard<std::mutex> locker(mLock);

    log_debug("Image Count: %d", _msg->image_size());
    for (int i = 0; i < _msg->image_size(); ++i) {
        getImage(_msg->image(i));
    }
}

int CameraDeviceGazebo::getImage(const gazebo::msgs::Image &_msg)
{
    int success = 0;
    int failure = 1;

    if (_msg.width() == 0 || _msg.height() == 0)
        return failure;

    // log_debug("Width: %u", _msg.width());
    // log_debug("Height: %u", _msg.height());
    mWidth = _msg.width();
    mHeight = _msg.height();

    switch (_msg.pixel_format()) {
    case gazebo::common::Image::L_INT8:
    case gazebo::common::Image::L_INT16: {
        log_error("Pixel Format Mono :: Unsupported");
        return failure;
    }
    case gazebo::common::Image::RGB_INT8:
    case gazebo::common::Image::RGBA_INT8: {
        mPixelFormat = CameraParameters::ID_PIXEL_FORMAT_RGB24;
        break;
    }
    default: {
        log_error("Pixel Format %d :: Unsupported", _msg.pixel_format());
        return failure;
    }
    }

    // copy image data to frame buffer
    // log_debug("Image Size: %lu Format:%d", _msg.data().size(), pixFormat);
    const char *buffer = (const char *)_msg.data().c_str();
    uint buffer_size = _msg.data().size();
    mFrameBuffer = std::vector<uint8_t>(buffer, buffer + buffer_size);

#if 0
    std::ofstream fout("imgframe.rgb", std::ios::binary);
    fout.write(reinterpret_cast<char*>(&mFrameBuffer[0]), mFrameBuffer.size() * sizeof(mFrameBuffer[0]));
    fout.close();
    std::cout<<"\nsaved";
#endif

    return success;
}
