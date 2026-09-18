<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 图片资源（Images）

本目录存放项目可复用的图片资源，如 UI 图标、背景、RGB565 资源等。

## 如何使用

- 图片文件复制到本目录，并在本项目 `README.md` 记录分辨率、格式、用途与来源。
- 与固件集成时，参考 [`components/bsp/include/bsp_display.h`](../../components/bsp/include/bsp_display.h) 与相关示例分支的图片资源管线，转换为固件所需格式（如 RGB565 数组）。
- 图片资源占用 Flash 与内存，集成前请评估 ESP32-C3 无 PSRAM 的限制。

## 目录说明

> 当前为空骨架，用于存放后续加入的图片资源。加入资源时请同步更新本 `README.md` 的索引。

用户提供的课程表、卡片数据和 ChatGPT Create image 分镜图位于 `spoken/`。运行 `tools/generate_spoken_visuals.py`，为 17 张卡片分别裁出四个定时动作画面。

角色参考图和场景顺序对应表保存在 `spoken/characters/`。17 个场景各用一个不同角色，严格沿用原萌可项目前 17 个角色的顺序。本项目只使用新生成的萌可分镜图。
