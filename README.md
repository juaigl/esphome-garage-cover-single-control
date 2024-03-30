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

* Position control
* Calculate the number of times the control need to be activated to perform the action requested or reach requested position
* Actuate the door many times as needed to perform requested action. For example if position in memory is wrong or unknow because a external control stops the door at middle.
* Detect and update position when the cover is externally commanded. Only if door is full open or closed when commanded or reachs end stop sensors.
* Configuration options for GPIOs, debounce time, open/close durations. time between control actuation...

## Instructions

* Use this repo as an [external component](https://esphome.io/components/external_components)
* Check the [example](example.yaml) provided in this repo

## TODO

* Detect if door is stopped at middle when commanded externally
