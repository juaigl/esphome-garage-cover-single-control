#include "sc_cover.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace sc_cover {

static const char *const TAG = "sc_cover.cover";

using namespace esphome::cover;

CoverTraits SingleControlCover::get_traits() {
  auto traits = CoverTraits();
  traits.set_supports_stop(true);
  traits.set_supports_position(true);
  traits.set_supports_toggle(true);
  traits.set_is_assumed_state(false);
  traits.set_supports_tilt(false);

  return traits;
}

void SingleControlCover::dump_config() {
  LOG_COVER("", "SingleControl Cover", this);
  LOG_BUTTON(" ", "Door Switch", this->door_activate_button_);
  ESP_LOGCONFIG(TAG, " Setup delay: %.1fs", this->setup_delay_ /1e3f);
  ESP_LOGCONFIG(TAG, " Switch Interval:  %.1fs", this->button_press_interval_ / 1e3f);
  LOG_BINARY_SENSOR("  ", "Open Endstop", this->open_endstop_);
  ESP_LOGCONFIG(TAG, "  Open Duration: %.1fs", this->open_duration_ / 1e3f);
  LOG_BINARY_SENSOR("  ", "Close Endstop", this->close_endstop_);
  ESP_LOGCONFIG(TAG, "  Close Duration: %.1fs", this->close_duration_ / 1e3f);
}

float SingleControlCover::get_setup_priority() const { return setup_priority::WIFI; }

void SingleControlCover::setup() {
  this->open_endstop_->add_on_state_callback([this](bool state) { this->open_endstop_callback_(state); });
  this->close_endstop_->add_on_state_callback([this](bool state) { this->close_endstop_callback_(state); });

  this->do_setup_();

  if (this->setup_delay_ > 0) {
    this->set_timeout(this->setup_delay_, [this] { this->do_setup_(); });
  }
}

void SingleControlCover::do_setup_() {
  if (this->is_open_()) {
    // door is open
    this->position = COVER_OPEN;
    this->last_operation_ = COVER_OPERATION_OPENING;
  } else if (this->is_closed_()) {
    // door is closed
    this->position = COVER_CLOSED;
    this->last_operation_ = COVER_OPERATION_CLOSING;
  } else {
    // door neither closed nor open
    // assume door its ad middle position
    this->position = 0.5f;
  }
  this->target_position_ = this->position;
  // publish states
  this->publish_state(false);
}

void SingleControlCover::control(const CoverCall &call) {
  // this function will be called every time the user requests a state change.

  if (call.get_stop()) {
    // requested to stop

    ESP_LOGD(TAG, "Stop command received.");

    if (this->current_operation != COVER_OPERATION_IDLE) {
      this->target_operation_ = TARGET_OPERATION_IDLE;
    }
  }

  if (call.get_toggle().has_value()) {
    this->target_operation_ = TARGET_OPERATION_NONE;
    this->toggle_ = true;
  }

  if (call.get_position().has_value()) {
    // requested to change position

    // get requested position
    auto pos = *call.get_position();

    // ensure position is between 0 and 1
    pos = clamp(pos, 0.0f, 1.0f);

    ESP_LOGD(TAG, "Position command received: %0.2f", pos);

    if (pos != this->position) {
      // not at target -> calculate target operation

      this->target_operation_ = pos < this->position ? TARGET_OPERATION_CLOSE : TARGET_OPERATION_OPEN;
      this->target_position_ = pos;
    }
  }
}

void SingleControlCover::loop() {
  // This will be called by App.loop()

  // store current time
  const uint32_t now = millis();

  // recompute position every loop cycle
  this->recompute_position_(now);

  if (this->toggle_) {
    // toggle requested
    if (this->activate_door_()) {
      this->toggle_ = false;
      if (this->current_operation == COVER_OPERATION_CLOSING) {
        this->target_position_ = COVER_CLOSED;
      } else if (this->current_operation == COVER_OPERATION_OPENING) {
        this->target_position_ = COVER_OPEN;
      } else {
        this->target_position_ = this->position;
      }
    }
  }

  else if ((this->target_operation_ != TARGET_OPERATION_NONE) && !this->is_operation_done_()) {
    // target operation not done -> activate switch
    this->activate_door_();
  }

  else if (this->is_operation_done_()) {
    // target operation done -> clear operation
    ESP_LOGD(TAG, "Target operation reached");
    this->target_operation_ = TARGET_OPERATION_NONE;
  }

  else if ((this->current_operation != COVER_OPERATION_IDLE) && this->is_at_target_()) {
    // target operation was done, door is moving and target position reached

    // let door stop by itself if FULL_OPEN or FULL_CLOSE requested
    if (this->target_position_ != COVER_CLOSED && this->target_position_ != COVER_OPEN) {
      this->activate_door_();
    }
  }

  // send current position every second
  if (this->current_operation != COVER_OPERATION_IDLE && (now - this->last_publish_time_) > 1000) {
    this->publish_state(false);
    this->last_publish_time_ = now;
  }
}

bool SingleControlCover::is_at_target_() const {
  return ((this->current_operation == COVER_OPERATION_OPENING && this->position >= this->target_position_) ||
          (this->current_operation == COVER_OPERATION_CLOSING && this->position <= this->target_position_));
}

bool SingleControlCover::is_operation_done_() const {
  return static_cast<uint8_t>(this->target_operation_) == static_cast<uint8_t>(this->current_operation);
}

void SingleControlCover::recompute_position_(const uint32_t now) {
  // only recompute position if door is moving
  if (this->current_operation != COVER_OPERATION_IDLE) {
    float dir;
    float action_dur;

    // set dir and duration depending on current movement
    if (this->current_operation == COVER_OPERATION_CLOSING) {
      // door closing
      dir = -1.0f;
      action_dur = this->close_duration_;
    } else {
      // door opening
      dir = 1.0f;
      action_dur = this->open_duration_;
    }

    // calculate position
    float change = (dir * (now - this->last_recompute_time_)) / action_dur;
    this->position = clamp(this->position + change, 0.0f, 1.0f);
  }

  // store time
  this->last_recompute_time_ = now;
}

bool SingleControlCover::activate_door_() {
  // store current time
  const uint32_t now = millis();

  if ((now - this->last_activation_time_) > this->button_press_interval_) {
    // cover state machine (recompute current operation)
    if (this->current_operation == COVER_OPERATION_OPENING || this->current_operation == COVER_OPERATION_CLOSING) {
      // door is moving -> new operation: stop (idle)
      this->current_operation = COVER_OPERATION_IDLE;
    } else {
      // door idle, check last direction
      if (this->last_operation_ == COVER_OPERATION_OPENING) {
        // last operation: opening -> new operation: closing
        this->current_operation = COVER_OPERATION_CLOSING;
        this->last_operation_ = COVER_OPERATION_CLOSING;
      } else {
        // last operation: closing -> new operation: opening
        this->current_operation = COVER_OPERATION_OPENING;
        this->last_operation_ = COVER_OPERATION_OPENING;
      }
    }

    // activate switch
    ESP_LOGD(TAG, "Switch activated");
    this->door_activate_button_->press();

    // send current state
    this->publish_state(false);
    this->last_publish_time_ = now;
    this->last_recompute_time_ = now;
    this->last_activation_time_ = now;

    // return switch activated
    return true;
  }

  // return switch not activated
  return false;
}

void SingleControlCover::open_endstop_callback_(bool state) {
  if (state) {
    // open end stop reached
    this->last_activation_time_ = millis();
    this->last_recompute_time_ = this->last_activation_time_;
    this->current_operation = COVER_OPERATION_IDLE;
    this->last_operation_ = COVER_OPERATION_OPENING;
    this->position = COVER_OPEN;
    this->publish_state(false);
  } else {
    // open end stop leaving
    if (this->current_operation != COVER_OPERATION_CLOSING) {
      // external close commanded (like external remote)
      this->current_operation = COVER_OPERATION_CLOSING;
      this->last_operation_ = COVER_OPERATION_CLOSING;
      this->target_position_ = COVER_CLOSED;
      this->last_activation_time_ = millis();
      this->last_recompute_time_ = this->last_activation_time_;
    }
  }
}

void SingleControlCover::close_endstop_callback_(bool state) {
  if (state) {
    // close end stop reached
    this->last_activation_time_ = millis();
    this->last_recompute_time_ = this->last_activation_time_;
    this->current_operation = COVER_OPERATION_IDLE;
    this->last_operation_ = COVER_OPERATION_CLOSING;
    this->position = COVER_CLOSED;
    this->publish_state(false);
  } else {
    // close end stop leaving
    if (this->current_operation != COVER_OPERATION_OPENING) {
      // external open commanded (like external remote)
      this->current_operation = COVER_OPERATION_OPENING;
      this->last_operation_ = COVER_OPERATION_OPENING;
      this->target_position_ = COVER_OPEN;
      this->last_activation_time_ = millis();
      this->last_recompute_time_ = this->last_activation_time_;
    }
  }
}

}  // namespace sc_cover
}  // namespace esphome
