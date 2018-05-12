const REQ_MAIN_LIST = "REQ_MAIN_LIST";
const REQ_MAIN_LIST_END = -1;
const REQ_TRIGGER_FLOW = "REQ_TRIGGER_FLOW";

const DEBUGGING = true;


//  APP messages
Pebble.addEventListener('ready',
  function(e) {
    console.log("PebbleKit JS READY");
    
    Pebble.sendAppMessage({ 'JS_READY': 1 });
  }
);

if (DEBUGGING) {
  // EMULATOR use only
  var flows = [
      {
        "id": 1, "name": "steve1", "is_running": 1, 
        "is_grid": 1, "choice_count": 4, 
        "choice_labels": "A,B,C,D", 
        "choice_payloads": "1,2,3,4",
        "is_textual_payload": 1
      },
      {
        "id": 2, "name": "steve2", "is_running": 0,
        "choice_count": 3, 
        "choice_labels": "B,C,D", 
        "choice_payloads": "2,3,4",
        "is_mandatory_payload": 1
      },
      {"id": 3, "name": "steve3", "is_running": 0},
      {"id": 4, "name": "steve4", "is_running": 1, "is_textual_payload": 1},
      {"id": 5, "name": "steve5", "is_running": 0}
    ];
  
  Pebble.addEventListener('appmessage',
    function(message) {
      console.log('App message received: ' + JSON.stringify(message));
      
      // process app requests
      var mapCodeToFn = {};
      mapCodeToFn[REQ_MAIN_LIST] = sendMainList;
      mapCodeToFn[REQ_TRIGGER_FLOW] = triggerFlow;
      
      var reqCode = message.payload.REQ_CODE;
      if (reqCode in mapCodeToFn) {
        mapCodeToFn[reqCode](message);
      } else {
        console.log('Cannot handle app request code for PebbleKit processing, ignoring: ' + reqCode);
      }
    }
  );
  
  function sendMainList(message) {
    console.log("Got main list from phone. Start sending items: " + flows.length);
    
    Pebble.sendAppMessage({
      'REQ_CODE': REQ_MAIN_LIST,
      'REQ_PARAMS': flows.length
    });
    
    sendNextItem(flows, 0, function() {
      Pebble.sendAppMessage({
        'REQ_CODE': REQ_MAIN_LIST,
        'REQ_PARAMS': REQ_MAIN_LIST_END
      });
    });
  }
  
  function sendNextItem(items, index, fnLastItem) {
    var item = items[index];
    console.log("sending " + (index+1) + " out of " + items.length);
    Pebble.sendAppMessage(item, function(e) {
      index++;
      if (index < items.length) {
        sendNextItem(items, index, fnLastItem);
      } else {
        console.log("Last item sent");
        if (fnLastItem) {
          fnLastItem();  
        }
      }
    });
  }
  
  function triggerFlow(message) {
    var id = message.payload.REQ_PARAMS;
    for (var i = 0; i < flows.length; i++) {
      var flow = flows[i];
      if (flow.id === id) {
        flow.is_running = (flow.is_running + 1) % 2;
        sendMainList();
      }
    }
  }
  
  // function sendAppMsg(reqCode, httpCode, msg, obj) {
  //   // FIXME needs to be a flat json
  //   var json = { 'REQ_CODE': reqCode, 'RESP_CODE': httpCode, 'RESP_MESSAGE': msg };
  //   //if (obj != undefined) {
  //     //json['RESP_DATA'] = obj;
  //   //}
  //   Pebble.sendAppMessage(json);
  // }   
}