#include "genvexv2_select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace genvexv2 {

static const char *TAG = "genvexv2.select";

using modbus_controller::ModbusCommandItem;

void Genvexv2Select::parse_and_publish(std::span<const uint8_t> data) {
  float received_value = payload_to_float(data, *this, this->offset);
  ESP_LOGD(TAG, "Genvexv2 Select index: %f", received_value);

  const auto &options = this->traits.get_options();

  if (received_value >= 0 && received_value < options.size()) {
    size_t index = static_cast<size_t>(received_value);
    ESP_LOGD(TAG, "Select new state : %s", options[index]);
    this->publish_state(index);
  }
}

void Genvexv2Select::control(const std::string &value) {
  ESP_LOGD(TAG, "Genvexv2 Select state: %s", value.c_str());

  const auto &options = this->traits.get_options();

  for (size_t i = 0; i < options.size(); ++i) {
    if (value == options[i]) {
      uint16_t speed = i;
      ESP_LOGD(TAG, "WRITING INDEX: %u", speed);
      auto write_cmd = ModbusCommandItem::create_write_single_command(modbus_controller_, this->start_address, speed);

      // publish new value once the write is ack'd from the device
      write_cmd.on_data_func = [this, value](modbus::EntityType register_type, uint16_t start_address,
                                             std::span<const uint8_t> data) {
        modbus_controller_->on_write_register_response(register_type, start_address, data);
        this->publish_state(value);
      };
      modbus_controller_->queue_command(std::move(write_cmd));
      return;
    }
  }
}

} // namespace genvexv2
} // namespace esphome
