// ============================================================================
//  main.cpp  ——  程序入口，只干"流程"这一件事
// ----------------------------------------------------------------------------
//  打开视频 → 一帧一帧读 → 调用 drawBox 画框 → 写进输出视频 → 显示
//  算法都在 detect.cpp 里，这里一行算法都没有。
// ============================================================================
#include <opencv2/opencv.hpp>
#include <iostream>   // std::cout 输出文字
#include <string>     // std::string 存字符串（一串字符）
#include "detect.h"
int main(int argc, char** argv)
{
    // ------------------------------------------------------------------
    //  1. 视频路径
    // ------------------------------------------------------------------
    //  ★ argc / argv：命令行参数。运行时写 ./armor xxx.avi，
    //    argv[1] 就是 "xxx.avi"，argc 是参数的个数。
    //    不写的话就用默认路径 data/armor.avi。
    std::string name = "data/armor.avi";
    if (argc > 1)
    {
        name = argv[1];
    }
    // ------------------------------------------------------------------
    //  2. 打开视频
    // ------------------------------------------------------------------
    //  ★ VideoCapture = 读视频。isOpened() 返回 true 表示打开成功了。
    cv::VideoCapture cap(name);
    if (!cap.isOpened())
    {
        // ★ std::cout = 往终端打印文字；std::endl = 换行。
        std::cout << "打不开视频：" << name << std::endl;
        std::cout << "检查一下路径对不对，或者视频有没有放到 data/ 里" << std::endl;
        return -1;
    }
    // 先读一帧出来。一是确认视频里真有画面，二是拿它的尺寸去建输出视频。
    cv::Mat frame;
    if (!cap.read(frame))
    {
        std::cout << "视频里读不到画面" << std::endl;
        return -1;
    }
    // ------------------------------------------------------------------
    //  3. 准备输出视频（只输出这一个视频）
    // ------------------------------------------------------------------
    //  ★ VideoWriter = 写视频。四个参数：
    //      文件名、编码格式、帧率、画面尺寸
    //    'M','J','P','G' 是一种编码方式，兼容性好，一般都能写出来。
    //    frame.size() 就是这一帧的宽高，这样不用手写 720x540。
    cv::VideoWriter out("out.avi",
                        cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                        25,
                        frame.size());
    if (!out.isOpened())
    {
        std::cout << "写不了 out.avi，检查一下当前目录能不能写文件" << std::endl;
    }
    // 窗口名字叫 "armor"，WINDOW_NORMAL 表示可以用鼠标拉大拉小。
    cv::namedWindow("armor", cv::WINDOW_NORMAL);
    std::cout << "空格 = 暂停 / 继续，ESC = 退出" << std::endl;
    // ------------------------------------------------------------------
    //  4. 一帧一帧处理
    // ------------------------------------------------------------------
    //  do { ... } while (cap.read(frame));
    //  意思是"先做一遍，然后每轮结尾再读下一帧，读到没有为止"。
    //  用 do-while 是因为第一帧上面已经读出来了，不用重复读。
    do
    {
        drawBox(frame);        // 在 frame 上画框（如果找到了的话）
        out.write(frame);      // write = 把这一帧写进 out.avi
        cv::imshow("armor", frame);
        // ★ waitKey 必须调用，窗口才会刷新。30 表示最多等 30 毫秒。
        //   返回值是你按下的键的编号：ESC 是 27，空格是 32。
        int key = cv::waitKey(30);
        if (key == 27)         // ESC：退出
        {
            break;
        }
        if (key == 32)         // 空格：暂停
        {
            std::cout << "暂停了，再按空格继续，按 ESC 退出" << std::endl;
            // 暂停时不再往下读帧，所以画面就停在当前这一帧。
            // 一直等，直到再按一次空格（继续）或者 ESC（直接退出）。
            bool paused = true;
            while (paused)
            {
                int k = cv::waitKey(30);
                if (k == 32)
                {
                    paused = false;
                }
                else if (k == 27)
                {
                    out.release();   // 先把视频文件收尾，再退出
                    return 0;
                }
            }
        }
    } while (cap.read(frame));   // ★ read = 读下一帧；读到返回 true，读完了返回 false
    return 0;
}