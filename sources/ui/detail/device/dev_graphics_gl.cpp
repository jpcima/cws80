#include "dev_graphics_gl.h"
#include "ui/cws80_ui_controller.h"
#include "ui/cws80_ui_nk.h"
#include "utility/scope_guard.h"
#include "utility/dynarray.h"
#include "utility/debug.h"
#include "utility/c++std/optional.h"
#include <GL/glew.h>
#include <fmt/format.h>
#include <unordered_set>
#include <cassert>

namespace cws80 {

#ifndef NDEBUG
enum { gl_enable_debug_output = true };
#endif

struct nk_vertex {
    f32 position[2];
    f32 uv[2];
    u8 col[4];
};

enum {
    MAX_VERTEX_MEMORY = 512 * 1024,
    MAX_ELEMENT_MEMORY = 128 * 1024,
};

#define SHADER_VERSION "#version 130\n"
#define SHADER_SOURCE(...) SHADER_VERSION #__VA_ARGS__

static const GLchar *nk_vertex_shader =
    SHADER_SOURCE(uniform mat4 ProjMtx; in vec2 Position; in vec2 TexCoord;
                  in vec4 Color; out vec2 Frag_UV; out vec4 Frag_Color; void main() {
                      Frag_UV = TexCoord;
                      Frag_Color = Color;
                      gl_Position = ProjMtx * vec4(Position.xy, 0, 1);
                  });

static const GLchar *nk_fragment_shader = SHADER_SOURCE(
    precision mediump float; uniform sampler2D Texture; in vec2 Frag_UV;
    in vec4 Frag_Color; out vec4 Out_Color;
    void main() { Out_Color = Frag_Color * texture(Texture, Frag_UV.st); });

struct GraphicsDevice_GL::Impl {
    std::unordered_set<GLuint> textures_;

    GraphicsDevice_GL *Q = nullptr;

    uint glvermaj = 0;
    uint glvermin = 0;

    bool glsetup_ = false;

    struct {
        dynarray<u8> vbufdata_;
        dynarray<u8> ebufdata_;
    } gl2_;

    struct {
        GLuint prog_ = 0;
        GLuint vert_shdr_ = 0;
        GLuint frag_shdr_ = 0;

        GLint uniform_tex_ = 0;
        GLint uniform_proj_ = 0;
        GLuint attrib_pos_ = 0;
        GLuint attrib_uv_ = 0;
        GLuint attrib_col_ = 0;

        GLuint vao_ = 0;
        GLuint vbo_ = 0;
        GLuint ebo_ = 0;
    } gl3_;

    nk_buffer cmds_{};
    dynarray<u8> cmdsdata_;
    static constexpr uint cmdslen_ = 64 * 1024;

    cxx::optional<nk_font_atlas> atlas_{};
    std::vector<nk_font *> font_;
    nk_convert_config config_{};

    nk_buffer vbuf_{};
    nk_buffer ebuf_{};

    void setup_context();
    void initialize_gl2();
    void initialize_gl3();
    void cleanup_gl2();
    void cleanup_gl3();
    void render_gl2();
    void render_gl3();

    void load_fonts(gsl::span<const FontRequest> fontreqs, const nk_rune range[]);
};

GraphicsDevice_GL::GraphicsDevice_GL(UIController &ctl)
    : GraphicsDevice(ctl)
    , P(new Impl)
{
    P->Q = this;
    P->cmdsdata_.reset(P->cmdslen_);
    nk_buffer_init_fixed(&P->cmds_, P->cmdsdata_.data(), P->cmdslen_);

    static const nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(nk_vertex, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(nk_vertex, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, offsetof(nk_vertex, col)},
        {NK_VERTEX_LAYOUT_END}};

    nk_convert_config &config = P->config_;
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.global_alpha = 1.0f;
    config.shape_AA = NK_ANTI_ALIASING_ON;
    config.line_AA = NK_ANTI_ALIASING_ON;
    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof(nk_vertex);
    config.vertex_alignment = NK_ALIGNOF(nk_vertex);
}

GraphicsDevice_GL::~GraphicsDevice_GL()
{
    P->font_.clear();
    if (P->atlas_)
        nk_font_atlas_clear(&*P->atlas_);
}

#ifndef NDEBUG
static void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id,
                                         GLenum severity, GLsizei length,
                                         const GLchar *message, const void *)
{
    (void)source;
    (void)type;
    (void)id;
    (void)length;

    const char *sev = "?";
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        sev = "high";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        sev = "med";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        sev = "low";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        sev = "note";
        break;
    default:
        break;
    }
    debug("[openGL:{}] {}", sev, (const char *)message);
}
#endif

void GraphicsDevice_GL::initialize(gsl::span<const FontRequest> fontreqs,
                                   const nk_rune range[])
{
    setup_context();

    uint glvermaj = P->glvermaj;
    if (false)  // force openGL 2
        glvermaj = P->glvermaj = 2;

#ifndef NDEBUG
    if (gl_enable_debug_output && glDebugMessageCallback) {
        glDebugMessageCallback(&gl_debug_callback, nullptr);
    }
#endif

    if (glvermaj < 3)
        P->initialize_gl2();
    else
        P->initialize_gl3();

    P->load_fonts(fontreqs, range);
}

#ifndef NDEBUG
static const char *gl_error_string(GLenum error)
{
    switch (error) {
    case GL_NO_ERROR:
        return "no error";
    case GL_INVALID_ENUM:
        return "invalid enumerant";
    case GL_INVALID_VALUE:
        return "invalid value";
    case GL_INVALID_OPERATION:
        return "invalid operation";
    case GL_STACK_OVERFLOW:
        return "stack overflow";
    case GL_STACK_UNDERFLOW:
        return "stack underflow";
    case GL_OUT_OF_MEMORY:
        return "out of memory";
    case GL_TABLE_TOO_LARGE:
        return "table too large";
#ifdef GL_EXT_framebuffer_object
    case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
        return "invalid framebuffer operation";
#endif
    default:
        return nullptr;
    }
}
#endif

void GraphicsDevice_GL::setup_context()
{
    if (P->glsetup_)
        return;

#ifndef NDEBUG
    const struct {
        GLenum item;
        const char *name;
    } gl_infos[] = {
        {GL_VERSION, "version"},
        {GL_SHADING_LANGUAGE_VERSION, "shading language version"},
        {GL_VENDOR, "vendor"},
        {GL_RENDERER, "renderer"},
    };

    for (const auto &info : gl_infos) {
        const char *value = (const char *)glGetString(info.item);
        debug("[openGL] {}: {}", info.name, value ? value : "<no value>");
    }
#endif

    uint glvermaj, glvermin;

    const char *glver = (const char *)glGetString(GL_VERSION);
    if (!glver || sscanf(glver, "%u.%u", &glvermaj, &glvermin) != 2)
        throw std::runtime_error("cannot determine openGL version numbers");

    const uint glreqmaj = 2, glreqmin = 0;
    if (((glvermaj << 16) | glvermin) < ((glreqmaj << 16) | glreqmin))
        throw std::runtime_error(
            fmt::format("insufficient openGL version, {}.{} is required", glreqmaj, glreqmin));

    P->glvermaj = glvermaj;
    P->glvermin = glvermin;

    glewExperimental = 1;
    GLenum glew_err = glewInit();
    if (glew_err != GLEW_OK)
        throw std::runtime_error(fmt::format("error initializing GLEW: {}",
                                             glewGetErrorString(glew_err)));

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    P->glsetup_ = true;
}

void GraphicsDevice_GL::Impl::initialize_gl2()
{
    debug("Initializing for openGL 2");
}

void GraphicsDevice_GL::Impl::initialize_gl3()
{
    debug("Initializing for openGL 3 or later");

    GLint status = 0;

    auto &gl3 = gl3_;
    gl3.prog_ = glCreateProgram();

    gl3.vert_shdr_ = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(gl3.vert_shdr_, 1, &nk_vertex_shader, 0);
    glCompileShader(gl3.vert_shdr_);
    glGetShaderiv(gl3.vert_shdr_, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
#ifndef NDEBUG
        const char *what = gl_error_string(glGetError());
        debug("error compiling vertex shader: {}", what ? what : "<no message>");
#endif
    }

    gl3.frag_shdr_ = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(gl3.frag_shdr_, 1, &nk_fragment_shader, 0);
    glCompileShader(gl3.frag_shdr_);
    glGetShaderiv(gl3.frag_shdr_, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
#ifndef NDEBUG
        const char *what = gl_error_string(glGetError());
        debug("error compiling fragment shader: {}", what ? what : "<no message>");
#endif
    }

    glAttachShader(gl3.prog_, gl3.vert_shdr_);
    glAttachShader(gl3.prog_, gl3.frag_shdr_);
    glLinkProgram(gl3.prog_);
    glGetProgramiv(gl3.prog_, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
#ifndef NDEBUG
        const char *what = gl_error_string(glGetError());
        debug("error linking program: {}", what ? what : "<no message>");
#endif
    }

    gl3.uniform_tex_ = glGetUniformLocation(gl3.prog_, "Texture");
    gl3.uniform_proj_ = glGetUniformLocation(gl3.prog_, "ProjMtx");
    gl3.attrib_pos_ = glGetAttribLocation(gl3.prog_, "Position");
    gl3.attrib_uv_ = glGetAttribLocation(gl3.prog_, "TexCoord");
    gl3.attrib_col_ = glGetAttribLocation(gl3.prog_, "Color");

    /* buffer setup */
    size_t vs = sizeof(struct nk_vertex);
    size_t vp = offsetof(struct nk_vertex, position);
    size_t vt = offsetof(struct nk_vertex, uv);
    size_t vc = offsetof(struct nk_vertex, col);

    glGenVertexArrays(1, &gl3.vao_);
    glGenBuffers(1, &gl3.vbo_);
    glGenBuffers(1, &gl3.ebo_);

    glBindVertexArray(gl3.vao_);
    glBindBuffer(GL_ARRAY_BUFFER, gl3.vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl3.ebo_);

    glEnableVertexAttribArray(gl3.attrib_pos_);
    glEnableVertexAttribArray(gl3.attrib_uv_);
    glEnableVertexAttribArray(gl3.attrib_col_);

    glVertexAttribPointer(gl3.attrib_pos_, 2, GL_FLOAT, GL_FALSE, vs, (void *)vp);
    glVertexAttribPointer(gl3.attrib_uv_, 2, GL_FLOAT, GL_FALSE, vs, (void *)vt);
    glVertexAttribPointer(gl3.attrib_col_, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void *)vc);
}

void GraphicsDevice_GL::cleanup()
{
    P->font_.clear();
    if (P->atlas_) {
        nk_font_atlas_clear(&*P->atlas_);
        P->atlas_.reset();
    }
    while (!P->textures_.empty()) {
        GLuint id = *P->textures_.begin();
        P->textures_.erase(P->textures_.begin());
        glDeleteTextures(1, &id);
    }

    if (P->glvermaj < 3)
        P->cleanup_gl2();
    else
        P->cleanup_gl3();
}

void GraphicsDevice_GL::Impl::cleanup_gl2()
{
}

void GraphicsDevice_GL::Impl::cleanup_gl3()
{
    auto &gl3 = gl3_;

    glDeleteVertexArrays(1, &gl3.vao_);
    gl3.vao_ = 0;
    glDeleteBuffers(1, &gl3.vbo_);
    gl3.vbo_ = 0;
    glDeleteBuffers(1, &gl3.ebo_);
    gl3.ebo_ = 0;
    glDeleteProgram(gl3.prog_);
    gl3.prog_ = 0;
    glDeleteShader(gl3.vert_shdr_);
    gl3.vert_shdr_ = 0;
    glDeleteShader(gl3.frag_shdr_);
    gl3.frag_shdr_ = 0;
}

im_texture GraphicsDevice_GL::load_texture(const u8 *data, uint w, uint h, uint channels)
{
    bool success = false;

    if (w > UINT16_MAX || h > UINT16_MAX)
        throw std::runtime_error("unsupported image dimensions");

    GLuint format, type;
    switch (channels) {
    case 3:
        format = GL_RGB;
        type = GL_UNSIGNED_BYTE;
        break;
    case 4:
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        break;
    default:
        throw std::runtime_error("unsupported image format for openGL");
    }

    GLuint id = 0;
    glGenTextures(1, &id);
    if (!id)
        throw std::runtime_error("could not generate openGL texture");

    SCOPE(exit)
    {
        if (!success)
            glDeleteTextures(1, &id);
    };

    glBindTexture(GL_TEXTURE_2D, id);
    // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (!glGenerateMipmap)  // for GL >= 1.4 && < 3.1
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, true);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, format, type, data);
    if (glGenerateMipmap)  // for GL >= 3.1
        glGenerateMipmap(GL_TEXTURE_2D);

    im_texture tex;
    tex->handle.ptr = nullptr;
    tex->handle.id = id;
    tex->w = w;
    tex->h = h;
    tex->region[0] = 0;
    tex->region[1] = 0;
    tex->region[2] = w;
    tex->region[3] = h;

    bool inserted = P->textures_.insert(id).second;
    assert(inserted);
    (void)inserted;

    success = true;
    return tex;
}

void GraphicsDevice_GL::unload_texture(nk_handle handle)
{
    GLuint id = handle.id;

    auto it = P->textures_.find(id);
    assert(it != P->textures_.end());
    P->textures_.erase(it);

    glDeleteTextures(1, &id);
}

void GraphicsDevice_GL::render()
{
    if (P->glvermaj < 3)
        P->render_gl2();
    else
        P->render_gl3();
}

void GraphicsDevice_GL::Impl::render_gl2()
{
    NkScreen &screen = Q->ctl_.screen();
    nk_context *ctx = screen.context();
    uint w = screen.width();
    uint h = screen.height();

    nk_buffer &cmds = cmds_;
    nk_buffer_clear(&cmds);

    auto &gl2 = gl2_;

    nk_buffer &vbuf = vbuf_;
    nk_buffer &ebuf = ebuf_;
    if (!gl2.vbufdata_) {
        gl2.vbufdata_.reset(MAX_VERTEX_MEMORY);
        nk_buffer_init_fixed(&vbuf, gl2.vbufdata_.data(), MAX_VERTEX_MEMORY);
    }
    if (!gl2.ebufdata_) {
        gl2.ebufdata_.reset(MAX_ELEMENT_MEMORY);
        nk_buffer_init_fixed(&ebuf, gl2.ebufdata_.data(), MAX_ELEMENT_MEMORY);
    }

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    SCOPE(exit)
    {
        glPopAttrib();
    };
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* setup viewport/project */
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    SCOPE(exit)
    {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    };
    glLoadIdentity();
    glOrtho(0.0f, (f32)w, (f32)h, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    SCOPE(exit)
    {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    };
    glLoadIdentity();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    /* convert from command queue into draw list and draw to screen */

    /* setup buffers to load vertices and elements */
    nk_buffer_clear(&vbuf);
    nk_buffer_clear(&ebuf);
    nk_convert(ctx, &cmds, &vbuf, &ebuf, &config_);

    /* setup vertex buffer pointer */
    const nk_vertex *vv = (const nk_vertex *)nk_buffer_memory_const(&vbuf);
    size_t vs = (const u8 *)&vv[1] - (const u8 *)&vv[0];
    glVertexPointer(2, GL_FLOAT, vs, &vv->position);
    glTexCoordPointer(2, GL_FLOAT, vs, &vv->uv);
    glColorPointer(4, GL_UNSIGNED_BYTE, vs, &vv->col);

    /* iterate over and execute each draw command */
    const nk_draw_command *cmd;
    const nk_draw_index *offset = (const nk_draw_index *)nk_buffer_memory_const(&ebuf);
    nk_draw_foreach(cmd, ctx, &cmds)
    {
        if (cmd->elem_count > 0) {
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)(cmd->clip_rect.x),
                      (GLint)((h - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h))),
                      (GLint)(cmd->clip_rect.w), (GLint)(cmd->clip_rect.h));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
    }

    nk_clear(ctx);
}

void GraphicsDevice_GL::Impl::render_gl3()
{
    NkScreen &screen = Q->ctl_.screen();
    nk_context *ctx = screen.context();
    uint w = screen.width();
    uint h = screen.height();

    nk_buffer &cmds = cmds_;
    nk_buffer_clear(&cmds);

    auto &gl3 = gl3_;

    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, -2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)w;
    ortho[1][1] /= (GLfloat)h;

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(gl3.prog_);
    glUniform1i(gl3.uniform_tex_, 0);
    glUniformMatrix4fv(gl3.uniform_proj_, 1, GL_FALSE, &ortho[0][0]);
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    /* convert from command queue into draw list and draw to screen */

    /* allocate vertex and element buffer */
    glBindVertexArray(gl3.vao_);
    glBindBuffer(GL_ARRAY_BUFFER, gl3.vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl3.ebo_);

    glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_MEMORY, nullptr, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_ELEMENT_MEMORY, nullptr, GL_STREAM_DRAW);

    /* load draw vertices & elements directly into vertex + element buffer */
    void *vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    void *elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

    /* setup buffers to load vertices and elements */
    nk_buffer &vbuf = vbuf_;
    nk_buffer &ebuf = ebuf_;
    nk_buffer_init_fixed(&vbuf, vertices, MAX_VERTEX_MEMORY);
    nk_buffer_init_fixed(&ebuf, elements, MAX_ELEMENT_MEMORY);
    nk_convert(ctx, &cmds, &vbuf, &ebuf, &config_);

    glUnmapBuffer(GL_ARRAY_BUFFER);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    /* iterate over and execute each draw command */
    const nk_draw_command *cmd;
    const nk_draw_index *offset = nullptr;
    nk_draw_foreach(cmd, ctx, &cmds)
    {
        if (cmd->elem_count > 0) {
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)(cmd->clip_rect.x),
                      (GLint)(((GLint)h -
                               (GLint)(cmd->clip_rect.y + cmd->clip_rect.h))),
                      (GLint)(cmd->clip_rect.w), (GLint)(cmd->clip_rect.h));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
    }

    nk_clear(ctx);
}

nk_user_font *GraphicsDevice_GL::get_font(uint id)
{
    std::vector<nk_font *> &fonts = P->font_;
    if (id >= fonts.size())
        return nullptr;
    return &fonts[id]->handle;
}

void GraphicsDevice_GL::Impl::load_fonts(gsl::span<const FontRequest> fontreqs,
                                         const nk_rune range[])
{
    bool success = false;
    nk_font_atlas &atlas = *(atlas_ = nk_font_atlas{});
    std::vector<nk_font *> &fonts = font_;

    nk_font_atlas_init_default(&atlas);
    SCOPE(exit)
    {
        if (!success) {
            nk_font_atlas_clear(&atlas);
            atlas_.reset();
            fonts.clear();
        };
    };

    nk_font_atlas_begin(&atlas);

    for (const FontRequest &req : fontreqs) {
        struct nk_font_config fcfg = nk_font_config(req.height);
        fcfg.range = range;
        fcfg.oversample_h = 8;
        fcfg.oversample_v = 8;

        nk_font *font = nullptr;
        switch (req.type) {
        case FontRequest::Type::Default:
            font = nk_font_atlas_add_default(&atlas, req.height, &fcfg);
            break;
        case FontRequest::Type::File:
            font = nk_font_atlas_add_from_file(&atlas, req.un.file.path, req.height, &fcfg);
            break;
        case FontRequest::Type::Memory:
            font = nk_font_atlas_add_from_memory(&atlas, (void *)req.un.memory.data,
                                                 req.un.memory.size, req.height, &fcfg);
            break;
        default:
            assert(false);
        }
        if (!font)
            throw std::runtime_error("could not load the requested font");
        fonts.push_back(font);
    }

    im_texture font_tex;
    {
        uint atw = 0, ath = 0;
        const u8 *image = (const u8 *)nk_font_atlas_bake(&atlas, (int *)&atw,
                                                         (int *)&ath, NK_FONT_ATLAS_RGBA32);
        if (!image)
            throw std::runtime_error("could not convert font into texture");
        font_tex = Q->load_texture(image, atw, ath, 4);
    }

    nk_draw_null_texture null{};
    nk_font_atlas_end(&atlas, font_tex->handle, &null);

    nk_convert_config &config = config_;
    config.null = null;

    success = true;
}

}  // namespace cws80
