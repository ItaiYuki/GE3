#pragma once
#include "DirectXCommon.h"
#include "Sprite.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <algorithm>
#include <cassert>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

class DirectXCommon;

class TextureManager {

public:
  // シングルトンインスタンスの取得
  static TextureManager *GetInstance();
  // 初期化
  void Initialize(DirectXCommon *dxCommon);
  // 終了
  void Finalize();
  // SRVインデックスの開始番号
  uint32_t GetTextureIndexByFilePath(const std::string &filePath);
  // テクスチャファイルの読み込み
  void LoadTexture(const std::string &filePath);

  // テクスチャ番号からGPUハンドルを取得
  D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

private:
  // SRVインデックスの開始信号
  static uint32_t kSRVIndexTop;

  DirectXCommon *dxCommon = nullptr;

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