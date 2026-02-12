#pragma once
#include "Logger.h"
#include "StringUtility.h"
#include "WinApp.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <Windows.h>
#include <array>
#include <cassert>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <format>
#include <wrl.h>

class DirectXCommon {
public:
  // 初期化処理
  void Initialize(WinApp *winApp);

  void CreateDevice();
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

  // コマンド関連
  void CreateCommandQueue();

  // スワップチェーン
  void CreateSwapChain();

  // 深度バッファ
  void CreateDepthBuffer();

  // 各種デスクリプタヒープ
  void CreateDescriptorHeapRTVDSV();

  // レンダーターゲットビュー
  void CreateRenderTargetViews();

  // 指定番号のCPUデスクリプタハンドルを取得する
  static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
      const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
      uint32_t descriptorSize, uint32_t index);
  // 指定番号のGPUデスクリプタハンドルを取得する
  static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
      const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
      uint32_t descriptorSize, uint32_t index);

  // 深度ステンシルビュー
  void CreateDepthStencilView();

  // フェンス
  void CreateFence();

  // ビューポート
  void InitializeViewport();

  // シザリング矩形
  void InitializeScissorRect();

  // DXCコンパイラ
  void CreateDXCCompiler();

  // ImGui
  void InitializeImGui();

  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

  // 描画前処理
  void PreDraw();
  // 描画後処理
  void PostDraw();
  // フェンス値
  UINT64 fenceVal = 0;
  // DirectX12デバイス
  Microsoft::WRL::ComPtr<ID3D12Device> device;

  ID3D12Device *GetDevice() const { return device.Get(); }
  ID3D12GraphicsCommandList *GetCommandList() const {
    return commandList.Get();
  }

  // シェーダーのコンパイル
  Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring &filePath,
                                                 const wchar_t *profile);

  // バッファリソースの生成
  Microsoft::WRL::ComPtr<ID3D12Resource>
  CreateBufferResource(size_t sizeInBytes);

private: // メンバ変数
         // getter

  

  // テクスチャソースの生成
  Microsoft::WRL::ComPtr<ID3D12Resource>
  CreateTextureResource(const DirectX::TexMetadata &metadata);

  // テクスチャデータの転送
  void UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource> &texture,
                         const DirectX::ScratchImage &mipImages);

  // 各種デスクリプタヒープ
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
  CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors,
                       bool shaderVisible);

  // レンダーターゲットビュー

  // SRVの指定指定番号のCPUデスクリプタハンドルを取得する
  D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
  // SRVの指定指定番号のGPUデスクリプタハンドルを取得する
  D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);
  // スワップチェーンリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2];

  // スワップチェーン
  Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
  // RTV用デスクリプタヒープ
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
  // SRV用デスクリプタヒープ
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
  // SRV用デスクリプタサイズ
  UINT descriptorSizeSRV = 0;

  // 深度ステンシルビュー
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

  // フェンス
  Microsoft::WRL::ComPtr<ID3D12Fence> fence;
  HANDLE fenceEvent = nullptr;

  // ビューポート

  D3D12_VIEWPORT viewport{};

  // シザリング矩形
  D3D12_RECT scissorRect{};

  // DXGIファクトリ
  Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
  // WindowsAPI
  WinApp *winApp = nullptr;
  std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> rtvHandles{};
};