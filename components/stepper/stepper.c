#include "stepper.h"
#include "esp_rom_sys.h" // for esp_rom_delay_us
#include "esp_timer.h"

#define PRINT_DELAY 1000 // ms
#define BATCH_STEPS 1    // number of steps in one quick burst

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

void stepper_set_flow_rate(StepperMotor *motor, float ml_per_hour)
{
    float steps_per_ml = (DRIVER_STEPS_PER_ML > 0.0f) ? DRIVER_STEPS_PER_ML : (float)DRIVER_STEPS_PER_ML;
    double steps_per_hour = (double)steps_per_ml * ml_per_hour;
    double steps_per_second = steps_per_hour / 3600.0;
    if (steps_per_second <= 0.0)
    {
        ESP_LOGE(TAG, "Invalid flow rate: %.3f ml/h", ml_per_hour);
        return;
    }
    // step_delay is per-step delay in microseconds
    motor->step_delay = (uint64_t)(1000000.0 / steps_per_second);
    ESP_LOGI(TAG, "Flow rate %.3f ml/h -> per-step delay %llu us (steps_per_ml=%.2f)",
             ml_per_hour, (unsigned long long)motor->step_delay, steps_per_ml);
}

void stepper_step_motor(StepperMotor *motor, int step)
{
    gpio_set_level(motor->in1, step_patterns[step][0]);
    gpio_set_level(motor->in2, step_patterns[step][1]);
    gpio_set_level(motor->in3, step_patterns[step][2]);
    gpio_set_level(motor->in4, step_patterns[step][3]);
}

void stepper_init(StepperMotor *motor,
                  gpio_num_t in1, gpio_num_t in2, gpio_num_t in3, gpio_num_t in4,
                  gpio_num_t full_open_switch, gpio_num_t full_closed_switch,
                  int steps)
{
    motor->in1 = in1;
    motor->in2 = in2;
    motor->in3 = in3;
    motor->in4 = in4;
    motor->number_of_steps = steps + 1;
    motor->step_number = 0;
    motor->steps_left = 0;
    motor->last_step_time = 0;
    motor->full_open_switch = full_open_switch;
    motor->full_closed_switch = full_closed_switch;

    gpio_set_direction(in1, GPIO_MODE_OUTPUT);
    gpio_set_direction(in2, GPIO_MODE_OUTPUT);
    gpio_set_direction(in3, GPIO_MODE_OUTPUT);
    gpio_set_direction(in4, GPIO_MODE_OUTPUT);
    gpio_set_direction(full_open_switch, GPIO_MODE_INPUT);
    gpio_set_direction(full_closed_switch, GPIO_MODE_INPUT);
    gpio_set_pull_mode(full_open_switch, GPIO_PULLDOWN_ENABLE);
    gpio_set_pull_mode(full_closed_switch, GPIO_PULLDOWN_ENABLE);

    stepper_release(motor);
    ESP_LOGI(TAG, "Stepper initialized IN1=%d IN2=%d IN3=%d IN4=%d Steps=%d",
             in1, in2, in3, in4, steps);
}

void stepper_stop(StepperMotor *motor)
{
    motor->steps_left = 0;
    motor->step_number = 0;
    motor->last_step_time = 0;
    stepper_release(motor);
    ESP_LOGI(TAG, "Stepper stopped");
}

void stepper_step_ml(StepperMotor *motor, float total_ml, float ml_per_hour, int print)
{
    // ensure calibration used from DRIVER_STEPS_PER_ML
    stepper_set_flow_rate(motor, ml_per_hour);

    // compute number of steps using DRIVER_STEPS_PER_ML
    int steps_signed = (int)(total_ml * DRIVER_STEPS_PER_ML);
    int steps_to_move = steps_signed < 0 ? -steps_signed : steps_signed;

    motor->steps_left = steps_to_move;
    int initial_steps = motor->steps_left;
    int direction = (steps_signed >= 0) ? 1 : 0; // 1 = forward, 0 = reverse
    float total_ml_target = (float)initial_steps / DRIVER_STEPS_PER_ML;

    if (print)
        lcd_clear();

    uint64_t per_step_delay_us = motor->step_delay; // per-step (µs)
    uint64_t last_print_us = esp_timer_get_time();
    uint64_t last_batch_start_us = 0; // 0 => allow immediate first batch

    while (motor->steps_left > 0)
    {
        uint64_t now_us = esp_timer_get_time();

        // ---------- periodic print ----------
        if (print && (now_us - last_print_us) >= (uint64_t)PRINT_DELAY * 1000ULL)
        {
            int steps_done = initial_steps - motor->steps_left;
            steps_done -= BATCH_STEPS;
            ESP_LOGI(TAG, "Steps done: %d/%d (%.1f ml)", steps_done, initial_steps, (float)steps_done / DRIVER_STEPS_PER_ML);
            float vtbi_dispensed = (float)steps_done / DRIVER_STEPS_PER_ML;

            char vtbi[50];
            snprintf(vtbi, sizeof(vtbi), "VTBI: %.1f/%.1fml", vtbi_dispensed, total_ml_target);
            lcd_put_cur(1, 0);
            lcd_send_string(vtbi);

            // --- time calculation ---
            int64_t remaining_us = (int64_t)motor->steps_left * (int64_t)per_step_delay_us;
            if (last_batch_start_us != 0)
            {
                int64_t elapsed_since_last_batch = (int64_t)(now_us - last_batch_start_us);
                remaining_us -= elapsed_since_last_batch;
            }
            if (remaining_us < 0)
                remaining_us = 0;

            // Round to nearest whole second
            int remaining_s = (int)((remaining_us + 500000) / 1000000);

            int hrs = remaining_s / 3600;
            int mins = (remaining_s % 3600) / 60;
            int secs = remaining_s % 60;

            char time_remaining[50];
            snprintf(time_remaining, sizeof(time_remaining), "TIME: %02d:%02d:%02d", hrs, mins, secs);
            lcd_put_cur(2, 0);
            lcd_send_string(time_remaining);

            // Always reset based on multiples of PRINT_DELAY to avoid drift
            last_print_us += (uint64_t)PRINT_DELAY * 1000ULL;
            if (now_us - last_print_us >= (uint64_t)PRINT_DELAY * 1000ULL)
            {
                last_print_us = now_us; // catch up if we missed cycles
            }
        }

        // ---------- batch scheduling ----------
        int steps_this_batch = (motor->steps_left > BATCH_STEPS) ? BATCH_STEPS : motor->steps_left;
        uint64_t batch_period_us = per_step_delay_us * (uint64_t)steps_this_batch;

        // If no batch has run yet, run immediately. Otherwise wait until period elapsed.
        if (last_batch_start_us == 0 || (now_us - last_batch_start_us) >= batch_period_us)
        {
            // do the burst
            // (optionally enable driver here — e.g. gpio_set_level(EN, 1))
            for (int i = 0; i < steps_this_batch && motor->steps_left > 0; ++i)
            {
                // Limit switch checks
                if (!direction && gpio_get_level(motor->full_open_switch) == 1)
                {
                    ESP_LOGW(TAG, "Reached full open switch, stopping");
                    stepper_stop(motor);
                    stepper_release(motor);
                    return;
                }
                else if (direction && gpio_get_level(motor->full_closed_switch) == 1)
                {
                    ESP_LOGW(TAG, "Reached full closed switch, stopping");
                    stepper_stop(motor);
                    stepper_release(motor);
                    return;
                }

                if (direction)
                    motor->step_number++;
                else
                    motor->step_number--;

                // wrap-around
                if (motor->step_number < 0)
                    motor->step_number = motor->number_of_steps - 1;
                else if (motor->step_number >= motor->number_of_steps)
                    motor->step_number = 0;

                stepper_step_motor(motor, motor->step_number % 4);
                motor->steps_left--;

                // micro settle between micro-steps
                esp_rom_delay_us(2000); // 2 ms
            }

            // release coils to save heat (optional)
            stepper_release(motor);
            // record batch start time
            last_batch_start_us = esp_timer_get_time();
            // (optionally disable driver here — e.g. gpio_set_level(EN, 0))
        }
        else
        {
            // let other tasks run; choose a small delay (keeping PRINT_DELAY responsiveness)
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    stepper_release(motor);
}
