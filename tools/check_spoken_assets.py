#!/usr/bin/env python3
"""Validate speech boundaries, card content, animation coverage and resource integrity."""

import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser()
parser.add_argument("--firmware", type=Path)
parser.add_argument("--require-video", action="store_true")
parser.add_argument("--require-storyboards", action="store_true")
args = parser.parse_args()
cards = json.loads((ROOT / "assets/images/spoken/lessons.json").read_text())
audio = json.loads((ROOT / "assets/music/spoken/manifest.json").read_text())
frames = json.loads((ROOT / "assets/images/spoken/frame-manifest.json").read_text())
characters = json.loads((ROOT / "assets/images/spoken/characters/order.json").read_text())
source_order = json.loads((ROOT / "assets/images/spoken/characters/source-order.json").read_text())["characters"]
stories = json.loads((ROOT / "assets/images/spoken/storyboards.json").read_text())
generation = json.loads((ROOT / "assets/images/spoken/chatgpt/generation.json").read_text())
assert generation["source"] == "ChatGPT Create image"
assert [card for sheet in generation["sheets"] for card in sheet["cards"]] == [card["id"] for card in cards]
for sheet in generation["sheets"]:
    assert sheet["accepted"], f"Image has not been accepted: {sheet['file']}"
    image = ROOT / "assets/images/spoken/chatgpt" / sheet["file"]
    assert hashlib.sha256(image.read_bytes()).hexdigest() == sheet["sha256"]
    assert sheet["characters"] == [card["character_id"] for card in cards if card["id"] in sheet["cards"]]
assert len(characters) == len(cards) == 17
assert len({c["character_id"] for c in characters}) == 17
for index, (character, card, frame) in enumerate(zip(characters, cards, frames)):
    assert character["order"] == index + 1 and character["source_index"] == index
    assert character["character_id"] == source_order[index]["id"] == card["character_id"]
    assert character["character_name"] == source_order[index]["name"] == card["title"]
    assert character["lesson_id"] == card["id"]
    assert frame["character_id"] == stories[card["id"]]["character_id"] == card["character_id"]
    reference = ROOT / "assets/images/spoken" / character["reference"]
    assert hashlib.sha256(reference.read_bytes()).hexdigest() == character["reference_sha256"]
blob = (ROOT / "assets/music/spoken/speech.bin").read_bytes()
assert len(cards) == len(audio["cards"]) == len(frames) == 17
assert len(set(c["id"] for c in cards)) == 17
assert blob[:8] == b"SPOKEN01"
assert struct.unpack("<II", blob[8:16]) == (len(blob)-16, zlib.crc32(blob[16:]))
assert len(blob) <= 0x3A0000
end = 16
for card, record, frame in zip(cards, audio["cards"], frames):
    assert card["id"] == record["id"] == frame["id"]
    assert all(card[k] for k in ("scene", "command", "meaning", "response", "usage", "action"))
    for mode in ("short", "lesson"):
        clip = record[mode]
        assert clip["offset"] == end
        assert clip["bytes"] == (clip["samples"]+1)//2
        end += clip["bytes"]
        assert end <= len(blob)
        assert hashlib.sha256(blob[clip["offset"]:end]).hexdigest() == clip["sha256"]
    stages = record["stages_ms"]
    assert len(stages) == 4 and stages[0] == 0
    assert all(a < b for a, b in zip(stages, stages[1:] + [record["lesson"]["duration_ms"]]))
    assert frame["frames"] >= 1
    assert frame["frame_ms"] > 0
    if args.require_storyboards:
        assert frame["source"] == "ChatGPT Create image", f"Wrong image source: {card['id']}"
        assert frame["frames"] == 4 and frame["frame_ms"] == 1000
        source = ROOT / "assets/images/spoken" / frame["file"]
        assert hashlib.sha256(source.read_bytes()).hexdigest() == frame["sha256"]
    if args.require_video:
        assert frame["frames"] > 1, f"Video missing: {card['id']}"
assert end == len(blob)
if args.firmware:
    merged = args.firmware.read_bytes()
    assert merged[0x360000:0x360000+len(blob)] == blob
print(f"Spoken assets: PASS (17 cards, 34 speech clips, {len(blob):,} audio bytes)")
