# Video assets

Both MP4s are embedded as C++ arrays in `LoginAssets.h` and `LoadingAssets.h`.
The copy of each clip under `media-files/` is not automatically synchronized with these headers.

Bundled source resolutions:
- `hachiware_login.mp4`: 640x360
- `arknight_card.mp4`: 960x540

To bundle original HD footage, replace the MP4s, run `python tools/embed_videos.py` from the project root, and rebuild.
Alternatively use the device-side `_hd.mp4` or `_4k.mp4` overrides described in `VIDEO_CONFIG_FIXES.md`.
Renderer caps the intermediate texture at 1920x1080 for Android GPU compatibility.
