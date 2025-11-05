/*
 * Copyright 2025 u-blox
 *
 * Auto-generated from Bluetooth SIG service_uuids.yaml
 * Do not edit manually - regenerate using convert_yaml_to_c.py
 */

#ifndef BT_SERVICE_UUIDS_H
#define BT_SERVICE_UUIDS_H

#include <stdint.h>

typedef struct {
    uint16_t uuid;
    const char *name;
} BtService_t;

static const BtService_t BT_SERVICES[] = {
    {6144, "GAP"},
    {6145, "GATT"},
    {6146, "Immediate Alert"},
    {6147, "Link Loss"},
    {6148, "Tx Power"},
    {6149, "Current Time"},
    {6150, "Reference Time Update"},
    {6151, "Next DST Change"},
    {6152, "Glucose"},
    {6153, "Health Thermometer"},
    {6154, "Device Information"},
    {6157, "Heart Rate"},
    {6158, "Phone Alert Status"},
    {6159, "Battery"},
    {6160, "Blood Pressure"},
    {6161, "Alert Notification"},
    {6162, "Human Interface Device"},
    {6163, "Scan Parameters"},
    {6164, "Running Speed and Cadence"},
    {6165, "Automation IO"},
    {6166, "Cycling Speed and Cadence"},
    {6168, "Cycling Power"},
    {6169, "Location and Navigation"},
    {6170, "Environmental Sensing"},
    {6171, "Body Composition"},
    {6172, "User Data"},
    {6173, "Weight Scale"},
    {6174, "Bond Management"},
    {6175, "Continuous Glucose Monitoring"},
    {6176, "Internet Protocol Support"},
    {6177, "Indoor Positioning"},
    {6178, "Pulse Oximeter"},
    {6179, "HTTP Proxy"},
    {6180, "Transport Discovery"},
    {6181, "Object Transfer"},
    {6182, "Fitness Machine"},
    {6183, "Mesh Provisioning"},
    {6184, "Mesh Proxy"},
    {6185, "Reconnection Configuration"},
    {6202, "Insulin Delivery"},
    {6203, "Binary Sensor"},
    {6204, "Emergency Configuration"},
    {6205, "Authorization Control"},
    {6206, "Physical Activity Monitor"},
    {6207, "Elapsed Time"},
    {6208, "Generic Health Sensor"},
    {6211, "Audio Input Control"},
    {6212, "Volume Control"},
    {6213, "Volume Offset Control"},
    {6214, "Coordinated Set Identification"},
    {6215, "Device Time"},
    {6216, "Media Control"},
    {6217, "Generic Media Control"},
    {6218, "Constant Tone Extension"},
    {6219, "Telephone Bearer"},
    {6220, "Generic Telephone Bearer"},
    {6221, "Microphone Control"},
    {6222, "Audio Stream Control"},
    {6223, "Broadcast Audio Scan"},
    {6224, "Published Audio Capabilities"},
    {6225, "Basic Audio Announcement"},
    {6226, "Broadcast Audio Announcement"},
    {6227, "Common Audio"},
    {6228, "Hearing Access"},
    {6229, "Telephony and Media Audio"},
    {6230, "Public Broadcast Announcement"},
    {6231, "Electronic Shelf Label"},
    {6232, "Gaming Audio"},
    {6233, "Mesh Proxy Solicitation"},
    {6234, "Industrial Measurement Device"},
    {6235, "Ranging"},
    {6236, "HID ISO"},
    {6237, "Cookware"},
};

#define BT_SERVICES_COUNT 73

static inline const char* btGetServiceName(uint16_t uuid) {
    for (int i = 0; i < BT_SERVICES_COUNT; i++) {
        if (BT_SERVICES[i].uuid == uuid) {
            return BT_SERVICES[i].name;
        }
    }
    return NULL;
}

#endif /* BT_SERVICE_UUIDS_H */
