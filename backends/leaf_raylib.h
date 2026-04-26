#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    Font small;
    Font large;
}
Leaf_RaylibFont;

Leaf_RaylibFont leaf_raylib_load_font(const char *font_path, int *codepoints, int codepoint_count);
void leaf_raylib_unload_font(Leaf_RaylibFont font);

void leaf_raylib_initialize(Leaf_RaylibFont *fonts);
void leaf_raylib_shutdown(void);
void leaf_raylib_render(Leaf_RenderCmdList cmd_list);

#ifdef LEAF_RAYLIB_IMPLEMENTATION

#include "rlgl.h"

static struct
{
    Leaf_RaylibFont *fonts;

    Shader round_image_shader;
    int32_t rounding_location;
    int32_t size_location;

    Shader gradient_shader;
    int32_t grad_color1_location;
    int32_t grad_color2_location;
    int32_t grad_angle_location;
    int32_t grad_rounding_location;
    int32_t grad_size_location;
    int32_t grad_line_width_location;
    int32_t grad_draw_lines_location;
    int32_t grad_fill_type_location;
}
leaf_raylib_ctx;

static inline Font leaf_raylib_get_base_font(float font_size, uint32_t font_id)
{
    Leaf_RaylibFont font = leaf_raylib_ctx.fonts[font_id];
    return font_size > 32.0f ? font.large : font.small;
}

static Leaf_Dimensions leaf_raylib_measure_text(const char *text, uint32_t length, const Leaf_TextConfig *config)
{
    float max_text_width = 0.0f;
    float line_text_width = 0.0f;

    Font font_to_use = leaf_raylib_get_base_font(config->font_size, config->font_id);
    if (!font_to_use.glyphs) font_to_use = GetFontDefault();

    float scale_factor = config->font_size / (float)font_to_use.baseSize;

    int i = 0;
    while (i < (int)length)
    {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&text[i], &codepointByteCount);

        if (codepoint == '\n')
        {
            if (line_text_width > max_text_width) max_text_width = line_text_width;
            line_text_width = 0.0f;
        }
        else
        {
            int index = GetGlyphIndex(font_to_use, codepoint);
            if (font_to_use.glyphs[index].advanceX != 0)
                line_text_width += font_to_use.glyphs[index].advanceX;
            else
                line_text_width += font_to_use.recs[index].width + font_to_use.glyphs[index].offsetX;
        }

        i += codepointByteCount;
    }

    if (line_text_width > max_text_width) max_text_width = line_text_width;

    return (Leaf_Dimensions){
        max_text_width * scale_factor,
        config->font_size
    };
}

static inline void leaf_color_to_vec4(Leaf_Color c, float out[4])
{
    out[0] = c.r / 255.0f;
    out[1] = c.g / 255.0f;
    out[2] = c.b / 255.0f;
    out[3] = c.a / 255.0f;
}

static void leaf_rlgl_draw_rect(float x, float y, float w, float h)
{
    rlBegin(RL_QUADS);
    rlNormal3f(0.0f, 0.0f, 1.0f);

    rlTexCoord2f(0.0f, 0.0f); rlVertex2f(x,     y);
    rlTexCoord2f(0.0f, 1.0f); rlVertex2f(x,     y + h);
    rlTexCoord2f(1.0f, 1.0f); rlVertex2f(x + w, y + h);
    rlTexCoord2f(1.0f, 0.0f); rlVertex2f(x + w, y);
    rlEnd();
}

Leaf_RaylibFont leaf_raylib_load_font(const char *font_path, int *codepoints, int codepoint_count)
{
    Leaf_RaylibFont font;
    font.small = LoadFontEx(font_path, 32,  codepoints, codepoint_count);
    font.large = LoadFontEx(font_path, 256, codepoints, codepoint_count);
    SetTextureFilter(font.small.texture, TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(font.large.texture, TEXTURE_FILTER_TRILINEAR);
    return font;
}

void leaf_raylib_unload_font(Leaf_RaylibFont font)
{
    UnloadFont(font.large);
    UnloadFont(font.small);
}

void leaf_raylib_initialize(Leaf_RaylibFont *fonts)
{
    leaf_raylib_ctx.fonts = fonts;
    leaf_set_measure_text(leaf_raylib_measure_text);

    leaf_raylib_ctx.round_image_shader = LoadShaderFromMemory(NULL,
    #ifdef EMSCRIPTEN
        "#version 100\n"
        "precision mediump float;"
        "varying vec2 fragTexCoord;"
        "uniform sampler2D texture0;"
        "uniform float rounding;"
        "uniform vec2 size;"
        "void main() {"
            "vec2 uv = fragTexCoord - 0.5;"
            "vec2 pixel = uv * size;"
            "float r = rounding;"
            "vec2 q = abs(pixel) - (size * 0.5 - r);"
            "float dist = length(max(q, 0.0)) - r;"
            "if (dist > 0.0) discard;"
            "gl_FragColor = texture2D(texture0, fragTexCoord);"
        "}\0"
    #else
        "#version 330\n"
        "in vec2 fragTexCoord;"
        "out vec4 finalColor;"
        "uniform sampler2D texture0;"
        "uniform float rounding;"
        "uniform vec2 size;"
        "void main() {"
            "vec2 uv = fragTexCoord - 0.5;"
            "vec2 pixel = uv * size;"
            "float r = rounding;"
            "vec2 q = abs(pixel) - (size * 0.5 - r);"
            "float dist = length(max(q, 0.0)) - r;"
            "if (dist > 0.0) discard;"
            "finalColor = texture(texture0, fragTexCoord);"
        "}\0"
    #endif
    );
    leaf_raylib_ctx.rounding_location = GetShaderLocation(leaf_raylib_ctx.round_image_shader, "rounding");
    leaf_raylib_ctx.size_location     = GetShaderLocation(leaf_raylib_ctx.round_image_shader, "size");

    leaf_raylib_ctx.gradient_shader = LoadShaderFromMemory(NULL,
    #ifdef EMSCRIPTEN
        "#version 100\n"
        "precision mediump float;"
        "varying vec2 fragTexCoord;"
        "uniform vec4 color1;"
        "uniform vec4 color2;"
        "uniform float angle;"
        "uniform float rounding;"
        "uniform vec2 size;"
        "uniform float lineWidth;"
        "uniform int drawLines;"
        "uniform int fillType;"
        "void main() {"
            "vec2 pixel = (fragTexCoord - 0.5) * size;"
            "float r = rounding;"
            "vec2 q = abs(pixel) - (size * 0.5 - r);"
            "float dist = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;"
            "if (dist > 0.5) discard;"
            "if (drawLines == 1 && dist < -lineWidth) discard;"
            "float t;"
            "if (fillType == 2) {"
                "t = clamp(length(fragTexCoord - 0.5) * 2.0, 0.0, 1.0);"
            "} else {"
                "vec2 dir = vec2(cos(angle), sin(angle));"
                "t = clamp(dot(fragTexCoord - 0.5, dir) + 0.5, 0.0, 1.0);"
            "}"
            "gl_FragColor = mix(color1, color2, t);"
        "}\0"
    #else
        "#version 330\n"
        "in vec2 fragTexCoord;"
        "out vec4 finalColor;"
        "uniform vec4 color1;"
        "uniform vec4 color2;"
        "uniform float angle;"
        "uniform float rounding;"
        "uniform vec2 size;"
        "uniform float lineWidth;"
        "uniform int drawLines;"
        "uniform int fillType;"
        "void main() {"
            "vec2 pixel = (fragTexCoord - 0.5) * size;"
            "float r = rounding;"
            "vec2 q = abs(pixel) - (size * 0.5 - r);"
            "float dist = length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;"
            "if (dist > 0.5) discard;"
            "if (drawLines == 1 && dist < -lineWidth) discard;"
            "float t;"
            "if (fillType == 2) {"
                "t = clamp(length(fragTexCoord - 0.5) * 2.0, 0.0, 1.0);"
            "} else {"
                "vec2 dir = vec2(cos(angle), sin(angle));"
                "t = clamp(dot(fragTexCoord - 0.5, dir) + 0.5, 0.0, 1.0);"
            "}"
            "finalColor = mix(color1, color2, t);"
        "}\0"
    #endif
    );
    leaf_raylib_ctx.grad_color1_location     = GetShaderLocation(leaf_raylib_ctx.gradient_shader, "color1");
    leaf_raylib_ctx.grad_color2_location     = GetShaderLocation(leaf_raylib_ctx.gradient_shader, "color2");
    leaf_raylib_ctx.grad_angle_location      = GetShaderLocation(leaf_raylib_ctx.gradient_shader, "angle");
    leaf_raylib_ctx.grad_rounding_location   = GetShaderLocation(leaf_raylib_ctx.gradient_shader, "rounding");
    leaf_raylib_ctx.grad_size_location       = GetShaderLocation(leaf_raylib_ctx.gradient_shader, "size");
    leaf_raylib_ctx.grad_line_width_location = GetShaderLocation(leaf_raylib_ctx.gradient_shader, "lineWidth");
    leaf_raylib_ctx.grad_draw_lines_location = GetShaderLocation(leaf_raylib_ctx.gradient_shader, "drawLines");
    leaf_raylib_ctx.grad_fill_type_location  = GetShaderLocation(leaf_raylib_ctx.gradient_shader, "fillType");
}

void leaf_raylib_shutdown(void)
{
    UnloadShader(leaf_raylib_ctx.gradient_shader);
    UnloadShader(leaf_raylib_ctx.round_image_shader);
}

static void leaf_raylib_draw_fill_rect(Leaf_BoundingBox bb, Leaf_ColorFill fill, float rounding, float line_width)
{
    float min_side = bb.width < bb.height ? bb.width : bb.height;
    float rounding_ratio = (min_side > 0) ? (rounding / (min_side * 0.5f)) : 0.0f;
    Rectangle rect = { bb.x, bb.y, bb.width, bb.height };

    if (fill.type == LEAF_SOLID_COLOR_FILL)
    {
        Leaf_Color c = fill.color1;
        Color rc = { c.r, c.g, c.b, c.a };
        if (line_width > 0.0f)
            DrawRectangleRoundedLinesEx(rect, rounding_ratio, 8, line_width, rc);
        else
            DrawRectangleRounded(rect, rounding_ratio, 8, rc);
        return;
    }

    float c1[4], c2[4];
    leaf_color_to_vec4(fill.color1, c1);
    leaf_color_to_vec4(fill.color2, c2);

    float size[2]  = { bb.width, bb.height };
    float angle    = fill.angle;
    int   lines    = (line_width > 0.0f) ? 1 : 0;
    int   filltype = (int)fill.type;

    SetShaderValue(leaf_raylib_ctx.gradient_shader, leaf_raylib_ctx.grad_color1_location,     c1,         SHADER_UNIFORM_VEC4);
    SetShaderValue(leaf_raylib_ctx.gradient_shader, leaf_raylib_ctx.grad_color2_location,     c2,         SHADER_UNIFORM_VEC4);
    SetShaderValue(leaf_raylib_ctx.gradient_shader, leaf_raylib_ctx.grad_angle_location,      &angle,     SHADER_UNIFORM_FLOAT);
    SetShaderValue(leaf_raylib_ctx.gradient_shader, leaf_raylib_ctx.grad_rounding_location,   &rounding,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(leaf_raylib_ctx.gradient_shader, leaf_raylib_ctx.grad_size_location,       size,       SHADER_UNIFORM_VEC2);
    SetShaderValue(leaf_raylib_ctx.gradient_shader, leaf_raylib_ctx.grad_line_width_location, &line_width,SHADER_UNIFORM_FLOAT);
    SetShaderValue(leaf_raylib_ctx.gradient_shader, leaf_raylib_ctx.grad_draw_lines_location, &lines,     SHADER_UNIFORM_INT);
    SetShaderValue(leaf_raylib_ctx.gradient_shader, leaf_raylib_ctx.grad_fill_type_location,  &filltype,  SHADER_UNIFORM_INT);

    BeginShaderMode(leaf_raylib_ctx.gradient_shader);
    rlSetTexture(rlGetTextureIdDefault());
    leaf_rlgl_draw_rect(bb.x, bb.y, bb.width, bb.height);
    EndShaderMode();
}

void leaf_raylib_render(Leaf_RenderCmdList cmd_list)
{
    #define SCISSOR_STACK_MAX 256
    Rectangle scissor_stack[SCISSOR_STACK_MAX];
    int scissor_count = 0;

    for (unsigned int i = 0; i < cmd_list.count; i++)
    {
        Leaf_RenderCmd cmd = cmd_list.cmds[i];
        switch (cmd.type)
        {
        case LEAF_RENDER_CMD_RECT:
            leaf_raylib_draw_fill_rect(cmd.bounding_box, cmd.color, cmd.rect.rounding, 0.0f);
            break;

        case LEAF_RENDER_CMD_RECT_LINES:
            leaf_raylib_draw_fill_rect(cmd.bounding_box, cmd.color, cmd.rect.rounding, cmd.rect.line_width);
            break;

        case LEAF_RENDER_CMD_TEXT:
        {
            Leaf_Color c = cmd.color.color1;
            DrawTextEx(
                leaf_raylib_get_base_font(cmd.text.font_size, cmd.text.font_id),
                cmd.text.text,
                (Vector2){ cmd.bounding_box.x, cmd.bounding_box.y },
                cmd.text.font_size, 0,
                (Color){ c.r, c.g, c.b, c.a });
            break;
        }

        case LEAF_RENDER_CMD_IMAGE:
        {
            Texture2D texture = *(Texture2D*)cmd.image.handle;
            float max_rounding = (cmd.bounding_box.height > cmd.bounding_box.width
                ? cmd.bounding_box.width : cmd.bounding_box.height) * 0.5f;
            float rounding = cmd.image.rounding > max_rounding ? max_rounding : cmd.image.rounding;
            Rectangle source = { 0, 0, (float)texture.width, (float)texture.height };
            Rectangle dest   = { cmd.bounding_box.x, cmd.bounding_box.y, cmd.bounding_box.width, cmd.bounding_box.height };
            float size[2]    = { cmd.bounding_box.width, cmd.bounding_box.height };
            Leaf_Color c     = cmd.color.color1;
            BeginShaderMode(leaf_raylib_ctx.round_image_shader);
            SetShaderValue(leaf_raylib_ctx.round_image_shader, leaf_raylib_ctx.rounding_location, &rounding, SHADER_UNIFORM_FLOAT);
            SetShaderValue(leaf_raylib_ctx.round_image_shader, leaf_raylib_ctx.size_location, size, SHADER_UNIFORM_VEC2);
            DrawTexturePro(texture, source, dest, (Vector2){0, 0}, 0.0f, (Color){ c.r, c.g, c.b, c.a });
            EndShaderMode();
            break;
        }

        case LEAF_RENDER_CMD_SCISSOR_PUSH:
        {
            Rectangle rect = { cmd.bounding_box.x, cmd.bounding_box.y, cmd.bounding_box.width, cmd.bounding_box.height };
            scissor_stack[scissor_count++] = rect;
            if (scissor_count > 0) EndScissorMode();
            BeginScissorMode((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height);
            break;
        }

        case LEAF_RENDER_CMD_SCISSOR_POP:
        {
            scissor_count--;
            EndScissorMode();
            if (scissor_count > 0)
            {
                Rectangle rect = scissor_stack[scissor_count - 1];
                BeginScissorMode((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height);
            }
            break;
        }

        case LEAF_RENDER_CMD_CUSTOM:
            cmd.custom.draw(cmd.bounding_box, cmd.custom.user_data);
            break;
        }
    }
}
#endif

#ifdef __cplusplus
}
#endif