# High-Level Radio Control

This is the upper-level interface to the radio controller.
Whereas the controller itself manages the radio transmissions and
receiving status, the high-level code arranges for queued messages to
be sent, and responses to be published.
It uses RabbitMQ as the message-passing layer for all communications.

It is expected that this control software will run in parallel to the
low-level board.
To communicate with devices over the radio link, send a message to this
interface using RabbitMQ and then listen on the appropriate Pub/Sub
channel for any responses.
Generally communications are of the "fire and forget" variety, so there
is no guarantee that the message was received.
In fact, there's no guarantee the message was even sent.
So the trick is to send the same message frequently until some sort of
response is acquired.
An example is enabling a down-stream device.
An activation message is sent and *if* the device receives it, it will
also respond to a subsequent status request.
Listening for a status response will confirm that the device is alive
and well.
The transmitter code does not maintain any list of active devices.
It is this code which performs that function.

Some requests can be sent directly to this device.
For example, to query the assigned channel and node ID.
Or to confirm that a status message has been received from the device.

## RabbitMQ RPC commands
