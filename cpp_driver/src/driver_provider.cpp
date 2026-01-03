#include "../include/driver_provider.h"
#include "../include/hmd_device.h"
#include <openvr_driver.h>

using namespace vr;

namespace vr_driver {

DriverProvider::DriverProvider()
    : m_pRustDevice(nullptr)
    , m_pHmdDevice(nullptr)
    , m_pControllerDevice(nullptr)
{
}

DriverProvider::~DriverProvider()
{
    Cleanup();
}

EVRInitError DriverProvider::Init(IVRDriverContext* pDriverContext)
{
    VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

    // Create Rust Device (connect to COM4)
    m_pRustDevice = vr_device_create("COM4");
    if (!m_pRustDevice) {
        printf("Failed to create Rust device (COM4)!\n");
        return VRInitError_Init_InterfaceNotFound;
    }

    // Create HMD device
    m_pHmdDevice = new HMDDevice(m_pRustDevice);

    // Add device to SteamVR
    VRServerDriverHost()->TrackedDeviceAdded(
        "my_vr_headset_serial_001",     // Serial number (unique ID)
        TrackedDeviceClass_HMD,         // Device type
        m_pHmdDevice                    // Device instance
    );

    // Create and add controller
    m_pControllerDevice = new ControllerDevice();
    VRServerDriverHost()->TrackedDeviceAdded(
        "virtual_controller_001",
        TrackedDeviceClass_Controller,
        m_pControllerDevice
    );

    printf("VR Driver initialized successfully!\n");
    return VRInitError_None;
}

void DriverProvider::Cleanup()
{
    // Clean up HMD device
    if (m_pHmdDevice) {
        delete m_pHmdDevice;
        m_pHmdDevice = nullptr;
    }

    if (m_pControllerDevice) {
        delete m_pControllerDevice;
        m_pControllerDevice = nullptr;
    }

    // Clean up Rust device
    if (m_pRustDevice) {
        vr_device_destroy(m_pRustDevice);
        m_pRustDevice = nullptr;
    }

    VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

void DriverProvider::RunFrame()
{
    // Update tracking data from Arduino (via Rust)
    if (m_pRustDevice) {
        vr_device_update(m_pRustDevice);
    }

    // Let HMD device update its pose
    if (m_pHmdDevice) {
        m_pHmdDevice->RunFrame();
    }

    if (m_pControllerDevice) {
        m_pControllerDevice->RunFrame();
    }
}

const char* const* DriverProvider::GetInterfaceVersions()
{
    return k_InterfaceVersions;
}

bool DriverProvider::ShouldBlockStandbyMode()
{
    return false;
}

void DriverProvider::EnterStandby()
{
}

void DriverProvider::LeaveStandby()
{
}

}