# Loading screen media

`System/UI/LoadingMedia.h` plays two files and turns the video into an ImGui texture.
It looks for them in this order.

## 1. Drop them on the device (no rebuild)

| media | accepted file names |
| --- | --- |
| video background | `loading.mp4`, `velina-aegard.3840x2160.mp4` |
| loading audio | `loading.mp3`, `lofi.mp3` |

Search order:

1. `/storage/emulated/0/Android/data/com.garena.game.codm/files/ethnir/<name>`
2. `/storage/emulated/0/Android/data/com.garena.game.codm/files/<name>`
3. `/storage/emulated/0/Android/media/com.garena.game.codm/ethnir/<name>`
4. `/sdcard/Android/data/com.garena.game.codm/files/ethnir/<name>`
5. `/sdcard/Download/<name>`
6. `/sdcard/Movies/<name>`
7. `$TMPDIR/<name>` (the app cache directory)
8. `/data/local/tmp/<name>`

Every entry is tried with the renamed file (`loading.mp4` / `loading.mp3`) first
and the original repo name second, so a file copied straight out of
`eliwoahzja/media-files` works without renaming.

Push them with adb:

```
adb shell mkdir -p /storage/emulated/0/Android/data/com.garena.game.codm/files/ethnir
adb push velina-aegard.3840x2160.mp4 /storage/emulated/0/Android/data/com.garena.game.codm/files/ethnir/loading.mp4
adb push lofi.mp3                      /storage/emulated/0/Android/data/com.garena.game.codm/files/ethnir/loading.mp3
```

The 3840x2160 file decodes at 4K on the device GPU. If the loader costs too much,
re-encode it to 1280x720 with the same name and nothing else changes.

Nothing has to be embedded for this to work: the media layer always tries the
disk paths above first and only falls back to compiled-in bytes. Files copied to
`/sdcard/Download` (for example from a file manager) are picked up too.

## 2. Embed it in the binary (optional)

Without a file on disk, the loader falls back to byte arrays compiled into
`.rodata`. Generate them once from this folder:

```
cd src/main/jni/Assets
xxd -i -n ethnir_loading_video loading.mp4 | sed 's/^unsigned char/inline const unsigned char/; s/^unsigned int/inline const unsigned int/' > LoadingAssets.h
xxd -i -n ethnir_loading_audio loading.mp3 | sed 's/^unsigned char/inline const unsigned char/; s/^unsigned int/inline const unsigned int/' >> LoadingAssets.h
```

`LoadingMedia.h` picks the header up through `__has_include`. Without it the
disk paths above still drive the loader; only the last-resort copy is missing, and
with no media at all the loader draws its procedural backdrop. Keep the arrays
under a few megabytes - a 30 MB mp4 becomes a ~135 MB translation unit and will
not compile on a phone-sized toolchain, so embed a short re-encoded clip rather
than `velina-aegard.3840x2160.mp4`.

## Android APIs used

- `android.media.MediaPlayer` with a `Surface` built from `SurfaceTexture` bound to
  `GL_TEXTURE_EXTERNAL_OES`, decoded each frame in `updateTexImage()` and blitted
  into an RGBA texture through a GLES3 `samplerExternalOES` pass, so ImGui can
  sample it with the normal `ImTextureID` path.
- A second `MediaPlayer` for the audio file, started when the loader's first frame
  ticks (same thread as the video, so both share one JNI attach) and released when
  the loader finishes.
- Video size can arrive a frame or two after `prepare()` on some decoders, so the
  RGBA target is created from whatever `getVideoWidth()`/`getVideoHeight()` report
  on each update until it is non-zero.
- Both go through JNI (`JavaVM* jvm` from `Includes/Jni_Stuff.h`). On a host build
  every entry point compiles to a no-op.
