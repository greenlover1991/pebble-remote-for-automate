#pragma once


/** Request Codes */
#define REQ_MAIN_LIST "REQ_MAIN_LIST"
#define REQ_MAIN_LIST_END -1
#define REQ_TRIGGER_FLOW "REQ_TRIGGER_FLOW"

/** True with PebbleKit JS is ready*/
bool comms_is_js_ready();

void comms_send(char* request_code);
void comms_send_params(char* request_code, char* request_params);

void comms_init(AppMessageInboxReceived inbox_received_handler,
    AppMessageInboxDropped inbox_dropped_handler,
    AppMessageOutboxSent outbox_sent_handler,
    AppMessageOutboxFailed outbox_failed_handler);
void comms_deinit();