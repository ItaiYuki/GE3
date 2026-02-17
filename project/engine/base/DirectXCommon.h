#pragma once
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <Windows.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

class WinApp;

class DirectXCommon {
public:
  // 初期化処理
  void Initialize(WinApp *winApp);

  void CreateDevice();

  void CreateCommandQueue();

  void CreateSwapChain();

  void CreateDepthBuffer();

  void CreateDescriptorHeapRTVDSV();

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
  CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors,
                       bool shaderVisible);

  void CreateRenderTargetViews();

  // SRVの指定指定番号のCPUデスクリプタハンドルを取得する
  D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
  // SRVの指定指定番号のGPUデスクリプタハンドルを取得する
  D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

  void CreateDepthStencilView();

  void CreateFence();

  void InitializeViewport();

  void InitializeScissorRect();

  void CreateDXCCompiler();

  void InitializeImGui();

  // 描画前処理
  void PreDraw();
  // 描画後処理
  void PostDraw();

  ID3D12Device *GetDevice() const { return device.Get(); }

  ID3D12GraphicsCommandList *GetCommandList() const {
    return commandList.Get();
  }

  // シェーダ－コンパイル
  Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring &filePath,
                                                 const wchar_t *profile);

  Microsoft::WRL::ComPtr<ID3D12Resource>
  CreateBufferResource(size_t sizeInBytes);

  Microsoft::WRL::ComPtr<ID3D12Resource>
  CreateTextureResource(const DirectX::TexMetadata &metadata);

  void UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource> &texture,
                         const DirectX::ScratchImage &mipImages);

  static DirectX::ScratchImage LoadTexture(const std::string &filePath);

private:
  // DirectX12デバイス
  Microsoft::WRL::ComPtr<ID3D12Device> device;

  // DXGIファクトリ
  Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;

  // コマンドキューを生成する
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;

  // コマンドアロケータを生成する
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;

  // コマンドリストを生成する
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;

  // スワップチェーンを生成する
  Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

  // WindowsAPI
  WinApp *winApp = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

  // 指定番号のCPUデスクリプタハンドルを取得する
  static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
      const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
      uint32_t descriptorSize, uint32_t index);
  // 指定番号のGPUデスクリプタハンドルを取得する
  static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
      const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
      uint32_t descriptorSize, uint32_t index);

  uint32_t descriptorSizeSRV;
  uint32_t descriptorSizeRTV;
  uint32_t descriptorSizeDSV;

  // スワップチェーンリソース
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

  // 初期値0でFenceを作る
  Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
  // フェンス値
  uint64_t fenceValue = 0;

  HANDLE fenceEvent;

  // ビューポート
  D3D12_VIEWPORT viewport{};

  // シザー矩形
  D3D12_RECT scissorRect{};

  // dxcCompilerを初期化
  Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
  Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
  // 現時点でincludeはしないが、includeに対応するための設定を行っておく
  Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;

  // RTVの設定
  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

  // RTVを2つ作るのでディスクリプタを2つ用意
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

  // TransitionBarrierの設定
  D3D12_RESOURCE_BARRIER barrier{};

  ////FPS固定初期化
  void InitializeFixFPS();
  ////FPS固定更新
  void UpdateFixFPS();

  ////記録時間（FPS固定用）
  std::chrono::steady_clock::time_point reference_;
};