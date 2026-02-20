#pragma once
#include "SpriteCommon.h"
#include "Mymath.h"
#include <cmath>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class SpriteCommon;
class Sprite {
public: // メンバ関数
  // 初期化
  void Initialize(SpriteCommon *spriteCommon, std::string textureFilePath);

  // 更新
  void Update();

  // 描画
  void Draw();

  struct Transform {
    math::Vector3 scale;
    math::Vector3 rotate;
    math::Vector3 translate;
  };

  struct VertexData {
    math::Vector4 position;
    math::Vector2 texcoord;
  };

  struct Material {
    math::Vector4 color;
  };

  struct TransformationMatrix {
    math::Matrix4x4 WVP;
    math::Matrix4x4 World;
  };

  // getter//
  const math::Vector2 &GetPosition() const { return position; }
  float GetRotation() const { return rotation; }
  const math::Vector4 GetColor() const { return materialData->color; }
  const math::Vector2 &GetSize() const { return size; }
  const math::Vector2 &GetAnchorPoint() const { return anchorPoint; }
  const bool IsFlipX() const { return isFlipX_; }
  const bool IsFlipY() const { return isFlipY_; }
  const math::Vector2 &GetTextureLeftTop() const { return textureLeftTop; }
  const math::Vector2 &GetTextureSize() const { return textureSize; }

  // setter//
  void SetPosition(const math::Vector2 &position) { this->position = position; }
  void SetRotation(float rotation) { this->rotation = rotation; }
  void SetColor(const math::Vector4 &color) { materialData->color = color; }
  void SetAnchorPoint(const math::Vector2 &anchorPoint) {
    this->anchorPoint = anchorPoint;
  }
  void SetFlipX(bool isFlipX_) { this->isFlipX_ = isFlipX_; }
  void SetFlipY(bool isFlipY_) { this->isFlipY_ = isFlipY_; }
  void SetSize(const math::Vector2 &size) { this->size = size; }
  void SetTextureLeftTop(const math::Vector2 &textureLeftTop) {
    this->textureLeftTop = textureLeftTop;
  }
  void SetTextureSize(const math::Vector2 &textureSize) {
    this->textureSize = textureSize;
  }

private:
  SpriteCommon *spriteCommon_ = nullptr;

  // テクスチャサイズをイメージに合わせる
  void AbjustTextureSize();

  // テクスチャ番号
  uint32_t textureIndex = 0;

  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;

  // 頂点インデックス
  Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;

  // バッファリソース内のデータを指すポインタ
  VertexData *vertexData = nullptr;
  uint32_t *indexData = nullptr;
  // バッファリソースの使い道を補足するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
  D3D12_INDEX_BUFFER_VIEW indexBufferView;

  Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;

  Material *materialData = nullptr;

  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
  // バッファリソース内のデータを指すポインタ
  TransformationMatrix *transformationMatrixData = nullptr;

  Transform transform;

  D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU{};
  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU{};

  Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

  // 座標
  math::Vector2 position = {0.0f, 0.0f};

  // 回転
  float rotation = 0.0f;

  // サイズ
  math::Vector2 size = {640.0f, 360.0f};


  math::Vector2 anchorPoint = {0.0f, 0.0f};

  // 左右フリップ
  bool isFlipX_ = false;
  // 上下フリップ
  bool isFlipY_ = false;

  // テクスチャ左上座標
  math::Vector2 textureLeftTop = {0.0f, 0.0f};
  // テクスチャ切り出しサイズ
  math::Vector2 textureSize = {1200.0f, 1200.0f};
};