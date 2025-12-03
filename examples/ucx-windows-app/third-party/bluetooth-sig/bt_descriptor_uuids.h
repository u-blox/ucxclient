/*
 * Copyright 2025 u-blox
 *
 * Auto-generated from Bluetooth SIG descriptor_uuids.yaml
 * Do not edit manually - regenerate using convert_yaml_to_c.py
 */

#ifndef BT_DESCRIPTOR_UUIDS_H
#define BT_DESCRIPTOR_UUIDS_H

#include <stdint.h>

typedef struct {
    uint16_t uuid;
    const char *name;
} BtDescriptor_t;

static const BtDescriptor_t BT_DESCRIPTORS[] = {
    {0x2900, "Characteristic Extended Properties"},
    {0x2901, "Characteristic User Description"},
    {0x2902, "Client Characteristic Configuration"},
    {0x2903, "Server Characteristic Configuration"},
    {0x2904, "Characteristic Presentation Format"},
    {0x2905, "Characteristic Aggregate Format"},
    {0x2906, "Valid Range"},
    {0x2907, "External Report Reference"},
    {0x2908, "Report Reference"},
    {0x2909, "Number of Digitals"},
    {0x290A, "Value Trigger Setting"},
    {0x290B, "Environmental Sensing Configuration"},
    {0x290C, "Environmental Sensing Measurement"},
    {0x290D, "Environmental Sensing Trigger Setting"},
    {0x290E, "Time Trigger Setting"},
    {0x290F, "Complete BR-EDR Transport Block Data"},
    {0x2910, "Observation Schedule"},
    {0x2911, "Valid Range and Accuracy"},
    {0x2912, "Measurement Description"},
    {0x2913, "Manufacturer Limits"},
    {0x2914, "Process Tolerances"},
    {0x2915, "IMD Trigger Setting"},
    {0x2916, "Cooking Sensor Info"},
    {0x2917, "Cooking Trigger Setting"},
};

#define BT_DESCRIPTORS_COUNT 24

static inline const char* btGetDescriptorName(uint16_t uuid) {
    for (int i = 0; i < BT_DESCRIPTORS_COUNT; i++) {
        if (BT_DESCRIPTORS[i].uuid == uuid) {
            return BT_DESCRIPTORS[i].name;
        }
    }
    return NULL;
}

#endif /* BT_DESCRIPTOR_UUIDS_H */
