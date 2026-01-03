#include "../include/display_component.h"
#include <cmath>

using namespace vr;

namespace vr_driver {
DisplayComponent::DisplayComponent()
{
}

DisplayComponent::~DisplayComponent()
{
}

void DisplayComponent::GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight)
{
    // Position window at 0,0 (extended mode) or use full screen
    *pnX = 0;
    *pnY = 0;
    *pnWidth = m_nWindowWidth;
    *pnHeight = m_nWindowHeight;
}

bool DisplayComponent::IsDisplayOnDesktop()
{
    // Extended mode (window on desktop) vs direct mode
    return true;
}

bool DisplayComponent::IsDisplayRealDisplay()
{
    return false;
}

void DisplayComponent::GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight)
{
    // Resolution to render at (per eye)
    *pnWidth = m_nRenderWidth / 2;  // Split for two eyes
    *pnHeight = m_nRenderHeight;
}

void DisplayComponent::GetEyeOutputViewport(EVREye eEye, uint32_t* pnX, uint32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight)
{
    // Define a viewport for each eye (side-by-side layout)
    *pnY = 0;
    *pnWidth = m_nWindowWidth / 2;
    *pnHeight = m_nWindowHeight;

    if (eEye == Eye_Left)
    {
        *pnX = 0;
    }
    else
    {
        *pnX = m_nWindowWidth / 2;
    }
}

void DisplayComponent::GetProjectionRaw(EVREye eEye, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom)
{
    // Define FOV for projection matrix
    // These are tangent values, not angles
    // ~90 degree horizontal FOV, ~90 degree vertical FOV
    *pfLeft = -1.0f;
    *pfRight = 1.0f;
    *pfTop = -1.0f;
    *pfBottom = 1.0f;
}

DistortionCoordinates_t DisplayComponent::ComputeDistortion(EVREye eEye, float fU, float fV)
{
    // NO LENS DISTORTION (passthrough for tracking-only)
    // Just return the input coordinates unchanged
    DistortionCoordinates_t coords;
    coords.rfRed[0] = fU;
    coords.rfRed[1] = fV;
    coords.rfGreen[0] = fU;
    coords.rfGreen[1] = fV;
    coords.rfBlue[0] = fU;
    coords.rfBlue[1] = fV;

    return coords;
}

bool DisplayComponent::ComputeInverseDistortion(HmdVector2_t* pResult, EVREye eEye, uint32_t unChannel, float fU, float fV)
{
    // No inverse distortion (passthrough)
    if (pResult)
    {
        pResult->v[0] = fU;
        pResult->v[1] = fV;
        return true;
    }
    return false;
}

}