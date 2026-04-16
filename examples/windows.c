#include <raylib.h>

#define LEAF_IMPLEMENTATION
#include "leaf.h"

#define LEAF_RAYLIB_IMPLEMENTATION
#include "leaf_raylib.h"

typedef struct
{
    Leaf_ID id;
    Leaf_Vec2 position;
    Leaf_Vec2 size;
    const char *title;
    bool docked;
}
Leaf_Window;

typedef struct Leaf_DockNode Leaf_DockNode;
struct Leaf_DockNode
{
    Leaf_Window *window;
    Leaf_DockNode *first_child;
    Leaf_DockNode *second_child;
};

#ifndef LEAF_CONFIG_MAX_DOCK_NODES
    #define LEAF_CONFIG_MAX_DOCK_NODES 128
#endif

typedef struct
{
    Leaf_DockNode dock_nodes[LEAF_CONFIG_MAX_DOCK_NODES];
    uint32_t dock_node_count;
    Leaf_DockNode *root_dock_node;
}
Leaf_DockingContext;

static Leaf_DockingContext *leaf_dock_ctx;

static bool leaf_dock_menu_button(Leaf_ID id)
{
    bool is_hovered = leaf_hovered(id);
    leaf({
        .id = id,
        .sizing = {LEAF_PERCENT(0.3f), LEAF_PERCENT(0.3f)},
        .color = leaf_rgba(255, 255, 255, is_hovered ? 150 : 100),
        .roundness = 8.0f
    });
}

static void leaf_render_docking_menu(void)
{
    Leaf_ElementConfig line_config = {
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
        .direction = LEAF_LAYOUT_VERTICAL,
        .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .sizing = {LEAF_PERCENT(1.0f), LEAF_PERCENT(1.0f)},
        .child_gap = 4.0f
    };

    leaf(line_config)
    {
        leaf_dock_menu_button(leaf_id("__leaf_dock_top"));
        leaf_dock_menu_button(leaf_id("__leaf_dock_middle"));
        leaf_dock_menu_button(leaf_id("__leaf_dock_bottom"));
    }

    line_config.direction = LEAF_LAYOUT_HORIZONAL;

    leaf(line_config)
    {
        leaf_dock_menu_button(leaf_id("__leaf_dock_left"));
        leaf({.sizing = {LEAF_PERCENT(0.3f), LEAF_FIT}, .aspect_ratio = 1.0f});
        leaf_dock_menu_button(leaf_id("__leaf_dock_right"));
    }
}

static void leaf_render_window(Leaf_Window *window)
{
    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
        .sizing = {LEAF_FIXED(window->size.x), LEAF_FIXED(window->size.y)},
        .floating_offset = {window->position.x, window->position.y},
        .border = {LEAF_DBG_BORDER, 1.0f},
        .color = LEAF_DBG_BG1
    })
    {
        const Leaf_ID titlebar_id = leaf_id_indexed(window->title, 1);
        const bool title_bar_hovered = leaf_hovered(titlebar_id);

        static Leaf_Vec2 drag_offset;
        static bool dragging;
        Vector2 mouse = GetMousePosition();

        if (title_bar_hovered)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                dragging = true;
                drag_offset.x = mouse.x - window->position.x;
                drag_offset.y = mouse.y - window->position.y;
            }
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        else
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);


        if (dragging)
        {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                window->position.x = mouse.x - drag_offset.x;
                window->position.y = mouse.y - drag_offset.y;
            }
            else dragging = false;
        }


        leaf({
            .id = titlebar_id,
            .sizing = {LEAF_GROW, LEAF_FIT},
            .border = {title_bar_hovered ? LEAF_COLOR_WHITE : LEAF_DBG_BORDER, 1.0f},
            .padding = {4.0f, 4.0f, 4.0f, 4.0f}
        })
        {
            leaf_text(window->title, {
                .font_size = 20.0f,
                .color = LEAF_DBG_TEXT_PRI
            });
        }
        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .floating_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .sizing = {LEAF_PERCENT(0.3f), LEAF_FIT},
            .color = leaf_rgb(0, 0, 0),
            .aspect_ratio = 1.0f
        })
        {
            leaf_render_docking_menu();
        }
    }
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Leaf Eaxmple");

    leaf_initialize();
    leaf_raylib_initialize("./Quicksand-SemiBold.ttf");

    leaf_dock_ctx = (Leaf_DockingContext*)malloc(sizeof(Leaf_DockingContext));
    Leaf_Window window;
    window.id = leaf_id("TestWindow");
    window.title = "Test Window";
    window.size = (Leaf_Vec2){800.0f, 400.0f};

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        leaf_begin_frame(GetScreenWidth(), GetScreenHeight());
        leaf_set_pointer_pos(GetMouseX(), GetMouseY());
        static bool show_debug = false;
        if (IsKeyPressed(KEY_F1))
            show_debug = !show_debug;
        leaf_debug(show_debug, 500, GetFrameTime(), GetMouseWheelMove())
        {
            leaf({
                .id = leaf_id("Main Body"),
                .sizing = {LEAF_GROW, LEAF_GROW},
                .color = leaf_rgb(37, 35, 33),
                .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            })
            {
                leaf_render_window(&window);
            }
        }

        Leaf_RenderCmdList list = leaf_end_frame();
        leaf_raylib_render(list);
        EndDrawing();
    }

    free(leaf_dock_ctx);
    leaf_raylib_shutdown();
    leaf_shutdown();
    CloseWindow();
}
