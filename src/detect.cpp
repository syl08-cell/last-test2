// ============================================================================
//  detect.cpp  ——  真正干活的地方
// ----------------------------------------------------------------------------
//  一共 4 步：转灰度 → 二值化 → 找轮廓 → 配对画框。
//  注释里带 ★ 的地方，是你这次新遇到的知识，就地解释，不用背。
// ============================================================================
#include "detect.h"
#include "config.h"
#include <algorithm>   // std::min / std::max
#include <cstdlib>     // std::abs（求绝对值）
// ============================================================================
//  小函数 1：这个矩形像不像一根灯条？
// ----------------------------------------------------------------------------
//  cv::Rect 是 OpenCV 的"矩形"。它有 4 个成员，直接当变量用：
//      r.x       左上角的 x（横坐标，向右变大）
//      r.y       左上角的 y（纵坐标，向下变大）
//      r.width   宽
//      r.height  高
//  注意：图像坐标的 y 是"向下"的，所以 y 越小越靠上。
// ============================================================================
bool isBar(cv::Rect r)
{
    // 太矮或太高，不要
    if (r.height < MIN_H || r.height > MAX_H)
    {
        return false;
    }
    // 太窄或太宽，不要
    if (r.width < MIN_W || r.width > MAX_W)
    {
        return false;
    }
    // 高 ÷ 宽 不够大，说明不够"细长"，不要。
    // 写成乘法是为了不做除法（除法会有小数，麻烦）。
    if (r.height < r.width * MIN_RATIO)
    {
        return false;
    }
    return true;
}
// ============================================================================
//  小函数 2：把地面倒影删掉
// ----------------------------------------------------------------------------
//  思路只有一句话：
//    如果一根矩形的正下方（左右位置差不多）还有一根更高的矩形，
//    那它多半是倒影，删掉。
//
//  ★ 新知识：std::vector（读作"维克特"）= 可以自动变长的数组。
//    你现在不用会它，先把它当成一个"盒子"：
//        bars.push_back(东西)   往盒子里放一个
//        bars.size()            盒子里有几个
//        bars[i]                第 i 个（从 0 开始数）
//    普通数组 int a[10] 长度写死，vector 想放多少放多少，所以 OpenCV 都用它。
//
//  ★ 参数里的 & 是"引用"：这里直接改调用者传进来的那个盒子，不复制。
// ============================================================================
void dropShadow(std::vector<cv::Rect>& bars)
{
    std::vector<cv::Rect> keep;   // 新的盒子，装"不是倒影"的
    for (int i = 0; i < (int)bars.size(); i++)
    {
        bool isShadow = false;    // bool = 真/假，只有 true 和 false 两个值
        for (int j = 0; j < (int)bars.size(); j++)
        {
            if (i == j)
            {
                continue;         // 不跟自己比
            }
            // 左右位置差不多？（比较左上角 x）
            // ★ std::abs = 求绝对值：差多少就是多少，不管谁大谁小。
            bool sameX = std::abs(bars[i].x - bars[j].x) < SHADOW_X_DIFF;
            // 自己明显在别人下面？（差半根灯条的高度以上才算"下面"）
            bool isBelow = bars[i].y > bars[j].y + bars[j].height / 2;
            // 自己还比上面那根矮？
            bool isShorter = bars[i].height < bars[j].height;
            if (sameX && isBelow && isShorter)
            {
                isShadow = true;
                break;            // 已经确定是倒影了，不用再看别人
            }
        }
        if (isShadow == false)
        {
            keep.push_back(bars[i]);   // 不是倒影，留下
        }
    }
    bars = keep;   // 用"留下来的"替换掉原来那一盒
}
// ============================================================================
//  主函数：找装甲板，画一个框
// ============================================================================
void drawBox(cv::Mat& frame)
{
    // ------------------------------------------------------------------
    //  第 1 步：彩色图 → 灰度图
    // ------------------------------------------------------------------
    //  ★ cv::Mat 是 OpenCV 里"一张图"的类型，你可以当成一个变量。
    //  ★ cvtColor = 转换颜色格式。COLOR_BGR2GRAY = BGR转灰度。
    //    OpenCV 的彩色图是 BGR 顺序（蓝绿红），不是 RGB，记一下就行。
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    // ------------------------------------------------------------------
    //  第 2 步：二值化（threshold = 阈值）
    // ------------------------------------------------------------------
    //  规则很简单：灰度 > GRAY_MIN 的像素变白(255)，其余变黑(0)。
    //  得到的 mask 是一张黑白图，白色就是"亮的地方"。
    //  ★ mask 叫"掩码"，就是一张黑白图，白色表示"这里我要"。
    cv::Mat mask;
    cv::threshold(gray, mask, GRAY_MIN, 255, cv::THRESH_BINARY);
    // ------------------------------------------------------------------
    //  第 3 步：找轮廓
    // ------------------------------------------------------------------
    //  ★ 轮廓（contour）= 白色区域的外边界，是一串点围成的一圈。
    //  ★ findContours 的输出类型长得吓人：
    //        std::vector< std::vector<cv::Point> >
    //    从里往外读：
    //        cv::Point                  一个像素点
    //        std::vector<cv::Point>     一串点 = 一个轮廓
    //        std::vector< 上面那个 >    一堆轮廓 = 图里所有白块
    //    所以 contours[i] 是第 i 个白块的边界。
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    // ------------------------------------------------------------------
    //  第 4 步：每个轮廓变成一个矩形，筛出像灯条的
    // ------------------------------------------------------------------
    //  ★ boundingRect = 求"外接矩形"：用一个正着的矩形把这一圈点框住。
    //  ★ size() 返回的类型是 size_t（无符号整数），直接和 int 比较会有警告，
    //    所以写成 (int)contours.size() 转一下。
    std::vector<cv::Rect> bars;   // 装所有"像灯条"的矩形
    for (int i = 0; i < (int)contours.size(); i++)
    {
        cv::Rect r = cv::boundingRect(contours[i]);
        if (isBar(r))
        {
            bars.push_back(r);
        }
    }
    // 把倒影删掉，剩下的才是真灯条
    dropShadow(bars);
    // ------------------------------------------------------------------
    //  第 5 步：两根灯条配对，画框
    // ------------------------------------------------------------------
    //  两层 for 把所有"两根的组合"都试一遍：
    //    i 从 0 开始，j 从 i+1 开始，这样每对只试一次，也不会自己配自己。
    for (int i = 0; i < (int)bars.size(); i++)
    {
        for (int j = i + 1; j < (int)bars.size(); j++)
        {
            // 规则 1：两根差不多高
            if (std::abs(bars[i].height - bars[j].height) > MAX_H_DIFF)
            {
                continue;   // continue = 这一对不行，直接试下一对
            }
            // 规则 2：两根差不多在同一水平线上
            if (std::abs(bars[i].y - bars[j].y) > MAX_Y_DIFF)
            {
                continue;
            }
            // 规则 3：水平间距在合理范围
            int gap = std::abs(bars[i].x - bars[j].x);
            if (gap < MIN_GAP || gap > MAX_GAP)
            {
                continue;
            }
            // 走到这里说明这两根配上了。先分清楚哪根在左、哪根在右。
            // ★ 交换两个变量要借助第三个变量，就像换两杯水要一个空杯子。
            cv::Rect L = bars[i];
            cv::Rect R = bars[j];
            if (L.x > R.x)
            {
                cv::Rect tmp = L;
                L = R;
                R = tmp;
            }
            // 用这两根灯条算出一个大矩形：
            //   左边 = 左灯条的 x
            //   右边 = 右灯条的 x + 右灯条的宽
            //   上边 = 两根里更靠上的那个 y
            //   下边 = 两根里更靠下的那个底边
            // ★ std::min = 两个数里更小的那个；std::max = 更大的那个。
            int x1 = L.x;
            int x2 = R.x + R.width;
            int y1 = std::min(L.y, R.y);
            int y2 = std::max(L.y + L.height, R.y + R.height);
            // ★ cv::Rect 的写法是 Rect(左上角x, 左上角y, 宽, 高)，注意第 3、4 个是宽和高，不是右下角。
            cv::Rect box(x1, y1, x2 - x1, y2 - y1);
            // ★ rectangle = 画矩形。(0,255,0) 是绿色（BGR），2 是线宽。
            cv::rectangle(frame, box, cv::Scalar(0, 255, 0), 2);
            // 只画一个框：找到第一对合格的，画完就走。
            return;
        }
    }
}