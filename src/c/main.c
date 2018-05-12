#include <pebble.h>
#include "utils/comms.h"

/* Action click data */
typedef struct {
  char *req_code;
  char *payload;
} ActionContext;

/* Flow structure */
typedef struct {
  int id;
  char *name;
  bool is_running;
  
  // payloads:
  bool is_mandatory_payload;
  
  // choice payloads
  bool is_grid;
  int choice_count;  
  char** choice_labels;
  char** choice_payloads;
  
  // OR freetext payload
  bool is_textual_payload;
} Flow;

/* Views */
static Window* s_window;
static StatusBarLayer* s_status_bar;
static MenuLayer* s_menu_layer;

/* States */
static bool s_bool_receiving;
static Flow* s_flows;
static int s_flow_index;

static void free_flows() {
  int i, j;
  for (i = 0; i < s_flow_index; i++) {
    Flow flow = s_flows[i];    
    for (j = 0; j < flow.choice_count;  j++) {
      free(flow.choice_labels[j]);
      free(flow.choice_payloads[j]);
    }
    free(flow.name);
    free(flow.choice_labels);
    free(flow.choice_payloads);
  }
  free(s_flows);
}

void log_tuple(Tuple* tuple) { 
  if (tuple->type == TUPLE_CSTRING) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tuple key: %d value: %s", (int) tuple->key, tuple->value->cstring);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tuple key: %d int value: %d", (int) tuple->key, (int) tuple->value->int32);
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

static char** from_csv(char **strp, int count) {
  char** result = malloc(count * sizeof(char*));
  char *dup, *tofree, *token;
  tofree = dup =  strdup(*strp);
  int i = 0;
  while ((token = strsep(&dup, ",")) != NULL) {
    result[i] = strdup(token);
    i++;
  }
  free(tofree);

  return result;
}

/* Utility method: construct flow from app message */
static Flow construct_flow(DictionaryIterator *iter) {
  Flow flow;
  flow.id = dict_find(iter, MESSAGE_KEY_id)->value->int32;
  flow.name = strdup(dict_find(iter, MESSAGE_KEY_name)->value->cstring);
  flow.is_running = dict_find(iter, MESSAGE_KEY_is_running)->value->int32 == 1;
  
  // optional params:
  Tuple* tuple;
  tuple = dict_find(iter, MESSAGE_KEY_is_mandatory_payload);
  flow.is_mandatory_payload = (tuple != NULL && tuple->value->int32 == 1);
  
  tuple = dict_find(iter, MESSAGE_KEY_is_grid);
  flow.is_grid = (tuple != NULL && tuple->value->int32 == 1);
  tuple = dict_find(iter, MESSAGE_KEY_choice_count);
  flow.choice_count = tuple == NULL ? 0 : tuple->value->int32;
  
  char* str;
  tuple = dict_find(iter, MESSAGE_KEY_choice_labels);
  str = flow.choice_count > 0 ? tuple->value->cstring : NULL;
  flow.choice_labels = flow.choice_count > 0 ? from_csv(&str, flow.choice_count) : NULL;
    
  tuple = dict_find(iter, MESSAGE_KEY_choice_payloads);
  str = flow.choice_count > 0 ? tuple->value->cstring : NULL;
  flow.choice_payloads = flow.choice_count > 0 ? from_csv(&str, flow.choice_count) : NULL;
    
  tuple = dict_find(iter, MESSAGE_KEY_is_textual_payload);
  flow.is_textual_payload = (tuple != NULL && tuple->value->int32 == 1);
  
  return flow;
}

static void free_action_context(ActionContext* ptr) {
  free(ptr->req_code);
  free(ptr->payload);
  free(ptr);
}

static ActionContext* construct_action_context(char* req_code, char* payload) {
  ActionContext* ptr = malloc(sizeof(ActionContext));
  ptr->req_code = strdup(req_code);
  if (payload == NULL) {
    ptr->payload = NULL;
  } else {
    ptr->payload = strdup(payload);  
  }
  return ptr;
}

static void action_performed(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  Flow* flow = (Flow*) action_menu_get_context(action_menu);
  ActionContext* ctx = (ActionContext*) action_menu_item_get_action_data(action);
  static char flow_id[8];
  snprintf(flow_id, sizeof(flow_id), "%d", flow->id);
  comms_send_params_payload(ctx->req_code, flow_id, ctx->payload);
}

/**
 * BEGIN Dictation methods
 **/
#if defined (PBL_MICROPHONE)
static DictationSession *s_dictation_session;
static int s_curr_dictation_flow_id;
static char s_dictation_buffer[512];
static void dictation_session_callback(DictationSession *session, DictationSessionStatus status,
                                       char *transcription, void *context) {
  if (status == DictationSessionStatusSuccess) {;
    // TODO hardcoded Request code for now                          
    static char flow_id[8];
    snprintf(flow_id, sizeof(flow_id), "%d", s_curr_dictation_flow_id);
    comms_send_params_payload(REQ_TRIGGER_FLOW, flow_id, transcription);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dictation failed %d", (int) status);
  }
}

// callback when Voice is selected
static void action_mic_performed(ActionMenu *action_menu, const ActionMenuItem *action, void *context) {
  Flow* flow = (Flow*) action_menu_get_context(action_menu);
  s_curr_dictation_flow_id = flow->id;
  dictation_session_start(s_dictation_session);  
}
#endif
/**
 * END Dictation methods
 **/

static void on_each_action_menu_item(const ActionMenuItem *item, void *context) {
  ActionContext* ctx = (ActionContext*) action_menu_item_get_action_data(item);
  if (ctx != NULL) {
    free_action_context(ctx);  
  }
}

static void action_menu_closed(ActionMenu *menu, const ActionMenuItem *performed_action, void *context) {
  action_menu_hierarchy_destroy(action_menu_get_root_level(menu), on_each_action_menu_item, NULL);
}

static void add_choices_level(ActionMenuLevel* parent, Flow* flow) {
  ActionMenuLevel* level = action_menu_level_create(flow->choice_count);
  if (flow->is_grid) {
    action_menu_level_set_display_mode(level, ActionMenuLevelDisplayModeThin);
  }
  
  int i;
  for (i = 0; i < flow->choice_count; i++) {
    char* label = flow->choice_labels[i];
    char* payload = flow->choice_payloads[i];
    action_menu_level_add_action(level, label, action_performed, construct_action_context(REQ_TRIGGER_FLOW, payload));
  }
  action_menu_level_add_child(parent, level, "Options");
}

static void add_textual_payload_actions(ActionMenuLevel* parent) {
  #if defined (PBL_MICROPHONE)
    action_menu_level_add_action(parent, "Voice", action_mic_performed, NULL); 
  #endif
  
  // FIXME use T3 window
//   action_menu_level_add_action(parent, "T3 input", action_performed, NULL);
}

static void start_action_menu(Flow* flow) {
  int root_count = 0;
  bool supports_mic = PBL_IF_MICROPHONE_ELSE(true, false);
  if (!flow->is_mandatory_payload) root_count++;
  root_count++;
  if (flow->choice_count > 0) root_count++;
  if (flow->is_textual_payload) root_count += (supports_mic ? 1 : 0); // FIXME increment for T3 when implemented
  
  ActionMenuLevel* root_level = action_menu_level_create(root_count);
  if (!flow->is_mandatory_payload) action_menu_level_add_action(root_level, "Start", action_performed, construct_action_context(REQ_TRIGGER_FLOW, NULL));
  action_menu_level_add_action(root_level, "Stop", action_performed, construct_action_context(REQ_STOP_FLOW, NULL));
  if (flow->choice_count > 0) add_choices_level(root_level, flow);
  if (flow->is_textual_payload) add_textual_payload_actions(root_level);
  
  ActionMenuConfig config = (ActionMenuConfig){
    .root_level = root_level,
    .did_close = action_menu_closed,
    .context = flow
  };
  action_menu_open(&config);
}

static uint16_t get_num_rows(struct MenuLayer* menu_layer, uint16_t section_index, void *callback_context) {
  return s_flow_index;
}

static void draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  Flow selected = s_flows[cell_index->row];
  menu_cell_basic_draw(ctx, cell_layer, selected.name, selected.is_running ? "Running" : "Stopped", NULL);
}

static void refresh_menu_layer() {
  menu_layer_reload_data(s_menu_layer);
}

static void select_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  Flow* flow = &s_flows[cell_index->row];
  start_action_menu(flow);
}

static void select_long_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  comms_send(REQ_MAIN_LIST);
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
        s_flows = malloc(item_length * sizeof(Flow));
      }
      
    // construct items if still receiving
    } else if (s_bool_receiving) {
      s_flows[s_flow_index] = construct_flow(iter);
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
    .select_click = select_click,
    .select_long_click = select_long_click
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
  
  #if defined (PBL_MICROPHONE)
    s_dictation_session = dictation_session_create(sizeof(s_dictation_buffer), 
       dictation_session_callback, NULL);
  #endif
}

static void deinit() {
  #if defined (PBL_MICROPHONE)
    dictation_session_destroy(s_dictation_session);
  #endif

  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();  
}