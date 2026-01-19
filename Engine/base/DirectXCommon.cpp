#include "DirectXCommon.h"

#include <cassert>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;

void DirectXCommon::Initialize(WinApp *winApp) {
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

/// =========================================
///  デバイス(ID3D12Device)の生成
/// =========================================
void DirectXCommon::CreateDevice() {
  HRESULT hr;

  // DXGIファクトリーの生成
  IDXGIFactory7 *dxgiFactory = nullptr;
  hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
  assert(SUCCEEDED(hr));

  // 使用するアダプタ用の変数。最初にnullptrを入れておく
  IDXGIAdapter4 *useAdapter = nullptr;
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
      Logger::Log(StringUtility::ConvertString(
          std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
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
    hr = D3D12CreateDevice(nullptr, featureLevels[i], IID_PPV_ARGS(&device));
    if (SUCCEEDED(hr)) {
      // 生成できたのでログ出力を行ってループを抜ける
      Logger::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
      break;
    }
  }
  // デバイスの生成がうまくいかなかったので起動できない
  assert(device != nullptr);

  Logger::Log("Complete create D3D12Device!!!\n");
}

/// =========================================
///  コマンド関連の生成
/// =========================================
void DirectXCommon::CreateCommandQueue() {
  HRESULT hr;

  D3D12_COMMAND_QUEUE_DESC queueDesc{};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

  // コマンドキューを生成する
  hr = device->CreateCommandQueue(&queueDesc,
                                  IID_PPV_ARGS(commandQueue.GetAddressOf()));
  // コマンドキューの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  // コマンドアロケータを生成する
  hr = device->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT,
      IID_PPV_ARGS(commandAllocator.GetAddressOf()));
  // コマンドアロケータの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));
  // コマンドリストを生成する
  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                 commandAllocator.Get(), nullptr,
                                 IID_PPV_ARGS(commandList.GetAddressOf()));
  // コマンドリストの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));
}

/// =========================================
/// スワップチェーンの生成
/// =========================================
void DirectXCommon::CreateSwapChain() {
  // スワップチェーンを生成する
  swapChainDesc.Width = WinApp::kClientWidth;
  swapChainDesc.Height = WinApp::kClientHeight;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 2;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  // DXGIファクトリーの生成
  HRESULT hr =
      CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)); // ←メンバ変数に直接代入
  assert(SUCCEEDED(hr));

  hr = dxgiFactory->CreateSwapChainForHwnd(
      commandQueue.Get(), winApp->GetHwnd(), &swapChainDesc, nullptr, nullptr,
      reinterpret_cast<IDXGISwapChain1 **>(swapChain.GetAddressOf()));
  assert(SUCCEEDED(hr));
}

/// =========================================
///  深度バッファの生成
/// =========================================

// DepthStencilTextureを作る
ID3D12Resource *CreateDepthStencilTextureResource(ID3D12Device *device,
                                                  int32_t width,
                                                  int32_t height) {
  // 生成するResourceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = width;        // Textureの幅
  resourceDesc.Height = height;      // Textureの高さ
  resourceDesc.MipLevels = 1;        // mipmapの数
  resourceDesc.DepthOrArraySize = 1; // 奥行きor配列Textureの配列
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

  // Resourceの生成
  ID3D12Resource *resource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties,                  // Heapの設定
      D3D12_HEAP_FLAG_NONE,             // Heapの特殊な設定。特になし。
      &resourceDesc,                    // Resourceの設定
      D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
      &depthClearValue,                 // Clear最適値
      IID_PPV_ARGS(&resource));
  assert(SUCCEEDED(hr));
  return resource;
}

void DirectXCommon::CreateDepthBuffer() {
  // DepthStencilTextureをウィンドウのサイズで作成
  ID3D12Resource *depthStencilResource = CreateDepthStencilTextureResource(
      device.Get(), WinApp::kClientWidth, WinApp::kClientHeight);

  // DSV用のヒープでデイスクリプタの数は１．DSVはShader内で触るものではないので、ShaderVisibleはfalse
  dsvDescriptorHeap =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

  // DSVの設定
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
  dsvDesc.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // Format.基本的にはResourceに合わせる
  dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  // DSVHeapの先頭にDSVをつくる
  device->CreateDepthStencilView(
      depthStencilResource, &dsvDesc,
      dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

/// =========================================
///  各種デスクリプタヒープの生成
/// =========================================
void DirectXCommon::CreateDescriptorHeapRTVDSV() {
  // RTV用のヒープでデイスクリプタの数は２。RTVはShader内で触るものではないので、ShaderVisibleはfasle
  rtvDescriptorHeap =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

  // SRV用のヒープでデイスクリプタの数は128。SRVはShader内で触るものなので、ShaderVIsibleはture
  srvDescriptorHeap =
      CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
}




Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                    UINT numDescriptors, bool shaderVisible) {
  // DescriptorHeapの作成関数
  ID3D12DescriptorHeap *CreateDescriptorHeap(
      ID3D12Device * deveice, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
      UINT numDescriptors, bool shaderVisible);
  {
    ID3D12DescriptorHeap *descriptorHeap = nullptr;
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
  return Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>();
}

/// =========================================
///  レンダーターゲットビューの初期化
/// =========================================

void DirectXCommon::CreateRenderTargetViews() {

  HRESULT hr;

  // スワップチェーンからリソースを引っ張ってくる
  hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
  assert(SUCCEEDED(hr));
  hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
  assert(SUCCEEDED(hr));

  // RTV用の設定

  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
  rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

  // ディスクリプタの先端を取得する
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
      rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

  // RTVハンドルの要素数を2個に変更する
  UINT incrementSize =
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  // 　裏表の2つ文
  for (uint32_t i = 0; i < 2; ++i) {
    D3D12_CPU_DESCRIPTOR_HANDLE currentHandle = rtvHandle;
    currentHandle.ptr += incrementSize * i;

    device->CreateRenderTargetView(swapChainResources[i].Get(), &rtvDesc,
                                   currentHandle);
  }
}

// 指定した番号の CPU デスクリプタハンドルを取得
D3D12_CPU_DESCRIPTOR_HANDLE
DirectXCommon::GetCPUDescriptorHandle(
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index) {
  D3D12_CPU_DESCRIPTOR_HANDLE handle =
      descriptorHeap->GetCPUDescriptorHandleForHeapStart();

  handle.ptr += descriptorSize * index;
  return handle;
}

// 指定した番号の GPU デスクリプタハンドルを取得
D3D12_GPU_DESCRIPTOR_HANDLE
DirectXCommon::GetGPUDescriptorHandle(
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> &descriptorHeap,
    uint32_t descriptorSize, uint32_t index) {
  D3D12_GPU_DESCRIPTOR_HANDLE handle =
      descriptorHeap->GetGPUDescriptorHandleForHeapStart();

  handle.ptr += descriptorSize * index;
  return handle;
}

// SRV用 CPU デスクリプタ取得
D3D12_CPU_DESCRIPTOR_HANDLE
DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index) {
  return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

// SRV用 GPU デスクリプタ取得
D3D12_GPU_DESCRIPTOR_HANDLE
DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index) {
  return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

/// =========================================
///  深度ステンシルビューの初期化
/// =========================================
void DirectXCommon::CreateDepthStencilView() {
  // DSV用の設定

  // DSVをデスクリプタヒープの先頭に作る
}

/// =========================================
///  フェンスの生成
/// =========================================
void DirectXCommon::CreateFence() {
  HRESULT hr;

  // 初期化θでFenceを作る
  hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(fence.GetAddressOf()));

  // FenceのSignalを待つためのイベントを作成する
  fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  assert(fenceEvent);
}

/// =========================================
///  ビューポートの初期化
/// =========================================
void DirectXCommon::InitializeViewport() {

  // ビューボート
  D3D12_VIEWPORT viewport{};
  // クライアント領域のサイズと一緒にして画面全体に表示
  viewport.Width = WinApp::kClientWidth;
  viewport.Height = WinApp::kClientHeight;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
}

/// =========================================
///  シザリング矩形の生成
/// =========================================
void DirectXCommon::InitializeScissorRect() {

  // シザー矩形
  D3D12_RECT scissorRect{};
  // 基本的にビューボートと同じ矩形が構成されるようにする
  scissorRect.left = 0;
  scissorRect.right = WinApp::kClientWidth;
  scissorRect.top = 0;
  scissorRect.bottom = WinApp::kClientHeight;
}

/// =========================================
///  DXCコンパイラの生成
/// =========================================
void DirectXCommon::CreateDXCCompiler() {

  HRESULT hr;

  // dxcCompilerを初期化
  IDxcUtils *dxcUtils = nullptr;
  IDxcCompiler3 *dxcCompiler = nullptr;
  hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
  assert(SUCCEEDED(hr));
  hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
  assert(SUCCEEDED(hr));

  // 現時点でincludeはしないが、includeに対応するための設定を行っておく
  IDxcIncludeHandler *includeHandler = nullptr;
  hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
  assert(SUCCEEDED(hr));
}

/// =========================================
///  ImGuiの初期化
/// =========================================

// スワップチェーン設定
DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
void DirectXCommon::InitializeImGui() {
  // ImGuiの初期化。詳細はさして重要ではないので解説は省略する。
  // こういうもんである
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplWin32_Init(winApp->GetHwnd());
  ImGui_ImplDX12_Init(device.Get(), swapChainDesc.BufferCount,
                      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, srvDescriptorHeap.Get(),
                      srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                      srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void DirectXCommon::PreDraw() {
  // バックバッファの番号取得
  UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
  // リソースバリアで書き込み可能に変更
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  commandList->ResourceBarrier(1, &barrier);
  // 描画先のRTVとDSVを指定する
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCPUDescriptorHandle(
      rtvDescriptorHeap,
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV),
      backBufferIndex);

  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
      dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

  // commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
  // 画面全体の色をクリア
  commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
  float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  // 画面全体の深度をクリア
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0,
                                     0, nullptr);
  // SRV用のデスクリプタヒープを指定する
  ID3D12DescriptorHeap *descriptorHeaps[] = {srvDescriptorHeap.Get()};
  commandList->SetDescriptorHeaps(1, descriptorHeaps);
  // ビューポート領域の設定
  commandList->RSSetViewports(1, &viewport);
  // シザー矩形の設定
  commandList->RSSetScissorRects(1, &scissorRect);
}

void DirectXCommon::PostDraw() {
  // バックバッファの番号取得
  UINT bbIndex = swapChain->GetCurrentBackBufferIndex();

  // リソースバリアで表示状態に変更
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = swapChainResources[bbIndex].Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  commandList->ResourceBarrier(1, &barrier);
  // グラフィックコマンドをクローズ
  commandList->Close();
  // GPUコマンドの実行
  ID3D12CommandList *cmdLists[] = {commandList.Get()};
  commandQueue->ExecuteCommandLists(1, cmdLists);
  // GPU画面の交換を通知
  swapChain->Present(1, 0);
  // Fenceの値を更新
  fenceVal++;
  // コマンドキューにシグナルを送る
  commandQueue->Signal(fence.Get(), fenceVal);
  // コマンド完了待ち
  if (fence->GetCompletedValue() < fenceVal) {
    fence->SetEventOnCompletion(fenceVal, fenceEvent);
    WaitForSingleObject(fenceEvent, INFINITE);
  }
  // コマンドアロケータのリセット
  commandAllocator->Reset();
  // コマンドリストのリセット
  commandList->Reset(commandAllocator.Get(), nullptr);
}



Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
    // CompilerするShaderファイルへのパス
    const std::wstring &filePath,
    // Compileに使用するProfile
    const wchar_t *profile,
    // 初期化で生成したものを３つ
    IDxcUtils *dxcUtils, IDxcCompiler3 *dxcCompiler,
    IDxcIncludeHandler *includeHandler) {
  // これからシェーダをコンバイルｓる旨をログに出す
        logger::log(stringutility::convertstring(std::format(l" begin
 compileshader,path:{},profile{}\n", filepath, profile)));
	//hlslファイルを読む
	idxcblobencoding* shadersource = nullptr;
	hresult hr = dxcutils->loadfile(filepath.c_str(), nullptr,
&shadersource);
	//読めなかったら止める
	assert(succeeded(hr));
	//読み込んだファイルの内容を設定する
	dxcbuffer shadersourcebuffer;
	shadersourcebuffer.ptr = shadersource->getbufferpointer();
	shadersourcebuffer.size = shadersource->getbuffersize();
	shadersourcebuffer.encoding =
 dxc_cp_utf8;//utf8の文字コードであることを通知 	lpcwstr arguments[] =
	{
	filePath.c_str(),//コンパイル対象のhlslファイル名
	L"-E",L"main",//エントリーポイントの指定。基本的にmain以外にはしない
	L"-T",profile,//shaderProfileの設定
	L"-Zi",L"-Qembed_debug",//デバッグ用の情報を埋め込む
	L"Od",//最適化を外しておく
	L"-Zpr",//メモリレイアウトは行優先
	};

	//実際にShaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile
	(
		&shaderSourceBuffer,//読み込んだファイル
		arguments,//コンパイルオプション
		_countof(arguments),//コンパイルオプションの数
		includeHandler,//includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)//コンパイル結果
	);
	//コンパイルエラーではなく<dxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));
	//警告・エラーが出てきたログに出して止める
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError),
 nullptr); 	if (shaderError != nullptr && shaderError->GetStringLength() !=
 0)
	{
    Logger::Log(shaderError->GetStringPointer());
    // 警告・エラーがダメ絶対
    assert(false);
	}

	//コンパイル結果から実行用のバイナリ部分を所得

	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob),
 nullptr); 	assert(SUCCEEDED(hr));
	//成功したログを出す
	Logger::Log(StringUtility::ConvertString(std::format(L"Compile
 Succeeded,path:{}, profile:{}\n", filePath, profile)));
	//もう使わないリソースを解放
	shaderSource->Release();
	shaderResult->Release();
	//実行用のバイナリを返却
	return shaderBlob;
}

 Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t
 sizeInBytes)
{
	//頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;//UploadHeapを使う
	//頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	//バッファリソース。テクスチャの場合はまた別の設定する
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width =
 sizeInBytes;//リリースのサイズ。今回はVector4を3頂点分
	//バッファの場合はこれからは１にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	//バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	//実際に頂点リソースを作る
	ID3D12Resource* vertexResource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties,
 D3D12_HEAP_FLAG_NONE, 		&vertexResourceDesc,
 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
 IID_PPV_ARGS(&vertexResource)); 	assert(SUCCEEDED(hr)); 	return
 vertexResource;
 }

  // ２DirectX12のTextureResourceを作る
 Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResourcee(ID3D12Device* device, const
 DirectX::TexMetadata& metadata)
{
	//１.metadataを基にResorceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);//Textureの幅
	resourceDesc.Height = UINT(metadata.height);//Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);//mipmapの数
	resourceDesc.DepthOrArraySize =
 UINT16(metadata.arraySize);//奥行きor配列Textureの配列数
 resourceDesc.Format = metadata.format;//TextureのFormat
 resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。１固定
 resourceDesc.Dimension =
 D3D12_RESOURCE_DIMENSION(metadata.dimension);//Textureの次元数。普段使っているのは２次元
	//２.利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;//細かい設定を行う
	heapProperties.CPUPageProperty =
 D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//WriteBackポリシーでCPUアクセス可能
	heapProperties.MemoryPoolPreference =
 D3D12_MEMORY_POOL_L0;//プロセッサの近くに配置
	//３.Resourceを生成する
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,//Heapの設定
		D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特になし
		&resourceDesc,//Resourceの設定
		D3D12_RESOURCE_STATE_GENERIC_READ,//初回のResourceState。Textureは基本読むだけ
		nullptr,//Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource));//作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
 }

 // ３TextureResourceにデータを転送する
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
                                     nullptr,     // 全領域へコピー
                                     img->pixels, // 元データアドレス
                                     UINT(img->rowPitch), // １ラインサイズ
                                     UINT(img->slicePitch) // １枚サイズ
         );
     assert(SUCCEEDED(hr));
   }
 }
