# armor — 装甲板灯条识别（灰度阈值版）

读一段视频，逐帧找出装甲板上的两根灯条，用一个绿框框住，最后输出一个处理好的视频。

- 灰度阈值二值化，只需要调一个数字
- 只画**一个**框，不画候选线、端点、文字
- 自动排除地面倒影
- 按空格可以暂停 / 继续
- 只输出 `out.avi` 一个文件，不打印检出率、分辨率之类的统计

## 环境要求

| 项 | 要求 |
| --- | --- |
| 系统 | Linux（WSL / Ubuntu / 双系统都行） |
| OpenCV | 4.5.4，装在 `/usr/local/opencv4.5.4`（任务一编译安装的那个） |
| CMake | ≥ 3.10 |
| 编译器 | 支持 C++14（g++ 5 以上就行） |

## 编译

```bash
cd armor
mkdir -p build
cd build
cmake ..
make
```

`cmake ..` 不报错、`make` 最后出现 `Built target armor` 就是成功了。

## 运行

先把视频放进来：

```bash
mkdir -p data
cp 你的视频.avi data/armor.avi
```

然后运行（注意路径是相对于你执行程序的目录）：

```bash
./armor ../data/armor.avi
```

不带参数时默认找 `data/armor.avi`。

| 按键 | 作用 |
| --- | --- |
| 空格 | 暂停 / 继续 |
| ESC | 退出 |

运行时会在**当前目录**生成 `out.avi`（和输入同尺寸、25 帧/秒）。

## 参数

全部在 `include/config.h`，改完重新 `make` 即可，不用重新 `cmake`。

| 参数 | 默认 | 作用 | 调大 | 调小 |
| --- | --- | --- | --- | --- |
| `GRAY_MIN` | 100 | 灰度阈值 | 只要更亮的，噪点少但灯条可能断 | 暗的也进来，噪点多 |
| `MIN_H` | 12 | 灯条最小高度 | 能滤掉倒影 | 远处的灯条也能收 |
| `MAX_H` | 120 | 灯条最大高度 | 允许更近的目标 | 太大的不要 |
| `MIN_RATIO` | 2 | 高 ÷ 宽 的下限 | 只留更细长的 | 方块也可能进来 |
| `MAX_H_DIFF` | 22 | 两根灯条高度差的容忍 | 配对更宽松 | 更严格 |
| `MAX_Y_DIFF` | 28 | 两根灯条上下位置差的容忍 | 倾斜的板也能配 | 更严格 |
| `MIN_GAP` / `MAX_GAP` | 20 / 170 | 两灯条水平间距范围 | — | — |
| `SHADOW_X_DIFF` | 14 | 判断倒影时「左右位置差不多」的容忍 | 倒影删得更干净 | 更严格 |

调参时可以在 `detect.cpp` 的 `threshold` 后面临时加一行 `cv::imshow("mask", mask);`，直接看二值化筛出了什么。

## 目录结构

```
armor/
├── CMakeLists.txt        构建脚本
├── README.md
├── .gitignore
├── data/                 放视频（自己建，视频改名 armor.avi）
├── include/
│   ├── config.h          所有参数
│   └── detect.h          drawBox 的声明
└── src/
    ├── main.cpp          读视频、按键、写视频
    └── detect.cpp        灰度 → 阈值 → 轮廓 → 配对 → 画框
```

## 算法四步

1. **转灰度**：`cvtColor`，把彩色图变成只有亮度的一张图。
2. **二值化**：`threshold`，比 `GRAY_MIN` 亮的变白，其余变黑。
3. **找轮廓并变成矩形**：`findContours` 拿到白色区域的边界，`boundingRect` 把它变成一个正着的矩形。
4. **筛选与配对**：`isBar` 判断像不像灯条，`dropShadow` 删掉倒影，剩下的两根按「差不多高、差不多齐、隔开一段距离」配成一对，画一个框。

倒影的判定只有一句话：**如果一根矩形的正下方还有一根更高的矩形，它就是倒影。**

## 常见问题

| 现象 | 原因 / 解决 |
| --- | --- |
| `Could not find ... OpenCVConfig.cmake` | CMake 没找到 OpenCV。检查 `/usr/local/opencv4.5.4/lib/cmake/opencv4/OpenCVConfig.cmake` 是否存在；不在就改 `CMakeLists.txt` 里的 `OPENCV_PATH` |
| `fatal error: opencv2/opencv.hpp: No such file` | `target_include_directories` 少了 `${OpenCV_INCLUDE_DIRS}` |
| `undefined reference to 'cv::...'` | `target_link_libraries` 少了 `${OPENCV_LIBS}` |
| `undefined reference to 'drawBox(...)'` | `add_executable` 里漏了 `src/detect.cpp` |
| `打不开视频` | 路径不对（相对路径是相对执行程序的目录）；或播放/解码库没装（任务一没开 FFMPEG） |
| `Unable to init server` / 没窗口 | WSL 没有图形界面，见下面 |
| 一帧框都没有 | 先看 mask 是不是全黑，把 `GRAY_MIN` 调小 |
| 框到倒影上 | `MIN_H` 调大，`SHADOW_X_DIFF` 调大 |
| `out.avi` 打不开 / 0 字节 | 程序没跑完就关了窗口；确认当前目录可写 |

### WSL 没有图形界面

装 WSLg（Win11 自带）或配 VcXsrv。最省事的办法是把 `main.cpp` 里的 `imshow`、`waitKey` 和按键判断注释掉，程序就只输出 `out.avi`，跑完再看视频——代价是没有窗口，也就没有空格暂停。

## 说明

- 本项目按培训要求**不做位姿解算**（PnP）。
- 框是一个正着的矩形，不是旋转矩形。这段视频里灯条几乎竖直，误差只有一两个像素；如果目标明显倾斜，需要换回 `minAreaRect`。
- 参数是针对配套视频（720×540）调的，换视频后需要重新看 mask 调 `GRAY_MIN`。