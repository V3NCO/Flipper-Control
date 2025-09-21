#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/canvas.h>
#include <input/input.h>
#include <flipper_http/flipper_http.h>

// I Have no idea what this does but its here and i wont touch it
#define TAG "FlipperControlSample"

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    bool running;
} FlipperControlApp;

static void render_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    int todraw[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    canvas_clear(canvas);


    for (uint8_t y = 0; y < 64; y++) {
        for (uint8_t x = 0; x < 128; x++) {
            if (todraw[x] == 1) {
                canvas_draw_dot(canvas, x, y);
            }
        }
    }
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
    FlipperHTTP *fhttp = flipper_http_alloc();

    if (!fhttp) {
        return -1;
    }



    app.view_port = view_port_alloc();
    view_port_draw_callback_set(app.view_port, render_callback, &app);
    view_port_input_callback_set(app.view_port, input_callback, &app);

    app.gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app.gui, app.view_port, GuiLayerFullscreen);

    app.running = true;
    while(app.running) {
        furi_delay_ms(7500);

        fhttp->state = IDLE;

        if (!flipper_http_request(fhttp, GET, "https://catfact.ninja/fact", "{\"Content-Type\":\"application/json\"}", NULL))
        {
            FURI_LOG_E(TAG, "Failed to send GET request");
            return -1;
        }

        fhttp->state = RECEIVING;

        while (fhttp->state != IDLE)
        {
            furi_delay_ms(100);
        }

        FURI_LOG_I(TAG, "Received response: %s", fhttp->last_response);

        char *fact = get_json_value(fhttp->last_response, "fact");
        if (fact)
        {
            FURI_LOG_I(TAG, "Cat fact: %s", fact);
        }
        else
        {
            FURI_LOG_E(TAG, "Failed to parse cat fact");
        }
    }

    gui_remove_view_port(app.gui, app.view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app.view_port);
    flipper_http_free(fhttp);

    return 0;
}
