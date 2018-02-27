const REQ_MAIN_LIST = "REQ_MAIN_LIST";
const REQ_MAIN_LIST_END = -1;

//  APP messages
Pebble.addEventListener('ready',
  function(e) {
    console.log("PebbleKit JS READY");
    
    Pebble.sendAppMessage({ 'JS_READY': 1 });
  }
);

Pebble.addEventListener('appmessage',
  function(message) {
    console.log('App message received: ' + JSON.stringify(message));
    
    // process app requests
    var mapCodeToFn = [];
    mapCodeToFn[REQ_MAIN_LIST] = sendMainList;
    
    var reqCode = message.payload.REQ_CODE;
    if (reqCode in mapCodeToFn) {
      mapCodeToFn[reqCode](message);
    } else {
      console.log('Cannot handle app request code for PebbleKit processing, ignoring: ' + reqCode);
    }
  }
);

function sendMainList(message) {
  // FIXME receive from automate
//   var flows = message.payload.FLOWS;
  var flows = [
    {"id": "item1", "name": "steve1", "description": "sample description1"},
    {"id": "item2", "name": "steve2", "description": "sample description2"},
    {"id": "item3", "name": "steve3", "description": "sample description3"},
    {"id": "item4", "name": "steve4", "description": "sample description4"},
    {"id": "item5", "name": "steve5", "description": "sample description5"}
  ];
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

// function sendAppMsg(reqCode, httpCode, msg, obj) {
//   // FIXME needs to be a flat json
//   var json = { 'REQ_CODE': reqCode, 'RESP_CODE': httpCode, 'RESP_MESSAGE': msg };
//   //if (obj != undefined) {
//     //json['RESP_DATA'] = obj;
//   //}
//   Pebble.sendAppMessage(json);
// }   