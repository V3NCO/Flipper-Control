#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/canvas.h>
#include <input/input.h>
#include <flipper_http/flipper_http.h>

#define TAG "FlipperControlSample"

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    bool running;
} FlipperControlApp;

static void render_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 24, AlignCenter, AlignCenter, "Flipper Control");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignCenter, "Sample app running");
    canvas_draw_str_aligned(canvas, 64, 56, AlignCenter, AlignCenter, "Press Back to exit");
}

static void input_callback(InputEvent* event, void* ctx) {
    FlipperControlApp* app = ctx;
    if(event->type == InputTypeShort && event->key == InputKeyBack) {
        app->running = false;
    }
}

int32_t flipper_control_app(void* p) {
    UNUSED(p);
    FlipperControlApp app = {0};

    app.view_port = view_port_alloc();
    view_port_draw_callback_set(app.view_port, render_callback, &app);
    view_port_input_callback_set(app.view_port, input_callback, &app);

    app.gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app.gui, app.view_port, GuiLayerFullscreen);

    app.running = true;
    while(app.running) {
        furi_delay_ms(10);
    }

    gui_remove_view_port(app.gui, app.view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app.view_port);

    return 0;
}
