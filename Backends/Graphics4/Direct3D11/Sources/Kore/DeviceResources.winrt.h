﻿//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "CameraResources.winrt.h"
#include <wrl.h>
#include <dxgi1_5.h>
#include <d3d11_4.h>
#include <map>
#include <mutex>

namespace DX
{
    // Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
    interface IDeviceNotify
    {
        virtual void OnDeviceLost()     = 0;
        virtual void OnDeviceRestored() = 0;
    };

    // Creates and manages a Direct3D device and immediate context, Direct2D device and context (for debug), and the holographic swap chain.
    class DeviceResources
    {
    public:
        DeviceResources();
		void InitWithDevice(Microsoft::WRL::ComPtr<ID3D11Device4> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext3>  context);
        // Public methods related to Direct3D devices.
        void HandleDeviceLost();
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
        void Trim();
        void Present(Windows::Graphics::Holographic::HolographicFrame^ frame);

        // Public methods related to holographic devices.
        void EnsureCameraResources(
            Windows::Graphics::Holographic::HolographicFrame^ frame,
            Windows::Graphics::Holographic::HolographicFramePrediction^ prediction);

		void LockCameraResources();
		void UnlockCameraResources();

		DX::CameraResources* GetResourcesForCamera(Windows::Graphics::Holographic::HolographicCamera^ camera);
        void AddHolographicCamera(Windows::Graphics::Holographic::HolographicCamera^ camera);
        void RemoveHolographicCamera(Windows::Graphics::Holographic::HolographicCamera^ camera);

        // Holographic accessors.
        template<typename RetType, typename LCallback>
        RetType                 UseHolographicCameraResources(const LCallback& callback);

        Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice^
                                GetD3DInteropDevice() const             { return m_d3dInteropDevice;    }

        // D3D accessors.
        ID3D11Device4*          GetD3DDevice() const                    { return m_d3dDevice.Get();     }
        ID3D11DeviceContext3*   GetD3DDeviceContext() const             { return m_d3dContext.Get();    }
        D3D_FEATURE_LEVEL       GetDeviceFeatureLevel() const           { return m_d3dFeatureLevel;     }

    private:
        // Direct3D objects.
        Microsoft::WRL::ComPtr<ID3D11Device4>                   m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext3>            m_d3dContext;

        // Direct3D interop objects.
        Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice^ m_d3dInteropDevice;

        // Properties of the Direct3D device currently in use.
        D3D_FEATURE_LEVEL                                       m_d3dFeatureLevel = D3D_FEATURE_LEVEL_10_0;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify*                                          m_deviceNotify = nullptr;

        // Back buffer resources, etc. for attached holographic cameras.
        std::map<UINT32, std::unique_ptr<CameraResources>>      m_cameraResources;
        std::mutex                                              m_cameraResourcesMutex;
		std::unique_lock<std::mutex>							m_cameraResLock;
    };
}

// Device-based resources for holographic cameras are stored in a std::map. Access this list by providing a
// callback to this function, and the std::map will be guarded from add and remove
// events until the callback returns. The callback is processed immediately and must
// not contain any nested calls to UseHolographicCameraResources.
// The callback takes a parameter of type std::map<UINT32, std::unique_ptr<DX::CameraResources>>&
// through which the list of cameras will be accessed.
template<typename RetType, typename LCallback>
RetType DX::DeviceResources::UseHolographicCameraResources(const LCallback& callback)
{
    std::lock_guard<std::mutex> guard(m_cameraResourcesMutex);
    return callback(m_cameraResources);
}
