/* Copyright (C) 2019 Brett Dong
 *
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "FLIRCamera.h"
#include <iostream>

FLIRCamera::FLIRCamera()
{
    m_system = Spinnaker::System::GetInstance();
    m_initialized = false;
}

FLIRCamera::~FLIRCamera()
{
    if(m_initialized)
    {
        m_camera->DeInit();
        m_camera = nullptr;
        m_initialized = false;
    }
    m_system->ReleaseInstance();
    std::cout << "FLIR camera deinitialized" << std::endl;
}

bool FLIRCamera::Initialize()
{
    std::cout << "FLIR camera initialization" << std::endl;
    const Spinnaker::LibraryVersion spinnaker_version = m_system->GetLibraryVersion();
    std::cout << "Spinnaker SDK version: " << spinnaker_version.major << "." << spinnaker_version.minor << "." << spinnaker_version.type << "." << spinnaker_version.build << std::endl;
    Spinnaker::CameraList camera_list = m_system->GetCameras();
    if(camera_list.GetSize() == 0)
    {
        std::cout << "FLIR camera not found" << std::endl;
        camera_list.Clear();
        return false;
    }
    std::cout << "Discovered " << camera_list.GetSize() << " FLIR cameras, using #0" << std::endl;
    m_camera = camera_list.GetByIndex(0);
    camera_list.Clear();
    m_camera->Init();
    m_TL_device = &(m_camera->GetTLDeviceNodeMap());
    m_node_map = &(m_camera->GetNodeMap());
    if(m_camera->PixelFormat == NULL || m_camera->PixelFormat.GetAccessMode() != Spinnaker::GenApi::RW)
    {
        m_camera->DeInit();
        std::cout << "Camera pixel format not available" << std::endl;
        return false;
    }
    m_camera->PixelFormat.SetValue(Spinnaker::PixelFormat_RGB8);
    if(m_camera->Width == NULL || m_camera->Width.GetAccessMode() != Spinnaker::GenApi::RW || m_camera->Width.GetInc() == 0 || m_camera->Width.GetMax() == 0)
    {
        m_camera->DeInit();
        std::cout << "Camera width not available" << std::endl;
        return false;
    }
    m_camera->Width.SetValue(640);
    if(m_camera->Height == NULL || m_camera->Height.GetAccessMode() != Spinnaker::GenApi::RW || m_camera->Height.GetInc() == 0 || m_camera->Height.GetMax() == 0)
    {
        m_camera->DeInit();
        std::cout << "Camera width not available" << std::endl;
        return false;
    }
    m_camera->Height.SetValue(480);
    Spinnaker::GenApi::CEnumerationPtr ptrEVAuto = m_node_map->GetNode("pgrExposureCompensationAuto");
    if(!Spinnaker::GenApi::IsAvailable(ptrEVAuto) || !Spinnaker::GenApi::IsWritable(ptrEVAuto))
    {
        m_camera->DeInit();
        std::cout << "EV auto switch is not available" << std::endl;
        return false;
    }
    Spinnaker::GenApi::CEnumEntryPtr ptrEVAutoOff = ptrEVAuto->GetEntryByName("Off");
    if(!Spinnaker::GenApi::IsAvailable(ptrEVAutoOff) || !Spinnaker::GenApi::IsReadable(ptrEVAutoOff))
    {
        m_camera->DeInit();
        std::cout << "EV auto switch off is not available" << std::endl;
        return false;
    }
    ptrEVAuto->SetIntValue(ptrEVAutoOff->GetValue());
    std::cout << "EV automatic switched off" << std::endl;
    Spinnaker::GenApi::CFloatPtr ptrEV = m_node_map->GetNode("pgrExposureCompensation");
    if(!Spinnaker::GenApi::IsAvailable(ptrEV) || !Spinnaker::GenApi::IsWritable(ptrEV))
    {
        m_camera->DeInit();
        std::cout << "EV not available" << std::endl;
        return false;
    }
    ptrEV->SetValue(-1.0f);
    m_initialized = true;
    std::cout << "Camera #0 initialized" << std::endl;
    std::cout << "Resolution: " << m_camera->Width.GetValue() << "x" << m_camera->Height.GetValue() << std::endl;
    std::cout << "Exposure time: " << m_camera->ExposureTime.GetValue() / 1000.0 << " ms" << std::endl;
    std::cout << "Pixel format: " << m_camera->PixelFormat.GetCurrentEntry()->GetSymbolic() << std::endl;
    return true;
}

bool FLIRCamera::BeginAcquisition()
{
    if(!m_initialized)
    {
        std::cout << "Attempt to begin acquisition before initialization" << std::endl;
        return false;
    }
    Spinnaker::GenApi::CEnumerationPtr p_acquisition_mode = m_node_map->GetNode("AcquisitionMode");
    if(!Spinnaker::GenApi::IsAvailable(p_acquisition_mode) || !Spinnaker::GenApi::IsWritable(p_acquisition_mode))
    {
        std::cout << "Acquisition mode unavailable or not writable" << std::endl;
        return false;
    }
    Spinnaker::GenApi::CEnumEntryPtr p_acquisition_mode_continuous = p_acquisition_mode->GetEntryByName("Continuous");
    if(!Spinnaker::GenApi::IsAvailable(p_acquisition_mode_continuous) || !Spinnaker::GenApi::IsReadable(p_acquisition_mode_continuous))
    {
        std::cout << "Continuous acqusition mode entry unavailable or not readable" << std::endl;
        return false;
    }
    const uint64_t acquisition_mode_continuous = p_acquisition_mode_continuous->GetValue();
    p_acquisition_mode->SetIntValue(acquisition_mode_continuous);
    std::cout << "Acquisition mode set to continuous" << std::endl;
    m_camera->BeginAcquisition();
    std::cout << "Acquisition begin" << std::endl;
    return true;
}

bool FLIRCamera::EndAcquisition()
{
    if(!m_initialized)
    {
        std::cout << "Attempt to end acquisition before initialization" << std::endl;
        return false;
    }
    m_camera->EndAcquisition();
    return true;
}

bool FLIRCamera::RetrieveImage(cv::Mat &image)
{
    Spinnaker::ImagePtr p_image = m_camera->GetNextImage();
    if(p_image->IsIncomplete())
    {
        std::cout << "Image incomplete: " << Spinnaker::Image::GetImageStatusDescription(p_image->GetImageStatus()) << std::endl;
        return false;
    }
    const size_t width = p_image->GetWidth();
    const size_t height = p_image->GetHeight();
    Spinnaker::ImagePtr p_converted_image = p_image->Convert(Spinnaker::PixelFormatEnums::PixelFormat_BGR8, Spinnaker::ColorProcessingAlgorithm::HQ_LINEAR);
    cv::Mat intermediate_image = cv::Mat(height, width, CV_8UC3, p_converted_image->GetData(), p_converted_image->GetStride());
    image = intermediate_image.clone();
    p_image->Release();
    return true;
}
