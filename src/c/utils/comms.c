#include <pebble.h>
#include "comms.h"

#define MIN(a,b)  ((a) < (b) ? (a) : (b))

// flag when PebbleKit JS is ready
static bool s_js_ready;

// app message handlers
static AppMessageInboxReceived s_inbox_received_handler;
static AppMessageInboxDropped s_inbox_dropped_handler;
static AppMessageOutboxSent s_outbox_sent_handler;
static AppMessageOutboxFailed s_outbox_failed_handler;

bool comms_is_js_ready() {
  return s_js_ready;
}

void comms_send(char* request_code) {
  DictionaryIterator *outbox;
  app_message_outbox_begin(&outbox);
  // Send the app message
  dict_write_cstring(outbox, MESSAGE_KEY_REQ_CODE, request_code);

  app_message_outbox_send();
}

char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message received.");
  Tuple *ready_tuple = dict_find(iter, MESSAGE_KEY_JS_READY);
  if (ready_tuple) {
    // PebbleKit JS is ready! Safe to send messages
    s_js_ready = true;
  }
 
//   // FIXME sgo persist preferences in watch storage
//   Tuple *flick_to_capture = dict_find(iter, MESSAGE_KEY_FLICK_TO_CAPTURE);
//   if (flick_to_capture) {
//     s_flick_to_capture = flick_to_capture->value->int32 == 1;
//   }
  
//   Tuple *vib_on_capture = dict_find(iter, MESSAGE_KEY_VIBRATE_ON_CAPTURE);
//   if (vib_on_capture) {
//     s_vibrate_on_capture = vib_on_capture->value->int32 == 1;
//   }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, comms_is_js_ready() ? "Ready!" : "nope");
  
  
  // log received app message
//   if (!ready_tuple && !flick_to_capture && !vib_on_capture) {
//     int req_code = dict_find(iter, MESSAGE_KEY_REQ_CODE)->value->int32;
//     int resp_code = dict_find(iter, MESSAGE_KEY_RESP_CODE)->value->int32;
//     char *resp_msg = dict_find(iter, MESSAGE_KEY_RESP_MESSAGE)->value->cstring;
  
//     char buf[512];
//     snprintf(buf, sizeof(buf), "Request code: %d, resp code: %d", 
//             req_code, resp_code);
    
//     APP_LOG(APP_LOG_LEVEL_DEBUG, buf);
//     APP_LOG(APP_LOG_LEVEL_DEBUG, resp_msg);
//   }
  
  
  if (s_inbox_received_handler) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "calling static handler");
    s_inbox_received_handler(iter, context);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Dropped message! Reason given: %s", translate_error(reason));
  
  if (s_inbox_dropped_handler) {
    inbox_dropped_handler(reason, context);
  }
}

static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message sent.");
  
  if (s_outbox_sent_handler) {
    s_outbox_sent_handler(iter, context);
  }
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to send message. Reason = %s", translate_error(reason));
  
  if (s_outbox_failed_handler) {
    s_outbox_failed_handler(iter, reason, context);
  }
}

void comms_init(AppMessageInboxReceived l_inbox_received_handler,
    AppMessageInboxDropped l_inbox_dropped_handler,
    AppMessageOutboxSent l_outbox_sent_handler,
    AppMessageOutboxFailed l_outbox_failed_handler) {
  s_inbox_received_handler = l_inbox_received_handler;
  s_inbox_dropped_handler = l_inbox_dropped_handler;
  s_outbox_sent_handler = l_outbox_sent_handler;
  s_outbox_failed_handler = l_outbox_failed_handler;
  
  // hook up app messaging
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Max buffer sizes are %li / %li",
    app_message_inbox_size_maximum(),
    app_message_outbox_size_maximum());
  app_message_open(app_message_inbox_size_maximum(), MIN(512, app_message_outbox_size_maximum()));  
}

void comms_deinit() {
  s_inbox_received_handler = NULL;
  s_inbox_dropped_handler = NULL;
  s_outbox_sent_handler = NULL;
  s_outbox_failed_handler = NULL;
  app_message_deregister_callbacks();
}