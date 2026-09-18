<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Music and Sound Effects

Store reusable music and sound-effect sources here.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.

Short instructions and complete lessons live in `spoken/`. `tools/generate_spoken_audio.py` synthesizes Samantha and Tingting speech; the resulting `speech.bin` occupies the speech resource partition.
