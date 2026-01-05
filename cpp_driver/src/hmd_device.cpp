#include "../include/hmd_device.h"
#include <cstdio>

using namespace vr;

namespace vr_driver {
HMDDevice::HMDDevice(VRDevice* pRustDevice)
    : m_pRustDevice(pRustDevice)
    , m_unObjectId(k_unTrackedDeviceIndexInvalid)
    , m_ulPropertyContainer(k_ulInvalidPropertyContainer)
    , m_pDisplayComponent(nullptr)
{
    m_pDisplayComponent = new DisplayComponent();
}

HMDDevice::~HMDDevice()
{
    delete m_pDisplayComponent;
    m_pDisplayComponent = nullptr;
}

EVRInitError HMDDevice::Activate(uint32_t unObjectId)
{
    m_unObjectId = unObjectId;
    m_ulPropertyContainer = VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

    SetupProperties();

    return VRInitError_None;
}

void HMDDevice::SetupProperties()
{
    // Basic device info
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, "CustomVRHeadset_V1");
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ManufacturerName_String, "CustomVR");
    VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, "generic_hmd");

    // Display properties (Dummy Values for tracking-only)
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserIpdMeters_Float, 0.063f);
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_DisplayFrequency_Float, 60.0f);
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_SecondsFromVsyncToPhotons_Float, 0.011f);

    // Dummy display resolution
    VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_DisplayMCImageWidth_Int32, 1920);
    VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_DisplayMCImageHeight_Int32, 1080);
    VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_DisplayMCImageNumChannels_Int32, 3);

    // Extended display mode (0 = direct mode, 1 = extended)
    VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_DisplayMCType_Int32, 0);
    VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_EdidVendorID_Int32, 0xD24E);
    VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_EdidProductID_Int32, 0x1019);

    // Mark as 3DOF device (rotation only, no positional tracking)
    VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_WillDriftInYaw_Bool, true);
    VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_DeviceProvidesBatteryStatus_Bool, false);

    // Lens/distortion properties (no distortion for tracking-only)
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_LensCenterLeftU_Float, 0.5f);
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_LensCenterLeftV_Float, 0.5f);
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_LensCenterRightU_Float, 0.5f);
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_LensCenterRightV_Float, 0.5f);
    VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.0f);

    printf("HMD properties configured\n");
}

void HMDDevice::Deactivate()
{
    m_unObjectId = k_unTrackedDeviceIndexInvalid;
}

void HMDDevice::EnterStandby()
{
}

void* HMDDevice::GetComponent(const char* pchComponentNameAndVersion)
{
    printf("GetComponent requested: %s\n", pchComponentNameAndVersion);

    // Return display component when requested
    if (0 == strcmp(pchComponentNameAndVersion, IVRDisplayComponent_Version))
    {
        printf("Returning display component\n");
        return m_pDisplayComponent;
    }

    printf("Component not found, returning nullptr\n");
    return nullptr;
}

void HMDDevice::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize)
{
    if (unResponseBufferSize >= 1)
        pchResponseBuffer[0] = 0;
}

DriverPose_t HMDDevice::GetPose()
{
    DriverPose_t pose = { 0 };

    if (!m_pRustDevice) {
        pose.result = TrackingResult_Uninitialized;
        pose.poseIsValid = false;
        return pose;
    }

    // Get quaternion from Rust
    Quaternion quat;
    vr_device_get_pose(m_pRustDevice, &quat);

    // Get position from Rust 
    Vec3 position;
    vr_device_get_position(m_pRustDevice, &position);

    // Check if device is connected
    bool connected = vr_device_is_connected(m_pRustDevice);

    // Fill in pose data
    pose.poseIsValid = connected;
    pose.result = connected ? TrackingResult_Running_OK : TrackingResult_Running_OutOfRange;
    pose.deviceIsConnected = connected;

    // Rotation from Arduino (quaternion)
    pose.qRotation.w = quat.w;
    pose.qRotation.x = quat.x;
    pose.qRotation.y = quat.y;
    pose.qRotation.z = quat.z;

    // Position from IR camera tracking
    pose.vecPosition[0] = position.x;
    pose.vecPosition[1] = position.y;
    pose.vecPosition[2] = position.z;

    // Coordinate system transforms (identity = no transform)
    pose.qWorldFromDriverRotation.w = 1.0;
    pose.qWorldFromDriverRotation.x = 0.0;
    pose.qWorldFromDriverRotation.y = 0.0;
    pose.qWorldFromDriverRotation.z = 0.0;

    pose.qDriverFromHeadRotation.w = 1.0;
    pose.qDriverFromHeadRotation.x = 0.0;
    pose.qDriverFromHeadRotation.y = 0.0;
    pose.qDriverFromHeadRotation.z = 0.0;

    // No velocity data (3DOF only)
    pose.vecVelocity[0] = 0.0;
    pose.vecVelocity[1] = 0.0;
    pose.vecVelocity[2] = 0.0;

    pose.vecAngularVelocity[0] = 0.0;
    pose.vecAngularVelocity[1] = 0.0;
    pose.vecAngularVelocity[2] = 0.0;

    pose.poseTimeOffset = 0.0;
    pose.shouldApplyHeadModel = false;

    return pose;
}

void HMDDevice::RunFrame()
{
    if (m_unObjectId != k_unTrackedDeviceIndexInvalid)
    {
        // Send updated pose to SteamVR
        VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, GetPose(), sizeof(DriverPose_t));
    }
}

}