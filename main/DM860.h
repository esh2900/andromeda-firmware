#ifndef DM860_H
#define DM860_H

#include "driver/mcpwm.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_log.h"

#define PWM_DEFAULT_FREQ   14400
#define PWM_MIN_DUTY       40.0
#define PWM_MAX_DUTY       80.0
#define PWM_DUTY_STEP      5.0
#define BLDC_MCPWM_GROUP   0
#define BLDC_MCPWM_TIMER_U 0
#define BLDC_MCPWM_TIMER_V 1
#define BLDC_MCPWM_TIMER_W 2
#define BLDC_MCPWM_GEN_HIGH MCPWM_GEN_A
#define BLDC_MCPWM_GEN_LOW  MCPWM_GEN_B


#define BLDC_DRV_EN_GPIO    18
#define BLDC_DRV_FAULT_GPIO 19
#define BLDC_DRV_OVER_CURRENT_FAULT MCPWM_SELECT_F0

#define BLDC_PWM_UH_GPIO 21
#define BLDC_PWM_UL_GPIO 22
#define BLDC_PWM_VH_GPIO 23
#define BLDC_PWM_VL_GPIO 25
#define BLDC_PWM_WH_GPIO 26
#define BLDC_PWM_WL_GPIO 27
#define HALL_CAP_U_GPIO  2
#define HALL_CAP_V_GPIO  4
#define HALL_CAP_W_GPIO  5

static const char *TAG = "example";

static inline uint32_t bldc_get_hall_sensor_value(bool ccw)
{
    uint32_t hall_val = gpio_get_level(HALL_CAP_U_GPIO) * 4 + gpio_get_level(HALL_CAP_V_GPIO) * 2 + gpio_get_level(HALL_CAP_W_GPIO) * 1;
    return ccw ? hall_val ^ (0x07) : hall_val;
}

static bool IRAM_ATTR bldc_hall_updated(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_channel, const cap_event_data_t *edata, void *user_data)
{
    TaskHandle_t task_to_notify = (TaskHandle_t)user_data;
    BaseType_t high_task_wakeup = pdFALSE;
    vTaskNotifyGiveFromISR(task_to_notify, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

static void update_bldc_speed(void *arg)
{
    static float duty = PWM_MIN_DUTY;
    static float duty_step = PWM_DUTY_STEP;
    duty += duty_step;
    if (duty > PWM_MAX_DUTY || duty < PWM_MIN_DUTY) {
        duty_step *= -1;
    }
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH, duty);
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_LOW, duty);
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH, duty);
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_LOW, duty);
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH, duty);
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_LOW, duty);
}

// U+V- / A+B-
static void bldc_set_phase_up_vm(void)
{
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // U+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // U- = _PWM_
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH); // V+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_LOW); // V- = 1
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH); // W+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_LOW);  // W- = 0
}

// W+U- / C+A-
static void bldc_set_phase_wp_um(void)
{
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH); // U+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_LOW); // U- = 1
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH); // V+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_LOW);  // V- = 0
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // W+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // W- = _PWM_
}

// W+V- / C+B-
static void bldc_set_phase_wp_vm(void)
{
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH); // U+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_LOW);  // U- = 0
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH); // V+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_LOW); // V- = 1
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // W+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // W- = _PWM_
}

// V+U- / B+A-
static void bldc_set_phase_vp_um(void)
{
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH); // U+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_LOW); // U- = 1
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // V+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // V- = _PWM_
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH); // W+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_LOW);  // W- = 0
}

// V+W- / B+C-
static void bldc_set_phase_vp_wm(void)
{
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH); // U+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_LOW);  // U- = 0
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // V+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // V- = _PWM_
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH); // W+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_LOW); // W- = 1
}

// U+W- / A+C-
static void bldc_set_phase_up_wm(void)
{
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // U+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // U- = _PWM_
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH); // V+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_LOW); // V- = 0
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH); // W+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_LOW); // W- = 1
}

typedef void (*bldc_hall_phase_action_t)(void);

static const bldc_hall_phase_action_t s_hall_actions[] = {
    [2] = bldc_set_phase_up_vm,
    [6] = bldc_set_phase_wp_vm,
    [4] = bldc_set_phase_wp_um,
    [5] = bldc_set_phase_vp_um,
    [1] = bldc_set_phase_vp_wm,
    [3] = bldc_set_phase_up_wm,
};

#endif