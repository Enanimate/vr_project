#pragma once

#include <openvr_driver.h>
#include "../../rust_core/src/rust_bridge.h"
#include "display_component.h"

namespace vr_driver {

class HMDDevice : public vr::ITrackedDeviceServerDriver
{
public: 
    HMDDevice(VRDevice* pRustDevice);
    virtual ~HMDDevice();

    // ITrackedDeviceServerDriver interface
    virtual vr::EVRInitError Activate(uint32_t unObjectId) override;
    virtual void Deactivate() override;
    virtual void EnterStandby() override;
    virtual void* GetComponent(const char* pchComponentNameAndVersion) override;
    virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
    virtual vr::DriverPose_t GetPose() override;

    void RunFrame();

private:
    VRDevice* m_pRustDevice;
    uint32_t m_unObjectId;
    vr::PropertyContainerHandle_t m_ulPropertyContainer;
    DisplayComponent* m_pDisplayComponent;

    void SetupProperties();
};

}