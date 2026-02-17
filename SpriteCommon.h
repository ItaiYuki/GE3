#pragma once
#include <d3d12.h>
#include <wrl.h>

class DirectXCommon;

class SpriteCommon

{
public: // メンバ関数
  // 初期化
  void Initialize(DirectXCommon *dxCommon);

  DirectXCommon *GetDxCommon() const { return dxCommon_; }

  // 共通描画設定
  void SetupCommonDrawing();

private:
  ////バイナリを元に生成
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

  ////実際に生成
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

  // ルードシグネチャの作成
  void CreateRootSignature();
  // グラフィックスパイプラインの生成
  void CreateGraphicsPipelineState();

  DirectXCommon *dxCommon_;
};