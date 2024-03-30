#include "esphome.h"

enum CoverTargetOperation : uint8_t
{
    // order matters to match CoverOperation enum

    // stop door
    TARGET_OPERATION_IDLE = 0,
    // open door
    TARGET_OPERATION_OPEN,
    // close door
    TARGET_OPERATION_CLOSE,
    // do nothing (no action)
    TARGET_OPERATION_NONE,
    // activate door switch once
    TARGET_OPERATION_ACTIVATE_ONCE,
};

class CustomGarageCover : public Component, public Cover
{
public:
    // Constructor
    CustomGarageCover(switch_::Switch *door_switch, binary_sensor::BinarySensor *open_endstop_sensor, binary_sensor::BinarySensor *close_endstop_sensor)
    {
        this->door_switch = door_switch;
        this->open_endstop_sensor = open_endstop_sensor;
        this->close_endstop_sensor = close_endstop_sensor;
        // initialize default values
        this->last_dir = COVER_OPERATION_IDLE;
        this->target_operation = TARGET_OPERATION_NONE;
        this->target_position = 0.0f;
        // assume door is idle. If not once one endstop is reached real state will be updated
        this->current_operation = COVER_OPERATION_IDLE;
        this->switch_activation_interval = 0;
        this->open_duration = 0;
        this->close_duration = 0;
    }

    void set_door_timings(uint32_t switch_activation_interval, uint32_t open_duration, uint32_t close_duration)
    {
        this->switch_activation_interval = switch_activation_interval;
        this->open_duration = open_duration;
        this->close_duration = close_duration;
    }

    float get_setup_priority() const override { return esphome::setup_priority::WIFI; }

    void setup() override
    {
        // check cover position on startup
        if (this->open_endstop_sensor->state)
        {
            // door is open
            this->position = COVER_OPEN;
            this->last_dir = COVER_OPERATION_OPENING;
        }
        else if (this->close_endstop_sensor->state)
        {
            // door is closed
            this->position = COVER_CLOSED;
            this->last_dir = COVER_OPERATION_CLOSING;
        }
        else
        {
            // door neither closed nor open
            // assume door its ad middle position
            this->position = 0.50f;
        }
        this->target_position = this->position;
        // publish states
        this->publish_state(false);
    }

    CoverTraits get_traits() override
    {
        auto traits = CoverTraits();
        traits.set_is_assumed_state(false);
        traits.set_supports_position(true);
        traits.set_supports_stop(true);
        traits.set_supports_tilt(false);
        return traits;
    }

    void control(const CoverCall &call) override
    {
        // This will be called every time the user requests a state change.

        if (call.get_position().has_value())
        {
            // get requested position
            float pos = *call.get_position();
            // ensure position is between 0 and 1
            pos = clamp(pos, 0.0f, 1.0f);

            ESP_LOGD("CustomGarageCover", "Position command received: %0.2f", pos);

            if (pos != this->position)
            {
                // not at target
                // calculate target operation
                this->target_operation = pos < this->position ? TARGET_OPERATION_CLOSE : TARGET_OPERATION_OPEN;
                this->target_position = pos;
            }
        }
        if (call.get_stop())
        {
            ESP_LOGD("CustomGarageCover", "Stop command received.");

            if (this->current_operation != COVER_OPERATION_IDLE)
            {
                this->target_operation = TARGET_OPERATION_IDLE;
            }
        }
    }

    void loop() override
    {
        // This will be called by App.loop()

        static uint32_t last_publish_time = 0;
        static uint32_t last_activation = 0;

        // store current time
        const uint32_t now = millis();

        // recompute position every loop cycle
        this->recompute_position(now);

        // perform one action if target operation different than current operation
        if (this->target_operation != TARGET_OPERATION_NONE && (static_cast<uint8_t>(this->target_operation) != static_cast<uint8_t>(this->current_operation)))
        {
            // only activate door if time greater than activation interval
            if ((now - last_activation) > this->switch_activation_interval)
            {
                this->do_one_action();
                last_activation = now;
                last_publish_time = now;

                // if door was commanded to activate once, delete target operation
                if (this->target_operation == TARGET_OPERATION_ACTIVATE_ONCE)
                {
                    this->target_operation = TARGET_OPERATION_NONE;
                    if (this->current_operation == COVER_OPERATION_CLOSING)
                    {
                        this->target_position = COVER_CLOSED;
                    }
                    else if (this->current_operation == COVER_OPERATION_OPENING)
                    {
                        this->target_position = COVER_OPEN;
                    }
                }
            }
        }

        // target reached, set target as None.
        else if (static_cast<uint8_t>(this->target_operation) == static_cast<uint8_t>(this->current_operation))
        {
            ESP_LOGD("CustomGarageCover", "Target operation reached");
            this->target_operation = TARGET_OPERATION_NONE;
        }

        // when all operations are done and if door is not idling check if target position was reached
        else if (this->current_operation != COVER_OPERATION_IDLE)
        {
            // do not stop door if position requested is FULL OPEN or FULL CLOSE
            // in these cases door will be stopped by end_stop sensors
            if (this->target_position != COVER_CLOSED && this->target_position != COVER_OPEN)
            {
                // check if target position was reached
                if ((this->current_operation == COVER_OPERATION_OPENING && this->position >= this->target_position) ||
                    (this->current_operation == COVER_OPERATION_CLOSING && this->position <= this->target_position))
                {
                    if ((now - last_activation) > this->switch_activation_interval)
                    {
                        ESP_LOGD("CustomGarageCover", "Target position: %0.2f - Stopped at %0.2f", this->target_position, this->position);
                        this->do_one_action();
                        last_activation = now;
                        last_publish_time = now;
                    }
                }
            }
        }

        // send current position every second
        if (this->current_operation != COVER_OPERATION_IDLE && (now - last_publish_time) > 1000)
        {
            this->publish_state(false);
            last_publish_time = now;
        }
    }

    void open_endstop_reached()
    {
        // stop all current actions
        this->target_operation = TARGET_OPERATION_NONE;
        // update states
        this->current_operation = COVER_OPERATION_IDLE;
        this->position = COVER_OPEN;
        this->last_dir = COVER_OPERATION_OPENING;
        this->publish_state(false);
    }

    void open_endstop_released()
    {
        // set state as closing. This will start position update and reporting
        this->last_dir = COVER_OPERATION_CLOSING;
        this->current_operation = COVER_OPERATION_CLOSING;
        this->publish_state(false);
    }

    void close_endstop_reached()
    {
        // stop all current actions
        this->target_operation = TARGET_OPERATION_NONE;
        // update states
        this->current_operation = COVER_OPERATION_IDLE;
        this->position = COVER_CLOSED;
        this->last_dir = COVER_OPERATION_CLOSING;
        this->publish_state(false);
    }

    void close_endstop_released()
    {
        // set state as opening. This will start position update and reporting
        this->last_dir = COVER_OPERATION_OPENING;
        this->current_operation = COVER_OPERATION_OPENING;
        this->publish_state(false);
    }

    void activate_door()
    {
        ESP_LOGD("CustomGarageCover", "Activate once command received");
        this->target_operation = TARGET_OPERATION_ACTIVATE_ONCE;
    }

private:
    switch_::Switch *door_switch;                      // switch that activates the door
    binary_sensor::BinarySensor *open_endstop_sensor;  // binary sensor to detect when door is full open
    binary_sensor::BinarySensor *close_endstop_sensor; // binary sensor to detect when door is full closed
    cover::CoverOperation last_dir;                    // last door direction (open/close)
    CoverTargetOperation target_operation;             // received action to execute
    float target_position;                             // target position to reach
    uint32_t switch_activation_interval;               // time between switch activations
    uint32_t open_duration;                            // time the door needs to fully open
    uint32_t close_duration;                           // time the door needs to fully close

    void recompute_position(const uint32_t now)
    {
        // recalculates door position

        static uint32_t last_recompute_time = 0;

        // only recompute position if door is moving
        if (this->current_operation != COVER_OPERATION_IDLE)
        {
            float dir;
            float action_dur;

            // set dir and duration depending on current movement
            if (this->current_operation == COVER_OPERATION_CLOSING)
            {
                // door closing
                dir = -1.0f;
                action_dur = this->close_duration;
            }
            else
            {
                // door opening
                dir = 1.0f;
                action_dur = this->open_duration;
            }
            // calculate position
            float position = this->position;
            position += (dir * (now - last_recompute_time)) / action_dur;
            this->position = clamp(position, 0.0f, 1.0f);
        }
        // store time
        last_recompute_time = now;
    }

    void do_one_action()
    {
        // Activates the door switch and update state

        // activate switch
        ESP_LOGD("CustomGarageCover", "Switch activated");
        this->door_switch->turn_on();

        // cover state machine (recompute current operation)
        if (this->current_operation == COVER_OPERATION_OPENING || this->current_operation == COVER_OPERATION_CLOSING)
        {
            // door moving -> Stop
            this->current_operation = COVER_OPERATION_IDLE;
        }
        else
        {
            // door idle, check last direction
            if (this->last_dir == COVER_OPERATION_OPENING)
            {
                // last action open -> Close
                this->current_operation = COVER_OPERATION_CLOSING;
                this->last_dir = COVER_OPERATION_CLOSING;
            }
            else
            {
                // last action close -> Open
                this->current_operation = COVER_OPERATION_OPENING;
                this->last_dir = COVER_OPERATION_OPENING;
            }
        }
        // send current state
        this->publish_state(false);
    }
};
