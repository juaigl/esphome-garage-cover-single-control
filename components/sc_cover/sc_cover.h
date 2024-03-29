#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace sc_cover {

enum CoverTargetOperation : uint8_t {
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

class SingleControlCover : public cover::Cover, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void set_door_switch(switch_::Switch *door_switch) { this->door_switch_ = door_switch; }
  void set_switch_interval(uint32_t switch_interval) { this->switch_interval_ = switch_interval; }
  void set_open_endstop(binary_sensor::BinarySensor *open_endstop) { this->open_endstop_ = open_endstop; }
  void set_close_endstop(binary_sensor::BinarySensor *close_endstop) { this->close_endstop_ = close_endstop; }
  void set_open_duration(uint32_t open_duration) { this->open_duration_ = open_duration; }
  void set_close_duration(uint32_t close_duration) { this->close_duration_ = close_duration; }

  cover::CoverTraits get_traits() override;

 protected:
  void control(const cover::CoverCall &call) override;
  bool is_open_() const { return this->open_endstop_->state; }
  bool is_closed_() const { return this->close_endstop_->state; }
  bool is_at_target_() const;
  bool is_operation_done_() const;

  void recompute_position_(const uint32_t now);

  bool activate_switch_();

  void open_endstop_callback_(bool state);
  void close_endstop_callback_(bool state);

  switch_::Switch *door_switch_;
  binary_sensor::BinarySensor *open_endstop_;
  binary_sensor::BinarySensor *close_endstop_;
  bool toggle_{false};
  uint32_t switch_interval_;
  uint32_t open_duration_;
  uint32_t close_duration_;

  uint32_t last_activation_time_{0};
  uint32_t last_recompute_time_{0};
  uint32_t last_publish_time_{0};
  float target_position_{0};
  cover::CoverOperation last_operation_{cover::COVER_OPERATION_OPENING};
  CoverTargetOperation target_operation_{TARGET_OPERATION_NONE};
};

}  // namespace sc_cover
}  // namespace esphome
