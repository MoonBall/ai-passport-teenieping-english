#!/usr/bin/env python3
"""Generate the compact Chinese and Latin fonts used by spoken cards."""

import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
text = (ROOT / "assets/images/spoken/lessons.json").read_text()
text += (ROOT / "main/spoken_app.c").read_text()
chars = "".join(sorted(set(re.findall(r"[\u4e00-\u9fff\u3000-\u303f\uff00-\uffef]", text)
                            + [chr(i) for i in range(32, 127)])))
(ROOT / "assets/fonts/spoken-charset.txt").write_text(chars)
for size in (14, 16):
    name = f"ui_font_spoken_{size}"
    subprocess.run(["npx", "--yes", "lv_font_conv@1.5.3", "--no-prefilter", "--bpp", "2",
                    "--size", str(size), "--format", "lvgl", "--lv-include", "lvgl.h", "--font",
                    str(ROOT / "assets/fonts/source/NotoSansCJKsc-Regular.otf"), "--symbols", chars,
                    "--lv-font-name", name, "-o", str(ROOT / f"main/{name}.c")], check=True)
    output = ROOT / f"main/{name}.c"
    output.write_text(output.read_text().rstrip() + "\n")
print(f"Fonts: {len(chars)} glyphs, 14px and 16px")
