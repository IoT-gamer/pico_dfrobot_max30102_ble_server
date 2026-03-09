#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "btstack.h"

#include "dfrobot_max30102.h"
#include "ble_server.h"

#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

// Exported for ble_server.c to use
i2c_inst_t *max30102_i2c = I2C_PORT; 
static btstack_timer_source_t sensor_timer;
static max30102_data_t current_data;

// Handle stack events (Connection/Disconnection)
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    // Pass everything to the abstracted server handler
    ble_server_handle_hci_event(packet_type, channel, packet, size);

    // When BTstack indicates we can send data, trigger the notification
    if (packet_type == HCI_EVENT_PACKET && hci_event_packet_get_type(packet) == ATT_EVENT_CAN_SEND_NOW) {
        ble_server_notify_health_data(&current_data);
    }
}

// Timer callback: Read sensor if active, and request BTstack to send data
static void sensor_timer_handler(btstack_timer_source_t *ts) {
    if (ble_server_is_measuring()) {
        dfrobot_max30102_get_data(I2C_PORT, &current_data);
        printf("SPO2: %d%%, HR: %ld bpm\n", current_data.spo2, current_data.heartbeat);

        if (ble_server_get_con_handle() != HCI_CON_HANDLE_INVALID) {
            att_server_request_can_send_now_event(ble_server_get_con_handle());
        }
    }

    // Reset timer for the next 4-second cycle
    btstack_run_loop_set_timer(ts, 4000); 
    btstack_run_loop_add_timer(ts);
}

int main() {
    stdio_init_all();

    // Initialize I2C
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Initialize MAX30102 Module (Ensure it starts off)
    if (!dfrobot_max30102_init(I2C_PORT)) {
        printf("MAX30102 initialization failed!\n");
        while (1) tight_loop_contents();
    }
    dfrobot_max30102_end_collect(I2C_PORT);
    printf("Hardware initialized.\n");

    // Initialize Bluetooth (CYW43)
    if (cyw43_arch_init()) {
        printf("CYW43 init failed\n");
        return -1;
    }

    // Setup BTstack
    l2cap_init();
    sm_init();
    ble_server_init(packet_handler);
    ble_server_start_advertising();

    // Start 4-second polling timer
    sensor_timer.process = &sensor_timer_handler;
    btstack_run_loop_set_timer(&sensor_timer, 4000);
    btstack_run_loop_add_timer(&sensor_timer);

    // Turn on Bluetooth hardware and start the run loop
    hci_power_control(HCI_POWER_ON);
    btstack_run_loop_execute();

    return 0;
}