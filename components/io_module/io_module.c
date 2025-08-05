#include "io_module.h"

QueueHandle_t gpio_evt_queue;
TaskHandle_t io_module_handle, gpio_task_handle;
gpio_num_t interrupt_inputs[3] = {BUTTON_01,
                                  BUTTON_02,
                                  BUTTON_03};
page_t page = MAIN_MENU;
main_select_t selected_mode = MANUAL_MODE;
manual_control_t manual_control_data = {0.0, 0.0};
manual_control_cursor_t manual_control_cursor = CURSOR_FLOW_RATE;

static void IRAM_ATTR
gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void gpio_task(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            int state = gpio_get_level(io_num);
            vTaskDelay(DEBOUNCE_DURATION / portTICK_PERIOD_MS); // Debounce delay
            xQueueReset(gpio_evt_queue);
            if (state != gpio_get_level(io_num))
                continue;
            if (!state)
                continue;
            switch (io_num)
            {
            case BUTTON_01:
                if (page == MAIN_MENU)
                {
                    selected_mode++;
                    if (selected_mode > PURGE_MODE)
                        selected_mode = MANUAL_MODE; // Loop back to manual mode
                    main_menu_screen(selected_mode);
                }
                else if (page == MANUAL_CONTROL)
                {
                    if (manual_control_cursor == CURSOR_HOME)
                        manual_control_cursor = CURSOR_FLOW_RATE; // Reset cursor to flow rate
                    else
                        manual_control_cursor++;
                    manual_control_screen(manual_control_data, manual_control_cursor);
                }
                break;
            case BUTTON_02:
                if (page == MANUAL_CONTROL)
                {
                    switch (manual_control_cursor)
                    {
                    case CURSOR_HOME:
                        page = MAIN_MENU;
                        selected_mode = MANUAL_MODE; // Reset to manual mode
                        main_menu_screen(selected_mode);
                        break;
                    case CURSOR_FLOW_RATE:
                        if (manual_control_data.flow_rate > 0.0)
                            manual_control_data.flow_rate -= 0.1; // Decrement flow rate
                        manual_control_screen(manual_control_data, manual_control_cursor);
                        break;
                    case CURSOR_VOLUME:
                        if (manual_control_data.volume > 0.0)
                            manual_control_data.volume -= 0.1; // Decrement volume
                        manual_control_screen(manual_control_data, manual_control_cursor);
                        break;
                    case CURSOR_START_STOP:
                        // Start/Stop logic here
                        break;
                    default:
                        break;
                    }
                }
                break;
            case BUTTON_03:
                if (page == MAIN_MENU)
                {
                    if (selected_mode == MANUAL_MODE)
                    {
                        page = MANUAL_CONTROL;
                        manual_control_data.flow_rate = 0.0;      // Reset flow rate
                        manual_control_data.volume = 0.0;         // Reset volume
                        manual_control_cursor = CURSOR_FLOW_RATE; // Reset cursor position
                        manual_control_screen(manual_control_data, manual_control_cursor);
                    }
                    else if (selected_mode == SCHEDULED_MODE)
                    {
                        page = SCHEDULED_DOSE;
                        scheduled_dose_screen();
                    }
                    else if (selected_mode == PURGE_MODE)
                    {
                        page = PURGE_UNPURGE;
                        // purge_unpurge_screen();
                    }
                }
                else if (page == MANUAL_CONTROL)
                {
                    switch (manual_control_cursor)
                    {
                    case CURSOR_HOME:
                        page = MAIN_MENU;
                        selected_mode = MANUAL_MODE; // Reset to manual mode
                        main_menu_screen(selected_mode);
                        break;
                    case CURSOR_FLOW_RATE:
                        manual_control_data.flow_rate += 0.1; // Increment flow rate
                        manual_control_screen(manual_control_data, manual_control_cursor);
                        break;
                    case CURSOR_VOLUME:
                        manual_control_data.volume += .1; // Increment volume
                        manual_control_screen(manual_control_data, manual_control_cursor);
                        break;
                    case CURSOR_START_STOP:
                        // Start/Stop logic here
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    page = MAIN_MENU;
                    selected_mode = MANUAL_MODE; // Reset to manual mode
                    main_menu_screen(selected_mode);
                }
                break;
            default:
                ESP_LOGI("GPIO_TASK", "Unknown GPIO interrupt: %ld", io_num);
                break;
            }
        }
    }
}

void io_module_init(void)
{

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_task, "gpio_task", 2048 * 2, NULL, 10, &gpio_task_handle);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    for (int INPUT_IDX = 0; INPUT_IDX < sizeof(interrupt_inputs) / sizeof(interrupt_inputs[0]); INPUT_IDX++)
    {
        gpio_num_t INPUT = interrupt_inputs[INPUT_IDX];
        gpio_set_direction(INPUT, GPIO_MODE_INPUT);
        gpio_set_pull_mode(INPUT, GPIO_PULLDOWN_ENABLE);
        gpio_set_intr_type(INPUT, GPIO_INTR_ANYEDGE);
        gpio_set_pull_mode(INPUT, GPIO_PULLDOWN_ENABLE);
        gpio_isr_handler_add(INPUT, gpio_isr_handler, (void *)INPUT);
    }
    main_menu_screen(selected_mode); // Display the main menu on the LCD
}
