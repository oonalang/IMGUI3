# Video resolution, transition and config fixes

## Video sources
The bundled Hachiware clip is **640x360** and Arknights is **960x540** (verified with ffprobe). The source files were already lower resolution, so a 1080p renderer cannot recreate detail that is not in the clips.

- The OpenGL render target now supports source frames up to **1920x1080**, with 4K decoded sources downsampled to 1080p to avoid a 33 MB GPU texture allocation on Android.
- The fragment shader is a direct high-precision sampler without blur.
- Login success preloads the Arknights media before the loading card begins. It opens once a frame is ready after the short success animation, or uses a persistent gradient if video preparation fails. The card never switches from gradient to video halfway through the progress animation.
- Loading still does not show unrelated login or menu artwork.

### Replacing your low-resolution clips
Option A, no rebuild: copy the original full-resolution clip as `hachiware_login_hd.mp4` and/or `arknight_card_hd.mp4` to the game's `files/ethnir/` folder (or to `/sdcard/Download/`) before launching. These explicit override names are checked before the compiled-in low-resolution clips. You can use `_4k.mp4` in place of `_hd.mp4`. Playback source size depends on the phone's hardware decoder; the intermediate GL texture caps at 1080p.

Option B, rebuild: put the original HD files at `media-files/hachiware_login.mp4` and `media-files/arknight_card.mp4`; run `python tools/embed_videos.py`, then rebuild. Simply swapping MP4s without regenerating `src/main/jni/Assets/*Assets.h` does not change the embedded clips.

## Profiles
- New config name is empty by default. Typing `F` saves `F.json`, not `EthnirF.json`.
- Saved configurations are listed from `configs/` and do not create a phantom default profile.
- `Manage active profile` lets you select a startup auto-load config, rename it, or delete it with confirmation. To select a different profile, use **LOAD** on its row.
- `autoload.profile` stores the selected startup profile name. Auto-load happens once when the main menu first initializes after login. Rename follows the auto-load selection; deleting the selected config disables auto-load.
- File names reject path separators and invalid characters to protect other files.

The Android app and game aren't runnable on this build host. Host-only tests check file operations and UI frame selection; confirm playback on your device after rebuilding.
