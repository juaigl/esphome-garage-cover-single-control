#include "esphome.h"

enum Action : uint8_t
{
    ACTION_IDLE = 0,
    ACTION_OPEN,
    ACTION_CLOSE,
    ACTION_NONE,
};

class CustomGarageCover : public Component, public Cover
{
public:
    // TODO add custom constructor

    void setup() override
    {
        // This will be called by App.setup()
        // pinMode(5, INPUT);
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

            if (pos != this->position)
            {
                // not at target
                target_action = pos < this->position ? ACTION_CLOSE : ACTION_OPEN;
            }
        }
        if (call.get_stop())
        {
            target_action = ACTION_IDLE;
        }
    }

    void loop() override
    {
        // This will be called by App.loop()

        static uint32_t last_publish_time = 0;
        static uint32_t last_push_time = 0;

        // store current time
        const uint32_t now = millis();

        // Recompute position every loop cycle
        this->recompute_position();

        // push button if target action different than current operation
        if (target_action != ACTION_NONE && target_action != this->current_operation)
        {
            if (now - last_push_time > push_interval)
            {
                this->push_button();
                last_push_time = now;
            }
        }
        else if (target_action == this->current_operation)
        {
            // target reached, set target as None (3).
            ESP_LOGD("target", "Target Reached");
            target_action = ACTION_NONE;
        }

        // Send current position every second
        if (this->current_operation != COVER_OPERATION_IDLE && now - this->last_publish_time_ > 1000)
        {
            this->publish_state(false);
            this->last_publish_time_ = now;
        }
    }

private:
    CoverOperation last_dir;
    Action target_action = ACTION_NONE;
    uint32_t push_interval;
    uint32_t open_duration;
    uint32_t close_duration;

    void recompute_position()
    {
        // Recalculates door position

        static uint32_t last_recompute_time = 0;
        // store current time
        const uint32_t now = millis();

        if (this->current_operation != COVER_OPERATION_IDLE) // Door moving
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
            position += dir * (now - last_recompute_time) / action_dur;
            this->position = clamp(position, 0.0f, 1.0f);
        }
        // store time
        last_recompute_time = now;
    }

    void push_button()
    {
        // Activates the door switch
        // TODO add door switch call

        // Cover state machine
        if (this->current_operation == COVER_OPERATION_OPENING || this->current_operation == COVER_OPERATION_CLOSING)
        {
            // door moving
            this->current_operation = COVER_OPERATION_IDLE;
        }
        else
        {
            // door idle
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
    }
};
