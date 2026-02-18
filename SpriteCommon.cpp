#include "SpriteCommon.h"
#include "DirectXCommon.h"

void SpriteCommon::Initialize(DirectXCommon *dxCommon) {
  // 引数で受け取ってメンバ変数に記録する
  dxCommon_ = dxCommon;

  CreateGraphicsPipelineState();
}

void SpriteCommon::SetupCommonDrawing() {
  // RootSignatureを設定。PSOに設定しているけど別途設定が必要
  dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
  dxCommon_->GetCommandList()->SetPipelineState(
      graphicsPipelineState.Get()); // PSOを設定
  // 形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばいいい
  dxCommon_->GetCommandList()->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SpriteCommon::CreateRootSignature() {
  HRESULT hr;

  // RootSignature作成
  D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
  descriptionRootSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  // DescriptorRange
  D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  descriptorRange[0].BaseShaderRegister = 0; // 0から始まる
  descriptorRange[0].NumDescriptors = 1;     // 数は１つ
  descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
  descriptorRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // offsetを自動計算

  // RootParameter作成。複数設定できるので配列。今回は結果１つのだけなので長さ１の配列
  D3D12_ROOT_PARAMETER rootParameters[3] = {};
  rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
  rootParameters[0].Descriptor.ShaderRegister = 0;
  rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameters[1].Descriptor.ShaderRegister = 0;
  rootParameters[2].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // CBVを使う
  rootParameters[2].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // VertexShaderで使う
  rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
  rootParameters[2].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRange); // Tableで利用する数

  descriptionRootSignature.pParameters =
      rootParameters; // ルートレートパラメータ配列へのポインタ
  descriptionRootSignature.NumParameters =
      _countof(rootParameters); // 配列の長さ

  // Samplerの設定
  D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
  staticSamplers[0].Filter =
      D3D12_FILTER_MIN_MAG_MIP_LINEAR; // パイリニアフィルタ
  staticSamplers[0].AddressU =
      D3D12_TEXTURE_ADDRESS_MODE_WRAP; // ０～１の範囲外をリピート
  staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
  staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMipmapを使う
  staticSamplers[0].ShaderRegister = 0; // レジスタ番号０を使う
  staticSamplers[0].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
  descriptionRootSignature.pStaticSamplers = staticSamplers;
  descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

  ////シリアライズしてバイナリにする
  Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
  Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
  hr = D3D12SerializeRootSignature(&descriptionRootSignature,
                                   D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob,
                                   &errorBlob);
  if (FAILED(hr)) {
    // Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
    assert(false);
  }

  hr = dxCommon_->GetDevice()->CreateRootSignature(
      0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
      IID_PPV_ARGS(&rootSignature));
  assert(SUCCEEDED(hr));
}

void SpriteCommon::CreateGraphicsPipelineState() {
  HRESULT hr;
  CreateRootSignature();

  // InputLayout
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
  inputElementDescs[0].SemanticName = "POSITION";
  inputElementDescs[0].SemanticIndex = 0;
  inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  inputElementDescs[1].SemanticName = "TEXCOORD";
  inputElementDescs[1].SemanticIndex = 0;
  inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
  inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

  D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
  inputLayoutDesc.pInputElementDescs = inputElementDescs;
  inputLayoutDesc.NumElements = _countof(inputElementDescs);

  // BlendStateの設定
  D3D12_BLEND_DESC blendDesc{};
  // すべての色要素を書き込む
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;

  // RasiterZerStateの設定
  D3D12_RASTERIZER_DESC rasterizeDesc{};
  // 裏面（時計周り）を表示しない
  rasterizeDesc.CullMode = D3D12_CULL_MODE_NONE;
  // 三角形の中を塗りつぶす
  rasterizeDesc.FillMode = D3D12_FILL_MODE_SOLID;
  // 裏面表示
  // rasterizeDesc.CullMode = D3D12_CULL_MODE_NONE;

  // shaderをコンバイルする
  Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob =
      dxCommon_->CompileShader(L"Object3d.VS.hlsl", L"vs_6_0");
  assert(vertexShaderBlob != nullptr);

  Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob =
      dxCommon_->CompileShader(L"Object3d.PS.hlsl", L"ps_6_0");
  assert(pixelShaderBlob != nullptr);

  // PSOを生成する//P38
  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelinStateDesc{};
  graphicsPipelinStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
  graphicsPipelinStateDesc.InputLayout = inputLayoutDesc;        // InputLayout
  graphicsPipelinStateDesc.VS = {
      vertexShaderBlob->GetBufferPointer(),
      vertexShaderBlob->GetBufferSize()}; // VertexShader
  graphicsPipelinStateDesc.PS = {
      pixelShaderBlob->GetBufferPointer(),
      pixelShaderBlob->GetBufferSize()};                    // PixelShader
  graphicsPipelinStateDesc.BlendState = blendDesc;          // BlendState
  graphicsPipelinStateDesc.RasterizerState = rasterizeDesc; // RaterizerState
  // 書き込むRTVの情報
  graphicsPipelinStateDesc.NumRenderTargets = 1;
  graphicsPipelinStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  // 利用するトボロジ（形状）のタイプ。三角形
  graphicsPipelinStateDesc.PrimitiveTopologyType =
      D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  // どのように画面に色を打ち込むかの設定（気にしなくて良い）
  graphicsPipelinStateDesc.SampleDesc.Count = 1;
  graphicsPipelinStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  // DepthstencilStateの設定
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  // Depthの機能を有効化する
  depthStencilDesc.DepthEnable = false;
  // 書き込みします
  depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
  // 比較関数はLessEqual。つまり、近ければ描画される
  depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  // DepthStencilの設定
  graphicsPipelinStateDesc.DepthStencilState = depthStencilDesc;
  graphicsPipelinStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
      &graphicsPipelinStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
  assert(SUCCEEDED(hr));
}