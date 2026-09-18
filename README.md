<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Teenieping English cards

An independent AI Passport application with 17 classroom English scenes. Every scene uses a different Teenieping character, following the first 17 characters of the original Teenieping project. ChatGPT Create image provides four timed poses for each scene.

UP/DOWN changes cards. Short OK plays the English phrase and illustration sequence; another short press stops. Long OK explains the scene, instruction, expected response and usage context in English and Mandarin.

The Grigio English application remains in the separate `ai-passport-spoken-english` project. This project has independent assets, source code and build outputs.

- [Controls, ordered characters, assets and validation](docs/spoken-guide.md)
- [Build and test](docs/development/build-and-test.md)
- [Protected identity and Recovery contract](docs/development/ble-recovery-compatibility.md)

After activating ESP-IDF 5.5.3, run `./tools/validate.sh`. The verified artifact is `build/FoloToy-AI-Passport-full.bin`. On an already provisioned device, use Recovery installation or verified segmented flashing; do not raw-flash the merged artifact across the protected identity area.
