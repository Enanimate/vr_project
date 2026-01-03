#ifndef RUST_BRIDGE_H
#define RUST_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct VRDevice VRDevice;

typedef struct {
    double w;
    double x;
    double y;
    double z;
} Quaternion;

VRDevice* vr_device_create(const char* port_name);
void vr_device_destroy(VRDevice* device);

uint8_t vr_device_update(VRDevice* device);
void vr_device_get_pose(const VRDevice* device, Quaternion* out_quat);
uint8_t vr_device_is_connected(const VRDevice* device);

#ifdef __cplusplus
}
#endif

#endif