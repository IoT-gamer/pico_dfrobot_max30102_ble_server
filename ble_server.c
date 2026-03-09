#include "ble_server.h"
#include <stdio.h>
#include <string.h>
#include "btstack.h"
#include "pico/stdlib.h"

// Include the sensor driver
#include "dfrobot_max30102.h" 

// Include the generated GATT header
#include "max30102.h"

// --- Globals ---
static hci_con_handle_t server_con_handle = HCI_CON_HANDLE_INVALID;
static bool is_measuring = false;

// Reference to the I2C port defined in main.c
extern i2c_inst_t *max30102_i2c; 

// --- Advertising Data ---
static uint8_t adv_data[] = {
    // Flags: General Discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // Name: "Pico-Health"
    0x0C, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 
    'P', 'i', 'c', 'o', '-', 'H', 'e', 'a', 'l', 't', 'h',
    // 16-bit Service UUIDs (Custom Service 0xAA10)
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x10, 0xAA
};
static const uint8_t adv_data_len = sizeof(adv_data);

// --- Forward Declarations ---
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

// --- Public API ---

void ble_server_init(btstack_packet_handler_t att_packet_handler) {
    att_server_init(profile_data, att_read_callback, att_write_callback);
    att_server_register_packet_handler(att_packet_handler);
}

void ble_server_start_advertising(void) {
    printf("Starting BLE advertising...\n");
    uint16_t adv_int = 800; 
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    
    gap_advertisements_set_params(adv_int, adv_int, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
    gap_advertisements_enable(1);
}

void ble_server_stop_advertising(void) {
    gap_advertisements_enable(0);
}

hci_con_handle_t ble_server_get_con_handle(void) {
    return server_con_handle;
}

bool ble_server_is_measuring(void) {
    return is_measuring;
}

void ble_server_handle_hci_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) return;
    uint8_t event_type = hci_event_packet_get_type(packet);

    switch (event_type) {
        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                server_con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                printf("BLE Client Connected.\n");
                ble_server_stop_advertising();
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            if (hci_event_disconnection_complete_get_connection_handle(packet) == server_con_handle) {
                server_con_handle = HCI_CON_HANDLE_INVALID;
                printf("BLE Client Disconnected.\n");
                
                // Safety: Turn off sensor on disconnect
                if (is_measuring) {
                    dfrobot_max30102_end_collect(max30102_i2c);
                    is_measuring = false;
                }
                ble_server_start_advertising();
            }
            break;
    }
}

void ble_server_notify_health_data(max30102_data_t *data) {
    if (server_con_handle == HCI_CON_HANDLE_INVALID) return;

    // Send Heart Rate (32-bit)
    if (data->heartbeat > 0) {
        uint8_t hr_buf[4];
        little_endian_store_32(hr_buf, 0, (uint32_t)data->heartbeat);
        att_server_notify(server_con_handle, ATT_CHARACTERISTIC_0000AA11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, hr_buf, 4);
    }
    
    // Send SPO2 (8-bit)
    if (data->spo2 > 0) {
        uint8_t spo2_buf = (uint8_t)data->spo2;
        att_server_notify(server_con_handle, ATT_CHARACTERISTIC_0000AA12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, &spo2_buf, 1);
    }
}

// --- Internal Callbacks ---

static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    return 0; 
}

static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    
    if (att_handle == ATT_CHARACTERISTIC_0000AA13_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        
        if (buffer_size < 1) return 0;

        uint8_t cmd = buffer[0];
        printf("BLE Write Cmd: 0x%02X\n", cmd);

        switch(cmd) {
            case 0x01: // Start
                if (!is_measuring) {
                    dfrobot_max30102_start_collect(max30102_i2c);
                    is_measuring = true;
                    printf("Sensor Collection Started.\n");
                }
                break;

            case 0x00: // Stop
                if (is_measuring) {
                    dfrobot_max30102_end_collect(max30102_i2c);
                    is_measuring = false;
                    printf("Sensor Collection Stopped.\n");
                }
                break;
                
            default:
                printf("Unknown Command\n");
        }
    }
    return 0;
}