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
#include <bits/stdc++.h>
using namespace std;
int main()
{
    FLIRCamera camera;
    if(!camera.Initialize()) return 1;
    if(!camera.BeginAcquisition()) return 1;
    cv::Mat img;
    int frame_count = 0;
    clock_t last_frame_count = clock();
    while(camera.RetrieveImage(img))
    {
        if(clock() - last_frame_count > CLOCKS_PER_SEC)
        {
            cout << frame_count << endl;
            last_frame_count = clock();
            frame_count = 0;
        }
        ++frame_count;
        cv::namedWindow("Image Acquisition");
        cv::imshow("Image Acquisition", img);
        cv::moveWindow("Image Acquisition", 0, 0);
        if(cv::waitKey(1) == 27) break;
    }
    camera.EndAcquisition();
    return 0;
}
