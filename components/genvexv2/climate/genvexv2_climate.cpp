#include "genvexv2_climate.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace genvexv2 {
static const char *TAG = "genvexv2.climate";

/// The custom fan modes exposed by this climate entity.
/// The Climate base class stores these pointers and compares against them by
/// identity, so the same array must be used everywhere a custom fan mode is set.
static const char *const GENVEXV2_FAN_MODES[] = {"1", "2", "3", "4"};

void Genvexv2Climate::setup() {
  // Custom fan modes now live on the Climate entity instead of on ClimateTraits.
  this->set_supported_custom_fan_modes(GENVEXV2_FAN_MODES);

  current_temp_sensor_->add_on_state_callback([this](float state) {
    ESP_LOGD(TAG, "CURRENT TEMP SENSOR CALLBACK: %f", state);
    current_temperature = state;
    publish_state();
  });
  temp_setpoint_number_->add_on_state_callback([this](float state) {
    ESP_LOGD(TAG, "TEMP SETPOINT SENSOR CALLBACK: %f", state);
    target_temperature = state;
    publish_state();
  });
  fan_speed_number_->add_on_state_callback([this](float state) {
    ESP_LOGD(TAG, "FAN SPEED SENSOR CALLBACK: %f", state);
    genvexv2fanspeed_to_fanmode(state);
    publish_state();
  });

  current_temperature = current_temp_sensor_->state;
  target_temperature  = temp_setpoint_number_->state;
  genvexv2fanspeed_to_fanmode(fan_speed_number_->state);
}

void Genvexv2Climate::control(const climate::ClimateCall& call) {
  if (call.get_target_temperature().has_value())
  {
    this->target_temperature = *call.get_target_temperature();
    float target = target_temperature;
    ESP_LOGD(TAG, "Target temperature changed to: %f", target);
    temp_setpoint_number_->make_call().set_value(target).perform();//set(target);
  }

  if (call.get_mode().has_value())
  {
    ESP_LOGD("TAG", "Mode changed from Home Assistant");
    auto new_mode = *call.get_mode();
    mode = new_mode;
    switch (mode) {
      case climate::CLIMATE_MODE_OFF: 
      {
        ESP_LOGD("TAG", "Mode changed to OFF");
        // The only valid fan mode that is not custom is "OFF";
        // setting it also clears any active custom fan mode.
        this->set_fan_mode_(climate::CLIMATE_FAN_OFF);

        ESP_LOGD(TAG, "Custom Fan mode set to: 0");
        fan_speed_number_->make_call().set_value(0).perform();//set(0);
        break;
      }
      case climate::CLIMATE_MODE_AUTO: 
      {
        ESP_LOGD("TAG", "Mode changed to AUTO");
        // Setting a custom fan mode clears the regular fan mode.
        this->set_custom_fan_mode_(GENVEXV2_FAN_MODES[1]);
        auto optional_genvexv2_fan_mode = parse_number<float>(GENVEXV2_FAN_MODES[1]);
        if(optional_genvexv2_fan_mode.has_value())
        {
          auto genvexv2_fan_mode = optional_genvexv2_fan_mode.value();
          ESP_LOGD(TAG, "Custom Fan mode set to: %i", static_cast<int>(genvexv2_fan_mode));
          fan_speed_number_->make_call().set_value(genvexv2_fan_mode).perform();;//set(genvexv2_fan_mode);
        }
        break;
      }
      default:
        break;
    }
  }

  if (call.get_fan_mode().has_value())
  {
    ESP_LOGD("TAG", "Fan mode changed to OFF from Home Assistant");
    // The only valid fan mode that is not custom is "OFF"
    this->set_fan_mode_(climate::CLIMATE_FAN_OFF);

    ESP_LOGD(TAG, "Fan mode set to: 0");
    fan_speed_number_->make_call().set_value(0).perform();//set(0);
  }

  if (call.has_custom_fan_mode())
  {
    ESP_LOGD("TAG", "Fan mode changed to custom fan mode 1-4 from Home Assistant");
    auto new_custom_fan_mode = call.get_custom_fan_mode();
    // Setting a custom fan mode clears the regular fan mode.
    this->set_custom_fan_mode_(new_custom_fan_mode);
    auto optional_genvexv2_fan_mode = parse_number<float>(new_custom_fan_mode.c_str());
    if(optional_genvexv2_fan_mode.has_value())
    {
      auto genvexv2_fan_mode = optional_genvexv2_fan_mode.value();
      ESP_LOGD(TAG, "Custom Fan mode set to: %i", static_cast<int>(genvexv2_fan_mode));
      fan_speed_number_->make_call().set_value(genvexv2_fan_mode).perform();//set(genvexv2_fan_mode);
    }
  }
  this->publish_state();
}

climate::ClimateTraits Genvexv2Climate::traits() {
  auto traits = climate::ClimateTraits();

  // The custom fan modes are registered on the entity itself in setup().

  traits.set_supported_fan_modes({ 
    climate::ClimateFanMode::CLIMATE_FAN_OFF 
  });

  traits.set_supported_modes({
    climate::ClimateMode::CLIMATE_MODE_OFF,
    climate::ClimateMode::CLIMATE_MODE_AUTO,
   });

  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_visual_temperature_step(0.1);
  traits.set_visual_min_temperature(5);
  traits.set_visual_max_temperature(30);

  return traits;
}

void Genvexv2Climate::dump_config() {
  LOG_CLIMATE("", "Genvexv2 Climate", this);
}

void Genvexv2Climate::genvexv2fanspeed_to_fanmode(const int state)
{
  ESP_LOGD("TAG", "In genvexv2fanspeed_to_fanmode");
  ESP_LOGD("TAG", "State is %i", state);

  switch (state) {
  case 1:
  case 2:
  case 3:
  case 4:
    ESP_LOGD("TAG", "Case %i", state);
    this->mode = climate::CLIMATE_MODE_AUTO;
    // Setting a custom fan mode clears the regular fan mode.
    this->set_custom_fan_mode_(GENVEXV2_FAN_MODES[state - 1]);
    break;
  case 0:
    ESP_LOGD("TAG", "Case 0");
    // Setting the regular fan mode clears any active custom fan mode.
    this->set_fan_mode_(climate::CLIMATE_FAN_OFF);
    this->mode = climate::CLIMATE_MODE_OFF;
    break;
  default: 
    ESP_LOGD("TAG", "Case default");
    this->set_fan_mode_(climate::CLIMATE_FAN_OFF);
    this->mode = climate::CLIMATE_MODE_OFF;
    break;
  }
}

} // namespace genvexv2
} // namespace esphome
