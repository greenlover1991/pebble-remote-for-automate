# pebble-remote-for-automate
Simple list to trigger Llamalab's Automate flows from watch

**Pebble watchapp:** https://apps.rebble.io/en_US/application/5afda2da461a8d6af4000d7b 

**Setup:**
1. Download [Llamalab's Automate](https://play.google.com/store/apps/details?id=com.llamalab.automate) Android app from the Play Store. It's similar to Tasker.
2. Download the ff. Automate flows:
  * [⌚ Pebble: Flow list generator](https://llamalab.com/automate/community/flows/21539) = this flow reads all the available flows in your Automate app, and sends them to the Pebble watchapp.
    * by default, it only broadcasts flows with "pbv=1" in the description description 
  * [⌚ Pebble: Trigger flows](https://llamalab.com/automate/community/flows/21540) = this flow listens for a button click from the Pebble watchapp, then starts the corresponding flow given the clicked flow ID.
3. Download some sample flows:
  * Simple toggle - [Turn on Flashlight](https://llamalab.com/automate/community/flows/35568) = on click from Pebble watchapp, turn on phone's flashlight for 5 seconds.
  * Flow with choices - [Select bus route and notify of arrival time](https://llamalab.com/automate/community/flows/37143) = a sample flow with 2 bus route choices, `12A` and `42`. The flow description contains the comma-separated values as payload to be sent to the triggered flow. So if you choose `KMB bus ETA > Options > 12A` from the watch app, it will send `12A` to the flow as payload. Currently, the flow will just print out from a dummy array web since the web services are no longer valid. So just replace the web request blocks with your own APIs. Specifying `"is_mandatory_payload":0` will allow you to start the flow without any payloads. 
  ```
  payload="{\"choice_count\":2,\"choice_labels\":\"12A,42\",\"choice_payloads\":\"12A,42\",\"is_mandatory_payload\":1}"
  ```
  * Flow with choices and voice input (requires Pebble 2 HR) - [Speak to watch & the phone talks back](https://llamalab.com/automate/community/flows/35569) = a sample triggering flow which allows user to talk to the Pebble and pass the spoken speech as payload to the flow. This example will simply wait for a spoken "Boy", then trigger the phone to speak back `I am steven muhaha` which is option `A`. Specifying `"is_textual_payload":1` will show a `Voice` option in the watch app.
  ```
  payload="{\"choice_count\":3,\"choice_labels\":\"Boy,Girl,Luke\",\"choice_payloads\":\"A,B,C\",\"is_mandatory_payload\":1,\"is_textual_payload\":1}"
  ```
4. After downloading the app and the flows, start the listing flow and the triggering flow. The listing flow will report to the pebble app the list of available flows that can be triggered. The triggering flow waits for button clicks from the pebble app. So both of the flows have to be kept running all the time. 
In the pebble watch app, on tap of the options, the triggering flow will reroute to the actual flow to be executed.

**Notes/disclaimer:**
1. The free Automate app has a limit of 30 running blocks. So you need to adjust the flow if the setup doesn't work. I'm currently a premium Automate user which removes the limit.
2. I only tested the Dictation functionality using the original Pebble services, but I have not tested the Dictation services in Rebble, so the watch app might fail when using voice. 
