#include <bits/stdc++.h>
#include <opencv2/opencv.hpp>
#ifdef FLIR
#include "FLIRCamera.h"
#endif
#include <signal.h>
#ifdef TX2
#include "opencv2/core/cuda.hpp"
#include "opencv2/cudaimgproc.hpp"
#endif

using namespace std;
using namespace cv;

bool condition_mask[16];
atomic<bool> running;

void sig_handler(int sig)
{
    running = false;
}

class Profiler
{
    public:
        void EnterSection() { clocks.clear(); clocks.push_back(clock()); }
        void Commit() { clocks.push_back(clock()); }
        string DumpRecords()
        {
            ostringstream oss;
            for(int i = 1; i < clocks.size(); i++)
            {
                oss << (clocks[i] - clocks[i-1]) * 1000.0 / CLOCKS_PER_SEC << " | ";
            }
            return oss.str();
        }
    private:
        vector<clock_t> clocks;
} profiler;

struct SingleLightbar
{
    Point2f center;
    float angle, breadth, length;
    SingleLightbar() : center(Point2f(0.0f, 0.0f)), angle(0.0f), breadth(0.0f), length(0.0f) {}
    SingleLightbar(const RotatedRect &rect)
    {
        center = rect.center;
        if(rect.size.width < rect.size.height)
        {
            breadth = rect.size.width;
            length = rect.size.height;
            angle = 90.0f - rect.angle;
        }
        else
        {
            breadth = rect.size.height;
            length = rect.size.width;
            angle = -rect.angle;
        }
    }
    // 1 2
    // 0 3
    void points(Point2f vertex[]) const
    {
        const float theta = angle * CV_PI / 180;
        const float a = cos(theta) * 0.5f, b = sin(theta) * 0.5f;
        vertex[0].x = center.x - a * length - b * breadth;
        vertex[0].y = center.y + b * length - a * breadth;
        vertex[1].x = center.x + a * length - b * breadth;
        vertex[1].y = center.y - b * length - a * breadth;
        vertex[2].x = 2 * center.x - vertex[0].x;
        vertex[2].y = 2 * center.y - vertex[0].y;
        vertex[3].x = 2 * center.x - vertex[1].x;
        vertex[3].y = 2 * center.y - vertex[1].y;
    }
    void expansion()
    {
        breadth *= 3.0f;
        length *= 1.5f;
    }
};

struct ArmorPlate
{
    Point2f vertex[4];
    double confidence;
};

void DrawSingleLightbar(cv::Mat &img, const SingleLightbar &light)
{
    static Point2f vertex[4];
    static char text[20];
    sprintf(text, "%.2f DEG | %.2f PX2", light.angle, light.breadth * light.length);
    putText(img, text, light.center, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2, 8, 0);
    light.points(vertex);
    for(int i = 0; i < 4; i++) line(img, vertex[i], vertex[(i+1)%4], Scalar(0, 255, 0), 2);
}

typedef Point3_<uint8_t> Pixel;
/*
ofstream fout("data");
void record(const Pixel &p)
{
    static mutex mtx;
    mtx.lock();
    fout << static_cast<int>(p.x) << ' ' << static_cast<int>(p.y) << ' ' << static_cast<int>(p.z) << endl;
    mtx.unlock();
}

void DumpPixels(Mat &img, const RotatedRect &rect)
{
    static Point2f vertex[4];
    rect.points(vertex);
    vector<Point2f> contour(vertex, vertex + 4);
    img.forEach<Pixel>([contour](Pixel &p, const int pos[]) -> void {
        if(pointPolygonTest(contour, Point(pos[1], pos[0]), false) >= 0)
            record(p);
    });
}*/

/*
 * Lab color space approach
inline bool EvaluateCondition(const Pixel &p)
{
    return !((p.x >= 250 || p.y >= 250 || p.z >= 250) && abs(p.x - p.y) + abs(p.x - p.z) + abs(p.y - p.z) < 15);
}

float EvaluatePixels(Mat &bgr, Mat &lab, const Mat &mask)
{
    int R = 0, B = 0;
    for(int y = 0; y < bgr.rows; y++)
    {
        for(int x = 0; x < bgr.cols; x++)
        {
            if(mask.at<uint8_t>(y, x) && EvaluateCondition(bgr.at<Pixel>(y, x)))
            {
                int a = lab.at<Pixel>(y, x).y;
                int b = lab.at<Pixel>(y, x).z;
                if(a + b > 255)
                    ++R;
                else
                    ++B;
            }
        }
    }
    return (R - B) * 1.0f / (R + B);
}
*/

float EvaluatePixels(cv::Mat img, const cv::Mat &mask)
{
    int Rcount = 0, Bcount = 0;
    for(int i = 0; i < img.rows; i++)
    {
        for(int j = 0; j < img.cols; j++)
        {
            Vec3b &p = img.at<Vec3b>(i, j);
            uint8_t R = p[2], G = p[1], B = p[0];
            float RGB = R + G + B;
            float r = R / RGB, g = G / RGB, b = B / RGB;
            if(b > 0.75f)
            {
                ++Bcount;
            }
            else if(r > 0.75f)
            {
                ++Rcount;
            }
        }
    }
    return (Rcount - Bcount) * 1.0f / (Rcount + Bcount);
}

int main(int argc, char *argv[])
{
    bool show_gui = false;
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--gui") == 0)
        {
            show_gui = true;
        }
        else if(strcmp(argv[i], "--mask") == 0 && i+1 < argc)
        {
            condition_mask[atoi(argv[i+1])] = true;
            ++i;
        }
    }
    Mat img;
#ifdef FLIR
    FLIRCamera camera;
    if(!camera.Initialize()) return 1;
    if(!camera.BeginAcquisition()) return 1;
    camera.RetrieveImage(img);
#else
    VideoCapture cap(0);
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    if(!cap.read(img))
    {
        cout << "Cannot read image from camera" << endl;
        return 1;
    }
#endif
    int frame_count = 0;
    clock_t last_frame_count = clock();
    signal(SIGINT, sig_handler);
    running = true;
    Mat hsv(img.rows, img.cols, CV_8UC3);
#ifdef FLIR
    while(camera.RetrieveImage(img) && running)
#else
    while(cap.read(img) && running)
#endif
    {
        ++frame_count;
        if(clock() - last_frame_count > CLOCKS_PER_SEC)
        {
            cout << frame_count << endl;
            frame_count = 0;
            last_frame_count = clock();
        }
        profiler.EnterSection();
        clock_t Tstart = clock();
        //Mat hsv = img.clone(); //, lab = img.clone();
#ifdef TX2
        cuda::cvtColor(img, hsv, COLOR_BGR2HSV);
#else
        cvtColor(img, hsv, COLOR_BGR2HSV);
        //cvtColor(lab, lab, COLOR_BGR2Lab);
#endif
        inRange(hsv, Scalar(0, 0, 250), Scalar(255, 255, 255), hsv);
        if(show_gui) imshow("Threshold - HSV", hsv);
        profiler.Commit();
        vector<vector<Point>> contours;
        findContours(hsv, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        profiler.Commit();
        vector<SingleLightbar> lights;
        vector<ArmorPlate> armors;
        for(auto &ct : contours)
        {
            if(!condition_mask[0] && (contourArea(ct) < 10 || contourArea(ct) > 2000)) continue;
            SingleLightbar light(minAreaRect(ct));
            if(!condition_mask[1] && abs(light.angle - 90.0f) > 30.f) continue;
            lights.push_back(light);
            //DrawSingleLightbar(img, light);
        }
        profiler.Commit();
        for(int i = 0; i < lights.size(); i++)
        {
            for(int j = i+1; j < lights.size(); j++)
            {
                if(!condition_mask[2] && abs(lights[i].angle - lights[j].angle) > 15.0f) continue;
                float average_angle = (lights[i].angle + lights[j].angle) / 2;
                SingleLightbar left, right;
                if(lights[i].center.x < lights[j].center.x)
                {
                    left = lights[i];
                    right = lights[j];
                }
                else
                {
                    left = lights[j];
                    right = lights[i];
                }
                float dx = right.center.x - left.center.x;
                float dy = right.center.y - left.center.y;
                float slope = (abs(dx) < 2.0f ? -90.0f : atan(-dy/dx) * 180 / CV_PI);
                if(!condition_mask[3] && abs(average_angle - slope - 90) > 30.0f) continue;
                Point2f v_left[4], v_right[4];
                left.points(v_left);
                right.points(v_right);
                if(!condition_mask[4])
                {
                    if(min(v_left[0].y, v_left[3].y) < max(v_right[1].y, v_right[2].y)) continue;
                    if(max(v_left[1].y, v_left[2].y) > min(v_right[0].y, v_right[3].y)) continue;
                }
                float distance = sqrt(dx*dx + dy*dy);
                if(!condition_mask[5] && distance > 4*min(left.length, right.length)) continue;
                ArmorPlate candidate;
                candidate.vertex[0] = v_left[3];
                candidate.vertex[1] = v_left[2];
                candidate.vertex[2] = v_right[1];
                candidate.vertex[3] = v_right[0];

                SingleLightbar eleft = left, eright = right;
                eleft.expansion(); eright.expansion();
                Point2f veleft[4], veright[4];
                eleft.points(veleft);
                eright.points(veright);
                Mat mask = Mat::zeros(img.size(), CV_8UC1);
                vector<Point2f> contour1(veleft, veleft + 4);
                vector<Point2f> contour2(veright, veright + 4);
                Point velefti[4], verighti[4];
                for(int i = 0; i < 4; i++) velefti[i] = veleft[i];
                for(int i = 0; i < 4; i++) verighti[i] = veright[i];
                fillConvexPoly(mask, velefti, 4, Scalar(255));
                fillConvexPoly(mask, verighti, 4, Scalar(255));
                //DumpPixels(img, eleft);
                /*{
                    ostringstream oss;
                    oss << EvaluatePixels(img, lab, eleft);
                    putText(img, oss.str().c_str(), v_left[2], FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2, 8, 0);
                }*/
                //DumpPixels(img, eright);
                if(!condition_mask[6]) candidate.confidence = EvaluatePixels(img, mask);
                    //candidate.confidence = (EvaluatePixels(img, lab, eleft) + EvaluatePixels(img, lab, eright)) / 2.0f;
                armors.push_back(candidate);
            }
        }
        profiler.Commit();
        clock_t Tend = clock();
        for(const ArmorPlate &armor : armors)
        {
            for(int i = 0; i < 4; i++)
            {
                ostringstream oss;
                oss << fixed << setprecision(3) << armor.confidence;
                putText(img, oss.str().c_str(), armor.vertex[2], FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2, 8, 0);
                line(img, armor.vertex[i], armor.vertex[(i+1)%4], Scalar(0, 0, 255), 2);
                /*
                if(!condition_mask[6] && armor.confidence > 0.7f)
                    line(img, armor.vertex[i], armor.vertex[(i+1)%4], Scalar(0, 0, 255), 2);
                else if(condition_mask[6] || armor.confidence < -0.7f)
                    line(img, armor.vertex[i], armor.vertex[(i+1)%4], Scalar(255, 0, 0), 2);
                */
            }
        }
        if(show_gui)
        {
            ostringstream oss;
            oss << "TIME " << fixed << setprecision(1) << (Tend - Tstart) * 1000.0 / CLOCKS_PER_SEC << " | MASK ";
            for(int i = 0; i < 10; i++)
                if(condition_mask[i]) oss << i << ' ';
            putText(img, oss.str().c_str(), Point(10, 40), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2, 8, 0);
            putText(img, profiler.DumpRecords().c_str(), Point(10, 70), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2, 8, 0);
        }
        if(show_gui)
        {
            imshow("Camera", img);
            char ch = waitKey(1);
            if(ch == 27) break;
            else if('0' <= ch && ch <= '9') condition_mask[ch-'0'] = !condition_mask[ch-'0'];
        }
    }
#ifdef FLIR
    camera.EndAcquisition();
#endif
    //fout.close();
    return 0;
}
