#pragma once

#include <openvr_driver.h>
#include "../../rust_core/src/rust_bridge.h"
#include "hmd_device.h"
#include "controller_device.h"

namespace vr_driver {

class HMDDevice;
class ControllerDevice;

class DriverProvider : public vr::IServerTrackedDeviceProvider
{
public:
    DriverProvider();
    virtual ~DriverProvider();

    // IServerTrackedDeviceProvider interface
    virtual vr::EVRInitError Init(vr::IVRDriverContext* pDriverContext) override;
    virtual void Cleanup() override;
    virtual const char* const* GetInterfaceVersions() override;
    virtual void RunFrame() override;
    virtual bool ShouldBlockStandbyMode() override;
    virtual void EnterStandby() override;
    virtual void LeaveStandby() override;

private: 
    VRDevice* m_pRustDevice;
    HMDDevice* m_pHmdDevice;
    ControllerDevice* m_pControllerDevice;
};

}