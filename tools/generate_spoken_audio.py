#!/usr/bin/env python3
"""Build bilingual speech with native English and Mandarin voices."""

import hashlib
import json
import re
import struct
import subprocess
import wave
import zlib
from pathlib import Path

from generate_vocab_audio import encode_ima

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "assets/music/spoken"
RATE = 16000


def chunks(text):
    for value in re.split(r"([A-Za-z][A-Za-z '.,!?-]*[A-Za-z.!?])", text):
        if not re.search(r"[A-Za-z\u4e00-\u9fff]", value):
            continue
        yield ("en" if re.search(r"[A-Za-z]", value) else "zh"), value


def synthesize(language, text):
    voice, speed = ("Samantha", 150) if language == "en" else ("Tingting", 225)
    key = hashlib.sha256(f"{voice}|{speed}|{RATE}|{text}".encode()).hexdigest()[:24]
    cache = OUT / "cache"
    cache.mkdir(parents=True, exist_ok=True)
    raw = cache / f"{key}.pcm"
    if not raw.exists():
        aiff = cache / f"{key}.aiff"
        subprocess.run(["say", "-v", voice, "-r", str(speed), "-o", str(aiff), text], check=True)
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", str(aiff),
                        "-af", "silenceremove=start_periods=1:start_threshold=-48dB,areverse,silenceremove=start_periods=1:start_threshold=-48dB,areverse,loudnorm=I=-18:TP=-2:LRA=7",
                        "-ar", str(RATE), "-ac", "1", "-f", "s16le", str(raw)], check=True)
    return raw.read_bytes()


def sequence(parts):
    silence = bytes(int(RATE * 0.22) * 2)
    return silence + silence.join(synthesize(lang, text) for lang, text in parts) + silence


def write_wav(path, data):
    with wave.open(str(path), "wb") as wav:
        wav.setparams((1, 2, RATE, 0, "NONE", "not compressed"))
        wav.writeframes(data)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    cards = json.loads((ROOT / "assets/images/spoken/lessons.json").read_text())
    payload = bytearray()
    manifest = {"sample_rate": RATE, "voices": {"en": "Samantha", "zh": "Tingting"}, "cards": []}
    for card in cards:
        short = sequence([("en", text) for text in card["short"]])
        stages = [
            ("场景", list(chunks("场景。" + card["scene"] + "。"))),
            ("英语指令", [("zh", "英语指令。"), *[("en", text) for text in card["short"]],
                       *list(chunks("意思是。" + card["meaning"]))]),
            ("怎样回应", [*list(chunks("希望的回答或动作。" + card["response_zh"])),
                       *[("en", text) for text in card["answer"] + card.get("answer_extra", [])]]),
            ("适用场景", list(chunks("适用场景。" + card["usage"]))),
        ]
        long_pcm = bytearray()
        stage_times = []
        for name, parts in stages:
            stage_times.append(len(long_pcm) // 2 * 1000 // RATE)
            long_pcm.extend(sequence(parts))
        record = {"id": card["id"], "stages_ms": stage_times}
        for mode, pcm in [("short", short), ("lesson", bytes(long_pcm))]:
            encoded = encode_ima(pcm)
            record[mode] = {"offset": 16 + len(payload), "bytes": len(encoded),
                            "samples": len(pcm) // 2, "duration_ms": len(pcm) // 2 * 1000 // RATE,
                            "sha256": hashlib.sha256(encoded).hexdigest()}
            payload.extend(encoded)
            write_wav(OUT / f"{card['id']}-{mode}.wav", pcm)
        manifest["cards"].append(record)
        print(card["id"], record["short"]["duration_ms"], record["lesson"]["duration_ms"], flush=True)
    crc = zlib.crc32(payload)
    manifest.update(payload_bytes=len(payload), crc32=crc)
    if len(payload) + 16 > 0x3A0000:
        raise RuntimeError(f"Speech exceeds resource partition: {len(payload)}")
    (OUT / "speech.bin").write_bytes(b"SPOKEN01" + struct.pack("<II", len(payload), crc) + payload)
    (OUT / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    print(f"Speech: {len(payload):,} bytes; {len(payload) / (RATE / 2):.1f} seconds")


if __name__ == "__main__":
    main()
