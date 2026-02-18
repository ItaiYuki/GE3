#include "TextureManager.h"
#include "DirectXCommon.h"
#include "StringUtility.h"

using namespace StringUtility;

TextureManager *TextureManager::instance = nullptr;

TextureManager *TextureManager::GetInstance() {
  if (instance == nullptr) {
    instance = new TextureManager;
  }
  return instance;
}

void TextureManager::Initialize() {
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

  // ミップマップ付きのデータを返す
  //return mipImages;
}