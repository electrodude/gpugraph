/*
 * Nuklear - v1.17 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 * modified to use GLFW events 2016-2017 by Albert Emanuel Milani
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_GLFW_GL2_H_
#define NK_GLFW_GL2_H_

#include <GLFW/glfw3.h>

struct nk_glfw_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint font_tex;
};

struct nk_glfw {
    GLFWwindow *win;
    int width, height;
    int display_width, display_height;
    struct nk_glfw_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_vec2 fb_scale;

    nk_handle userdata;

    void (*glfw_key_callback)(struct nk_glfw *win, int key, int scancode, int action, int mods);
    void (*glfw_char_callback)(struct nk_glfw *win, unsigned int codepoint);
    void (*glfw_mousebutton_callback)(struct nk_glfw *win, int button, int action, int mods);
    void (*glfw_cursorpos_callback)(struct nk_glfw *win, double x, double y);
    void (*glfw_scroll_callback)(struct nk_glfw *win, double xoff, double yoff);
};

NK_API struct nk_context   *nk_glfw3_new(struct nk_glfw *glfw, const char *title);
NK_API void                 nk_glfw3_destroy(struct nk_glfw *glfw);

NK_API void                 nk_glfw3_font_stash_begin(struct nk_glfw *glfw, struct nk_font_atlas **atlas);
NK_API void                 nk_glfw3_font_stash_end(struct nk_glfw *glfw);

NK_API void                 nk_glfw3_new_frame(struct nk_glfw *glfw);
NK_API void                 nk_glfw3_render(struct nk_glfw *glfw, enum nk_anti_aliasing , int max_vertex_buffer, int max_element_buffer);

#endif

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_GLFW_GL2_IMPLEMENTATION

NK_API void                 nk_glfw3_key_callback(GLFWwindow *win, int key, int scancode, int action, int mods);
NK_API void                 nk_glfw3_char_callback(GLFWwindow *win, unsigned int codepoint);
NK_API void                 nk_glfw3_mousebutton_callback(GLFWwindow *win, int button, int action, int mods);
NK_API void                 nk_glfw3_cursorpos_callback(GLFWwindow *win, double x, double y);
NK_API void                 nk_glfw3_scroll_callback(GLFWwindow *win, double xoff, double yoff);

struct nk_glfw_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

NK_INTERN void
nk_glfw3_device_upload_atlas(struct nk_glfw *glfw, const void *image, int width, int height)
{
    struct nk_glfw_device *dev = &glfw->ogl;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
}

NK_API void
nk_glfw3_render(struct nk_glfw *glfw, enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer)
{
    /* setup global state */
    struct nk_glfw_device *dev = &glfw->ogl;
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_TRANSFORM_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* setup viewport/project */
    glViewport(0,0,(GLsizei)glfw->display_width,(GLsizei)glfw->display_height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, glfw->width, glfw->height, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    {
        GLsizei vs = sizeof(struct nk_glfw_vertex);
        size_t vp = offsetof(struct nk_glfw_vertex, position);
        size_t vt = offsetof(struct nk_glfw_vertex, uv);
        size_t vc = offsetof(struct nk_glfw_vertex, col);

        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        const nk_draw_index *offset = NULL;
        struct nk_buffer vbuf, ebuf;

        /* fill convert configuration */
        struct nk_convert_config config;
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, position)},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, uv)},
            {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_glfw_vertex, col)},
            {NK_VERTEX_LAYOUT_END}
        };
        NK_MEMSET(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct nk_glfw_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct nk_glfw_vertex);
        config.null = dev->null;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = AA;
        config.line_AA = AA;

        /* convert shapes into vertexes */
        nk_buffer_init_default(&vbuf);
        nk_buffer_init_default(&ebuf);
        nk_convert(&glfw->ctx, &dev->cmds, &vbuf, &ebuf, &config);

        /* setup vertex buffer pointer */
        {const void *vertices = nk_buffer_memory_const(&vbuf);
        glVertexPointer(2, GL_FLOAT, vs, (const void*)((const nk_byte*)vertices + vp));
        glTexCoordPointer(2, GL_FLOAT, vs, (const void*)((const nk_byte*)vertices + vt));
        glColorPointer(4, GL_UNSIGNED_BYTE, vs, (const void*)((const nk_byte*)vertices + vc));}

        /* iterate over and execute each draw command */
        offset = (const nk_draw_index*)nk_buffer_memory_const(&ebuf);
        nk_draw_foreach(cmd, &glfw->ctx, &dev->cmds) {
            if (!cmd->elem_count)
                continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x * glfw->fb_scale.x),
                (GLint)((glfw->height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * glfw->fb_scale.y),
                (GLint)(cmd->clip_rect.w * glfw->fb_scale.x),
                (GLint)(cmd->clip_rect.h * glfw->fb_scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(&glfw->ctx);
        nk_buffer_free(&vbuf);
        nk_buffer_free(&ebuf);
    }

    /* default OpenGL state */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

NK_API void
nk_glfw3_key_callback(GLFWwindow *win, int key, int scancode, int action, int mods)
{
    struct nk_glfw *glfw = glfwGetWindowUserPointer(win);
    if (glfw->glfw_key_callback)
        glfw->glfw_key_callback(glfw, key, scancode, action, mods);

    struct nk_context *ctx = &glfw->ctx;

    int down = action == GLFW_PRESS || action == GLFW_REPEAT;

    if (!(mods & GLFW_MOD_CONTROL)) {
        switch (key) {
            case GLFW_KEY_DELETE       : nk_input_key(ctx, NK_KEY_DEL            , down); break;
            case GLFW_KEY_ENTER        : nk_input_key(ctx, NK_KEY_ENTER          , down); break;
            case GLFW_KEY_TAB          : nk_input_key(ctx, NK_KEY_TAB            , down); break;
            case GLFW_KEY_BACKSPACE    : nk_input_key(ctx, NK_KEY_BACKSPACE      , down); break;
            case GLFW_KEY_UP           : nk_input_key(ctx, NK_KEY_UP             , down); break;
            case GLFW_KEY_DOWN         : nk_input_key(ctx, NK_KEY_DOWN           , down); break;
            case GLFW_KEY_LEFT         : nk_input_key(ctx, NK_KEY_LEFT           , down); break;
            case GLFW_KEY_RIGHT        : nk_input_key(ctx, NK_KEY_RIGHT          , down); break;
            case GLFW_KEY_HOME         : nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down);
                                         nk_input_key(ctx, NK_KEY_SCROLL_START   , down); break;
            case GLFW_KEY_END          : nk_input_key(ctx, NK_KEY_TEXT_LINE_END  , down);
                                         nk_input_key(ctx, NK_KEY_SCROLL_END     , down); break;
            case GLFW_KEY_PAGE_DOWN    : nk_input_key(ctx, NK_KEY_SCROLL_DOWN    , down); break;
            case GLFW_KEY_PAGE_UP      : nk_input_key(ctx, NK_KEY_SCROLL_UP      , down); break;
            case GLFW_KEY_LEFT_SHIFT   : nk_input_key(ctx, NK_KEY_SHIFT          , down); break;
            case GLFW_KEY_RIGHT_SHIFT  : nk_input_key(ctx, NK_KEY_SHIFT          , down); break;
            case GLFW_KEY_LEFT_CONTROL : nk_input_key(ctx, NK_KEY_CTRL           , down); break;
            case GLFW_KEY_RIGHT_CONTROL: nk_input_key(ctx, NK_KEY_CTRL           , down); break;

        }
    } else {
        switch (key) {
            case GLFW_KEY_A        : nk_input_key(ctx, NK_KEY_TEXT_SELECT_ALL, down); break;
            case GLFW_KEY_C        : nk_input_key(ctx, NK_KEY_COPY           , down); break;
            case GLFW_KEY_V        : nk_input_key(ctx, NK_KEY_PASTE          , down); break;
            case GLFW_KEY_X        : nk_input_key(ctx, NK_KEY_CUT            , down); break;
            case GLFW_KEY_Z        : nk_input_key(ctx, NK_KEY_TEXT_UNDO      , down); break;
            case GLFW_KEY_R        : nk_input_key(ctx, NK_KEY_TEXT_REDO      , down); break;
            case GLFW_KEY_LEFT     : nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT , down); break;
            case GLFW_KEY_RIGHT    : nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down); break;
            case GLFW_KEY_B        : nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down); break;
            case GLFW_KEY_E        : nk_input_key(ctx, NK_KEY_TEXT_LINE_END  , down); break;
        }
    }
}

NK_API void
nk_glfw3_char_callback(GLFWwindow *win, unsigned int codepoint)
{
    struct nk_glfw *glfw = glfwGetWindowUserPointer(win);
    if (glfw->glfw_char_callback)
        glfw->glfw_char_callback(glfw, codepoint);

    if (codepoint < 0x10FFFF) {
        nk_input_unicode(&glfw->ctx, codepoint);
    }
}

NK_API void
nk_glfw3_mousebutton_callback(GLFWwindow *win, int button, int action, int mods)
{
    struct nk_glfw *glfw = glfwGetWindowUserPointer(win);
    if (glfw->glfw_mousebutton_callback)
        glfw->glfw_mousebutton_callback(glfw, button, action, mods);

    enum nk_buttons id;
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT  : id = NK_BUTTON_LEFT  ; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: id = NK_BUTTON_MIDDLE; break;
        case GLFW_MOUSE_BUTTON_RIGHT : id = NK_BUTTON_RIGHT ; break;
        default: return;
    }
    double x, y;
    glfwGetCursorPos(win, &x, &y);
    nk_input_button(&glfw->ctx, id, (int)x, (int)y, action == GLFW_PRESS);
}

NK_API void
nk_glfw3_cursorpos_callback(GLFWwindow *win, double x, double y)
{
    struct nk_glfw *glfw = glfwGetWindowUserPointer(win);
    if (glfw->glfw_cursorpos_callback)
        glfw->glfw_cursorpos_callback(glfw, x, y);

    nk_input_motion(&glfw->ctx, (int)x, (int)y);

    struct nk_context *ctx = &glfw->ctx;

    /* optional grabbing behavior */
    /*
    if (ctx->input.mouse.grab)
        glfwSetInputMode(glfw->win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    else if (ctx->input.mouse.ungrab)
        glfwSetInputMode(glfw->win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (ctx->input.mouse.grabbed) {
        glfwSetCursorPos(glfw->win, ctx->input.mouse.prev.x, ctx->input.mouse.prev.y);
        ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
        ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
    }
    */
}

NK_API void
nk_glfw3_scroll_callback(GLFWwindow *win, double xoff, double yoff)
{
    struct nk_glfw *glfw = glfwGetWindowUserPointer(win);
    if (glfw->glfw_scroll_callback && !nk_item_is_any_active(&glfw->ctx)) {
        glfw->glfw_scroll_callback(glfw, xoff, yoff);
    } else {
        (void)xoff;
        nk_input_scroll(&glfw->ctx, (float)yoff);
    }
}

NK_INTERN void
nk_glfw3_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    struct nk_glfw *glfw = usr.ptr;
    const char *text = glfwGetClipboardString(glfw->win);
    if (text)
        nk_textedit_paste(edit, text, nk_strlen(text));
}

NK_INTERN void
nk_glfw3_clipboard_copy(nk_handle usr, const char *text, int len)
{
    struct nk_glfw *glfw = usr.ptr;
    char *str = 0;
    if (!len)
        return;
    str = (char*)malloc((size_t)len+1);
    if (!str)
        return;
    NK_MEMCPY(str, text, (size_t)len);
    str[len] = '\0';
    glfwSetClipboardString(glfw->win, str);
    free(str);
}

NK_API struct nk_context*
nk_glfw3_new(struct nk_glfw *glfw, const char *title)
{
    GLFWwindow *win = glfwCreateWindow(glfw->width, glfw->height, title, NULL, NULL);

    if (!win) {
        fprintf(stderr, "glfwCreateWindow failed\n");
        return NULL;
    }

    glfw->win = win;

    glfwSetWindowUserPointer(glfw->win, glfw);

    glfwGetWindowSize(glfw->win, &glfw->width, &glfw->height);
    glfwGetFramebufferSize(glfw->win, &glfw->display_width, &glfw->display_height);

    glfwSetScrollCallback(glfw->win, nk_glfw3_scroll_callback);
    glfwSetKeyCallback(glfw->win, nk_glfw3_key_callback);
    glfwSetCharCallback(glfw->win, nk_glfw3_char_callback);
    glfwSetMouseButtonCallback(glfw->win, nk_glfw3_mousebutton_callback);
    glfwSetCursorPosCallback(glfw->win, nk_glfw3_cursorpos_callback);

    nk_init_default(&glfw->ctx, 0);
    glfw->ctx.clip.copy = nk_glfw3_clipboard_copy;
    glfw->ctx.clip.paste = nk_glfw3_clipboard_paste;
    glfw->ctx.clip.userdata = nk_handle_ptr(glfw);
    nk_buffer_init_default(&glfw->ogl.cmds);

    glfwMakeContextCurrent(glfw->win);

    return &glfw->ctx;
}

NK_API void
nk_glfw3_font_stash_begin(struct nk_glfw *glfw, struct nk_font_atlas **atlas)
{
    nk_font_atlas_init_default(&glfw->atlas);
    nk_font_atlas_begin(&glfw->atlas);
    *atlas = &glfw->atlas;
}

NK_API void
nk_glfw3_font_stash_end(struct nk_glfw *glfw)
{
    const void *image; int w, h;
    image = nk_font_atlas_bake(&glfw->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_glfw3_device_upload_atlas(glfw, image, w, h);
    nk_font_atlas_end(&glfw->atlas, nk_handle_id((int)glfw->ogl.font_tex), &glfw->ogl.null);
    if (glfw->atlas.default_font)
        nk_style_set_font(&glfw->ctx, &glfw->atlas.default_font->handle);
}

NK_API void
nk_glfw3_new_frame(struct nk_glfw *glfw)
{
    int i;
    struct nk_context *ctx = &glfw->ctx;
    struct GLFWwindow *win = glfw->win;

    glfwGetWindowSize(win, &glfw->width, &glfw->height);
    glfwGetFramebufferSize(win, &glfw->display_width, &glfw->display_height);
    glfw->fb_scale.x = (float)glfw->display_width/(float)glfw->width;
    glfw->fb_scale.y = (float)glfw->display_height/(float)glfw->height;

    glfwMakeContextCurrent(glfw->win);
}

NK_API
void nk_glfw3_destroy(struct nk_glfw *glfw)
{
    struct nk_glfw_device *dev = &glfw->ogl;
    nk_font_atlas_clear(&glfw->atlas);
    nk_free(&glfw->ctx); // ?
    glDeleteTextures(1, &dev->font_tex);
    nk_buffer_free(&dev->cmds);
    glfwDestroyWindow(glfw->win);
    NK_MEMSET(&glfw, 0, sizeof(glfw));
}

#endif
