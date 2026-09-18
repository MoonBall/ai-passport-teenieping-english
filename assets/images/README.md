<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Images

Store reusable source images and generated display assets here.

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

The supplied lesson table, card metadata and ChatGPT Create image storyboards live in `spoken/`. Run `tools/generate_spoken_visuals.py` to crop four timed frames for each of the 17 cards.

Character references and the ordered scene mapping are stored in `spoken/characters/`. Each of the 17 scenes uses a distinct character, following the first 17 entries of the original Teenieping project. Only the newly generated Teenieping storyboards are included in this project.
