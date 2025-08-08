#include "stepper.h"

static const char *TAG = "STEPPER";
static const int step_patterns[4][4] = {
    {1, 0, 1, 0}, // 1010
    {0, 1, 1, 0}, // 0110
    {0, 1, 0, 1}, // 0101
    {1, 0, 0, 1}  // 1001
};

void stepper_release(StepperMotor *motor)
{
    gpio_set_level(motor->in1, 0);
    gpio_set_level(motor->in2, 0);
    gpio_set_level(motor->in3, 0);
    gpio_set_level(motor->in4, 0);
}

void stepper_set_speed(StepperMotor *motor, float rpm)
{
    motor->step_delay = (uint32_t)(60 * 1000000.0f / motor->number_of_steps / rpm);
    if (motor->step_delay < motor->max_speed_delay)
        motor->step_delay = motor->max_speed_delay;
    ESP_LOGI(TAG, "Stepper motor speed set to %.2f RPM with delay %ld us", rpm, motor->step_delay);
}

void stepper_step_motor(StepperMotor *motor, int step)
{
    gpio_set_level(motor->in1, step_patterns[step][0]);
    gpio_set_level(motor->in2, step_patterns[step][1]);
    gpio_set_level(motor->in3, step_patterns[step][2]);
    gpio_set_level(motor->in4, step_patterns[step][3]);
    ESP_LOGI(TAG, "Stepper motor step %d: IN1=%d, IN2=%d, IN3=%d, IN4=%d", step, step_patterns[step][0], step_patterns[step][1], step_patterns[step][2], step_patterns[step][3]);
}

void stepper_init(StepperMotor *motor, gpio_num_t in1, gpio_num_t in2, gpio_num_t in3, gpio_num_t in4, gpio_num_t full_open_switch, gpio_num_t full_closed_switch, int steps, float max_speed_rpm)
{
    motor->in1 = in1;
    motor->in2 = in2;
    motor->in3 = in3;
    motor->in4 = in4;
    motor->number_of_steps = steps + 1;
    motor->step_number = 0;
    motor->steps_left = 0;
    motor->last_step_time = 0;

    motor->max_speed_delay = (uint32_t)(60 * 1000000.0f / motor->number_of_steps / max_speed_rpm);
    stepper_set_speed(motor, max_speed_rpm / 2);

    gpio_set_direction(in1, GPIO_MODE_OUTPUT);
    gpio_set_direction(in2, GPIO_MODE_OUTPUT);
    gpio_set_direction(in3, GPIO_MODE_OUTPUT);
    gpio_set_direction(in4, GPIO_MODE_OUTPUT);
    gpio_set_direction(full_open_switch, GPIO_MODE_INPUT);
    gpio_set_direction(full_closed_switch, GPIO_MODE_INPUT);

    stepper_release(motor);
    ESP_LOGI(TAG, "Stepper motor initialized: IN1=%d, IN2=%d, IN3=%d, IN4=%d, Steps=%d, Max Speed RPM=%.2f", in1, in2, in3, in4, steps, max_speed_rpm);
}

void stepper_stop(StepperMotor *motor)
{
    motor->steps_left = 0;
    motor->step_number = 0;
    motor->last_step_time = 0;
    stepper_release(motor);
    ESP_LOGI(TAG, "Stepper motor stopped");
}

void stepper_step(StepperMotor *motor, int steps_to_move, float speed_rpm)
{
    if (speed_rpm > 0)
        stepper_set_speed(motor, speed_rpm);

    motor->steps_left = steps_to_move < 0 ? -steps_to_move : steps_to_move;
    int direction = (steps_to_move > 0) ? 1 : 0;

    while (motor->steps_left > 0)
    {
        uint64_t now = esp_timer_get_time();
        if (now - motor->last_step_time >= motor->step_delay)
        {
            if (direction)
            {
                if (gpio_get_level(motor->full_open_switch) == 1)
                {
                    ESP_LOGW(TAG, "Reached full open switch, stopping stepper");
                    stepper_stop(motor);
                    break;
                }
            }
            else
            {
                if (gpio_get_level(motor->full_closed_switch) == 1)
                {
                    ESP_LOGW(TAG, "Reached full closed switch, stopping stepper");
                    stepper_stop(motor);
                    break;
                }
            }
            motor->last_step_time = now;

            if (direction)
                motor->step_number++;
            else
            {
                if (motor->step_number == 0)
                    motor->step_number = motor->steps_left;
                motor->step_number--;
            }

            stepper_step_motor(motor, motor->step_number % 4);
            motor->steps_left--;
        }
    }

    if (motor->step_number == motor->steps_left)
        motor->step_number = 0;

    stepper_release(motor);
}
