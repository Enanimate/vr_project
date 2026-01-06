#include "../include/controller_device.h"
#include <cstdio>
#include <windows.h>

using namespace vr;

namespace vr_driver {

ControllerDevice::ControllerDevice(VRDevice* pRustDevice)
    : m_unObjectId(k_unTrackedDeviceIndexInvalid)
    , m_ulPropertyContainer(k_ulInvalidPropertyContainer)
    , m_menuButton(k_ulInvalidInputComponentHandle)
    , m_menuPressed(false)
    , m_pRustDevice(pRustDevice)
{
}

ControllerDevice::~ControllerDevice()
{
}

EVRInitError ControllerDevice::Activate(uint32_t unObjectId)
{
    m_unObjectId = unObjectId;
    m_ulPropertyContainer = VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

    SetupProperties();

    // Create menu button input component (using system button to open dashboard)
    VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, "/input/system/click", &m_menuButton);

    printf("Controller menu button initialized\n");

    return VRInitError_None;
}

void ControllerDevice::SetupProperties()
{
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, "VirtualController");
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ManufacturerName_String, "CustomVR");
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, "{htc}vr_tracker_vive_1_0");

    // Controller properties - set as left hand controller for input binding
    VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_ControllerRoleHint_Int32, TrackedControllerRole_LeftHand);
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_InputProfilePath_String, "{custom_vr_driver}/input/devboard_profile.json");
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ControllerType_String, "dev_board");

    printf("Virtual controller configured\n");
}

void ControllerDevice::Deactivate()
{
    m_unObjectId = k_unTrackedDeviceIndexInvalid;
}

void ControllerDevice::EnterStandby()
{
}

void* ControllerDevice::GetComponent(const char* pchComponentNameAndVersion)
{
    return nullptr;
}

void ControllerDevice::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize)
{
    if (unResponseBufferSize >= 1)
        pchResponseBuffer[0] = 0;
}

DriverPose_t ControllerDevice::GetPose()
{
    DriverPose_t pose = { 0 };

    // Controller sits at origin (invisible)
    pose.poseIsValid = true;
    pose.result = TrackingResult_Running_OK;
    pose.deviceIsConnected = true;

    pose.qRotation.w = 1.0;
    pose.qRotation.x = 0.0;
    pose.qRotation.y = 0.0;
    pose.qRotation.z = 0.0;

    pose.vecPosition[0] = 0.0;
    pose.vecPosition[1] = -1.0;
    pose.vecPosition[2] = 0.0;

    return pose;
}

void ControllerDevice::RunFrame()
{
    if (m_unObjectId == k_unTrackedDeviceIndexInvalid)
        return;

    // Get button state from Rust
    bool buttonM = vr_device_get_button_m(m_pRustDevice);

    // Update button state
    if (buttonM && !m_menuPressed) {
        printf("Button PRESSED - updating component to TRUE\n");
        VRDriverInput()->UpdateBooleanComponent(m_menuButton, true, 0);
        m_menuPressed = true;
    } else if (!buttonM && m_menuPressed) {
        printf("Button RELEASED - updating component to FALSE\n");
        VRDriverInput()->UpdateBooleanComponent(m_menuButton, false, 0);
        m_menuPressed = false;
    }

    // Update pose
    VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, GetPose(), sizeof(DriverPose_t));
}

}