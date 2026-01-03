#include "../include/controller_device.h"
#include <cstdio>
#include <windows.h>

using namespace vr;

namespace vr_driver {

ControllerDevice::ControllerDevice()
    : m_unObjectId(k_unTrackedDeviceIndexInvalid)
    , m_ulPropertyContainer(k_ulInvalidPropertyContainer)
    , m_menuButton(k_ulInvalidInputComponentHandle)
    , m_menuPressed(false)
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

    // Create menu button input component
    VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, "/input/application_menu/click", &m_menuButton);

    printf("Controller menu button initialized\n");

    return VRInitError_None;
}

void ControllerDevice::SetupProperties()
{
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, "VirtualController");
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ManufacturerName_String, "CustomVR");
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, "{htc}vr_tracker_vive_1_0");

    // Controller properties - identify as Vive controller for automatic bindings
    VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_ControllerRoleHint_Int32, TrackedControllerRole_LeftHand);
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_InputProfilePath_String, "{htc}/input/vive_controller_profile.json");
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ControllerType_String, "vive_controller");

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

    // Y key for menu button
    bool yKey = (GetAsyncKeyState('Y') & 0x8000) != 0;

    // Update menu button state
    if (yKey && !m_menuPressed) {
        VRDriverInput()->UpdateBooleanComponent(m_menuButton, true, 0);
        m_menuPressed = true;
    } else if (!yKey && m_menuPressed) {
        VRDriverInput()->UpdateBooleanComponent(m_menuButton, false, 0);
        m_menuPressed = false;
    }

    // Update pose
    VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, GetPose(), sizeof(DriverPose_t));
}

}