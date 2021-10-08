#ifndef A4988_H
#define A4988_H


void turn_azim(bool direction){

    uint32_t hall_sensor_value = 0;
    TaskHandle_t cur_task = xTaskGetCurrentTaskHandle();

    ESP_LOGI(TAG, "Disable gate driver");
    gpio_config_t drv_en_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1 << BLDC_DRV_EN_GPIO,
    };
    ESP_ERROR_CHECK(gpio_config(&drv_en_config));
    gpio_set_level(BLDC_DRV_EN_GPIO, 0);

    ESP_LOGI(TAG, "Setup PWM and Hall GPIO (pull up internally)");
    mcpwm_pin_config_t mcpwm_gpio_config = {
        .mcpwm0a_out_num = BLDC_PWM_UH_GPIO,
        .mcpwm0b_out_num = BLDC_PWM_UL_GPIO,
        .mcpwm1a_out_num = BLDC_PWM_VH_GPIO,
        .mcpwm1b_out_num = BLDC_PWM_VL_GPIO,
        .mcpwm2a_out_num = BLDC_PWM_WH_GPIO,
        .mcpwm2b_out_num = BLDC_PWM_WL_GPIO,
        .mcpwm_cap0_in_num   = HALL_CAP_U_GPIO,
        .mcpwm_cap1_in_num   = HALL_CAP_V_GPIO,
        .mcpwm_cap2_in_num   = HALL_CAP_W_GPIO,
        .mcpwm_sync0_in_num  = -1,  //Not used
        .mcpwm_sync1_in_num  = -1,  //Not used
        .mcpwm_sync2_in_num  = -1,  //Not used
        .mcpwm_fault0_in_num = BLDC_DRV_FAULT_GPIO,
        .mcpwm_fault1_in_num = -1,  //Not used
        .mcpwm_fault2_in_num = -1   //Not used
    };
    ESP_ERROR_CHECK(mcpwm_set_pin(BLDC_MCPWM_GROUP, &mcpwm_gpio_config));
    // In case there's no pull-up resister for hall sensor on board
    gpio_pullup_en(HALL_CAP_U_GPIO);
    gpio_pullup_en(HALL_CAP_V_GPIO);
    gpio_pullup_en(HALL_CAP_W_GPIO);
    gpio_pullup_en(BLDC_DRV_FAULT_GPIO);

    ESP_LOGI(TAG, "Initialize PWM (default to turn off all MOSFET)");
    mcpwm_config_t pwm_config = {
        .frequency = PWM_DEFAULT_FREQ,
        .cmpr_a = PWM_MIN_DUTY,
        .cmpr_b = PWM_MIN_DUTY,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_HAL_GENERATOR_MODE_FORCE_LOW,
    };
    ESP_ERROR_CHECK(mcpwm_init(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, &pwm_config));
    ESP_ERROR_CHECK(mcpwm_init(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, &pwm_config));
    ESP_ERROR_CHECK(mcpwm_init(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, &pwm_config));

    ESP_LOGI(TAG, "Initialize over current fault action");
    ESP_ERROR_CHECK(mcpwm_fault_init(BLDC_MCPWM_GROUP, MCPWM_LOW_LEVEL_TGR, BLDC_DRV_OVER_CURRENT_FAULT));
    ESP_ERROR_CHECK(mcpwm_fault_set_cyc_mode(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_DRV_OVER_CURRENT_FAULT, MCPWM_ACTION_FORCE_LOW, MCPWM_ACTION_FORCE_LOW));
    ESP_ERROR_CHECK(mcpwm_fault_set_cyc_mode(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_DRV_OVER_CURRENT_FAULT, MCPWM_ACTION_FORCE_LOW, MCPWM_ACTION_FORCE_LOW));
    ESP_ERROR_CHECK(mcpwm_fault_set_cyc_mode(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_DRV_OVER_CURRENT_FAULT, MCPWM_ACTION_FORCE_LOW, MCPWM_ACTION_FORCE_LOW));

    ESP_LOGI(TAG, "Initialize Hall sensor capture");
    mcpwm_capture_config_t cap_config = {
        .cap_edge = MCPWM_BOTH_EDGE,
        .cap_prescale = 1,
        .capture_cb = bldc_hall_updated,
        .user_data = cur_task,
    };
    ESP_ERROR_CHECK(mcpwm_capture_enable_channel(BLDC_MCPWM_GROUP, 0, &cap_config));
    ESP_ERROR_CHECK(mcpwm_capture_enable_channel(BLDC_MCPWM_GROUP, 1, &cap_config));
    ESP_ERROR_CHECK(mcpwm_capture_enable_channel(BLDC_MCPWM_GROUP, 2, &cap_config));
    ESP_LOGI(TAG, "Please turn on the motor power");
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "Enable gate driver");
    gpio_set_level(BLDC_DRV_EN_GPIO, 1);
    ESP_LOGI(TAG, "Changing speed at background");
    const esp_timer_create_args_t bldc_timer_args = {
        .callback = update_bldc_speed,
        .name = "bldc_speed"
    };
    esp_timer_handle_t bldc_speed_timer;
    ESP_ERROR_CHECK(esp_timer_create(&bldc_timer_args, &bldc_speed_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(bldc_speed_timer, 2000000));

  while (1) {
        // The rotation direction is controlled by inverting the hall sensor value
        hall_sensor_value = bldc_get_hall_sensor_value(direction);
        if (hall_sensor_value >= 1 && hall_sensor_value <= sizeof(s_hall_actions) / sizeof(s_hall_actions[0])) {
            s_hall_actions[hall_sensor_value]();
        } else {
            ESP_LOGE(TAG, "invalid bldc phase, wrong hall sensor value:%d", hall_sensor_value);
        }
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}


#endif 