<p align="right"><a href="spoken-guide.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Teenieping spoken English cards

An offline AI Passport application containing the 17 classroom phrases in the supplied lesson table. Each card retains its scene, English instruction, expected response or action, and usage context.

## Independent project and character order

This is `ai-passport-teenieping-english`; the original `ai-passport-spoken-english` project remains intact. Code, assets, build outputs and future edits are independent.

The order from the original `ai-passport-teenieping` card manifest is frozen in `characters/source-order.json`. `characters/order.json` records the scene mapping, reference provenance and hashes. Each card shows its character name; lesson content and narration retain the previous English version, with Heartsping as the name example and age five matching the raised-hand illustration. Validation rejects duplicate characters, wrong ordering, corrupt references and inconsistent storyboard mappings.

| Order | Character | English scene |
| --- | --- | --- |
| 1 | heartsping | What's your name? |
| 2 | shimmerping | How are you? |
| 3 | sparkleping | How old are you? |
| 4 | twinkleping | Listen. / Look at me. |
| 5 | auroraping | Read after me. |
| 6 | princeping | Circle. / Draw a line. |
| 7 | ellaping | Can you try? |
| 8 | bellaping | What color is it? |
| 9 | togetherping | I don't know. |
| 10 | mightyping | Can you say that again? |
| 11 | giverping | Slow down, please. |
| 12 | dingdongping | Can you type it? |
| 13 | hopping | Can you hear / see me? |
| 14 | petitping | The line is not OK! |
| 15 | allureping | Good job! / Well done! |
| 16 | stickerping | High five! |
| 17 | lucyping | See you next time! |

## Controls

- UP/DOWN changes cards with wraparound and stops current playback.
- A short OK press reads the English instruction. Slash-separated alternatives are spoken separately. A second short press stops playback.
- A long OK press starts the current card's complete bilingual lesson: scene, instruction and meaning, expected response or action, and usage context. Long-pressing during short playback starts the lesson from its beginning.
- Boot and navigation are silent. Finished playback does not repeat. Animation can finish after a short utterance, without replaying the audio.
- Name and age answers are examples; children should use their own details. The connection card retains the supplied wording and adds a more natural alternative during explanation.
- The display dims after 60 idle seconds. A button restores brightness. Holding UP for five seconds at boot enters permanent Recovery on devices where its image is installed.

## Assets and reproducibility

The supplied lesson table is `assets/images/spoken/lesson-reference.png`. Original Teenieping storyboards generated with ChatGPT Create image are in `assets/images/spoken/chatgpt/`.

`lessons.json` contains all text and action descriptions. macOS Samantha reads English and Tingting reads Mandarin; these are synthesized teaching voices, not character or actor recordings. Speech is normalized and stored as 16 kHz mono IMA ADPCM. The worker duplicates mono samples for the board's stereo codec path.

All visuals for the 17 cards come from Create image on the ChatGPT website, using 17 distinct characters referenced from the original Teenieping project in its existing order. `storyboards.json` maps image-grid cells to cards. Each card displays four key poses for one second each, approximately four seconds in total. This is a four-frame storyboard rather than a generated continuous video. `frame-manifest.json` records provenance, file hashes and frame timing. Generation and delivery checks fail if any card lacks its storyboard.

Cropped images become 216 by 144 baseline JPEGs, centered with their aspect ratio preserved. Tiled JPEG decoding suits the board without PSRAM. Generated dividers can differ from a uniform grid, so the mapping records measured grid boundaries and crops with an inset to exclude neighboring cells.

```bash
python3 tools/generate_spoken_audio.py
python3 tools/generate_spoken_visuals.py
python3 tools/generate_spoken_font.py
./tools/validate.sh
```

The speech resource starts at `0x360000` and ends before `0x700000`. The application remains below 3 MiB. Preserve device identity at `0x356000` and permanent Recovery at `0x700000`; flash verified segments only. Never write a merged image across the protected gap.

## Validation

Host tests cover wraparound, silent cancellation, switching short playback to a lesson, stale completion rejection, speech-stage boundaries, and finishing visual motion after speech ends. Asset checks validate every clip boundary and hash, the four lesson sections, resource CRC and capacity, and embedded firmware speech bytes.

USB accepts `FAP_SCREENSHOT_V1`, `FAP_KEY_UP_V1`, `FAP_KEY_DOWN_V1`, `FAP_KEY_OK_V1`, and `FAP_KEY_OK_LONG_V1`, followed by a newline. Test events follow the physical-button event queue. They cannot establish physical button feel or subjective speaker quality.

Build: PASS (complete `./tools/validate.sh`). Host tests: PASS. Device tests: PASS (user verified). Application: 1,069,232 bytes; merged firmware: 6,906,449 bytes; speech: 3,367,505 bytes.

The Teenieping firmware was installed by USB and all four written segments were verified. Device identity and the reserved Recovery region remained unchanged. The user subsequently confirmed successful testing on the device. The original Grigio English project and firmware remain available separately.

Unverified: automated device playback checks did not run because the device disconnected before the script started. The tested device had no Recovery image installed; preserving its reserved region does not install Recovery.

Run `tools/test_spoken_device.py --serial <device-serial>` for full acceptance. The default plays all 17 cards. `--lesson-cards 2 13 --capture-lesson-stages` performs selected lesson regression after display changes while retaining all 17 short-phrase checks.

## Implementation decisions

JPEG header inspection reserves a 4 KiB local buffer inside LVGL, even when the image comes from a C array. The initial 4 KiB main-task stack overflowed in `decoder_info` on the board. Main initialization uses an 8 KiB stack; the deeper LVGL render and USB screenshot paths use 12 KiB; rendering still decodes small tiles, preserving RAM for speech and UI. The verified ELF is retained for symbolizing future board failures.

Speech uses a separate resource partition because complete bilingual lessons would consume the application limit. The resource header carries length and CRC, and boot rejects missing or mismatched speech instead of playing unrelated bytes.
