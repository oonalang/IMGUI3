#include "backdrop_blur.h"

#include <GLES3/gl3.h>

#include <cstdint>

// The capture half of the backdrop blur. Called once per frame from the swap
// hook, on the render thread, while the menu is on screen.
//
// Cost control is the whole design: the game frame is copied at 1/8 resolution
// (a 1080p frame becomes 240x135 = 130 KB), smoothed twice through bilinear
// filtering, and never read back to the CPU. Every pass is a GPU blit inside
// the frame that is already being submitted, so the work does not grow with the
// panel size and does not touch the game's own rendering.
namespace backdrop_blur
{
    namespace
    {
        GLuint g_texA = 0, g_texB = 0;
        GLuint g_fboA = 0, g_fboB = 0;
        int    g_texAW = 0, g_texAH = 0;
        int    g_texBW = 0, g_texBH = 0;
        int    g_allocW = 0, g_allocH = 0;
        int    g_failures = 0;

        constexpr int kDownscale  = 8;   // game frame -> the sampled copy
        constexpr int kMidDivide  = 4;   // the copy -> the pass that smooths it
        constexpr int kMaxFailures = 3;  // a device that refuses FBOs stops retrying

        bool MakeTarget(GLuint& tex, GLuint& fbo, int w, int h)
        {
            if (tex != 0) { glDeleteTextures(1, &tex); tex = 0; }
            if (fbo != 0) { glDeleteFramebuffers(1, &fbo); fbo = 0; }

            glGenTextures(1, &tex);
            if (tex == 0) return false;
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenFramebuffers(1, &fbo);
            if (fbo == 0) { glDeleteTextures(1, &tex); tex = 0; return false; }

            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
            const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                glDeleteFramebuffers(1, &fbo);
                glDeleteTextures(1, &tex);
                fbo = 0;
                tex = 0;
                return false;
            }
            return true;
        }

        bool EnsureTargets(int screenW, int screenH)
        {
            if (screenW <= 0 || screenH <= 0) return false;

            int aw = screenW / kDownscale;
            int ah = screenH / kDownscale;
            if (aw < 8) aw = 8;
            if (ah < 8) ah = 8;

            int bw = aw / kMidDivide;
            int bh = ah / kMidDivide;
            if (bw < 2) bw = 2;
            if (bh < 2) bh = 2;

            if (g_texA != 0 && g_texB != 0 && g_allocW == screenW && g_allocH == screenH)
                return true;

            if (!MakeTarget(g_texA, g_fboA, aw, ah)) return false;
            if (!MakeTarget(g_texB, g_fboB, bw, bh)) {
                glDeleteFramebuffers(1, &g_fboA);
                glDeleteTextures(1, &g_texA);
                g_fboA = 0; g_texA = 0;
                return false;
            }

            g_texAW = aw; g_texAH = ah;
            g_texBW = bw; g_texBH = bh;
            g_allocW = screenW;
            g_allocH = screenH;
            return true;
        }

        void Copy(GLuint readFbo, int rw, int rh, GLuint drawFbo, int dw, int dh)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
            glBlitFramebuffer(0, 0, rw, rh, 0, 0, dw, dh, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }
    }

    // Keeps the blurred copy current. Safe to call every frame: the targets are
    // rebuilt only when the surface size changes.
    void Capture(int screenW, int screenH)
    {
        if (screenW <= 0 || screenH <= 0) return;
        if (g_failures >= kMaxFailures) return;
        if (!EnsureTargets(screenW, screenH)) { g_failures++; return; }

        GLint prevRead = 0, prevDraw = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevRead);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDraw);

        // The game frame is the default framebuffer here (ImGui has not drawn
        // yet this frame). 1/8 -> 1/32 -> 1/8: the bilinear pass in the middle
        // is what turns a mosaic into a blur.
        Copy(0, screenW, screenH, g_fboA, g_texAW, g_texAH);
        Copy(g_fboA, g_texAW, g_texAH, g_fboB, g_texBW, g_texBH);
        Copy(g_fboB, g_texBW, g_texBH, g_fboA, g_texAW, g_texAH);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)prevRead);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)prevDraw);
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevDraw);

        g_texture = (ImTextureID)(intptr_t)g_texA;
        g_screenW = screenW;
        g_screenH = screenH;
        g_ready   = true;
    }

    void Release()
    {
        if (g_fboA != 0) { glDeleteFramebuffers(1, &g_fboA); g_fboA = 0; }
        if (g_fboB != 0) { glDeleteFramebuffers(1, &g_fboB); g_fboB = 0; }
        if (g_texA != 0) { glDeleteTextures(1, &g_texA); g_texA = 0; }
        if (g_texB != 0) { glDeleteTextures(1, &g_texB); g_texB = 0; }
        g_texAW = g_texAH = g_texBW = g_texBH = 0;
        g_allocW = g_allocH = 0;
        g_texture = nullptr;
        g_ready   = false;
    }
}
