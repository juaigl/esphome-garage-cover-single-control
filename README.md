# ESPHome Single Button Cover
Project to control a garage cover or gate with [ESPHome](https://esphome.io/) and [Home Assistant](https://www.home-assistant.io/).

This project uses an esp board, a relay and two reed switches.

## Cover description
Cover is controlled with a single button push. Each time the button is pushed, it performs an action according following state machine:  
* Sequence: open -> stop -> close -> stop -> open
* When cover reach the end (open or close) it counts as a stop action

## Project features
* Position reporting based on time (no position control for now)
* Calculate the number of times the button need to be pushed to perform the action requested
* Detect and update position when the cover is externally commanded when full open or closed
* Configuration options for GPIOs, debounce time, open/close durations. time between clicks...

## Instructions
* Copy this files to your esphome directory in home assistant
* Modify cover.yaml with your desired configuration
* Define the following variables in your home assistant secrets.yaml
```yaml
esphome_api_ota_password: password used for esphome api and ota update
wifi_ssid: your wifi ssid
wifi_password: your wifi password
```

You can change the name of the previous variables editing config_base.yaml and wifi_base.yaml

Note: secrets.yaml will inherit all your data from home assistant secrets.yaml. It's possible that you need to edit [this line](https://github.com/juaigl/esphome-single-button-cover/blob/master/common/secrets.yaml#L1) to point yo your home assistant secrets.yaml. If you want to change this behavior, delete the line and define the variables there.
```yaml
<<: !include ../../secrets.yaml
```

## TODO
* Store target action and perform action in main loop instead of on_action
* Position control
