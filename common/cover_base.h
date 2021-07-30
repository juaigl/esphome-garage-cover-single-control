#include "esphome.h"

enum CoverTargetOperation : uint8_t
{
    // order matters to match CoverOperation enum
    TARGET_OPERATION_IDLE = 0,
    TARGET_OPERATION_OPEN,
    TARGET_OPERATION_CLOSE,
    TARGET_OPERATION_NONE,
};

class CustomGarageCover : public Component, public Cover
{
public:
    // Constructor
    CustomGarageCover(Switch *door_switch, BinarySensor *open_endstop_sensor, BinarySensor *close_endstop_sensor)
    {
        this->door_switch = door_switch;
        this->open_endstop_sensor = open_endstop_sensor;
        this->close_endstop_sensor = close_endstop_sensor;
        // initialize default values
        this->last_dir = COVER_OPERATION_IDLE;
        this->target_operation = TARGET_OPERATION_NONE;
        this->switch_activation_interval = 0;
        this->open_duration = 0;
        this->close_duration = 0;
    }

    void set_door_timings(uint32_t switch_activation_interval, uint32_t open_duration, uint32_t close_duration)
    {
        this->switch_activation_interval = switch_interval;
        this->open_duration = open_duration;
        this->close_duration = close_duration;
    }

    float get_setup_priority() const override { return esphome::setup_priority::DATA; }

    void setup() override
    {
        // check cover position on startup
        if (if this->open_endstop_sensor->state)
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
        // assume door is idle. If not once endstop is reached real state will be updated
        this->current_operation = COVER_OPERATION_IDLE;
        // publish states
        this->publish_state(false);
    }

    CoverTraits get_traits() override
    {
        auto traits = CoverTraits();
        traits.set_is_assumed_state(false);
        traits.set_supports_position(true);
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

            ESP_LOGD("CustomGarageCover", "Position command received: %0.2f", pos);

            if (pos != this->position)
            {
                // not at target
                // calculate target operation
                this->target_operation = pos < this->position ? TARGET_OPERATION_CLOSE : TARGET_OPERATION_OPEN;
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
        this->recompute_position();

        // perform one action if target operation different than current operation
        if (this->target_operation != TARGET_OPERATION_NONE && (static_cast<uint8_t>(this->target_operation) != static_cast<uint8_t>(this->current_operation)))
        {
            // only activate door if time greater than activation interval
            if (now - last_activation > switch_activation_interval)
            {
                ESP_LOGD("CustomGarageCover", "Switch activated");
                this->do_one_action();
                last_activation = now;
                last_publish_time = now;
            }
        }
        else if (static_cast<uint8_t>(this->target_operation) == static_cast<uint8_t>(this->current_operation))
        {
            // target reached, set target as None.
            ESP_LOGD("CustomGarageCover", "Target operation reached");
            this->target_operation = TARGET_OPERATION_NONE;
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

private:
    Switch *door_switch;                   // switch that activates the door
    this->open_endstop_sensor;             // binary sensor to detect when door is full open
    this->close_endstop_sensor;            // binary sensor to detect when door is full closed
    CoverOperation last_dir;               // last door direction (open/close)
    CoverTargetOperation target_operation; // received action to execute
    uint32_t switch_activation_interval;   // time between switch activations
    uint32_t open_duration;                // time the door needs to fully open
    uint32_t close_duration;               // time the door needs to fully close

    void recompute_position()
    {
        // recalculates door position

        static uint32_t last_recompute_time = 0;
        // store current time
        const uint32_t now = millis();

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
        this->door_switch->turn_on();
        // cover state machine
        if (this->current_operation == COVER_OPERATION_OPENING || this->current_operation == COVER_OPERATION_CLOSING)
        {
            // door moving
            this->current_operation = COVER_OPERATION_IDLE;
        }
        else
        {
            // door idle, check last direction
            if (this->last_dir == COVER_OPERATION_OPENING)
            {
                // last action open
                this->current_operation = COVER_OPERATION_CLOSING;
                this->last_dir = COVER_OPERATION_CLOSING;
            }
            else
            {
                // last action close
                this->current_operation = COVER_OPERATION_OPENING;
                this->last_dir = COVER_OPERATION_OPENING;
            }
        }
        // send current state
        this->publish_state(false);
    }
};
