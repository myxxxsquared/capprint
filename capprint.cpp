#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv/cv.hpp>
#include <rapidjson/document.h>

#include <string>
#include <fstream>
#include <cstdio>
#include <algorithm>

class ImageOutputer
{
public:
    int pageheight, pagewidth;
    int curloc;
    cv::Mat curmat;
    int curindex;

    std::string imgfile;

    ImageOutputer(rapidjson::Document& settings)
    {
        this->pageheight = settings["pageheight"].GetInt();
        this->pagewidth = settings["videoright"].GetInt() - settings["videoleft"].GetInt();
        curmat = cv::Mat(pageheight, pagewidth, CV_8UC3);
        this->imgfile = settings["outputstring"].GetString();
        this->curindex = settings["firstindex"].GetInt();
        curloc = 0;
    }

    void PutImage(const cv::Mat& mat, int begin, int end)
    {
        if(begin == end)
            return;

        if (end - begin > pageheight - curloc)
        {
            ClearPage();
            printf("warning: cut-----------------\n");
        }

        int curbegin = curloc;
        int cpyheight = std::min(end - begin, pageheight - curloc);

        mat(cv::Rect(0, begin, pagewidth, cpyheight))
            .copyTo(
                curmat(cv::Rect(0, curbegin, pagewidth, cpyheight))
            );
        curloc += cpyheight;
    }

    void ClearPage()
    {
        cv::Vec3b v{ 255,255,255 };
        for (int i = curloc; i < pageheight; ++i)
            for (int j = 0; j < pagewidth; ++j)
                curmat.at<cv::Vec3b>(i, j) = v;


        char filename[1024];
        sprintf(filename, this->imgfile.c_str(), curindex);
        cv::imwrite(filename, curmat);
        printf("clearpage : %s\n", filename);
        curindex++;
        curloc = 0;
    }
};

class VideoLoader
{
public:

    VideoLoader(rapidjson::Document& settings,ImageOutputer& out, cv::VideoCapture& capture)
        :output(out),
        cap(capture)
    {
        bgwidth = settings["bgwidth"].GetInt();
        smallestdelta = settings["smallestdelta"].GetInt();
        bgcolorerror = settings["bgcolorerror"].GetInt();
        bgcolorr = settings["bgcolor"].GetArray()[0].GetInt();
        bgcolorg = settings["bgcolor"].GetArray()[1].GetInt();
        bgcolorb = settings["bgcolor"].GetArray()[2].GetInt();

        edgeignore = settings["edgeignore"].GetInt();

        const int imgwidth = settings["videoright"].GetInt() - settings["videoleft"].GetInt();
        const int imgheight = settings["videobottom"].GetInt() - settings["videotop"].GetInt();

        rectroi = cv::Rect{
            settings["videoleft"].GetInt(),
            settings["videotop"].GetInt(),
            imgwidth,
            imgheight
        };

        firstimgae = true;
        totalmove = 0;
        curframe = 0;
    }

    //检测分页常数
    int bgwidth;
    int bgcolorr, bgcolorg, bgcolorb;
    int bgcolorerror;
    int smallestdelta;
    cv::Rect rectroi;
    int edgeignore;

    //输出器
    ImageOutputer& output;
    
    //输入器
    cv::VideoCapture& cap;

    //处理循环使用
    cv::Mat lastcmpframe;
    cv::Mat lastframe;
    bool firstimgae;
    int totalmove;
    int curframe;

    bool processoenframe()
    {
        cv::Mat frame;

        printf("frame: %d\n", curframe++);

        if (cap.read(frame))
        {
            frame = frame(rectroi);

            cv::Mat cmpframe;
            cv::cvtColor(frame, cmpframe, CV_BGR2GRAY);
            cmpframe.convertTo(cmpframe, CV_32FC1);

            if (firstimgae)
            {
                firstimgae = false;
                process(frame, 0, frame.rows);
            }
            else
            {
                double moved = cv::phaseCorrelate(cmpframe, lastcmpframe).y;
                int move = moved >= 0 ? (int)(moved + 0.5) : (int)(moved - 0.5);
                totalmove += move;

                if (totalmove > smallestdelta)
                {
                    process(frame, frame.rows - totalmove, frame.rows);
                    totalmove = 0;
                }
            }

            lastframe = frame;
            lastcmpframe = cmpframe;
        }
        else
        {
            process(lastframe, lastframe.rows - totalmove, lastframe.rows);
            output.ClearPage();

            return false;
        }
        return true;
    }

    void process(const cv::Mat& img, int begin, int end)
    {
        printf("process: %d, %d \n", begin, end);

        int newpage = detectnewpage(img, begin, end);
        if (newpage == -1)
        {
            output.PutImage(img, begin, end);
        }
        else
        {
            output.PutImage(img, begin, newpage);
            output.ClearPage();
            output.PutImage(img, newpage + bgwidth, end);
        }
    }

    int detectnewpage(const cv::Mat& img, int begin, int end)
    {
        const int looph = end - bgwidth;
        const int loopw = img.cols - edgeignore;
        bool fail;

        for (int h = begin; h <= looph; ++h)
        {
            fail = false;

            for (int dh = 0; dh < bgwidth && !fail; ++dh)
            {
                for (int w = edgeignore; w < loopw && !fail; ++w)
                {
                    decltype(auto) pt = img.at<cv::Vec3b>(h + dh, w);
                    if (std::abs(pt[0] - bgcolorb) > bgcolorerror)
                        fail = true;
                    if (std::abs(pt[1] - bgcolorg) > bgcolorerror)
                        fail = true;
                    if (std::abs(pt[2] - bgcolorr) > bgcolorerror)
                        fail = true;
                }
            }

            if (!fail)
                return h;
        }

        return -1;
    }

};

int main()
{
    cv::VideoCapture cap;

    rapidjson::Document settings;

    {
        std::ifstream ifs{ "capprint.settings.json" };
        std::string str{ std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>{} };
        settings.Parse(str.c_str());
    }

    cap.open(settings["videofile"].GetString());
    cv::Mat frame, realframe, lastrealframe, cmplast, cmpcur;

    ImageOutputer output{ settings };
    VideoLoader loader{ settings, output, cap };

    

    while (loader.processoenframe());

    getchar();
    
    return 0;
}