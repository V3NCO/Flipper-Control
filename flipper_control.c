#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/canvas.h>
#include <input/input.h>
#include <cJSON/cJSON.h>
#include <stdlib.h>
#include <string.h>
#include <flipper_http/flipper_http.h>

// I Have no idea what this does but its here and i wont touch it
#define TAG "FlipperControlApp"
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT)

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    bool running;
    uint8_t* todraw;
} FlipperControlApp;

static void render_callback(Canvas* canvas, void* ctx) {
    FlipperControlApp* app = ctx;
    canvas_clear(canvas);


    if(app->todraw) {
            for(uint8_t y = 0; y < DISPLAY_HEIGHT; y++) {
                for(uint8_t x = 0; x < DISPLAY_WIDTH; x++) {
                    size_t index = y * DISPLAY_WIDTH + x;
                    if(app->todraw[index] == 1) {
                        canvas_draw_dot(canvas, x, y);
                    }
                }
            }
        } else {
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str(canvas, 5, 32, "Welcome ! Waiting for data...");
        }
}

static void input_callback(InputEvent* event, void* ctx) {
    FlipperControlApp* app = ctx;
    if(event->type == InputTypeShort && event->key == InputKeyBack) {
        app->running = false;
    }
}

static bool parse_json_response(FlipperControlApp* app, const char* json_str) {
    cJSON* json = cJSON_Parse(json_str);
    if(!json) {
        FURI_LOG_E(TAG, "Failed to parse JSON");
        return false;
    }

    cJSON* todraw = cJSON_GetObjectItem(json, "todraw");
    if(!todraw || !cJSON_IsArray(todraw)) {
        FURI_LOG_E(TAG, "JSON does not contain 'todraw' array");
        cJSON_Delete(json);
        return false;
    }

    int array_size = cJSON_GetArraySize(todraw);
    if(array_size <= 0) {
        FURI_LOG_E(TAG, "Empty 'todraw' array");
        cJSON_Delete(json);
        return false;
    }

    // Allocate buffer if first time
    if(!app->todraw) {
        app->todraw = malloc(DISPLAY_SIZE * sizeof(uint8_t));
        if(!app->todraw) {
            FURI_LOG_E(TAG, "Failed to allocate memory for todraw data");
            cJSON_Delete(json);
            return false;
        }
    }

    // Fill with zeros first
    memset(app->todraw, 0, DISPLAY_SIZE * sizeof(uint8_t));

    // Copy data from JSON array to our buffer (only what fits)
    int copy_size = (array_size < DISPLAY_SIZE) ? array_size : DISPLAY_SIZE;
    for(int i = 0; i < copy_size; i++) {
        cJSON* item = cJSON_GetArrayItem(todraw, i);
        if(cJSON_IsNumber(item)) {
            app->todraw[i] = (uint8_t)item->valueint;
        }
    }

    cJSON_Delete(json);
    return true;
}

int32_t flipper_control_app(void* p) {
    UNUSED(p);
    FlipperControlApp app = {0};
    app.todraw = NULL;
    FlipperHTTP *fhttp = flipper_http_alloc();

    if (!fhttp) {
        FURI_LOG_E(TAG, "Failed to allocate HTTP client");
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

        if (!flipper_http_request(fhttp, GET, "http://192.168.0.24:8000/screen/", NULL, NULL))
        {
            FURI_LOG_E(TAG, "Failed to send GET request");
            continue;
        }

        uint32_t timeout_ms = 5000;
        uint32_t elapsed_ms = 0;

        while (fhttp->state != RECEIVING && elapsed_ms < timeout_ms)
        {
            furi_delay_ms(100);
            elapsed_ms += 100;
        }

        if(fhttp->state != IDLE || !fhttp->last_response) {
            FURI_LOG_E(TAG, "Failed to receive response or timeout occurred");
            continue;
        }

        // make something where it gets an item with all the intents of that command and then do if to execute each defined actions.
        FURI_LOG_I(TAG, "Received response: %s", fhttp->last_response);

        if(parse_json_response(&app, fhttp->last_response)) {
            FURI_LOG_I(TAG, "Successfully parsed todraw data");
            view_port_update(app.view_port);
        } else {
            FURI_LOG_E(TAG, "Failed to parse JSON data");
        }
    }

    if(app.todraw) {
        free(app.todraw);
        app.todraw = NULL;
    }

    gui_remove_view_port(app.gui, app.view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app.view_port);
    flipper_http_free(fhttp);

    return 0;
}
