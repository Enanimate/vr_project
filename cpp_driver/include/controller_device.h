#pragma once

#include <openvr_driver.h>

namespace vr_driver {

class ControllerDevice : public vr::ITrackedDeviceServerDriver
{
public:
    ControllerDevice();
    virtual ~ControllerDevice();

    // ITrackedDeviceServerDriver interface
    virtual vr::EVRInitError Activate(uint32_t unObjectId) override;
    virtual void Deactivate() override;
    virtual void EnterStandby() override;
    virtual void* GetComponent(const char* pchComponentNameAndVersion) override;
    virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
    virtual vr::DriverPose_t GetPose() override;

    void RunFrame();

private:
    uint32_t m_unObjectId;
    vr::PropertyContainerHandle_t m_ulPropertyContainer;
    vr::VRInputComponentHandle_t m_menuButton;
    bool m_menuPressed;

    void SetupProperties();
};

}