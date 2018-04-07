#include <pebble.h>
#include "utils/comms.h"


/* Flow xtra actions */
typedef struct Action {
  char *name;
  char *payload_on_click;
  bool input_payload;
  struct Action* child_actions;
  bool grid_layout;
  int child_actions_count;
} Action;


/* Flow structure */
typedef struct {
  char *id;
  char *name;
  bool is_running;
  Action* actions;
  int action_count;
} Flow;

/* Views */
static Window* s_window;
static StatusBarLayer* s_status_bar;
static MenuLayer* s_menu_layer;

/* States */
static bool s_bool_receiving;
static Flow* s_flows;
static int s_flow_index;


// method declarations
static void free_actions(Action* actions, int count);


static void free_action(Action action) {
  free_actions(action.child_actions, action.child_actions_count);
  free(action.name);
  free(action.payload_on_click);
}

static void free_actions(Action* actions, int count) {
  int i;
  for (i = 0; i < count; i++) {
    free_action(actions[i]);
  }
  free(actions);
}

static void free_flows() {
  int i;
  for (i = 0; i < s_flow_index; i++) {
    free(s_flows[i].id);
    free(s_flows[i].name);
    free_actions(s_flows[i].actions, s_flows[i].action_count);
  }
  free(s_flows);
}

void log_tuple(Tuple* tuple) {
  if (tuple->type == TUPLE_CSTRING) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tuple key: %d value: %s", (int) tuple->key, tuple->value->cstring);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tuple key: %d value: %d", (int) tuple->key, (int) tuple->value->int32);
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

/* Utility method: duplicates string */
static char* strdup(char* from) {
  size_t len_from = strlen(from) + 1;
  char* result = malloc(len_from);
  strncpy(result, from, len_from);
  return result;
}

/* Utility method: split string in a loop */
static char *strsep(char **stringp, const char *delim) {
  if (*stringp == NULL) { return NULL; }
  char *token_start = *stringp;
  *stringp = strpbrk(token_start, delim);
  if (*stringp) {
    **stringp = '\0';
    (*stringp)++;
  }
  return token_start;
}

/* Utility method: construct flow from app message */
static Flow construct_flow(DictionaryIterator *iter) {
  Flow flow;
  flow.id = strdup(dict_find(iter, MESSAGE_KEY_id)->value->cstring);
  flow.name = strdup(dict_find(iter, MESSAGE_KEY_name)->value->cstring);
  flow.is_running = dict_find(iter, MESSAGE_KEY_is_running)->value->int32 == 1;
  flow.action_count = 0;
  return flow;
}


void action_performed(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "ACTION performed");
  // FIXME call this in on close instead
  action_menu_hierarchy_destroy(action_menu_get_root_level(action_menu), NULL, NULL);
}

static ActionMenuLevel* construct_level(Action* actions, int action_count) {
  ActionMenuLevel* level = action_menu_level_create(action_count);
  int i;
  for (i = 0; i < action_count; i++) {
    Action action = actions[i];
    if (action.child_actions_count <= 0) {
      // if no children, add as an action
      action_menu_level_add_action(level, action.name, action_performed, NULL);
    } else {
      // if with children, recurse and add as level
      ActionMenuLevel* child = construct_level(action.child_actions, action.child_actions_count);
      if (action.grid_layout) {
        action_menu_level_set_display_mode(child, ActionMenuLevelDisplayModeThin);  
      }
      action_menu_level_add_child(level, child, action.name);
    }
  }
  return level;
}

static uint16_t get_num_rows(struct MenuLayer* menu_layer, uint16_t section_index, void *callback_context) {
  return s_flow_index;
}

static void draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  Flow selected = s_flows[cell_index->row];
  menu_cell_basic_draw(ctx, cell_layer, selected.name, selected.is_running ? "Running" : "Stopped", NULL);
}

static void select_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
//   Flow flow = s_flows[cell_index->row];
//   comms_send_params(REQ_TRIGGER_FLOW, flow.id);
  
  // FIXME destroy root level
  ActionMenuLevel* root_level = action_menu_level_create(2);
  
  ActionMenuLevel* wide_level = action_menu_level_create(4);
  action_menu_level_add_action(wide_level, "Action 1", action_performed, NULL);
  action_menu_level_add_action(wide_level, "Action 2", action_performed, NULL);
  action_menu_level_add_action(wide_level, "Action 3", action_performed, NULL);
  action_menu_level_add_action(wide_level, "Action 4", action_performed, NULL);
  
  ActionMenuLevel* grid_level = action_menu_level_create(7);
  action_menu_level_set_display_mode(grid_level, ActionMenuLevelDisplayModeThin);
  action_menu_level_add_action(grid_level, "G 1", action_performed, NULL);
  action_menu_level_add_action(grid_level, "G 2", action_performed, NULL);
  action_menu_level_add_action(grid_level, "G 3", action_performed, NULL);
  action_menu_level_add_action(grid_level, "GSI 4", action_performed, NULL);
  action_menu_level_add_action(grid_level, "G 5", action_performed, NULL);
  action_menu_level_add_action(grid_level, "Gwas 6", action_performed, NULL);
  action_menu_level_add_action(grid_level, "G 7", action_performed, NULL);

  // add the levels
  action_menu_level_add_child(root_level, wide_level, "This is wide level");
  action_menu_level_add_child(root_level, grid_level, "Grid level");
  
  // FIXME construct actions
  ActionMenuConfig config = (ActionMenuConfig){
    .root_level = root_level,
    .context = NULL,
    //.colors = (),
    .will_close = NULL,
    .did_close = NULL,
    .align = ActionMenuAlignCenter
  };
  ActionMenu* action_menu = action_menu_open(&config);
  
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
        free_flows(); // recycle data
        s_bool_receiving = true;
        s_flow_index = 0;
        s_flows =  malloc(item_length * sizeof(Flow));
      }
      
    // construct items if still receiving
    } else if (s_bool_receiving) {
      s_flows[s_flow_index] = construct_flow(iter);
      
      
      // FIXME testing hardcoded actions
      if (s_flow_index == 0) {
        
  
        strsep(&s_flows[0].name, ",");
      }

      s_flow_index++;
    }
  }
}

static void window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect root_bounds = layer_get_bounds(window_layer);
  
  // add the StatusBarLayer
  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, GColorWhite, GColorBlack);
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeDotted);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
  
  // add menu layer
  s_menu_layer = menu_layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT, root_bounds.size.w, root_bounds.size.h));
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
  status_bar_layer_destroy(s_status_bar);
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