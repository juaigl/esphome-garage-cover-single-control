# ESPHome Garage Cover Single Control

Project to control a garage cover or gate with [ESPHome](https://esphome.io/)
and [Home Assistant](https://www.home-assistant.io/).

This project uses:
* Esp board [compatible](https://esphome.io/#devices) with ESPHome
* Relay to activate the cover control
* Two reed switches to detect end positions of the door.

## Cover description

Cover is controlled with a single control using a relay. Each time the control is activated, it performs an action according following
state machine:

* Sequence: open -> stop -> close -> stop -> open
* When cover reach the end (open or close) it counts as a stop action

## Project features

* Position reporting based on time (no position control for now)
* Calculate the number of times the control need to be activated to perform the action requested
* Actuate the door many times as needed to perform requested action. For example if position in memory is wrong or unknow because a external control stops the door at middle.
* Detect and update position when the cover is externally commanded. Only if door is full open or closed when commanded or reachs end stop sensors.
* Configuration options for GPIOs, debounce time, open/close durations. time between control actuation...

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

Note: secrets.yaml will inherit all your data from home assistant secrets.yaml. It's possible that you need to
edit [this line](https://github.com/juaigl/esphome-single-button-cover/blob/master/common/secrets.yaml#L1) to point to
your home assistant secrets.yaml. If you want to change this behavior, delete the line and define the variables there.

```yaml
<<: !include ../../secrets.yaml
```

## Code

The code is divided in packages to allow easy configuration and reutilization.  
[cover.yaml](cover.yaml) is the main file that contains configuration options and import the used packages.

The door logic is placed at file [config_base.yaml](common/config_base.yaml)

## TODO

* Migrate to custom component
* Detect if door is stopped at middle when commanded externally
* Position control
