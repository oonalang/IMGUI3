# Video playback and transition fixes

1. Login uses the Hachiware embedded clip and its matching still fallback.
2. Loading uses the Arknights embedded clip only. The old menu art fallback is removed.
3. Distinct SurfaceTextures prevent old Hachiware frames from being recycled into Arknights playback. Only one MediaPlayer is running at a time.
4. Video shader performs an unblurred single texture sample.
5. Video texture resizes correctly for 640x360 login and 960x540 loading, avoiding downsampling of the Arknights clip.
6. Videos remain un-tinted by full-card glass layers or login gradients.
7. The Android downloader dialog no longer asks for blurred window backgrounds.

Validation: headless loading and login ImGui tests pass. Android JNI code passes a host-side syntax check with the project's Android stubs. Real-device playback requires an Android build and run.
