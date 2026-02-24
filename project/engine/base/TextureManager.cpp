#include "TextureManager.h"
#include "DirectXCommon.h"
#include "StringUtility.h"

using namespace StringUtility;

TextureManager *TextureManager::instance = nullptr;

// ImGuiで0番を使用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

TextureManager *TextureManager::GetInstance() {
  if (instance == nullptr) {
    instance = new TextureManager;
  }
  return instance;
}

void TextureManager::Initialize(DirectXCommon *dxCommon) {

  this->dxCommon = dxCommon;

  // SRVの数と同数
  textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

void TextureManager::Finalize() {
  delete instance;
  instance = nullptr;
}

void TextureManager::LoadTexture(const std::string &filePath) {

  // テクスチャファイルを読んでプログラムで扱えるようにする
  DirectX::ScratchImage image{};
  auto it = std::find_if(textureDatas.begin(), textureDatas.end(),
                         [&](TextureData &textureData) {
                           return textureData.filePath == filePath;
                         });
  if (it != textureDatas.end()) {
    // 読み込み済みなら早期return
    return;
  }

  // テクスチャ枚数上限チェック
  assert(textureDatas.size() + kSRVIndexTop < DirectXCommon::kMaxSRVCount);

  std::wstring filePathW = ConvertString(filePath);
  HRESULT hr = DirectX::LoadFromWICFile(
      filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_RGB, nullptr, image);
  assert(SUCCEEDED(hr));

  // ミップマップの作成
  DirectX::ScratchImage mipImages{};
  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                4, mipImages);
  assert(SUCCEEDED(hr));

  // テクスチャデータを追加
  textureDatas.resize(textureDatas.size() + 1);
  // 追加したテクスチャデータの参照を取得する
  TextureData &textureData = textureDatas.back();
  textureData.filePath = filePath;
  textureData.metadata = mipImages.GetMetadata();
  textureData.resource = dxCommon->CreateTextureResource(textureData.metadata);
  // テクスチャデータの要素数番号をSRVのインデックスを計算する
  uint32_t srvIndex =
      static_cast<uint32_t>(textureDatas.size() - 1) + kSRVIndexTop;

  textureData.srvHandleCPU = dxCommon->GetSRVCPUDescriptorHandle(srvIndex);
  textureData.srvHandleGPU = dxCommon->GetSRVGPUDescriptorHandle(srvIndex);
  // metaDataを基にSRVの設定
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = textureData.metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // ２Dテクスチャ
  srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels);

  // SRVの生成
  dxCommon->GetDevice()->CreateShaderResourceView(
      textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);

  dxCommon->UploadTextureData(textureData.resource, mipImages);
}

D3D12_GPU_DESCRIPTOR_HANDLE
TextureManager::GetSrvHandleGPU(uint32_t textureIndex) {

  // 範囲外指定違反チェック
  assert(textureIndex < textureDatas.size());

  TextureData &textureData = textureDatas[textureIndex];
  return textureData.srvHandleGPU;
}

const DirectX::TexMetadata &TextureManager::GetMetaData(uint32_t textureIndex) {

  // 範囲外指定違反チェック
  assert(textureIndex < textureDatas.size());

  TextureData &textureData = textureDatas[textureIndex];
  return textureData.metadata;
}

uint32_t
TextureManager::GetTextureIndexByFilePath(const std::string &filePath) {
  // 読み込み済みテクスチャを検索
  auto it = std::find_if(textureDatas.begin(), textureDatas.end(),
                         [&](TextureData &textureData) {
                           return textureData.filePath == filePath;
                         });
  if (it != textureDatas.end()) {
    // 読み込み済みなら要素番号を返す
    uint32_t textureIndex =
        static_cast<uint32_t>(std::distance(textureDatas.begin(), it));
    return textureIndex;
  }

  assert(0);
  return 0;
}
