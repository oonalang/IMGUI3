#include <cstdint>
#include "System/UI/LoadingMedia.h"

int main()
{
    ui_media::Tick();
    ui_media::Tick();
    const bool video = ui_media::VideoReady();
    const bool audio = ui_media::StartAudio();
    const float aspect = ui_media::VideoAspect();
    ui_media::StopAudio();
    ui_media::Shutdown();
    ui_media::g_videoW = 0;
    ui_media::g_videoH = 0;
    ui_media::g_inited = false;
    return (video || audio || aspect < 0.0f) ? 1 : 0;
}
