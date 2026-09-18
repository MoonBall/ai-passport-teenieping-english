#!/usr/bin/env python3
"""Crop ChatGPT storyboards into JPEG frames and generate the card table."""

import hashlib
import io
import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
IMAGES = ROOT / "assets/images/spoken"
WIDTH, HEIGHT = 216, 144


def c(value):
    return json.dumps(value, ensure_ascii=False)


def jpeg(image):
    canvas = Image.new("RGB", (WIDTH, HEIGHT), "#f5f0e7")
    image.thumbnail((WIDTH, HEIGHT), Image.Resampling.LANCZOS)
    canvas.paste(image, ((WIDTH-image.width)//2, (HEIGHT-image.height)//2),
                 image if image.mode == "RGBA" else None)
    out = io.BytesIO()
    canvas.save(out, "JPEG", quality=65, optimize=True, progressive=False)
    return out.getvalue()


def storyboard_frames(story):
    image = Image.open(IMAGES / story["file"]).convert("RGB")
    columns, rows = story["columns"], story["rows"]
    xs = story.get("x_bounds", [round(i * image.width / columns) for i in range(columns + 1)])
    ys = story.get("y_bounds", [round(i * image.height / rows) for i in range(rows + 1)])
    assert len(xs) == columns + 1 and len(ys) == rows + 1
    assert all(a < b for a, b in zip(xs, xs[1:])) and all(a < b for a, b in zip(ys, ys[1:]))
    inset = story.get("inset", 4)
    frames = []
    for cell in story["cells"]:
        column, row = cell % columns, cell // columns
        assert 0 <= cell < columns * rows
        box = (xs[column] + (inset if column else 0),
               ys[row] + (inset if row else 0),
               xs[column + 1] - (inset if column + 1 < columns else 0),
               ys[row + 1] - (inset if row + 1 < rows else 0))
        frames.append(jpeg(image.crop(box)))
    return frames


def main():
    lessons = json.loads((IMAGES / "lessons.json").read_text())
    characters = json.loads((IMAGES / "characters/order.json").read_text())
    audio = json.loads((ROOT / "assets/music/spoken/manifest.json").read_text())
    story_path = IMAGES / "storyboards.json"
    stories = json.loads(story_path.read_text())
    assert set(stories) == {lesson["id"] for lesson in lessons}, "Missing ChatGPT storyboards"
    code = ['#include "spoken_data.h"', ""]
    records, manifest = [], []
    for index, lesson in enumerate(lessons):
        character = characters[index]
        assert character["lesson_id"] == lesson["id"]
        assert character["character_id"] == lesson["character_id"]
        story = stories[lesson["id"]]
        assert story["character_id"] == character["character_id"]
        frames = storyboard_frames(story)
        frame_ms = story.get("frame_ms", 1000)
        assert len(frames) == 4 and frame_ms == 1000
        name = f"FRAMES_{index}"
        for n, data in enumerate(frames):
            code.append(f"static const uint8_t JPEG_{index}_{n}[] = {{")
            for off in range(0, len(data), 24):
                code.append(",".join(str(v) for v in data[off:off+24]) + ",")
            code.append("};")
        code.append(f"static const lv_image_dsc_t {name}[] = {{")
        for n, data in enumerate(frames):
            code.append(f"{{.header={{.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RAW,.w={WIDTH},.h={HEIGHT}}},.data_size={len(data)},.data=JPEG_{index}_{n}}},")
        code.append("};")
        entry = audio["cards"][index]
        assert entry["id"] == lesson["id"]
        fields = [f".{key}={c(lesson[key])}" for key in ["id", "title", "scene", "command", "meaning", "response", "usage"]]
        for key in ("short", "lesson"):
            fields.append(f".{key}_audio={{" + ",".join(f".{k}={entry[key][k]}" for k in ["offset", "bytes", "samples", "duration_ms"]) + "}")
        fields.extend([".stages_ms={" + ",".join(map(str, entry["stages_ms"])) + "}",
                       f".frames={name}", f".frame_count={len(frames)}", f".frame_ms={frame_ms}"])
        records.append("{" + ",\n".join(fields) + "},")
        manifest.append({"id": lesson["id"], "frames": len(frames), "frame_ms": frame_ms,
                         "character_id": character["character_id"],
                         "character_name": character["character_name"],
                         "bytes": sum(map(len, frames)), "source": "ChatGPT Create image",
                         "file": story["file"],
                         "sha256": hashlib.sha256((IMAGES / story["file"]).read_bytes()).hexdigest()})
    code.extend([f"const uint32_t SPOKEN_AUDIO_BYTES={audio['payload_bytes']};",
                 f"const uint32_t SPOKEN_AUDIO_CRC={audio['crc32']}u;",
                 "const spoken_card_t SPOKEN_CARDS[SPOKEN_CARD_COUNT] = {", *records, "};", ""])
    (ROOT / "main/spoken_data.c").write_text("\n".join(code))
    (IMAGES / "frame-manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    print(f"Frames: {sum(r['frames'] for r in manifest)}, JPEG bytes: {sum(r['bytes'] for r in manifest):,}")


if __name__ == "__main__":
    main()
