#include <pebble.h>
#include "utils/comms.h"

/* Flow structure */
typedef struct {
  char *id;
  char *name;
  char *description;
} Flow;

/* Views */
static Window* s_window;
static MenuLayer* s_menu_layer;

/* States */
static bool s_bool_receiving;
static Flow* s_flows;
static int s_flow_index;

static void free_flows() {
  int i;
  for (i = 0; i < s_flow_index; i++) {
    free(s_flows[i].id);
    free(s_flows[i].name);
    free(s_flows[i].description);
  }
  free(s_flows);
}

void log_tuple(Tuple* tuple) {
  if (tuple->type == TUPLE_CSTRING) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tuple key: %d value: %s", tuple->key, tuple->value->cstring);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tuple key: %d value: %d", tuple->key, tuple->value->int32);
  }
}

void log_dict(DictionaryIterator* iter) {
  Tuple* tuple = dict_read_first(iter);
  if (tuple != NULL) {
    log_tuple(tuple);    
    while ((tuple = dict_read_next(iter)) != NULL) {
      log_tuple(tuple);
    }
  }
}

static char* strdup(char* from) {
  size_t len_from = strlen(from) + 1;
  char* result = malloc(len_from);
  strncpy(result, from, len_from);
  return result;
}

static uint16_t get_num_rows(struct MenuLayer* menu_layer, uint16_t section_index, void *callback_context) {
  return s_flow_index;
}

static void draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  Flow selected = s_flows[cell_index->row];
  menu_cell_basic_draw(ctx, cell_layer, selected.id, selected.description, NULL);
}

static void select_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  // FIXME sgo send app message to start flow
}

static void refresh_menu_layer() {
  menu_layer_reload_data(s_menu_layer);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "local inbox received handler");
  
  log_dict(iter);
  
  if (dict_find(iter, MESSAGE_KEY_JS_READY)) {
    // FIXME sgo correct the message or use progress dialog
    s_bool_receiving = false;
    comms_send(REQ_MAIN_LIST);
    return;
  }
  
  // await signal for start and end of listing with number of items
  if (comms_is_js_ready()) {

    // list signals
    if (dict_find(iter, MESSAGE_KEY_REQ_CODE) && 
        strcmp(dict_find(iter, MESSAGE_KEY_REQ_CODE)->value->cstring, REQ_MAIN_LIST) == 0) {
      int item_length = dict_find(iter, MESSAGE_KEY_REQ_PARAMS)->value->int32;
      
      APP_LOG(APP_LOG_LEVEL_DEBUG, "item length: %d", item_length);
      
      if (item_length == REQ_MAIN_LIST_END) {
        s_bool_receiving = false;
        refresh_menu_layer();
      } else {
        s_bool_receiving = true;
        s_flow_index = 0;
        s_flows =  malloc(item_length * sizeof(Flow));
      }
      
    // construct items if still receiving
    } else if (s_bool_receiving) {
      Flow flow;
      flow.id = strdup(dict_find(iter, MESSAGE_KEY_id)->value->cstring);
      flow.name = strdup(dict_find(iter, MESSAGE_KEY_name)->value->cstring);
      flow.description = strdup(dict_find(iter, MESSAGE_KEY_description)->value->cstring);
      s_flows[s_flow_index] = flow;

      s_flow_index++;
    }
  }
}

static void window_load(Window* window) {
  // add menu layer
  Layer* window_layer = window_get_root_layer(window);
  s_menu_layer = menu_layer_create(layer_get_bounds(window_layer));
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = get_num_rows,
    .draw_row = draw_row,
    .select_click = select_click
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, s_window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  
  comms_init(inbox_received_handler, NULL, // FIXME sgo handle no app inbox connection
             NULL, NULL);
}

static void window_unload(Window* window) {
  comms_deinit();
  menu_layer_destroy(s_menu_layer);
  free_flows();
}

static void init() {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  
  window_stack_push(s_window, true);
}

static void deinit() {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();  
}