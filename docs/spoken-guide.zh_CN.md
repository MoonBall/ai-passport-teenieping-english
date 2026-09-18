<p align="right"><strong>简体中文</strong> · <a href="spoken-guide.md">English</a></p>

# 萌可英语口语卡片

AI Passport 离线应用，包含用户提供表格中的 17 条课堂英语。每张保留场景、英语指令、期望回答或动作，以及适用场景。

## 独立项目与角色顺序

本工程为 `ai-passport-teenieping-english`，原工程 `ai-passport-spoken-english` 完整保留。代码、图片、构建产物和后续改动互相独立，无需切换主题或覆盖原项目。

顺序从原 `ai-passport-teenieping` 的 `cards.json` 固定保存到 `characters/source-order.json`；`characters/order.json` 保存场景映射和原图来源、哈希。每张卡片上方显示萌可姓名，语音及课程内容沿用原英语版，姓名示例改为爱心萌可 Heartsping，年龄示例改成与五指画面一致的五岁。校验器拒绝角色重复、顺序错误、参考图损坏或分镜角色映射不一致。

| 顺序 | 萌可 | 英语场景 |
| --- | --- | --- |
| 1 | 爱心萌可 | What's your name? |
| 2 | 银光萌可 | How are you? |
| 3 | 星光萌可 | How old are you? |
| 4 | 月光萌可 | Listen. / Look at me. |
| 5 | 极光萌可 | Read after me. |
| 6 | 王子萌可 | Circle. / Draw a line. |
| 7 | 缤缤萌可 | Can you try? |
| 8 | 纷纷萌可 | What color is it? |
| 9 | 共共萌可 | I don't know. |
| 10 | 熊熊萌可 | Can you say that again? |
| 11 | 光明萌可 | Slow down, please. |
| 12 | 叮咚萌可 | Can you type it? |
| 13 | 兔兔萌可 | Can you hear / see me? |
| 14 | 流浪萌可 | The line is not OK! |
| 15 | 狐狐萌可 | Good job! / Well done! |
| 16 | 贴贴萌可 | High five! |
| 17 | 透明萌可 | See you next time! |

## 操作

- 上／下翻卡，首尾循环，并停止当前播放。
- 短按确定读英语指令。斜线分隔的多个说法逐句朗读。再次短按停止。
- 长按确定，从头讲解当前卡片的完整内容：场景、指令及意思、期望回答或动作、适用场景。短播放途中长按，会从头开始完整讲解。
- 开机和翻卡保持静音，播放完不自动重复。短句读完后允许动作自然做完，不会重复朗读。
- 姓名和年龄回答是示例，孩子需要换成自己的信息。网络问题卡片保留原表表达，并在讲解里补充更自然的说法。
- 闲置 60 秒调暗屏幕，按键恢复亮度。设备已安装永久 Recovery 固件时，开机按住上键五秒可进入 Recovery。

## 素材和重新生成

用户表格保存在 `assets/images/spoken/lesson-reference.png`，ChatGPT Create image 生成的萌可分镜原图保存在 `assets/images/spoken/chatgpt/`。

`lessons.json` 保存全部文本和动作描述。英文由 macOS Samantha 朗读，中文由 Tingting 朗读，是教学合成音，不是角色或演员原声。语音经过响度归一化，采用 16 kHz 单声道 IMA ADPCM；播放时复制成设备音频芯片需要的双声道。

17 张卡片的全部画面统一来自 ChatGPT 网页的 Create image，参考原萌可项目的角色原图，依次使用 17 个不同的萌可。`storyboards.json` 将原图的网格对应到卡片；每张裁出四幅关键动作图，按每幅一秒顺序显示，总计约四秒。它是四帧动作分镜，不是连续生成的视频。`frame-manifest.json` 记录图片来源、文件哈希和帧时长。缺少任意卡片的分镜时，生成和发布检查会失败。

裁切后的图片压缩为 216×144 基线 JPEG，按原比例居中显示。JPEG 采用小块解码，适合没有 PSRAM 的设备。 原图分隔线可能偏离均匀网格，因此映射文件保存实际格线坐标，裁切时留出内边距，避免混入相邻画面。

```bash
python3 tools/generate_spoken_audio.py
python3 tools/generate_spoken_visuals.py
python3 tools/generate_spoken_font.py
./tools/validate.sh
```

语音资源从 `0x360000` 开始，结束地址低于 `0x700000`，应用保持在 3 MiB 内。保留 `0x356000` 的设备身份区和 `0x700000` 的永久 Recovery，只分段写入通过验证的内容，禁止用合并镜像覆盖受保护间隙。

## 验证

主机测试覆盖首尾循环、静音取消、从短播放切换到讲解、忽略过期完成事件、四段讲解边界，以及语音结束后继续完成动画。素材检查验证全部音频边界和哈希、四项讲解内容、资源 CRC 和容量，以及固件中的语音字节。

USB 接受 `FAP_SCREENSHOT_V1`、`FAP_KEY_UP_V1`、`FAP_KEY_DOWN_V1`、`FAP_KEY_OK_V1`、`FAP_KEY_OK_LONG_V1`，每条命令后加换行。测试事件通过实体按键共用的队列处理，但不能代替实体按键手感和扬声器主观听感验收。

Build：PASS（完整 `./tools/validate.sh`）。Host tests：PASS。Device tests：PASS（用户实测确认）。应用 1,069,232 字节，合并固件 6,906,449 字节，语音 3,367,505 字节。

萌可版已通过 USB 安装，四个写入分段均校验通过，设备身份区和 Recovery 保留区保持不变。用户随后确认实机测试没有问题。原奥特曼英语项目及固件仍独立保留。

Unverified：自动实机播放检查未执行，因为设备在脚本开始前断开。测试设备原先未安装 Recovery 镜像，保留该区域不等于安装 Recovery。

实机验收脚本为 `tools/test_spoken_device.py --serial <设备序列号>`；默认完整播放全部 17 张卡片。`--lesson-cards 2 13 --capture-lesson-stages` 用于显示调整后的局部讲解复测，仍会检查全部 17 条短句。

## 实现决策

LVGL 的 JPEG 头检查会在栈上预留 4 KiB 局部缓冲，即使图片来自 C 数组也一样。实机首次启动时，原先的 4 KiB 主任务栈在 `decoder_info` 中溢出。因此主任务初始化使用 8 KiB 栈，调用更深的 LVGL 渲染和 USB 截图任务使用 12 KiB 栈；渲染仍按小块解码，给语音和界面保留内存。验证后的 ELF 一并保留，用于定位后续实机错误。

完整双语讲解会占用较多空间，因此语音放入独立资源分区，避免突破应用容量上限。资源头记录长度和 CRC，启动时拒绝缺失或不匹配的语音，避免播放无关字节。
