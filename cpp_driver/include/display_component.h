#pragma once

#include <openvr_driver.h>

namespace vr_driver {

class DisplayComponent : public vr::IVRDisplayComponent
{
public:
    DisplayComponent();
    virtual ~DisplayComponent();

    // IVRDisplayComponent interface
    virtual void GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) override;
    virtual bool IsDisplayOnDesktop() override;
    virtual bool IsDisplayRealDisplay() override;
    virtual void GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) override;
    virtual void GetEyeOutputViewport(vr::EVREye eEye, uint32_t* pnX, uint32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) override;
    virtual void GetProjectionRaw(vr::EVREye eEye, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom) override;
    virtual vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eEye, float fU, float fV) override;
    virtual bool ComputeInverseDistortion(vr::HmdVector2_t* pResult, vr::EVREye eEye, uint32_t unChannel, float fU, float fV) override;

private:
    // Display configuration
    static constexpr uint32_t m_nRenderWidth = 2560;
    static constexpr uint32_t m_nRenderHeight = 1440;
    static constexpr uint32_t m_nWindowWidth = 2560;
    static constexpr uint32_t m_nWindowHeight = 1440;
};

}