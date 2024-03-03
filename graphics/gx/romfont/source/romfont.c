#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#define FIFO_SIZE (256*1024)

static void *xfb = NULL;
static GXRModeObj *vmode;
static sys_fontheader *fontdata;
static GXTexObj fonttex;
static int text_x;
static int text_y;
static int text_size;
static u32 text_color = 0xffffffff;

static inline void set_text_pos(int x, int y)
{
    text_x = x;
    text_y = y;
}

static inline void set_text_size(int size)
{
    text_size = size;
}

static inline void set_text_color(u32 color)
{
    text_color = color;
}

static void activate_font_texture()
{
    u32 texture_size;
    void *texels;

    texels = (void *)fontdata + fontdata->sheet_image;
    GX_InitTexObj(&fonttex, texels,
                  fontdata->sheet_width, fontdata->sheet_height,
                  fontdata->sheet_format, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjLOD(&fonttex, GX_LINEAR, GX_LINEAR, 0., 0., 0.,
                     GX_TRUE, GX_TRUE, GX_ANISO_1);
    GX_LoadTexObj(&fonttex, GX_TEXMAP0);

    texture_size = GX_GetTexBufferSize(fontdata->sheet_width,
                                       fontdata->sheet_height,
                                       fontdata->sheet_format,
                                       GX_FALSE, 0);
    DCStoreRange(texels, texture_size);
    GX_InvalidateTexAll();
}

static void draw_font_cell(int16_t x1, int16_t y1, uint32_t c, int16_t s1, int16_t t1)
{
    int16_t x2 = x1 + fontdata->cell_width * text_size / fontdata->cell_height;
    int16_t y2 = y1 - text_size;

    int16_t s2 = s1 + fontdata->cell_width;
    int16_t t2 = t1 + fontdata->cell_height;

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

    GX_Position2s16(x1, y2);
    GX_Color1u32(c);
    GX_TexCoord2s16(s1, t1);

    GX_Position2s16(x2, y2);
    GX_Color1u32(c);
    GX_TexCoord2s16(s2, t1);

    GX_Position2s16(x2, y1);
    GX_Color1u32(c);
    GX_TexCoord2s16(s2, t2);

    GX_Position2s16(x1, y1);
    GX_Color1u32(c);
    GX_TexCoord2s16(s1, t2);

    GX_End();
}

static void setup_font()
{
    if (SYS_GetFontEncoding() == 0) {
        fontdata = memalign(32, SYS_FONTSIZE_ANSI);
    } else {
        fontdata = memalign(32, SYS_FONTSIZE_SJIS);
    }

    SYS_InitFont(fontdata);
    activate_font_texture();

    text_size = fontdata->cell_height;
}

static void setup_gx()
{
    GXColor backgroundColor = {0, 0, 0, 255};
    void *fifoBuffer = NULL;
    Mtx mv;

    fifoBuffer = MEM_K0_TO_K1(memalign(32,FIFO_SIZE));
    memset(fifoBuffer, 0, FIFO_SIZE);

    GX_Init(fifoBuffer, FIFO_SIZE);

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S16, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, 0);

    /* This is needed so that we can specify the texture coordinates using
     * integers, where one unit in coordinate space is equivalent to a texel.
     */
    GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 1, 1);

    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

    GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetCopyClear(backgroundColor, GX_MAX_Z24);

    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, 0,
                   GX_DF_NONE, GX_AF_NONE);
    GX_SetNumTexGens(1);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

    guMtxIdentity(mv);
    guMtxTransApply(mv, mv, 0.4, 0.4, 0);
    GX_LoadPosMtxImm(mv, GX_PNMTX0);
}

static void setup_viewport()
{
    Mtx44 proj;
    u32 w, h;

    w = vmode->fbWidth;
    h = vmode->efbHeight;

    // matrix, t, b, l, r, n, f
    guOrtho(proj, 0, h, 0, w, 0, 1);
    GX_LoadProjectionMtx(proj, GX_ORTHOGRAPHIC);

    GX_SetViewport(0, 0, w, h, 0, 1);

    f32 yscale = GX_GetYScaleFactor(h, vmode->xfbHeight);
    GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0, 0, w, h);
    GX_SetDispCopySrc(0, 0, w, h);
    GX_SetDispCopyDst(w, vmode->xfbHeight);
    GX_SetCopyFilter(vmode->aa, vmode->sample_pattern, GX_TRUE, vmode->vfilter);
}

static int process_string(const char *text, bool should_draw)
{
    void *image;
    int32_t xpos, ypos, width, x;

    x = text_x;
    for (; *text != '\0'; text++) {
        char c = *text;
        if (c < fontdata->first_char) {
            continue;
        }
        SYS_GetFontTexture(c, &image, &xpos, &ypos, &width);
        if (should_draw) {
            draw_font_cell(x, text_y, text_color, xpos, ypos);
        }
        x += width * text_size / fontdata->cell_height;
    }

    return x - text_x;
}

static int draw_string(const char *text)
{
    return process_string(text, true);
}

static int string_width(const char *text)
{
    return process_string(text, false);
}

static void draw_text()
{
    int w, h, x, y;

    w = vmode->fbWidth;
    h = vmode->efbHeight;

    y = h / 2 - 100;

    set_text_size(fontdata->cell_height);
    x = (w - string_width("COLOR")) / 2;
    set_text_pos(x, y);
    set_text_color(0x0000ffff);
    x += draw_string("C");
    set_text_pos(x, y);
    set_text_color(0x00c0c0ff);
    x += draw_string("O");
    set_text_pos(x, y);
    set_text_color(0x00ff00ff);
    x += draw_string("L");
    set_text_pos(x, y);
    set_text_color(0xc0c000ff);
    x += draw_string("O");
    set_text_pos(x, y);
    set_text_color(0xff0000ff);
    x += draw_string("R");

    set_text_size(16);
    y += 100;
    x = 0;
    set_text_color(0x00ff00ff);
    set_text_pos(x, y);
    draw_string("Left aligned");

    set_text_color(0xff0000ff);
    x = w - string_width("Right aligned");
    set_text_pos(x, y);
    draw_string("Right aligned");

    x = (w - string_width("Centered")) / 2;
    set_text_pos(x, y);
    set_text_color(0xffffffff);
    draw_string("Centered");

    set_text_size(10);
    y += 100;
    x = (w - string_width("Tiny text")) / 2;
    set_text_color(0xffffffff);
    set_text_pos(x, y);
    draw_string("Tiny text");
}

int main(int argc, char **argv)
{
    VIDEO_Init();
    WPAD_Init();

    vmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
    VIDEO_Configure(vmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();

    // Wait for Video setup to complete
    VIDEO_WaitVSync();
    if (vmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    setup_gx();
    setup_font();
    setup_viewport();

    while (true) {
        WPAD_ScanPads();

        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_HOME) exit(0);

        draw_text();

        GX_DrawDone();
        GX_CopyDisp(xfb, GX_TRUE);
        GX_Flush();

        VIDEO_WaitVSync();
    }

    return 0;
}
