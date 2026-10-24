# Login and slider UI update

- Login button: idle, progress fill, success, and failure states. Authentication runs through a background pthread, not the rendering frame. A success state is shown only after the original licence service returns `OK`.
- Login illustration: a single muted Hachiware MP4 decoder reused by the login-card artwork and the loading card. A still frame from the same video appears before playback is ready. The previous static Miku artwork is not included by Main.cpp.
- Numeric sliders: double tap the control to focus an editable numeric field. Apply with the arrow or keyboard Enter, or cancel with Escape. Values are checked and clamped to each slider's bounds. Both raspody and custom ImGui slider styles use the same helper.
- Old unused Arknights MP4 files removed. Existing anti-login/session hooks and in-game feature logic were not changed.

Test status: host-side login-card UI, loading-screen render checks, numeric parsing (10 cases), and syntax checks for the updated widgets passed. No Android NDK build or live-device test was possible in this environment.
