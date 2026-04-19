#include <raylib.h>

#define LEAF_IMPLEMENTATION
#include "leaf.h"

#define LEAF_RAYLIB_IMPLEMENTATION
#include "leaf_raylib.h"

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Leaf Eaxmple");

    leaf_initialize();
    leaf_raylib_initialize("./fonts/Quicksand-SemiBold.ttf");

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        leaf_begin_frame(GetScreenWidth(), GetScreenHeight());
        leaf_set_pointer_pos(GetMouseX(), GetMouseY());
        static bool show_debug = false;
        if (IsKeyPressed(KEY_TAB))
            show_debug = !show_debug;
        leaf_debug(show_debug, 500, GetFrameTime(), GetMouseWheelMove())
        {
            leaf({
                .id = leaf_id("Main Body"),
                .sizing = {LEAF_GROW, LEAF_GROW},
                .color = LEAF_SOLID(leaf_rgb(37, 35, 33)),
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            })
            {
                leaf_text("Hello, Leaf!",{
                    .font_size = 128.0f,
                    .color = LEAF_SOLID(leaf_rgb(237, 226, 231)),
                    .alignment = LEAF_TEXT_ALIGN_CENTER
                });
                leaf_text("[Secondary title] :3",{
                    .font_size = 48.0f,
                    .color = LEAF_SOLID(leaf_rgb(141, 133, 135)),
                    .alignment = LEAF_TEXT_ALIGN_CENTER
                });
            }
        }

        Leaf_RenderCmdList list = leaf_end_frame();
        leaf_raylib_render(list);
        EndDrawing();
    }

    leaf_raylib_shutdown();
    leaf_shutdown();
    CloseWindow();
}