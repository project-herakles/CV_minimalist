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

#pragma once
#ifndef FLIRCAMERA_H_
#define FLIRCAMERA_H_

#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>
#include <opencv2/highgui.hpp>

class FLIRCamera
{
    private:
        bool m_initialized;
        Spinnaker::SystemPtr m_system;
        Spinnaker::CameraPtr m_camera;
        Spinnaker::GenApi::INodeMap *m_TL_device;
        Spinnaker::GenApi::INodeMap *m_node_map;
    public:
        FLIRCamera();
        ~FLIRCamera();
        bool Initialize();
        bool BeginAcquisition();
        bool EndAcquisition();
        bool RetrieveImage(cv::Mat &image);
};

#endif // FLIRCAMERA_H_
