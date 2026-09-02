# Sticky Salah

Sticky Salah 是面向 Seeed Studio reTerminal Sticky 的原生穆斯林礼拜时间固件。它根据设备保存的经纬度、日期、UTC 偏移和用户选择的教法参数，在设备本地计算每天的礼拜时间，不依赖在线礼拜时间表。

## 主要功能

- 显示 `Fajr`、`Sunrise`、`Dhuhr`、`Asr`、`Maghrib`、`Isha`、`Qiyam` 七个时间节点。
- 主画面显示当前礼拜窗口、开始与结束时间、剩余时长和下一次必行礼拜；`Sunrise` 到 `Dhuhr` 显示为礼拜窗口之间的时段。
- `Qiyam` 表示夜晚最后三分之一的开始：从当天 `Maghrib` 到次日 `Fajr` 的夜间长度计算得出，它作为与 `Isha` 窗口重叠的自愿礼拜时段显示。
- 提供常见计算机构、`Standard` 与 `Hanafi` 两类 Asr 规则，以及七个时间各自的 ±30 分钟微调。
- 默认竖屏显示；设备真实横放后，由 IMU 唤醒并立即切换横屏页面。
- 横竖屏礼拜页使用画图模型创作的米哈拉布尖拱、Girih 交织纹、阿拉伯蔓草纹与清真寺灯装饰，顶部信息、当前礼拜和七项时间分别占用独立区域。
- 热点网页只负责 Wi-Fi。Wi-Fi 页自动扫描附近网络，列表点击与手动输入可以同时使用。
- 礼拜设置是设备上的原生竖屏触摸菜单，包含地点、机构、Asr 规则、七项微调和提示音。
- 设备界面统一使用至少 2 倍点阵字号，当前选项、状态、日期和辅助提示都保持近距离可读。
- 竖屏顶部集中显示城市、公历日期、计算机构与教法学派；横屏利用更宽空间补充回历日期和同步状态。
- 设置页使用粗体标题与常规字重内容建立视觉层级；首次设置只提供保存并开始的完成入口。
- 原生设置页持续保持可操作状态，直到用户主动完成设置或在日常设置中退出。
- 主页面从屏幕底部向上滑动即可打开设备设置；设置页从顶部向下滑动会验证、保存并返回主页面，横竖屏主页面使用相同手势方向。
- 地点支持设备联网自动估算和触摸屏手动经纬度两种入口；UTC 偏移可按 15 分钟或 1 小时调整。
- 联网时仅校准系统时间与板载 RTC；网络位置估算为可选功能。礼拜时间的日常计算可离线完成。
- 设备醒来后由 GT911 触摸和 IMU 持续监听，底部上滑可随时打开设置，转动设备会立即重排页面。
- 任意触摸或转动都会重新计时；连续 3 分钟无操作后，左上角显示小型弯月并进入深度睡眠，由定时器、顶部按键或 IMU 姿态变化唤醒。

一句话总结：这是一台按太阳位置本地计算、能显示礼拜窗口的低功耗墨水屏礼拜时间牌。

## 礼拜时间的定义

固件把七个名称解释为：

- `Fajr`：晨礼开始，同时结束前一晚的 `Isha` 窗口。
- `Sunrise`：日出，结束 `Fajr` 窗口；它是时间边界，不是五番必行礼拜之一。
- `Dhuhr`：太阳越过当地正午后，晌礼窗口开始。
- `Asr`：晡礼窗口开始；具体时间受 `Standard` 或 `Hanafi` 影长规则影响。
- `Maghrib`：日落，昏礼窗口开始。
- `Isha`：暮光达到所选机构标准后，宵礼窗口开始，并持续到次日 `Fajr`。
- `Qiyam`：夜晚最后三分之一开始，即 `Maghrib + 2/3 × (次日 Fajr - Maghrib)`。

计算机构主要决定 `Fajr` 与 `Isha` 使用的太阳角度。Asr 学派设置只改变 `Asr`。高纬度暮光在存在正常日出和日落的日期采用角度比例限制。用户参加集体礼拜时，仍以所跟随清真寺公布的实际安排为准。

一句话总结：页面上的钟点是窗口的边界，主画面告诉用户当前处在哪个窗口、还剩多久。

## 硬件与软件

- [Seeed Studio reTerminal Sticky](https://www.seeedstudio.com/sticky/docs/en/device-guide/hardware-overview/)，ESP32-S3R8，32 MB Flash；LSM6DS3TR-C 中断连接 GPIO7，GT911 触摸中断连接 GPIO21。
- USB-C 数据线。
- [PlatformIO](https://platformio.org/) Core 或 Visual Studio Code PlatformIO 插件。
- `espressif32@6.11.0` 与 ESP-IDF 5.4.1。

礼拜计算器使用项目内原生 C++ 实现，没有增加计算库。第三方运行依赖只有 Espressif Button 组件 `4.1.6`。

一句话总结：准备 Sticky、USB 数据线和 PlatformIO 就可以构建与烧录。

## 构建、烧录与日志

在项目根目录构建发布版：

```bash
pio run -e sticky-salah-release
```

生成的应用固件位于 `.pio/build/sticky-salah-release/firmware.bin`。调试版使用：

```bash
pio run -e sticky-salah-debug
```

查看串口并烧录：

```bash
pio device list
pio run -e sticky-salah-release -t upload --upload-port /dev/cu.usbmodemXXXX
pio device monitor --port /dev/cu.usbmodemXXXX --baud 115200
```

把 `/dev/cu.usbmodemXXXX` 换成第一条命令列出的实际端口。构建或烧录成功时最后会显示 `SUCCESS`。

一句话总结：先找到串口，再用 release 固件烧录；需要排查时改用 debug。

## 首次设置

1. 手机连接设备显示的 `Sticky-Salah-XXXX` 热点，密码为 `sticky-salah`。
2. 手机通常会自动打开页面，也可以访问 `http://192.168.4.1`。
3. Wi-Fi 页面会自动扫描附近网络；点击列表项后名称会进入可编辑输入框，隐藏网络也可直接手动输入。
4. 点击 `Check and save`。联网与校时成功后，设备保存 Wi-Fi 并自动重启。
5. 设备屏幕自动进入 `SALAH SETTINGS` 原生触摸菜单，并先用公网出口 IP 估算城市、经纬度和 UTC 偏移。
6. 进入 `LOCATION` 核对地点。点经纬度数值可打开数字键盘精确输入，左右按钮可按 0.1° 调整；时区支持 ±15 分钟和 ±1 小时。
7. 进入 `CALCULATION` 选择计算机构，以及 `Standard` 或 `Hanafi` Asr 规则。中国默认使用 `Muslim World League + Hanafi`。
8. 进入 `MINUTE ADJUSTMENTS`，分别为七个时间做 ±30 分钟微调；按需开启 `PRAYER REMINDER`。
9. 点击 `SAVE & START`。设备会先验证当天能否得到完整时间表，再保存并显示主页面。

公网 IP 只能估算大致地点，所以自动结果需要在设备屏幕上核对。所有礼拜设置最终保存在 NVS。正常运行时从主页面底部向上滑可再次打开设备原生设置；设置完成后从顶部向下滑会先验证并保存，再返回主页面。在原生菜单中点 `WI-FI SETUP` 可重新进入热点配网。

一句话总结：手机只负责让设备联网，礼拜规则全部在设备触摸屏上完成。

## 设备运行流程

程序入口 `app_main()` 每次唤醒按下面顺序运行：

1. 初始化 NVS、供电、I2C、RTC、电池、墨水屏、按键、蜂鸣器和 IMU。
2. 读取 Wi-Fi、位置和礼拜参数；没有 Wi-Fi 时启动热点网页，首次联网后自动进入设备原生设置。
3. 读取 RTC；需要校时时连接 Wi-Fi，并用 SNTP 更新系统时间与 RTC。
4. 按昨天、今天、明天三个日期本地计算时间，保证跨午夜的 `Isha` 和 `Qiyam` 窗口能够正确判断。
5. 找出当前礼拜窗口、剩余分钟和下一次必行礼拜。
6. 默认绘制竖屏；IMU 确认设备横放时绘制横屏。
7. 到达礼拜边界时播放可选提示音。
8. 持续处理触摸和姿态变化，自动刷新页面时保留用户的空闲计时；连续 3 分钟无操作后，配置定时器、顶部按键和低功耗姿态中断并进入深度睡眠。

调试版进入睡眠前会看到类似日志：

```text
imu=wakeup state=armed mode=4d ... interrupt_pin=7 active_level=low result=ok
power=deep_sleep wake_button=4 wake_orientation=7 orientation_enabled=1 wake_touch=21 touch_enabled=0 touch_level=0 timer_us=... result=entering
```

重新启动时，`wake_source` 会显示 `timer`、`button` 或 `orientation`。设备醒着时会看到 `input=active idle_timeout_ms=180000`，表示触摸和姿态正在持续监听。无操作满三分钟时会记录 `input=inactive reason=idle_timeout idle_ms=180000 result=sleep`，随后进入深度睡眠。

设备原生设置可从顶部向下滑来保存并关闭；从左侧边缘向页面中部滑动会返回上一级，并保留尚未保存的设置草稿。设置主页不会被左滑直接关闭。保存或取消后会直接返回礼拜页面，并记录 `settings=closed ... mode=in_place`。仅当时区发生变化时，设备才会在重绘前重新同步网络时间。

Location 页面会使用已保存的 Wi-Fi 自动估算当前城市、国家、经纬度和时区，并覆盖之前手动填写的坐标。坐标键盘的 `DELETE` 清空全部输入，`BACK` 每次删除一个字符，`OK` 确认输入。

一句话总结：主控醒来算三天、画一屏，然后把计时和转动检测交给低功耗硬件。

## 代码结构

```text
boards/                 Sticky 的 PlatformIO 板卡定义
components/             墨水屏、触摸与日志组件
src/app/                唤醒周期与设备原生设置流程
src/board/              供电、引脚、I2C 与共享 SPI
src/devices/            RTC、电池与蜂鸣器
src/display/            SSD1677 显示驱动封装
src/input/              按键与触摸驱动
src/location/           ISO 国家、城市建议和地区默认参数
src/network/            Wi-Fi、SNTP、位置估算与 Wi-Fi 配网页面
src/prayer/             太阳位置计算、日期和礼拜窗口规则
src/storage/            Wi-Fi、位置与礼拜参数持久化
src/ui/                 画布、字体、阿拉伯字形与页面
test/                   可在电脑直接运行的逻辑与策略测试
tools/                  测试、网页预览与墨水屏预览工具
```

一句话总结：网络只负责校时与辅助定位，礼拜规则集中在 `src/prayer/`。

## 本机测试

本机测试不需要连接 Sticky：

```bash
bash tools/run_host_tests.sh
```

成功时会看到：

```text
[host-test] PASS 22/22
```

测试覆盖 HTTP 请求头容量、Wi-Fi 网页职责、Wi-Fi 列表和手动输入、设备最小字号策略、粗体与常规字重、首次设置完成入口、原生设置页持续停留策略、原生设置页触摸区域、横竖屏边缘滑动、外部供电持续输入窗口、GT911 四方向配置、横竖屏花纹安全区与左右视觉重量、横屏顶部信息居中规则、经纬度本地计算、Hanafi Asr、七项微调、Qiyam 公式、跨午夜礼拜窗口、页面绘制保护、屏幕方向、深睡唤醒准备、DNS、Wi-Fi 扫描、地区建议以及固件不编译远程礼拜表管线的策略检查。

一句话总结：不接设备也能先验证核心数学、页面数据流和构建策略。

## 页面预览

Wi-Fi 配网页预览直接读取固件内嵌的 HTML：

```bash
python3 tools/preview_setup_portal.py --port 8769
```

浏览器打开 `http://127.0.0.1:8769/` 查看 Wi-Fi 页。任意其他路径也会回到同一 Wi-Fi 页面，以适配手机系统的配网探测地址。

墨水屏预览器调用固件实际使用的画布和页面代码：

```bash
g++ -std=c++17 -Isrc/prayer -Isrc/storage -Isrc/ui tools/render_ui_preview.cpp src/prayer/prayer_time.cpp src/storage/settings_model.cpp src/ui/arabic_prayer_assets.cpp src/ui/islamic_background_asset.cpp src/ui/islamic_landscape_background_asset.cpp src/ui/islamic_setup_background_asset.cpp src/ui/canvas.cpp src/ui/font.cpp src/ui/prayer_pages.cpp src/ui/prayer_settings_pages.cpp src/ui/sleep_indicator.cpp -o /tmp/sticky_salah_ui_preview
/tmp/sticky_salah_ui_preview portrait schedule /tmp/sticky_salah_portrait.pgm
/tmp/sticky_salah_ui_preview landscape schedule /tmp/sticky_salah_landscape.pgm
/tmp/sticky_salah_ui_preview portrait schedule-sleep /tmp/sticky_salah_sleep_portrait.pgm
/tmp/sticky_salah_ui_preview landscape schedule-sleep /tmp/sticky_salah_sleep_landscape.pgm
/tmp/sticky_salah_ui_preview portrait setup /tmp/sticky_salah_setup.pgm
/tmp/sticky_salah_ui_preview portrait settings-main /tmp/sticky_salah_settings.pgm
/tmp/sticky_salah_ui_preview portrait settings-main-first-time /tmp/sticky_salah_first_time.pgm
/tmp/sticky_salah_ui_preview portrait settings-location /tmp/sticky_salah_location.pgm
/tmp/sticky_salah_ui_preview portrait settings-calculation /tmp/sticky_salah_calculation.pgm
/tmp/sticky_salah_ui_preview portrait settings-adjustments /tmp/sticky_salah_adjustments.pgm
```

画图模型输出的高分辨率横竖屏设计稿和适配设备尺寸的 PNG 位于 `assets/ui/`，对应的 1-bit 固件资源位于 `src/ui/`。竖屏稿按 480×800 布局，横屏稿按 800×480 独立构图，适配过程保持原始比例。

一句话总结：Wi-Fi 网页、主页面和设备原生设置都能在电脑上先看效果。

## 数据与网络边界

- Wi-Fi 凭据、城市、国家、坐标、UTC 偏移、机构、Asr 规则和微调保存在 ESP32 NVS。
- 每次计算只读取本地设置、日期和 RTC 时间，不向礼拜时间网站发送请求。
- 联网用于 SNTP 校时；首次进入原生设置或用户点击 `DETECT LOCATION` 时，设备请求 `ipwho.is` 的免费公开端点获取近似城市、坐标和 UTC 偏移。
- 固件保存固定 UTC 偏移。进入或退出夏令时后，通过设置页确认新的偏移。
- 每次只计算昨天、今天、明天三天，没有月表下载和日程缓存。

一句话总结：礼拜计算本身离线完成，联网只解决“现在几点”和可选的“我大概在哪”。

## 驱动来源与许可证

板级供电、SSD1677 墨水屏、GT911 触摸、PCF8563 RTC、BQ27220 电池、按键、蜂鸣器和 IMU 驱动参考并整理自相邻的 `sticky_demo_limengdu` ESP-IDF Demo。项目采用 [MIT License](LICENSE)。
