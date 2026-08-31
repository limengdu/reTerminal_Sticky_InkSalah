# Sticky Salah

Sticky Salah 是面向 Seeed Studio reTerminal Sticky 的原生穆斯林礼拜时间固件。设备默认以竖屏显示当天五场礼拜、下一场礼拜和倒计时；检测到 Sticky 真实横向摆放时，才切换到横屏页面。

## 主要功能

- 默认竖屏大字号日程，横向摆放时切换为双栏礼拜时间轴。
- 英文礼拜名称与预先生成的阿拉伯文字形。
- 公历、伊斯兰历、当前城市、同步状态与电池电量。
- 首次开机使用标准 Captive Portal，手机连接热点后可自动打开配网页。
- 自动扫描附近 Wi-Fi 并显示为可点击列表；点击后填入下方输入框，输入框也支持直接编辑或手动填写。
- 249 个 ISO 国家选项、常用城市建议和保存前地点验证。
- 手机自动填写当前 UTC 偏移，并允许从下拉框调整。
- 23 个 AlAdhan 官方计算机构/地区选项，以及 Standard / Hanafi 两种 Asr 学派选项。
- 通过 HTTPS 获取本月和下月礼拜时间，并把日程与设置保存在设备本地。
- 断网时继续使用本地缓存；联网后校准系统时间和板载 RTC。
- 距离下一场礼拜较远时每 10 分钟刷新，进入一小时后每 5 分钟刷新，最后 10 分钟每分钟刷新。
- 可选三音礼拜提醒，并在刷新完成后通过 AI 按键双击重新进入设置。
- 每次刷新结束后进入深度睡眠，墨水屏在断电后继续保留画面。

一句话总结：它把 Sticky 变成一台低功耗、无需常亮网络连接的礼拜时间牌。

## 硬件与软件

- Seeed Studio reTerminal Sticky，ESP32-S3R8，32 MB Flash。
- USB-C 数据线。
- [Visual Studio Code](https://code.visualstudio.com/) 与 [PlatformIO](https://platformio.org/)，或可直接使用 PlatformIO Core。
- PlatformIO 平台固定为 `espressif32@6.11.0`，对应 ESP-IDF 5.4.1。

第三方依赖只有 Espressif Button 组件 `4.1.6`。DNS 重定向使用项目内实现的 ESP-IDF/LwIP 网络能力，不增加运行依赖。PlatformIO 第一次构建时会自动下载平台、ESP-IDF 与组件。

一句话总结：准备 Sticky、USB 数据线和 PlatformIO 就可以构建与烧录。

## 构建

在项目根目录执行发布版构建：

```bash
pio run -e sticky-salah-release
```

生成的应用固件位于：

```text
.pio/build/sticky-salah-release/firmware.bin
```

需要更详细日志时，构建调试版：

```bash
pio run -e sticky-salah-debug
```

一句话总结：日常烧录使用 release，排查设备日志使用 debug。

## 烧录与串口日志

先连接 Sticky，然后查看系统识别到的串口：

```bash
pio device list
```

把下面的 `/dev/cu.usbmodemXXXX` 替换为上一步显示的端口：

```bash
pio run -e sticky-salah-release -t upload --upload-port /dev/cu.usbmodemXXXX
```

调试版的串口日志命令是：

```bash
pio device monitor --port /dev/cu.usbmodemXXXX --baud 115200
```

烧录成功时，PlatformIO 最后会显示 `SUCCESS`。调试版正常启动后，日志会包含 `boot=start firmware=0.1.0 profile=debug`。

一句话总结：先找到串口，再把同一个串口填进烧录或日志命令。

## 首次设置

1. 首次启动时，竖屏会显示一个形如 `Sticky-Salah-A1B2` 的 Wi-Fi 名称。
2. 手机连接这个 Wi-Fi，密码是 `sticky-salah`。
3. 手机通常会自动打开配网页；也可以在浏览器访问 `http://192.168.4.1`。
4. 点击上方扫描列表中的家庭 Wi-Fi，名称会自动填入下方输入框；也可以直接编辑或手动输入隐藏网络名称。
5. 从下拉框选择国家和城市；目录外城市使用 `Other city` 输入。
6. 确认手机自动选择的 UTC 偏移。
7. 检查页面推荐的计算机构，并选择本地清真寺使用的 Asr 学派。
8. 按 `Check and save`。设备会先验证 Wi-Fi、互联网时间和 AlAdhan 礼拜数据，全部成功后才保存并重启。

计算机构的编号与名称来自 [AlAdhan 官方方法接口](https://api.aladhan.com/v1/methods)。国家名称来自 ISO 3166，常用城市建议来自 AlAdhan 公开城市目录。目录外城市会调用 AlAdhan `timingsByCity` 接口验证，验证通过后才会保存。礼拜时间应与当地清真寺公布的时刻表核对；当地清真寺的做法是最终依据。

进入夏令时或冬令时后，再次打开设置页即可让手机写入新的 UTC 偏移。正常页面刷新后双击 AI 按键，可以重新进入设置。

一句话总结：手机只在首次设置或主动修改设置时连接 Sticky 的本地网页。

### 配网串口验收

调试版会把配网链路拆成独立日志。正常启动时会依次看到：

```text
wifi=setup_ap state=started result=ok
wifi=setup_ap ... address=192.168.4.1 dhcp=started ... result=ok
dns=redirect address=192.168.4.1 port=53 result=ok
portal=self_test address=http://192.168.4.1/health result=ok
portal=ready address=http://192.168.4.1/ result=ok
```

手机连入并打开页面后，还会看到：

```text
wifi=setup_client state=connected ...
wifi=setup_client state=address_assigned ip=192.168.4.x ...
portal=request method=GET path=/
portal=request method=GET path=/api/wifi/scan
wifi=scan visible=... returned=... result=ok
```

启动日志证明设备内部的 AP、DHCP、DNS 和 HTTP 已经就绪；客户端日志证明手机连接、获得地址并把请求送到了 Sticky。保存时还会依次记录 Wi-Fi 连接、网络时间和地点数据验证结果。

一句话总结：串口会直接告诉你配网进行到了哪一层。

## 设备运行流程

程序入口是 `app_main()`，可以把它理解为整套固件的总开关。它依次完成以下工作：

1. 初始化 NVS、供电、I2C、RTC、电池、墨水屏、按键、蜂鸣器和 IMU。
2. 读取本地设置；新设备直接进入设置页面。
3. 读取 RTC 和缓存日程。
4. 在首次启动、跨天或缓存缺少日期时连接 Wi-Fi。
5. 使用 SNTP 校准时间，再从 AlAdhan 下载本月和下月日程。
6. 用当前时间计算下一场礼拜和倒计时。
7. 以竖屏为默认方向，读取 IMU 后在真实横放时绘制横屏页面。
8. 在礼拜时刻播放可选提示音。
9. 计算下一次唤醒时间，关闭外设并进入深度睡眠。

一句话总结：每次醒来只完成“读时间、算下一场、画一屏”，然后马上继续省电。

## 代码结构

```text
boards/                 Sticky 的 PlatformIO 板卡定义
components/             墨水屏、触摸与日志组件
src/app/                一次完整唤醒周期的业务入口
src/board/              供电、引脚、I2C 与共享 SPI
src/devices/            RTC、电池与蜂鸣器
src/display/            SSD1677 显示驱动封装
src/input/              按键与触摸驱动
src/location/           ISO 国家、常用城市和推荐计算方法
src/network/            Wi-Fi、SNTP 与首次设置网页
src/prayer/             日期、礼拜时间与下一场计算
src/services/           AlAdhan HTTPS 客户端
src/storage/            设置和日程缓存
src/ui/                 画布、字体、阿拉伯文字形与页面
test/                   可在电脑直接运行的逻辑测试
tools/                  测试、界面预览与字形生成工具
```

一句话总结：硬件驱动、网络、礼拜规则和界面各放在自己的“抽屉”里，后续修改互不打扰。

## 本机测试

本机测试不需要连接 Sticky：

```bash
bash tools/run_host_tests.sh
```

成功时会看到：

```text
[host-test] PASS 12/12
```

十二项检查分别覆盖：

- Debug、Release 和默认配置的 HTTP 请求头容量至少为 4096 字节。
- Wi-Fi 扫描列表、可编辑名称输入框、点击填入和页面启动时自动扫描。
- 时间解析、跨天寻找下一场礼拜、闰年和动态刷新间隔。
- 日程添加、排序、重复日期与容量边界。
- 首次设置表单、URL 解码、UTC 范围、必填项和超长输入。
- 横屏、竖屏、设置页、错误页与空数据保护。
- 冷启动默认竖屏、实体横放切换和未知方向回退。
- AP 启动、DHCP 运行与 `192.168.4.1` 地址三层就绪条件。
- HTTPS 暂时性故障的三次尝试上限与重试间隔。
- DNS 请求解析及 `192.168.4.1` A 记录响应。
- Wi-Fi 扫描结果去重、信号排序和隐藏 SSID 过滤。
- 249 个国家、常用城市及地区计算方法推荐。

一句话总结：这些测试先把不依赖硬件的核心规则在电脑上验证清楚。

## 页面预览

首次配网页面直接从固件源文件中读取，因此预览内容与设备页面保持一致：

```bash
python3 tools/preview_setup_portal.py --port 8766
```

浏览器打开 `http://127.0.0.1:8766`。

墨水屏页面预览器会调用固件实际使用的 `Canvas` 与 `prayer_pages`：

```bash
g++ -std=c++17 -Isrc/prayer -Isrc/storage -Isrc/ui tools/render_ui_preview.cpp src/prayer/prayer_time.cpp src/storage/settings_model.cpp src/ui/arabic_prayer_assets.cpp src/ui/islamic_background_asset.cpp src/ui/islamic_landscape_background_asset.cpp src/ui/islamic_setup_background_asset.cpp src/ui/canvas.cpp src/ui/font.cpp src/ui/prayer_pages.cpp -o /tmp/sticky_salah_ui_preview
/tmp/sticky_salah_ui_preview landscape schedule /tmp/sticky_salah_landscape.pgm
/tmp/sticky_salah_ui_preview portrait schedule /tmp/sticky_salah_portrait.pgm
/tmp/sticky_salah_ui_preview portrait setup /tmp/sticky_salah_setup.pgm
```

竖屏礼拜页、首次 Wi-Fi 设置页和横屏礼拜页分别使用可追溯的黑白伊斯兰
几何背景。设计源图统一保存在 `assets/ui/`，安装 `ffmpeg` 后可重新生成
固件中的 1-bit 位图：

```bash
python3 tools/generate_islamic_background_asset.py assets/ui/islamic_prayer_background_source.png src/ui/islamic_background_asset.cpp
python3 tools/generate_islamic_background_asset.py assets/ui/islamic_setup_background_source.png src/ui/islamic_setup_background_asset.cpp --width 480 --height 800 --symbol kIslamicSetupBackground --function islamic_setup_background_asset
python3 tools/generate_islamic_background_asset.py assets/ui/islamic_landscape_background_source.png src/ui/islamic_landscape_background_asset.cpp --width 800 --height 480 --symbol kIslamicLandscapeBackground --function islamic_landscape_background_asset
```

一句话总结：配网页和墨水屏页面都能先在电脑上看，再烧录到硬件。

## 数据与时间边界

- Wi-Fi 凭据、城市、国家和选择项保存在 ESP32 的 NVS 中。
- HTTPS 请求只向 `api.aladhan.com` 发送城市、国家、计算机构和 Asr 学派。
- 配网提交后先连接用户选择的 Wi-Fi，并调用 AlAdhan 验证五次礼拜时间；HTTPS 暂时性故障最多尝试三次，验证失败时保留配网页和原有设置。
- 固件缓存当前月与下一个月，最多保存 64 天。
- 时区采用设置页保存的固定 UTC 偏移；季节性时钟变化通过再次设置完成更新。
- 当前版本以 AlAdhan 返回的当日有序时间为准，适合每天五场时间都落在本地同一自然日内的地区。

一句话总结：设备只保存运行所需的数据，并让时间边界保持清楚、可核对。

## 驱动来源与许可证

板级供电、SSD1677 墨水屏、GT911 触摸、PCF8563 RTC、BQ27220 电池、按键、蜂鸣器和 IMU 驱动参考并整理自相邻的 `sticky_demo_limengdu` ESP-IDF Demo。项目采用 [MIT License](LICENSE)。
