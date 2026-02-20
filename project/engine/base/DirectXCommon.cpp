#include "DirectXCommon.h"
#include "StringUtility.h"
#include "WinApp.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "Logger.h"
#include <cassert>
#include <thread>
using namespace StringUtility;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "dxcompiler.lib")

using namespace Microsoft::WRL;

const uint32_t DirectXCommon::kMaxSRVCount = 512;

void DirectXCommon::Initialize(WinApp *winApp) {
  // FPS固定初期化
  InitializeFixFPS();

  // NULL検出
  assert(winApp);

  // メンバ変数に記録
  this->winApp = winApp;

  CreateDevice();

  CreateCommandQueue();

  CreateSwapChain();

  CreateDepthBuffer();

  CreateDescriptorHeapRTVDSV();

  CreateRenderTargetViews();

  CreateDepthStencilView();

  CreateFence();

  InitializeViewport();

  InitializeScissorRect();

  CreateDXCCompiler();

  InitializeImGui();
}

void DirectXCommon::CreateDevice() {
#ifdef _DEBUG
  ComPtr<ID3D12Debug1> debugController = nullptr;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    debugController->EnableDebugLayer();

    debugController->SetEnableGPUBasedValidation(TRUE);
  }
#endif

  HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
  assert(SUCCEEDED(hr));

  // 使用するアダプタ用の変数。最初にnullptrを入れておく
  ComPtr<IDXGIAdapter4> useAdapter = nullptr;
  // 良い順にアダプタを頼む
  for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
                       i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                       IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
       ++i) {
    // アダプターの情報を取得する
    DXGI_ADAPTER_DESC3 adapterDesc{};
    hr = useAdapter->GetDesc3(&adapterDesc);
    assert(SUCCEEDED(hr));
    // ソフトウェアアダプタでなければ採用！
    if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
      //	Log(ConvertString(std::format(L"Use Adapater:{}\n",
      // adapterDesc.Description)));
      break;
    }
    useAdapter = nullptr;
  }
  // 適切なアダプタが見つからなかったので起動できない
  assert(useAdapter != nullptr);

  // 機能レベルとログ出力用の文字列
  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0};
  const char *featureLevelStrings[] = {"12.2", "12.1", "12.0"};
  for (size_t i = 0; i < _countof(featureLevels); ++i) {
    // 採用したアダプターでデバイスを生成
    hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i],
                           IID_PPV_ARGS(&device));
    if (SUCCEEDED(hr)) {
      // 生成できたのでログ出力を行ってループを抜ける
      /*Logger::Log(std::format("FeatureLevel : {}\n",
       * featureLevelStrings[i]));*/
      break;
    }
  }

#ifdef _DEBUG
  ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
  if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    // やばいエラー時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    // エラー時に止まる
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    // 警告時に止まる
    /*infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,
true);*/

    // 抑制するメッセージのID
    D3D12_MESSAGE_ID denyIds[] = {
        //
        // Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用によるエラーメッセージ

        D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};

    // 抑制するレベル
    D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
    D3D12_INFO_QUEUE_FILTER
    filter{};
    filter.DenyList.NumIDs = _countof(denyIds);
    filter.DenyList.pIDList = denyIds;
    filter.DenyList.NumSeverities = _countof(severities);
    filter.DenyList.pSeverityList = severities;
    // 指定したメッセージの表示を抑制する
    infoQueue->PushStorageFilter(&filter);
  }
#endif

  // デバイスの生成がうまくいかなかったので起動できない
  assert(device != nullptr);

  /* Logger::Log("Complete create D3D12Device!!!\n");*/
}

void DirectXCommon::CreateCommandQueue() {

  HRESULT hr;

  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  hr = device->CreateCommandQueue(&commandQueueDesc,
                                  IID_PPV_ARGS(&commandQueue));
  // コマンドキューの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      IID_PPV_ARGS(&commandAllocator));
  // コマンドアロケータの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                 commandAllocator.Get(), nullptr,
                                 IID_PPV_ARGS(&commandList));
  // コマンドリストの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateSwapChain() {
  HRESULT hr;

  swapChainDesc.Width = WinApp::kClientWidth;
  swapChainDesc.Height = WinApp::kClientHeight;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 2;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  // コマンドキュー、ウィンドウハンドル、設定を渡して生成する
  hr = dxgiFactory->CreateSwapChainForHwnd(
      commandQueue.Get(), winApp->GetHwnd(), &swapChainDesc, nullptr, nullptr,
      reinterpret_cast<IDXGISwapChain1 **>(swapChain.GetAddressOf()));
  assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateDepthBuffer() {
  // 生成するResourceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = winApp->kClientWidth;   // Textureの幅
  resourceDesc.Height = winApp->kClientHeight; // Textureの高さ
  resourceDesc.MipLevels = 1;                  // mipmapの数
  resourceDesc.DepthOrArraySize = 1;           // 奥行きor配列Textureの配列
  resourceDesc.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとして利用可能なフォーマット
  resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。１固定
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // ２次元
  resourceDesc.Flags =
      D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

  // 利用するHeapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

  // 深度値のクリア設定
  D3D12_CLEAR_VALUE depthClearValue{};
  depthClearValue.DepthStencil.Depth = 1.0f; // 1.0f（最大値）でクリア
  depthClearValue.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる

  HRESULT hr = device->CreateCommittedResource(
      &heapProperties,                  // Heapの設定
      D3D12_HEAP_FLAG_NONE,             // Heapの特殊な設定。特になし。
      &resourceDesc,                    // Resourceの設定
      D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
      &depthClearValue,                 // Clear最適値
      IID_PPV_ARGS(&depthStencilResource));
  assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateDescriptorHeapRTVDSV() {
  descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  // DSV用のヒープでデイスクリプタの数は１．DSVはShader内で触るものではないので、ShaderVisibleはfalse
  dsvDescriptorHeap =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

  // RTV用のヒープでデイスクリプタの数は２。RTVはShader内で触るものではないので、ShaderVisibleはfasle
  rtvDescriptorHeap =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

  // SRV用のヒープでデイスクリプタの数は128。SRVはShader内で触るものなので、ShaderVIsibleはture
  srvDescriptorHeap =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                    UINT numDescriptors, bool shaderVisible) {

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
  D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
  descriptorHeapDesc.Type = heapType;
  descriptorHeapDesc.NumDescriptors = numDescriptors;
  descriptorHeapDesc.Flags = shaderVisible
                                 ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                 : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc,
                                            IID_PPV_ARGS(&descriptorHeap));
  assert(SUCCEEDED(hr));
  return descriptorHeap;
}

void DirectXCommon::CreateRenderTargetViews() {
  HRESULT hr;

  hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
  // うまく取得できなければ起動できない
  assert(SUCCEEDED(hr));
  hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
  assert(SUCCEEDED(hr));

  rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
  // ディスクリプタの先頭を取得する
  D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle =
      GetCPUDescriptorHandle(rtvDescriptorHeap, descriptorSizeRTV, 0);

  UINT incrementSize =
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvStartHandle;

  // 表裏の2つ分
  for (uint32_t i = 0; i < 2; ++i) {
    rtvHandles[i] = handle;
    device->CreateRenderTargetView(swapChainResources[i].Get(), &rtvDesc,
                                   rtvHandles[i]);
    handle.ptr += incrementSize;
  }
}

D3D12_CPU_DESCRIPTOR_HANDLE
DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index) {
  return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE
DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index) {
  return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

void DirectXCommon::CreateDepthStencilView() {

  // DSVの設定
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
  dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  // DSVHeapの先頭にDSVをつくる
  device->CreateDepthStencilView(
      depthStencilResource.Get(), &dsvDesc,
      dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::CreateFence() {
  HRESULT hr;

  hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(&fence));
  assert(SUCCEEDED(hr));

  // FenceのSignalを待つためのイベントを作成する
  fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  assert(fenceEvent != nullptr);
}

void DirectXCommon::InitializeViewport() {
  // ビューボート

  // クライアント領域のサイズと一緒にして画面全体に表示
  viewport.Width = WinApp::kClientWidth;
  viewport.Height = WinApp::kClientHeight;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
}

void DirectXCommon::InitializeScissorRect() {

  // 基本的にビューボートと同じ矩形が構成されるようにする
  scissorRect.left = 0;
  scissorRect.right = WinApp::kClientWidth;
  scissorRect.top = 0;
  scissorRect.bottom = WinApp::kClientHeight;
}

void DirectXCommon::CreateDXCCompiler() {
  HRESULT hr;

  hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
  assert(SUCCEEDED(hr));
  hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
  assert(SUCCEEDED(hr));

  hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
  assert(SUCCEEDED(hr));
}

void DirectXCommon::InitializeImGui() {
  // ImGuiの初期化。詳細はさして重要ではないので解説は省略する。
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplWin32_Init(winApp->GetHwnd());
  ImGui_ImplDX12_Init(device.Get(), swapChainDesc.BufferCount, rtvDesc.Format,
                      srvDescriptorHeap.Get(),
                      srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                      srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void DirectXCommon::PreDraw() {

  // これから書き込むバックバッファのインデックスを取得
  UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

  // 今回のバリアはTransition
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  // Noneにしておく
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  // バリアを張る対象のリソース。現在のバックバッファに対して行う
  barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
  // 遷移前（現在）のResourceState
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  // 遷移後のResourceState
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  // TransitionBarrierを張る
  commandList->ResourceBarrier(1, &barrier);

  // 描画先のRTVとDSVを設定する
  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
      GetCPUDescriptorHandle(dsvDescriptorHeap, descriptorSizeDSV, 0);

  // 描画先のRTVを設定する
  commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false,
                                  nullptr);

  // 指定した色で画面全体をクリアする
  float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f}; // 青っぽい色。RGBAの順
  commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0,
                                     nullptr);

  commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false,
                                  &dsvHandle);

  // 指定した深度で画面全体をクリアする
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0,
                                     0, nullptr);

  // 描画用のDescriptorHeapの設定
  ID3D12DescriptorHeap *descriptorHeaps[] = {srvDescriptorHeap.Get()};
  commandList->SetDescriptorHeaps(1, descriptorHeaps);

  // コマンドを積む
  commandList->RSSetViewports(1, &viewport);       // Viewportを設定
  commandList->RSSetScissorRects(1, &scissorRect); // Scissorを設定
}

void DirectXCommon::PostDraw() {
  HRESULT hr;

  UINT bbIndex = swapChain->GetCurrentBackBufferIndex();

  // 画面表示できるようにする

  // 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
  // 今回はRenderTargetからPresentにする
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  // TransitionBarrierを張る
  commandList->ResourceBarrier(1, &barrier);

  // CommandListを閉じる

  // コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
  hr = commandList->Close();
  assert(SUCCEEDED(hr));

  // コマンドをキックする

  // GPUにコマンドリストの実行を行わせる
  ID3D12CommandList *commandLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(1, commandLists);
  // GPUとOSに画面の交換を行うよう通知する
  swapChain->Present(1, 0);

  // GPUにSignalを送る

  // Fenceの値を更新
  fenceValue++;
  // GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
  commandQueue->Signal(fence.Get(), fenceValue);

  // Fenceの値が指定したSignal値にたどり着いているか確認する
  // GetCompletedValueの初期値はFence作成時に渡した初期値
  if (fence->GetCompletedValue() < fenceValue) {
    // 指定したSignalにたどりついていないので、たどり着くまで待つようにイベントを設定する
    fence->SetEventOnCompletion(fenceValue, fenceEvent);
    // イベント待つ
    WaitForSingleObject(fenceEvent, INFINITE);
  }

  // FPS固定
  UpdateFixFPS();

  // 次のフレーム用のコマンドリストを準備
  hr = commandAllocator->Reset();
  assert(SUCCEEDED(hr));
  hr = commandList->Reset(commandAllocator.Get(), nullptr);
  assert(SUCCEEDED(hr));
}

Microsoft::WRL::ComPtr<IDxcBlob>
DirectXCommon::CompileShader(const std::wstring &filePath,
                             const wchar_t *profile) {
  // これからシェーダをコンバイルｓる旨をログに出す
  // Logger::Log(StringUtility::ConvertString(std::format(L" Begin
  // CompileShader,path:{},profile{}\n", filePath, profile)));
  // hlslファイルを読む
  Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
  HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
  // 読めなかったら止める
  assert(SUCCEEDED(hr));
  // 読み込んだファイルの内容を設定する
  DxcBuffer shaderSourceBuffer;
  shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
  shaderSourceBuffer.Size = shaderSource->GetBufferSize();
  shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF8の文字コードであることを通知
  LPCWSTR arguments[] = {
      filePath.c_str(), // コンパイル対象のhlslファイル名
      L"-E",
      L"main", // エントリーポイントの指定。基本的にmain以外にはしない
      L"-T",
      profile, // shaderProfileの設定
      L"-Zi",
      L"-Qembed_debug", // デバッグ用の情報を埋め込む
      L"Od",            // 最適化を外しておく
      L"-Zpr",          // メモリレイアウトは行優先
  };

  // 実際にShaderをコンパイルする
  Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
  hr = dxcCompiler->Compile(&shaderSourceBuffer,  // 読み込んだファイル
                            arguments,            // コンパイルオプション
                            _countof(arguments),  // コンパイルオプションの数
                            includeHandler.Get(), // includeが含まれた諸々
                            IID_PPV_ARGS(&shaderResult) // コンパイル結果
  );
  // コンパイルエラーではなく<dxcが起動できないなど致命的な状況
  assert(SUCCEEDED(hr));
  // 警告・エラーが出てきたログに出して止める
  Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
  shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
  if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
    Logger::Log(shaderError->GetStringPointer());
    // 警告・エラーがダメ絶対
    assert(false);
  }

  // コンパイル結果から実行用のバイナリ部分を所得

  Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
  hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob),
                               nullptr);
  assert(SUCCEEDED(hr));
  // 成功したログを出す
  // Logger::Log(StringUtility::ConvertString(std::format(L"Compile
  // Succeeded,path:{}, profile:{}\n", filePath, profile)));
  // もう使わないリソースを解放
  /*shaderSource->Release();
  shaderResult->Release();*/
  // 実行用のバイナリを返却
  return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource>
DirectXCommon::CreateBufferResource(size_t sizeInBytes) {
  // 頂点リソース用のヒープの設定
  D3D12_HEAP_PROPERTIES uploadHeapProperties{};
  uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使う
  // 頂点リソースの設定
  D3D12_RESOURCE_DESC vertexResourceDesc{};
  // バッファリソース。テクスチャの場合はまた別の設定する
  vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  vertexResourceDesc.Width =
      sizeInBytes; // リリースのサイズ。今回はVector4を3頂点分
  // バッファの場合はこれからは１にする決まり
  vertexResourceDesc.Height = 1;
  vertexResourceDesc.DepthOrArraySize = 1;
  vertexResourceDesc.MipLevels = 1;
  vertexResourceDesc.SampleDesc.Count = 1;
  // バッファの場合はこれにする決まり
  vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  // 実際に頂点リソースを作る
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&vertexResource));
  assert(SUCCEEDED(hr));
  return vertexResource;
}

Microsoft::WRL::ComPtr<ID3D12Resource>
DirectXCommon::CreateTextureResource(const DirectX::TexMetadata &metadata) {
  // １.metadataを基にResorceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = UINT(metadata.width);           // Teffxtureの幅
  resourceDesc.Height = UINT(metadata.height);         // Textureの高さ
  resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
  resourceDesc.DepthOrArraySize =
      UINT16(metadata.arraySize);        // 奥行きor配列Textureの配列数
  resourceDesc.Format = metadata.format; // TextureのFormat
  resourceDesc.SampleDesc.Count = 1;     // サンプリングカウント。１固定
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(
      metadata.dimension); // Textureの次元数。普段使っているのは２次元
  // ２.利用するHeapの設定
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM; // 細かい設定を行う
  heapProperties.CPUPageProperty =
      D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
  heapProperties.MemoryPoolPreference =
      D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置
  // ３.Resourceを生成する
  Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties,                   // Heapの設定
      D3D12_HEAP_FLAG_NONE,              // Heapの特殊な設定。特になし
      &resourceDesc,                     // Resourceの設定
      D3D12_RESOURCE_STATE_GENERIC_READ, // 初回のResourceState。Textureは基本読むだけ
      nullptr,                           // Clear最適値。使わないのでnullptr
      IID_PPV_ARGS(&resource));          // 作成するResourceポインタへのポインタ
  assert(SUCCEEDED(hr));
  return resource;
}

void DirectXCommon::UploadTextureData(
    const Microsoft::WRL::ComPtr<ID3D12Resource> &texture,
    const DirectX::ScratchImage &mipImages) {
  // Meta情報を取得
  const DirectX::TexMetadata &metadata = mipImages.GetMetadata();
  // 全MipMapについて
  for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
    // MipMapLevelを指定して各Imageを取得
    const DirectX::Image *img = mipImages.GetImage(mipLevel, 0, 0);
    // Textureに転送
    HRESULT hr =
        texture->WriteToSubresource(UINT(mipLevel),
                                    nullptr,              // 全領域へコピー
                                    img->pixels,          // 元データアドレス
                                    UINT(img->rowPitch),  // １ラインサイズ
                                    UINT(img->slicePitch) // １枚サイズ
        );
    assert(SUCCEEDED(hr));
  }
}

DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string &filePath) {
  // テクスチャファイルを読んでプログラムで扱えるようにする
  DirectX::ScratchImage image{};
  std::wstring filePathW = ConvertString(filePath);
  HRESULT hr = DirectX::LoadFromWICFile(
      filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_RGB, nullptr, image);
  assert(SUCCEEDED(hr));

  // ミップマップの作成
  DirectX::ScratchImage mipImages{};
  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                0, mipImages);
  assert(SUCCEEDED(hr));

  // ミップマップ付きのデータを返す
  return mipImages;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index) {
  D3D12_CPU_DESCRIPTOR_HANDLE handle =
      descriptorHeap->GetCPUDescriptorHandleForHeapStart();
  handle.ptr += descriptorSize * index;
  return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index) {
  D3D12_GPU_DESCRIPTOR_HANDLE handle =
      descriptorHeap->GetGPUDescriptorHandleForHeapStart();

  handle.ptr += descriptorSize * index;
  return handle;
}

void DirectXCommon::InitializeFixFPS() {
  // 現在時間を記録する
  reference_ = std::chrono::steady_clock::now();
}

void DirectXCommon::UpdateFixFPS() {
  // 1/60秒ぴったりの時間
  const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
  // 1/60秒よりわずかに短い時間
  const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

  // 現在時間を取得する
  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  // 前回記録からの経過時間を所得する
  std::chrono::microseconds elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);
  // 1/60秒（よりわかずに短い時間）経ってない場合
  if (elapsed < kMinCheckTime) {
    // 1/60秒経過するまで微小なスリープを繰り返す
    while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
      // １マイクロ秒スリープ
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }

  // 現在の時間を記録する
  reference_ = std::chrono::steady_clock::now();
}