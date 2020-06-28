# pebble-remote-for-automate
Simple list to trigger Llamalab's Automate flows from watch

**Pebble watchapp:** https://apps.rebble.io/en_US/application/5afda2da461a8d6af4000d7b 

**Setup:**
1. Download [Llamalab's Automate](https://play.google.com/store/apps/details?id=com.llamalab.automate) Android app from the Play Store. It's similar to Tasker.
2. Download the ff. Automate flows:
  * [Listing flow](https://llamalab.com/automate/community/flows/21539) = this flow reads all the available flows in your Automate app, and sends them to the Pebble watchapp.
    * by default, it only broadcasts flows with "pbv=1" in the description description 
  * [Triggering flow](https://llamalab.com/automate/community/flows/21540) = this flow listens for a button click from the Pebble watchapp, then starts the corresponding flow given the clicked flow ID.
  * Sample - [Turn on Flashlight](https://llamalab.com/automate/community/flows/35568) = on click from Pebble watchapp, turn on phone's flashlight.
  * Sample (requires Pebble 2 HR) - [Speak to watch & the phone talks back](https://llamalab.com/automate/community/flows/35569) = a sample triggering flow which allows user to talk to the Pebble and pass the spoken speech as payload to the flow. This example will simply wait for a spoken "Boy", then trigger the phone to speak back 
3. After downloading the app and the flows, start the listing flow and the triggering flow. The listing flow will report to the pebble app the list of available flows that can be triggered. The triggering flow waits for button clicks from the pebble app.
In the pebble app, on tap of the options, the triggering flow will reroute to the actual flow to be executed.

**Notes/disclaimer:**
1. You may have to edit the listing flow to point to the correct flow ID of the downloaded triggering and sample flows.
2. The free Automate app has a limit of 30 running blocks. So you may have to adjust the flow as well if the setup doesn't work. I'm currently a premium Automate user which removes the limit.

