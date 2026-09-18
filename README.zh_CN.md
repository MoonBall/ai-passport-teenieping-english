<p align="right"><strong>简体中文</strong> · <a href="README.md">English</a></p>

# 萌可英语口语卡片

独立的 AI Passport 应用，包含 17 个课堂英语场景。每个场景使用不同的萌可，顺序沿用原萌可项目的前 17 个角色。每个场景使用 ChatGPT Create image 生成的四幅定时动作图。

上／下翻卡。短按确定播放英语和动作图，再次短按停止。长按确定，用中英文讲解场景、指令、期望回答和适用场景。

格丽乔英语版完整保留在另一个 `ai-passport-spoken-english` 项目中。本项目的图片、代码和构建产物独立维护。

- [操作、角色顺序、素材和验证](docs/spoken-guide.zh_CN.md)
- [构建与测试](docs/development/build-and-test.zh_CN.md)
- [设备身份和 Recovery 保护约定](docs/development/ble-recovery-compatibility.zh_CN.md)

激活 ESP-IDF 5.5.3 后运行 `./tools/validate.sh`。通过验证的固件位于 `build/FoloToy-AI-Passport-full.bin`。已有身份信息的设备应通过 Recovery 安装或经过验证的分段刷写，禁止将合并固件直接覆盖设备身份区。
