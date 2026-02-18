#pragma once
#include <cmath>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

class SpriteCommon;
class Sprite {
public: // メンバ関数
  // 初期化
  void Initialize(SpriteCommon *spriteCommon);

  // 更新
  void Update();

  // 描画
  void Draw();

  struct Vector4 {
    float x;
    float y;
    float z;
    float w;
  };
  struct Vector3 {
    float x;
    float y;
    float z;
  };

  struct Vector2 {
    float x;
    float y;
  };

  struct Matrix4x4 {
    float m[4][4];
  };

  struct Transform {
    Vector3 scale;
    Vector3 rotate;
    Vector3 translate;
  };

  // 単位行列
  Matrix4x4 MakeIdentity4x4() {
    Matrix4x4 identity;
    identity.m[0][0] = 1.0f;
    identity.m[0][1] = 0.0f;
    identity.m[0][2] = 0.0f;
    identity.m[0][3] = 0.0f;
    identity.m[1][0] = 0.0f;
    identity.m[1][1] = 1.0f;
    identity.m[1][2] = 0.0f;
    identity.m[1][3] = 0.0f;
    identity.m[2][0] = 0.0f;
    identity.m[2][1] = 0.0f;
    identity.m[2][2] = 1.0f;
    identity.m[2][3] = 0.0f;
    identity.m[3][0] = 0.0f;
    identity.m[3][1] = 0.0f;
    identity.m[3][2] = 0.0f;
    identity.m[3][3] = 1.0f;
    return identity;
  }

  // X軸で回転
  Matrix4x4 MakeRotateXMatrix(float radian) {
    float cosTheta = std::cos(radian);
    float sinTheta = std::sin(radian);
    return {1.0f, 0.0f,      0.0f,     0.0f, 0.0f, cosTheta, sinTheta, 0.0f,
            0.0f, -sinTheta, cosTheta, 0.0f, 0.0f, 0.0f,     0.0f,     1.0f};
  }

  // Y軸で回転
  Matrix4x4 MakeRotateYMatrix(float radian) {
    float cosTheta = std::cos(radian);
    float sinTheta = std::sin(radian);
    return {cosTheta, 0.0f, -sinTheta, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            sinTheta, 0.0f, cosTheta,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  }

  // Z軸で回転
  Matrix4x4 MakeRotateZMatrix(float radian) {
    float cosTheta = std::cos(radian);
    float sinTheta = std::sin(radian);
    return {cosTheta, sinTheta, 0.0f, 0.0f, -sinTheta, cosTheta, 0.0f, 0.0f,
            0.0f,     0.0f,     1.0f, 0.0f, 0.0f,      0.0f,     0.0f, 1.0f};
  }

  // Affine変換
  Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate,
                             const Vector3 &translate) {
    Matrix4x4 result = Multiply(
        Multiply(MakeRotateXMatrix(rotate.x), MakeRotateYMatrix(rotate.y)),
        MakeRotateZMatrix(rotate.z));
    result.m[0][0] *= scale.x;
    result.m[0][1] *= scale.x;
    result.m[0][2] *= scale.x;

    result.m[1][0] *= scale.y;
    result.m[1][1] *= scale.y;
    result.m[1][2] *= scale.y;

    result.m[2][0] *= scale.z;
    result.m[2][1] *= scale.z;
    result.m[2][2] *= scale.z;

    result.m[3][0] = translate.x;
    result.m[3][1] = translate.y;
    result.m[3][2] = translate.z;
    return result;
  }

  Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                   float bottom, float nearClip,
                                   float farClip) {
    return {2.0f / (right - left),
            0.0f,
            0.0f,
            0.f,
            0.0f,
            2.0f / (top - bottom),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f / (farClip - nearClip),
            0.0f,
            (left + right) / (left - right),
            (top + bottom) / (bottom - top),
            (nearClip - farClip),
            1.0f};
  }

  // 4x4の掛け算
  Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2) {
    Matrix4x4 result;
    result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] +
                     m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0];
    result.m[0][1] = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] +
                     m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1];
    result.m[0][2] = m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] +
                     m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2];
    result.m[0][3] = m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] +
                     m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3];

    result.m[1][0] = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] +
                     m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0];
    result.m[1][1] = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] +
                     m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1];
    result.m[1][2] = m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] +
                     m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2];
    result.m[1][3] = m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] +
                     m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3];

    result.m[2][0] = m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] +
                     m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0];
    result.m[2][1] = m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] +
                     m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1];
    result.m[2][2] = m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] +
                     m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2];
    result.m[2][3] = m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] +
                     m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3];

    result.m[3][0] = m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] +
                     m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0];
    result.m[3][1] = m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] +
                     m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1];
    result.m[3][2] = m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] +
                     m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2];
    result.m[3][3] = m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] +
                     m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3];

    return result;
  }

  struct VertexData {
    Vector4 position;
    Vector2 texcoord;
  };

  struct Material {
    Vector4 color;
    /*  int32_t enableLighting;
      float padding[3];
      Matrix4x4 uvTransform;
      float shininess;*/
  };

  struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
  };

  // getter//
  const Vector2 &GetPosition() const { return position; }
  float GetRotation() const { return rotation; }
  const Vector4 GetColor() const { return materialData->color; }
  const Vector2 &GetSize() const { return size; }

  // setter//
  void SetPosition(const Vector2 &position) { this->position = position; }
  void SetRotation(float rotation) { this->rotation = rotation; }
  void SetColor(const Vector4 &color) { materialData->color = color; }
  void SetSize(const Vector2 &size) { this->size = size; }

private:
  SpriteCommon *spriteCommon_ = nullptr;

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
  Vector2 position = {0.0f, 0.0f};

  // 回転
  float rotation = 0.0f;

  // サイズ
  Vector2 size = {640.0f, 360.0f};
};