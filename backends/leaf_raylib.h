#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void leaf_raylib_initialize(const char *font_name);
void leaf_raylib_shutdown(void);
void leaf_raylib_render(Leaf_RenderCmdList cmd_list);

#ifdef LEAF_RAYLIB_IMPLEMENTATION

struct
{
    Font small_font;
    Font large_font;
    Shader round_image_shader;
    int32_t roundness_location;
    int32_t size_location;
}
leaf_raylib_ctx;

static Leaf_Dimensions leaf_raylib_measure_text(const char *text, uint32_t length, const Leaf_TextConfig *config)
{
    Vector2 size = MeasureTextEx(config->font_size > 32.0f ? leaf_raylib_ctx.large_font : leaf_raylib_ctx.small_font, text, config->font_size, 2);
    return (Leaf_Dimensions){ size.x, size.y };
}

void leaf_raylib_initialize(const char *font_name)
{
    leaf_raylib_ctx.small_font = LoadFontEx(font_name, 32, NULL, 0);
    leaf_raylib_ctx.large_font = LoadFontEx(font_name, 256, NULL, 0);
    SetTextureFilter(leaf_raylib_ctx.small_font.texture, TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(leaf_raylib_ctx.large_font.texture, TEXTURE_FILTER_TRILINEAR);
    leaf_set_measure_text(leaf_raylib_measure_text);

    leaf_raylib_ctx.round_image_shader = LoadShaderFromMemory(NULL,
    #ifdef EMSCRIPTEN
        "#version 100\n"
        "precision mediump float;"
        "varying vec2 fragTexCoord;"
        "uniform sampler2D texture0;"
        "uniform float roundness;"
        "uniform vec2 size;"
        "void main() {"
            "vec2 uv = fragTexCoord - 0.5;"
            "vec2 pixel = uv * size;"
            "float r = roundness;"
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
        "uniform float roundness;"
        "uniform vec2 size;"
        "void main() {"
            "vec2 uv = fragTexCoord - 0.5;"
            "vec2 pixel = uv * size;"
            "float r = roundness;"
            "vec2 q = abs(pixel) - (size * 0.5 - r);"
            "float dist = length(max(q, 0.0)) - r;"
            "if (dist > 0.0) discard;"
                "finalColor = texture(texture0, fragTexCoord);"
        "}\0"
    #endif
    );

    leaf_raylib_ctx.roundness_location = GetShaderLocation(leaf_raylib_ctx.round_image_shader, "roundness");
    leaf_raylib_ctx.size_location = GetShaderLocation(leaf_raylib_ctx.round_image_shader, "size");
}

void leaf_raylib_shutdown(void)
{
    UnloadShader(leaf_raylib_ctx.round_image_shader);
    UnloadFont(leaf_raylib_ctx.large_font);
    UnloadFont(leaf_raylib_ctx.small_font);
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
        {
            float min_side = (cmd.bounding_box.width < cmd.bounding_box.height)
                ? cmd.bounding_box.width
                : cmd.bounding_box.height;
            float roundness = (min_side > 0) ? (cmd.rect.roundness / (min_side * 0.5f)) : 0.0f;
            DrawRectangleRounded((Rectangle){cmd.bounding_box.x, cmd.bounding_box.y, cmd.bounding_box.width, cmd.bounding_box.height},
                roundness, 8, (Color){cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a});
            break;
        }
        case LEAF_RENDER_CMD_RECT_LINES:
        {
            float min_side = (cmd.bounding_box.width < cmd.bounding_box.height)
                ? cmd.bounding_box.width
                : cmd.bounding_box.height;
            float roundness = (min_side > 0) ? (cmd.rect.roundness / (min_side * 0.5f)) : 0.0f;
            DrawRectangleRoundedLinesEx((Rectangle){cmd.bounding_box.x, cmd.bounding_box.y, cmd.bounding_box.width, cmd.bounding_box.height},
                roundness, 8, cmd.rect.line_width, (Color){cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a});
            break;
        }
        case LEAF_RENDER_CMD_TEXT:
        {
            char tmp[1024];
            uint32_t len = cmd.text.length < sizeof(tmp) - 1 ? cmd.text.length : sizeof(tmp) - 1;
            memcpy(tmp, cmd.text.text, len);
            tmp[len] = '\0';
            DrawTextEx(cmd.text.font_size > 32.0f ? leaf_raylib_ctx.large_font : leaf_raylib_ctx.small_font, tmp, (Vector2){cmd.bounding_box.x, cmd.bounding_box.y},
                cmd.text.font_size, 2, (Color){cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a});
            break;
        }
        case LEAF_RENDER_CMD_IMAGE:
        {
            Texture2D texture = *(Texture2D*)cmd.image.handle;
            float max_roundness = (cmd.bounding_box.height > cmd.bounding_box.width ? cmd.bounding_box.width : cmd.bounding_box.height) * 0.5f;
            float roundness = cmd.image.roundness > max_roundness ? max_roundness : cmd.image.roundness;

            Rectangle source = { 0, 0, (float)texture.width, (float)texture.height };
            Rectangle dest = { cmd.bounding_box.x, cmd.bounding_box.y, cmd.bounding_box.width, cmd.bounding_box.height };

            float size[2] = { cmd.bounding_box.width, cmd.bounding_box.height };

            BeginShaderMode(leaf_raylib_ctx.round_image_shader);
            SetShaderValue(leaf_raylib_ctx.round_image_shader, leaf_raylib_ctx.roundness_location, &roundness, SHADER_UNIFORM_FLOAT);
            SetShaderValue(leaf_raylib_ctx.round_image_shader, leaf_raylib_ctx.size_location, size, SHADER_UNIFORM_VEC2);
            DrawTexturePro(texture, source, dest, (Vector2){0, 0}, 0.0f,
                (Color){cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a});
            EndShaderMode();
            break;
        }
            case LEAF_RENDER_CMD_SCISSOR_PUSH:
            {
                Rectangle rect = {
                    cmd.bounding_box.x,
                    cmd.bounding_box.y,
                    cmd.bounding_box.width,
                    cmd.bounding_box.height
                };

                scissor_stack[scissor_count++] = rect;

                EndScissorMode();
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
        }

    }
}
#endif

#ifdef __cplusplus
}
#endif
