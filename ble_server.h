#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include "btstack.h"
#include <stdbool.h>
#include "dfrobot_max30102.h" // For max30102_data_t

void ble_server_init(btstack_packet_handler_t att_packet_handler);
void ble_server_start_advertising(void);
void ble_server_stop_advertising(void);
void ble_server_handle_hci_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

// Replaces ble_server_notify_spectral_data
void ble_server_notify_health_data(max30102_data_t *data);

hci_con_handle_t ble_server_get_con_handle(void);

// Expose the measurement state so main.c knows when to poll the I2C bus
bool ble_server_is_measuring(void);

#endif // BLE_SERVER_H