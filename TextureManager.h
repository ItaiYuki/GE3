#pragma once
#include "externals/DirectXTex/DirectXTex.h"
#include <algorithm>
#include <cassert>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

class TextureManager {

public:
  // シングルトンインスタンスの取得
  static TextureManager *GetInstance();
  // 初期化
  void Initialize();
  // 終了
  void Finalize();

  // テクスチャファイルの読み込み
  void LoadTexture(const std::string &filePath);

private:
  static TextureManager *instance;
  TextureManager() = default;
  ~TextureManager() = default;
  TextureManager(TextureManager &) = delete;
  TextureManager &operator=(TextureManager &) = delete;

  // テクスチャ1枚分のデータ
  struct TextureData {
    std::string filePath;
    DirectX::TexMetadata metadata;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
  };

  // テクスチャデータ
  std::vector<TextureData> textureDatas;
};