# Loading screen media

`System/UI/LoadingMedia.h` plays the loading card's video and turns it into an
ImGui texture. The card's background is the video; the fullscreen backdrop
behind the card is the menu artwork in `IMAGE/bg.h`.

## The card video

The clip is compiled into the binary from `LoadingAssets.h`: the Arknights
gentle-glow encounter background, 960x540, 12 s, no audio, 653 KB. It is the
source for `media-files/arknight_card.mp4`.

On the first run the media layer writes those bytes to

```
/storage/emulated/0/Android/data/com.garena.game.codm/files/ethnir/arknights-eyjafjallas-gentle-glow.1920x1080.mp4
```

and plays them from there. That directory is the first one searched, so the
compiled-in clip is what the card plays. Nothing has to be pushed to the device.

To use a different clip, overwrite that file (any resolution up to 1920x1080,
H.264 in an mp4). A file named `loading.mp4` is also accepted. The full search
order is:

1. `/storage/emulated/0/Android/data/com.garena.game.codm/files/ethnir/<name>`
2. `/storage/emulated/0/Android/data/com.garena.game.codm/files/<name>`
3. `/storage/emulated/0/Android/media/com.garena.game.codm/ethnir/<name>`
4. `/sdcard/Android/data/com.garena.game.codm/files/ethnir/<name>`
5. `/sdcard/Download/<name>`
6. `/sdcard/Movies/<name>`
7. `$TMPDIR/<name>` (the app cache directory)
8. `/data/local/tmp/<name>`

with the arknights file name tried before `loading.mp4` in each of them.

```bash
adb push my-clip.mp4 /storage/emulated/0/Android/data/com.garena.game.codm/files/ethnir/arknights-eyjafjallas-gentle-glow.1920x1080.mp4
```

The video is drawn into the card and nowhere else. The frame behind it is the
menu artwork, dimmed, so the card stays the focal point.

## Regenerating the clip

`media-files/arknight.mp4` is the full source (1920x1080, 60 fps, 24 s). The
committed clip is a 12 s cut at 960x540, which is the card's own size:

```bash
ffmpeg -i media-files/arknight.mp4 -t 12 -an \
       -vf "scale=960:540:flags=lanczos,fps=30" \
       -c:v libx264 -profile:v baseline -level 3.1 -pix_fmt yuv420p \
       -crf 31 -preset slow -movflags +faststart media-files/arknight_card.mp4

cd src/main/jni/Assets
xxd -i -n ethnir_loading_video ../../../media-files/arknight_card.mp4 \
  | sed -e 's/^unsigned char/inline const unsigned char/' \
        -e 's/^unsigned int/inline const unsigned int/' > LoadingAssets.h
```

The header is a C++17 inline variable, so every translation unit that includes
`LoadingMedia.h` shares one copy. Keep it under a few megabytes: a 30 MB mp4
becomes a ~180 MB translation unit and will not compile on a phone-sized
toolchain.

## Loading audio

| media | accepted file names |
| --- | --- |
| loading audio | `loading.mp3`, `lofi.mp3` |

Same search order, started when the loader's first frame ticks and released
when the loader ends. Nothing is compiled in for it, so the loader is silent
unless one of those files is on the device.

## Android APIs used

- `android.media.MediaPlayer` with a `Surface` built from `SurfaceTexture` bound to
  `GL_TEXTURE_EXTERNAL_OES`, decoded each frame in `updateTexImage()` and blitted
  into an RGBA texture through a GLES3 `samplerExternalOES` pass, so ImGui can
  sample it with the normal `ImTextureID` path.
- Both the video and the audio player are opened on their own threads: `prepare()`
  blocks for as long as it takes to read the file, and running that on the render
  thread froze the game.
- The video target is capped at 1920x1080. A 4K source is 33 MB of RGBA8 that
  many devices refuse to allocate.
- The login screen warms the video (`ui_media::Warmup()`) before the loader is
  shown, so the card has a frame in its texture on its first frame instead of a
  black gap followed by the artwork and a swap.
- Both go through JNI (`JavaVM* jvm` from `Includes/Jni_Stuff.h`). On a host build
  every entry point compiles to a no-op.
