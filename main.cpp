#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include <Windows.h>
#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <format>
#include <string>
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")
#include "externals/DirectXTex/DirectXTex.h"
#include <fstream>
#include <sstream>
#include "Input.h"

/// ------------------------------- GE3
/// -------------------------------------------
// DirectInputインクルード
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

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

struct VertexData {
  Vector4 position;
  Vector2 texcoord;
};

struct MaterialData {
  std::string textureFilePath;
};

struct ModelData {
  std::vector<VertexData> vertices;
  MaterialData material;
};

#pragma region 行列

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

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                   float nearClip, float farClip) {
  float cotHalfFovV = 1.0f / std::tan(fovY / 2.0f);
  return {(cotHalfFovV / aspectRatio),
          0.0f,
          0.0f,
          0.0f,
          0.0f,
          cotHalfFovV,
          0.0f,
          0.0f,
          0.0f,
          0.0f,
          farClip / (farClip - nearClip),
          1.0f,
          0.0f,
          0.0f,
          -(nearClip * farClip) / (farClip - nearClip),
          0.0f};
}

Matrix4x4 Inverse(const Matrix4x4 &m) {
  float determinant = +m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3] +
                      m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1] +
                      m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2]

                      - m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1] -
                      m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3] -
                      m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2]

                      - m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3] -
                      m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1] -
                      m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2]

                      + m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1] +
                      m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3] +
                      m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2]

                      + m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3] +
                      m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1] +
                      m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2]

                      - m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1] -
                      m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3] -
                      m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2]

                      - m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0] -
                      m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0] -
                      m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0]

                      + m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0] +
                      m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0] +
                      m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0];

  Matrix4x4 result;
  float recpDeterminant = 1.0f / determinant;
  result.m[0][0] =
      (m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] +
       m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][3] * m.m[2][2] * m.m[3][1] -
       m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[1][1] * m.m[2][3] * m.m[3][2]) *
      recpDeterminant;
  result.m[0][1] =
      (-m.m[0][1] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[2][3] * m.m[3][1] -
       m.m[0][3] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[2][2] * m.m[3][1] +
       m.m[0][2] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[2][3] * m.m[3][2]) *
      recpDeterminant;
  result.m[0][2] =
      (m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1] +
       m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[3][1] -
       m.m[0][2] * m.m[1][1] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[3][2]) *
      recpDeterminant;
  result.m[0][3] =
      (-m.m[0][1] * m.m[1][2] * m.m[2][3] - m.m[0][2] * m.m[1][3] * m.m[2][1] -
       m.m[0][3] * m.m[1][1] * m.m[2][2] + m.m[0][3] * m.m[1][2] * m.m[2][1] +
       m.m[0][2] * m.m[1][1] * m.m[2][3] + m.m[0][1] * m.m[1][3] * m.m[2][2]) *
      recpDeterminant;

  result.m[1][0] =
      (-m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[1][2] * m.m[2][3] * m.m[3][0] -
       m.m[1][3] * m.m[2][0] * m.m[3][2] + m.m[1][3] * m.m[2][2] * m.m[3][0] +
       m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[1][0] * m.m[2][3] * m.m[3][2]) *
      recpDeterminant;
  result.m[1][1] =
      (m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0] +
       m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[2][2] * m.m[3][0] -
       m.m[0][2] * m.m[2][0] * m.m[3][3] - m.m[0][0] * m.m[2][3] * m.m[3][2]) *
      recpDeterminant;
  result.m[1][2] =
      (-m.m[0][0] * m.m[1][2] * m.m[3][3] - m.m[0][2] * m.m[1][3] * m.m[3][0] -
       m.m[0][3] * m.m[1][0] * m.m[3][2] + m.m[0][3] * m.m[1][2] * m.m[3][0] +
       m.m[0][2] * m.m[1][0] * m.m[3][3] + m.m[0][0] * m.m[1][3] * m.m[3][2]) *
      recpDeterminant;
  result.m[1][3] =
      (m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] +
       m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][3] * m.m[1][2] * m.m[2][0] -
       m.m[0][2] * m.m[1][0] * m.m[2][3] - m.m[0][0] * m.m[1][3] * m.m[2][2]) *
      recpDeterminant;

  result.m[2][0] =
      (m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] +
       m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][3] * m.m[2][1] * m.m[3][0] -
       m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[1][0] * m.m[2][3] * m.m[3][1]) *
      recpDeterminant;
  result.m[2][1] =
      (-m.m[0][0] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][3] * m.m[3][0] -
       m.m[0][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[2][1] * m.m[3][0] +
       m.m[0][1] * m.m[2][0] * m.m[3][3] + m.m[0][0] * m.m[2][3] * m.m[3][1]) *
      recpDeterminant;
  result.m[2][2] =
      (m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0] +
       m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][3] * m.m[1][1] * m.m[3][0] -
       m.m[0][1] * m.m[1][0] * m.m[3][3] - m.m[0][0] * m.m[1][3] * m.m[3][1]) *
      recpDeterminant;
  result.m[2][3] =
      (-m.m[0][0] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] -
       m.m[0][3] * m.m[1][0] * m.m[2][1] + m.m[0][3] * m.m[1][1] * m.m[2][0] +
       m.m[0][1] * m.m[1][0] * m.m[2][3] + m.m[0][0] * m.m[1][3] * m.m[2][1]) *
      recpDeterminant;

  result.m[3][0] =
      (-m.m[1][0] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][2] * m.m[3][0] -
       m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[1][2] * m.m[2][1] * m.m[3][0] +
       m.m[1][1] * m.m[2][0] * m.m[3][2] + m.m[1][0] * m.m[2][2] * m.m[3][1]) *
      recpDeterminant;
  result.m[3][1] =
      (m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0] +
       m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][2] * m.m[2][1] * m.m[3][0] -
       m.m[0][1] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][2] * m.m[3][1]) *
      recpDeterminant;
  result.m[3][2] =
      (-m.m[0][0] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[3][0] -
       m.m[0][2] * m.m[1][0] * m.m[3][1] + m.m[0][2] * m.m[1][1] * m.m[3][0] +
       m.m[0][1] * m.m[1][0] * m.m[3][2] + m.m[0][0] * m.m[1][2] * m.m[3][1]) *
      recpDeterminant;
  result.m[3][3] =
      (m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] +
       m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][2] * m.m[1][1] * m.m[2][0] -
       m.m[0][1] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][2] * m.m[2][1]) *
      recpDeterminant;

  return result;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                 float bottom, float nearClip, float farClip) {
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

#pragma endregion

std::wstring ConvertString(const std::string &str) {
  if (str.empty()) {
    return std::wstring();
  }
  auto sizeNeebed =
      MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]),
                          static_cast<int>(str.size()), NULL, 0);
  if (sizeNeebed == 0) {
    return std::wstring();
  }
  std::wstring result(sizeNeebed, 0);
  MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char *>(&str[0]),
                      static_cast<int>(str.size()), &result[0], sizeNeebed);
  return result;
}

//// １Textureデータを読む
// DirectX::ScratchImage LoadTexture(const std::string &filePath) {
//   // テクスチャファイルを読んでプログラムで扱えるようにする
//   DirectX::ScratchImage image{};
//   std::wstring filePathW = ConvertString(filePath);
//   HRESULT hr = DirectX::LoadFromWICFile(
//       filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_RGB, nullptr, image);
//   assert(SUCCEEDED(hr));
//
//   // ミップマップの作成
//   DirectX::ScratchImage mipImages{};
//   hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
//                                 image.GetMetadata(),
//                                 DirectX::TEX_FILTER_SRGB, 0, mipImages);
//   assert(SUCCEEDED(hr));
//
//   // ミップマップ付きのデータを返す
//   return mipImages;
// }

// ２DirectX12のTextureResourceを作る
ID3D12Resource *CreateTextureResource(ID3D12Device *device,
                                      const DirectX::TexMetadata &metadata) {
  // １.metadataを基にResorceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = UINT(metadata.width);           // Textureの幅
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
  ID3D12Resource *resource = nullptr;
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

// ３TextureResourceにデータを転送する
void UploadTextureData(ID3D12Resource *texture,
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
    /*assert(false && "assertのテストだよ");*/
  }
}

// MaterialData読み込み関数
MaterialData LoadMaterialTemplateFile(const std::string &directoryPath,
                                      const std::string &filename) {
  // １．中で必要となる変数の宣言
  MaterialData materialData; // 構築するMaterialData
  std::string line;          // ファイルから読んだ１行を格納するもの

  // ２．ファイルを開く
  std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
  assert(file.is_open()); // とりあえず開けなかったら止める

  // ３．実際にファイルを読み、MaterialDataを構築していく
  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier;

    // identifierに応じた処理
    if (identifier == "map_Kd") {
      std::string textureFilename;
      s >> textureFilename;
      // 連結してファイルパスにする
      materialData.textureFilePath = directoryPath + "/" + textureFilename;
    }
  }
  // ４．MaterialDataを返す
  return materialData;
}

// OBj読み込み関数
ModelData LoadObjFile(const std::string &directoryPath,
                      const std::string &filename) {
  // １、 中で必要となる変数の宣言
  ModelData modelData;            // 構築するModelData
  std::vector<Vector4> positions; // 位置
  std::vector<Vector3> normals;   // 法線
  std::vector<Vector2> texcoords; // テクスチャ座標
  std::string line;               // ファイルから読んで１行を格納するもの

  // ２、 ファイルを開く
  std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
  assert(file.is_open()); // とりあえず開けなかったら止める

  // ３、 実際にファイルを読み、ModelDataを構築していく
  while (std::getline(file, line)) {
    std::string identifier;
    std::istringstream s(line);
    s >> identifier; // 先頭の識別子を読む

    // identifierに応じた処理
    if (identifier == "v") {
      Vector4 position;
      s >> position.x >> position.y >> position.z;
      position.w = 1.0f;
      positions.push_back(position);
    } else if (identifier == "vt") {
      Vector2 texcoord;
      s >> texcoord.x >> texcoord.y;
      texcoords.push_back(texcoord);
    } else if (identifier == "vn") {
      Vector3 normal;
      s >> normal.x >> normal.y >> normal.z;
      normals.push_back(normal);
    } else if (identifier == "f") {
      VertexData triangle[3];
      // 面は三角形限定。その他は未対応
      for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
        std::string vertexDefinition;
        s >> vertexDefinition;
        // 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してindexを取得する
        std::istringstream v(vertexDefinition);
        uint32_t elementIndices[3];
        for (int32_t element = 0; element < 3; ++element) {
          std::string index;
          std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
          elementIndices[element] = std::stoi(index);
        }
        // 要素へのIndexから、実際の要素の値を所得して、頂点を構築する
        Vector4 position = positions[elementIndices[0] - 1];
        position.x *= -1.0f;
        Vector2 texcoord = texcoords[elementIndices[1] - 1];
        texcoord.y = 1.0f - texcoord.y;

        Vector3 normal = normals[elementIndices[2] - 1];
        // normal.x *= -1.0f;
        // VertexData vertex = { position,texcoord};
        // VertexData vertex = { position,texcoord,normal };
        // modelData.vertices.push_back(vertex);
        triangle[faceVertex] = {position, texcoord};
        // triangle[faceVertex] = { position,texcoord,normal };
      }
      // 頂点を逆順で登録することで、回り順を逆にする
      modelData.vertices.push_back(triangle[2]);
      modelData.vertices.push_back(triangle[1]);
      modelData.vertices.push_back(triangle[0]);
    } else if (identifier == "mtllib") {
      // materialTemplateLibraryファイルの名前を取得する
      std::string materialFilename;
      s >> materialFilename;
      // 基本的にobjファイルと同一階層にmtlは存在させるので、デイレクトリ名とファイル名を渡す
      modelData.material =
          LoadMaterialTemplateFile(directoryPath, materialFilename);
    }
  }

  // ４，ModelDataを返す
  return modelData;
}

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

  if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
    return true;
  }
  // メッセージに応じてゲーム固有の処理を行う
  switch (msg) {
    // ウィンドウが破棄された
  case WM_DESTROY:
    // osに応じて、アプリの終了を伝える
    PostQuitMessage(0);
    return 0;
  }
  // 標準のメッセージ処理を行う
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string &message) { OutputDebugStringA(message.c_str()); }

// wstring->string
std::string ConvertString(const std::wstring &str) {
  if (str.empty()) {
    return std::string();
  }
  auto sizeNeebed =
      WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
                          NULL, 0, NULL, NULL);
  if (sizeNeebed == 0) {
    return std::string();
  }
  std::string result(sizeNeebed, 0);
  WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
                      result.data(), sizeNeebed, NULL, NULL);
  return result;
}

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

IDxcBlob *CompileShader(
    // CompilerするShaderファイルへのパス
    const std::wstring &filePath,
    // Compilerに使用するProfile
    const wchar_t *profile,
    // 初期化で生成したものを3つ
    IDxcUtils *dxcUtils, IDxcCompiler3 *dxcCompiler,
    IDxcIncludeHandler *includeHandler) {
  // これからシェーダーをコンパイルする旨をログに出す
  Log(ConvertString(
      std::format(L"Begin CompileShader,path:{}\n", filePath, profile)));
  // hlslファイルを読む
  IDxcBlobEncoding *shaderSource = nullptr;
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
      L"main", // エントリーポイントの指定。基本的にmain意外にはしない
      L"-T",
      profile, // ShaderProfileの設定
      L"-Zi",
      L"-Qembed_debug", // デバッグ用の情報を埋め込む
      L"-Od",           // 最適化を外しておく
      L"-Zpr"           // メモリアウトは行優先
  };
  // 実際にShaderをコンパイルする
  IDxcResult *shaderResult = nullptr;
  hr = dxcCompiler->Compile(&shaderSourceBuffer, // 読み込んだファイル
                            arguments,           // コンパイルオプション
                            _countof(arguments), // コンパイルオプションの数
                            includeHandler,      // includeが含まれた諸々
                            IID_PPV_ARGS(&shaderResult) // コンパイル結果
  );
  // コンパイルエラーではなくdxcが起動できないなどの致命的な状況
  assert(SUCCEEDED(hr));

  // 警告・エラーが出てたらログを出して止める
  IDxcBlobUtf8 *shaderError = nullptr;
  shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
  if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
    Log(shaderError->GetStringPointer());
    // 警告・エラーダメ絶対
    assert(false);
  }

  // コンパイル結果から実行用のバイナリ部分を取得
  IDxcBlob *shaderBlob = nullptr;
  hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob),
                               nullptr);
  assert(SUCCEEDED(hr));
  //  成功したログを出す
  Log(ConvertString(std::format(L"Compile Succeeded,path:{}, profile:{}\n",
                                filePath, profile)));
  // もう使わないリソースを開放
  shaderSource->Release();
  shaderResult->Release();

  // 実行用のバイナリを返却
  return shaderBlob;
}

ID3D12Resource *CreateBufferResource(ID3D12Device *device, size_t sizeInBytes) {
  // 頂点リソース用のヒープの設定
  D3D12_HEAP_PROPERTIES uploadHeapProperties{};
  uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // uploadHeapを使う
  // 頂点リソースの設定
  D3D12_RESOURCE_DESC vertexResourceDesc{};
  // バッファリソース。テクスチャの場合はまたは別の設定をする
  vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  vertexResourceDesc.Width =
      sizeInBytes; // リソースのサイズ。もらったサイズを使用
                   // バッファの場合は1にする決まり
  vertexResourceDesc.Height = 1;
  vertexResourceDesc.DepthOrArraySize = 1;
  vertexResourceDesc.MipLevels = 1;
  vertexResourceDesc.SampleDesc.Count = 1;
  // バッファの場合はこれにする決まり
  vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  // 実際に頂点リソースを作る
  ID3D12Resource *vertexResource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&vertexResource));
  assert(SUCCEEDED(hr));

  return vertexResource;
};

ID3D12DescriptorHeap *CreateDescriptorHeap(ID3D12Device *device,
                                           D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                           UINT numDescriptors,
                                           bool shaderVisible) {
  // ディスクリプタヒープの生成
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

DirectX::ScratchImage LoadTexture(const std::string &filePath) {
  // テクスチャファイルを読んでプログラムで扱えるようにする
  DirectX::ScratchImage image{};
  std::wstring filePathw = ConvertString(filePath);
  HRESULT hr = DirectX::LoadFromWICFile(
      filePathw.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
  assert(SUCCEEDED(hr));

  // ミニマップの作成
  DirectX::ScratchImage mipImages{};
  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                0, mipImages);
  assert(SUCCEEDED(hr));

  // ミニマップデータ付きのデータを返す
  return mipImages;
}

ID3D12Resource *CreateTextureresource(ID3D12Device *device,
                                      const DirectX::TexMetadata &metadata) {
  // metadataを基にResourceの設定
  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Width = UINT(metadata.width);           // Textureの幅
  resourceDesc.Height = UINT(metadata.height);         // Textureの高さ
  resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipLevelsの数
  resourceDesc.DepthOrArraySize =
      UINT16(metadata.arraySize);        // 奥行き or 配列Textureの配列数
  resourceDesc.Format = metadata.format; // TextureのFormat
  resourceDesc.SampleDesc.Count = 1;     // サンプリングカウント。1固定。
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(
      metadata.dimension); // Textureの次元数。普段使っているのは2次元
  // 利用するHEAPの設定。非常に特殊な運用。02_04exで一般的なケース版がある
  D3D12_HEAP_PROPERTIES heapProperties{};
  heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM; // 細かい設定を行う
  heapProperties.CPUPageProperty =
      D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // writeBackポリシーでCPUアクセス可能
  heapProperties.MemoryPoolPreference =
      D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置
  // Resourecの生成
  ID3D12Resource *resource = nullptr;
  HRESULT hr = device->CreateCommittedResource(
      &heapProperties,                   // Heapの設定
      D3D12_HEAP_FLAG_NONE,              // Heapの特殊な設定。特になし。
      &resourceDesc,                     // Resourceの設定
      D3D12_RESOURCE_STATE_GENERIC_READ, // 初回のResourceState。Textureは基本読むだけ
      nullptr,                           // Clear最適値。使わないのでnullptr
      IID_PPV_ARGS(&resource));          // 作成するResourceポインタへのポインタ
  assert(SUCCEEDED(hr));
  return resource;
}

// ウィンメイン
// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  CoInitializeEx(0, COINIT_MULTITHREADED);

  // 出力ウィンドウへの文字入力
  OutputDebugStringA("Hell,DirectX!\n");

  WNDCLASS wc{};
  // ウィンドウプロシージャ
  wc.lpfnWndProc = WindowProc;
  // ウィンドウクラス名
  wc.lpszClassName = L"CG2WindowClass";
  // インスタンスハンドル
  wc.hInstance = GetModuleHandle(nullptr);
  // カーソル
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

  // ウィンドウクラスを登録する
  RegisterClass(&wc);

  // クライアント領域のサイズ
  const int32_t kClientWidth = 1280;
  const int32_t kClientHeight = 720;

  // ウィンドウサイズを表す構造体にクライアント領域を入れる
  RECT wrc = {0, 0, kClientWidth, kClientHeight};

  // クライアント領域をもとに実際のサイズにwrcを変更してもらう
  AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

  // ウィンドウの生成
  HWND hwnd = CreateWindow(wc.lpszClassName,     // 利用するクラス名
                           L"CG2",               // タイトルバーの文字
                           WS_OVERLAPPEDWINDOW,  // よく見るウィンドウスタイル
                           CW_USEDEFAULT,        // 表示X座標(Windowsに任せる)
                           CW_USEDEFAULT,        // 表示Y座標(WindowsOSに任せる)
                           wrc.right - wrc.left, // ウィンドウ横幅
                           wrc.bottom - wrc.top, // ウィンドウ縦幅
                           nullptr,              // 親ウィンドウハンドル
                           nullptr,              // メニューハンドル
                           wc.hInstance,         // インスタンスハンドル
                           nullptr               // オプション
  );

  // ウィンドウを表示する
  ShowWindow(hwnd, SW_SHOW);

#ifdef _DEBUG
  ID3D12Debug1 *debugController = nullptr;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    debugController->EnableDebugLayer();

    debugController->SetEnableGPUBasedValidation(TRUE);
  }
#endif

  // DXGIファクトリーの生成
  IDXGIFactory7 *dxgiFactory = nullptr;
  HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
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
      Log(ConvertString(
          std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
      break;
    }
    useAdapter = nullptr;
  }
  // 適切なアダプタが見つからなかったので起動できない
  assert(useAdapter != nullptr);

  ID3D12Device *device = nullptr;
  // 機能レベルとログ出力用の文字列
  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0};
  const char *featureLevelStrings[] = {"12.2", "12.1", "12.0"};
  for (size_t i = 0; i < _countof(featureLevels); ++i) {
    // 採用したアダプターでデバイスを生成
    hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
    if (SUCCEEDED(hr)) {
      // 生成できたのでログ出力を行ってループを抜ける
      Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
      break;
    }
  }
  // デバイスの生成がうまくいかなかったので起動できない
  assert(device != nullptr);
  Log("Complete create D3D12Device!!!\n");

  // コマンドキューを生成する
  ID3D12CommandQueue *commandQueue = nullptr;
  D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
  hr = device->CreateCommandQueue(&commandQueueDesc,
                                  IID_PPV_ARGS(&commandQueue));
  // コマンドキューの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  // コマンドアロケータを生成する
  ID3D12CommandAllocator *commandAllocator = nullptr;
  hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      IID_PPV_ARGS(&commandAllocator));
  // コマンドキューの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  // コマンドリストを生成する
  ID3D12GraphicsCommandList *commandList = nullptr;
  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                 commandAllocator, nullptr,
                                 IID_PPV_ARGS(&commandList));
  // コマンドキューの生成がうまくいかなかったので起動できない
  assert(SUCCEEDED(hr));

  // スワップチェーンを生成する
  IDXGISwapChain4 *swapChain = nullptr;
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
  swapChainDesc.Width = kClientWidth;
  swapChainDesc.Height = kClientHeight;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 2;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  hr = dxgiFactory->CreateSwapChainForHwnd(
      commandQueue, hwnd, &swapChainDesc, nullptr, nullptr,
      reinterpret_cast<IDXGISwapChain1 **>(&swapChain));
  assert(SUCCEEDED(hr));

  // RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisibleはfalse
  ID3D12DescriptorHeap *rtvDescriptorHeap =
      CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

  // SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
  ID3D12DescriptorHeap *srvDescriptorHeap = CreateDescriptorHeap(
      device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

  ID3D12Resource *swapChainResources[2] = {nullptr};
  hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));

  assert(SUCCEEDED(hr));

  hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));

  assert(SUCCEEDED(hr));

  D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
  rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

  D3D12_CPU_DESCRIPTOR_HANDLE rtvStarHandle =
      rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

  rtvHandles[0] = rtvStarHandle;

  device->CreateRenderTargetView(swapChainResources[0], &rtvDesc,
                                 rtvHandles[0]);

  rtvHandles[1].ptr =
      rtvHandles[0].ptr +
      device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  device->CreateRenderTargetView(swapChainResources[1], &rtvDesc,
                                 rtvHandles[1]);

  // 初期化θでFenceを作る
  ID3D12Fence *fence = nullptr;
  uint64_t fenceValue = 0;
  hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE,
                           IID_PPV_ARGS(&fence));
  assert(SUCCEEDED(hr));

  // FenceのSignalを待つためのイベントを作成する
  HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  assert(fenceEvent != nullptr);

  // dxCompilerを初期化
  IDxcUtils *dxcUtils = nullptr;
  IDxcCompiler3 *dxcCompiler = nullptr;
  hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
  assert(SUCCEEDED(hr));
  hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
  assert(SUCCEEDED(hr));

  // 現時点でincludeはしないが、includeに対応するための設定を行っておく
  IDxcIncludeHandler *includehandler = nullptr;
  hr = dxcUtils->CreateDefaultIncludeHandler(&includehandler);
  assert(SUCCEEDED(hr));

  // RootSignature作成
  D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
  descriptionRootSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  // DescriptorRange
  D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  descriptorRange[0].BaseShaderRegister = 0;                      // 0から始まる
  descriptorRange[0].NumDescriptors = 1;                          // 数は1つ
  descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
  descriptorRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

  // RootSignature作成。複数設定できるので配列。今回は結果1つだけなので長さ1の配列
  D3D12_ROOT_PARAMETER rootParameters[3] = {};
  rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
  rootParameters[0].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL;               // PixelShaderで使う
  rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド

  rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
  rootParameters[1].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_VERTEX;              // VertexShaderで使う
  rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0を使う

  rootParameters[2].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
  rootParameters[2].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
  rootParameters[2].DescriptorTable.pDescriptorRanges =
      descriptorRange; // Tableの中身の配列を指定
  rootParameters[2].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRange); // Tableで利用する数

  descriptionRootSignature.pParameters =
      rootParameters; // ルートレートパラメータ配列へのポインタ
  descriptionRootSignature.NumParameters =
      _countof(rootParameters); // 配列の長さ

  // Samplerの設定
  D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
  staticSamplers[0].Filter =
      D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
  staticSamplers[0].AddressU =
      D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0～1の範囲外をリピート
  staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
  staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMipmapを使う
  staticSamplers[0].ShaderRegister = 0;         // レジスタ番号0を使う
  staticSamplers[0].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // PixxelShaderで使う
  descriptionRootSignature.pStaticSamplers = staticSamplers;
  descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

  // シリアライズしてバイナリにする
  ID3DBlob *signatureBlob = nullptr;
  ID3DBlob *errorBlob = nullptr;
  hr = D3D12SerializeRootSignature(&descriptionRootSignature,
                                   D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob,
                                   &errorBlob);
  if (FAILED(hr)) {
    Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
    assert(false);
  }
  // バイナリを元に生成
  ID3D12RootSignature *rootSignature = nullptr;
  hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                   signatureBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&rootSignature));
  assert(SUCCEEDED(hr));

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

  // 全ての色要素を書き込む
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;

  // RasiterzerStsteの設定
  D3D12_RASTERIZER_DESC rasteriZerDesc{};
  // 裏面（時計回り）を表示しない
  rasteriZerDesc.CullMode = D3D12_CULL_MODE_BACK;
  /*rasteriZerDesc.CullMode = D3D12_CULL_MODE_NONE;*/
  // 三角形の中を塗りつぶす
  rasteriZerDesc.FillMode = D3D12_FILL_MODE_SOLID;

  // Shaderをコンパイルする
  IDxcBlob *vertexShaderBlob =
      CompileShader(L"resources/shaders/Object3D.VS.hlsl", L"vs_6_0", dxcUtils,
                    dxcCompiler, includehandler);
  assert(vertexShaderBlob != nullptr);

  IDxcBlob *pixelShaderBlob =
      CompileShader(L"resources/shaders/Object3D.PS.hlsl", L"ps_6_0", dxcUtils,
                    dxcCompiler, includehandler);
  assert(pixelShaderBlob != nullptr);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
  graphicsPipelineStateDesc.pRootSignature = rootSignature; // RootSignature
  graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;  // InputLayout
  graphicsPipelineStateDesc.VS = {
      vertexShaderBlob->GetBufferPointer(),
      vertexShaderBlob->GetBufferSize()}; // vertexShader
  graphicsPipelineStateDesc.PS = {
      pixelShaderBlob->GetBufferPointer(),
      pixelShaderBlob->GetBufferSize()};                      // pixelShader
  graphicsPipelineStateDesc.BlendState = blendDesc;           // BlendState
  graphicsPipelineStateDesc.RasterizerState = rasteriZerDesc; // RasterizerState
  graphicsPipelineStateDesc.NumRenderTargets = 1;
  graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  // 利用するトポロジ（形状）のタイプ。三角形
  graphicsPipelineStateDesc.PrimitiveTopologyType =
      D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  // どのように画面に色を打ち込むかの設定（気にしなくていい）
  graphicsPipelineStateDesc.SampleDesc.Count = 1;
  graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  // DepthstencilStateの設定
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  // Depthの機能を有効化する
  depthStencilDesc.DepthEnable = true;
  // 書き込みします
  depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  // 比較関数はLessEqual。つまり、近ければ描画される
  depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  // DepthStencilの設定
  graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
  graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  // 実際に生成
  ID3D12PipelineState *graphicsPipelineState = nullptr;
  hr = device->CreateGraphicsPipelineState(
      &graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
  assert(SUCCEEDED(hr));

  /// ---------------------------------- GE3 ------------------------
  // Direct Input 初期化
  IDirectInput8 *directInput = nullptr;
  hr = DirectInput8Create(wc.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
                          (void **)&directInput, nullptr);
  assert(SUCCEEDED(hr));

  // キーボードデバイスの生成
  //IDirectInputDevice8 *keyboard = nullptr;
  //hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
  //assert(SUCCEEDED(hr));

  //// 入力データ形式のセット
  //hr = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
  //assert(SUCCEEDED(hr));

  // 排他制御レベルのセット(どんな時に入力を受け取るか)
 /* hr = keyboard->SetCooperativeLevel(
      hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
  assert(SUCCEEDED(hr));*/

  // ポインタ
  Input *input = nullptr;

  // 入力の初期化
  input = new Input();
  input->Initialize(wc.hInstance,hwnd);

// 入力の更新
  input->Update();

  // 入力解放
  delete input;

  

  // 三角形２個
  // ID3D12Resource *vertexResource =
  //    CreateBufferResource(device, sizeof(VertexData) * 6);

  //// 頂点バッファビューを作成する
  // D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
  //// リソースの先頭のアドレスから使う
  // vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
  //// 使用するリソースのサイズは頂点3つ分のサイズ
  // vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
  //// 1頂点当たりのサイズ
  // vertexBufferView.StrideInBytes = sizeof(VertexData);

  //// 頂点リソースにデータを書き込む
  // VertexData *vertexData = nullptr;
  //// 書き込むためのアドレスを取得
  // vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
  //// 左下
  // vertexData[0].position = {-0.5f, -0.5f, 0.0f, 1.0f};
  // vertexData[0].texcoord = {0.0f, 1.0f};
  //// 上
  // vertexData[1].position = {0.0f, 0.5f, 0.0f, 1.0f};
  // vertexData[1].texcoord = {0.5f, 0.0f};
  //// 右下
  // vertexData[2].position = {0.5f, -0.5f, 0.0f, 1.0f};
  // vertexData[2].texcoord = {1.0f, 1.0f};

  //// 左下2
  // vertexData[3].position = {-0.5f, -0.5f, 0.5f, 1.0f};
  // vertexData[3].texcoord = {0.0f, 1.0f};
  //// 上2
  // vertexData[4].position = {0.0f, 0.0f, 0.0f, 1.0f};
  // vertexData[4].texcoord = {0.5f, 0.0f};
  //// 右下2
  // vertexData[5].position = {0.5f, -0.5f, -0.5f, 1.0f};
  // vertexData[5].texcoord = {1.0f, 1.0f};

  // モデル読み込み
  // ModelData modelData = LoadObjFile("resources", "plane.obj");
  ModelData modelData = LoadObjFile("resources", "axis.obj");

  // 頂点リソースを作る
  ID3D12Resource *vertexResource = CreateBufferResource(
      device, sizeof(VertexData) * modelData.vertices.size());
  // 頂点バッファビューを作成する
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
  vertexBufferView.BufferLocation =
      vertexResource
          ->GetGPUVirtualAddress(); // リソースの先頭のアドレスから使う
  vertexBufferView.SizeInBytes =
      UINT(sizeof(VertexData) *
           modelData.vertices.size()); // 使用するリソースのサイズは頂点のサイズ
  vertexBufferView.StrideInBytes = sizeof(VertexData); // １頂点あたりのサイズ

  // 頂点リソースにデータを書き込む
  VertexData *vertexData = nullptr;
  vertexResource->Map(
      0, nullptr,
      reinterpret_cast<void **>(&vertexData)); // 書き込むためのアドレスを取得
  std::memcpy(vertexData, modelData.vertices.data(),
              sizeof(VertexData) * modelData.vertices.size());

  // ビューボート
  D3D12_VIEWPORT viewport{};
  // クライアント領域のサイズと一緒にして画面全体に表示
  viewport.Width = kClientWidth;
  viewport.Height = kClientHeight;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;

  // シザー矩形
  D3D12_RECT scissorRect{};
  // 基本的にビューボートと同じ矩形が構成されるようにする
  scissorRect.left = 0;
  scissorRect.right = kClientWidth;
  scissorRect.top = 0;
  scissorRect.bottom = kClientHeight;

  // マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
  ID3D12Resource *materialResource =
      CreateBufferResource(device, sizeof(Vector4));
  // マテリアルにデータを書き込む
  Vector4 *materialData = nullptr;
  // 書き込むためのアドレスを取得
  materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
  // 今回は赤を書き込んでみる
  *materialData = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

  // wvp用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
  ID3D12Resource *wvpResource = CreateBufferResource(device, sizeof(Matrix4x4));
  // データを書き込む
  Matrix4x4 *wvpData = nullptr;
  // 書き込むためのアドレスを取得
  wvpResource->Map(0, nullptr, reinterpret_cast<void **>(&wvpData));
  // 単位行列を書き込んでおく
  *wvpData = MakeIdentity4x4();

  Transform transform{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  Transform cameraTransform{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -5.0f}};

  // ImGuiの初期化。詳細はさして重要ではないので解説は省略する。
  // こういうもんである
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX12_Init(device, swapChainDesc.BufferCount, rtvDesc.Format,
                      srvDescriptorHeap,
                      srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                      srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

  // Textureを読んで転送する
  DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
  const DirectX::TexMetadata &metadata = mipImages.GetMetadata();
  ID3D12Resource *textureResource = CreateTextureresource(device, metadata);
  UploadTextureData(textureResource, mipImages);

  // metaDataを基にSRVの設定
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
  srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

  // SRVを作成するDescriptorHeapの場所を決める
  D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU =
      srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU =
      srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

  // 先頭はImGuiが使っているのでその次を使う
  textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  // SRVの生成
  device->CreateShaderResourceView(textureResource, &srvDesc,
                                   textureSrvHandleCPU);

  // DepthStencilTextureをウィンドウのサイズで作成
  ID3D12Resource *depthStencilResource =
      CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);

  // DSV用のヒープでデイスクリプタの数は１．DSVはShader内で触るものではないので、ShaderVisibleはfalse
  ID3D12DescriptorHeap *dsvDescriptorHeap =
      CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

  // DSVの設定
  D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
  dsvDesc.Format =
      DXGI_FORMAT_D24_UNORM_S8_UINT; // Format.基本的にはResourceに合わせる
  dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  // DSVHeapの先頭にDSVをつくる
  device->CreateDepthStencilView(
      depthStencilResource, &dsvDesc,
      dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

  // Sprite用の頂点リソースを作る
  ID3D12Resource *vertexResourceSprite =
      CreateBufferResource(device, sizeof(VertexData) * 6);

  // 頂点バッファビューを作成する
  D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
  // リソースの先頭のアドレスから使う
  vertexBufferViewSprite.BufferLocation =
      vertexResourceSprite->GetGPUVirtualAddress();
  // 使用するリソースのサイズは頂点4つ分のサイズ
  vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;
  // １頂点あたりのサイズ
  vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

  VertexData *vertexDataSprite = nullptr;
  vertexResourceSprite->Map(0, nullptr,
                            reinterpret_cast<void **>(&vertexDataSprite));
  // １枚目の三角形
  vertexDataSprite[0].position = {0.0f, 360.0f, 0.0f, 1.0f}; // 左下
  vertexDataSprite[0].texcoord = {0.0f, 1.0f};
  vertexDataSprite[1].position = {0.0f, 0.0f, 0.0f, 1.0f}; // 左上
  vertexDataSprite[1].texcoord = {0.0f, 0.0f};
  vertexDataSprite[2].position = {640.0f, 360.0f, 0.0f, 1.0f}; // 右下
  vertexDataSprite[2].texcoord = {1.0f, 1.0f};
  vertexDataSprite[3].position = {640.0f, 0.0f, 0.0f, 1.0f}; // 右上
  vertexDataSprite[3].texcoord = {1.0f, 0.0f};

  // Sparite用のTransformationMatrix用のリソースを作る。Matrix4x4
  // １つ分のサイズを用意する
  ID3D12Resource *transformtionMatirxResourceSprite =
      CreateBufferResource(device, sizeof(Matrix4x4));
  // データを書き込む
  Matrix4x4 *transformationMatrixDataSprite = nullptr;
  // 書き込むためのアドレスを取得
  transformtionMatirxResourceSprite->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixDataSprite));
  // 単位行列を書きこんでおく
  *transformationMatrixDataSprite = MakeIdentity4x4();

  // CPUで動かす用のTransformを作る
  Transform transformSprite{
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

  // 頂点インデックス
  ID3D12Resource *indexResourceSprite =
      CreateBufferResource(device, sizeof(uint32_t) * 6);

  D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
  // リソースの先頭のアドレスから使う
  indexBufferViewSprite.BufferLocation =
      indexResourceSprite->GetGPUVirtualAddress();
  // 使用するリソースのサイズはインデックス６つ分のサイズ
  indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
  // インデックスはuint32_tとする
  indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

  // インデックスリソースにデータを書き込む
  uint32_t *indexDataSprite = nullptr;
  indexResourceSprite->Map(0, nullptr,
                           reinterpret_cast<void **>(&indexDataSprite));
  indexDataSprite[0] = 0;
  indexDataSprite[1] = 1;
  indexDataSprite[2] = 2;
  indexDataSprite[3] = 1;
  indexDataSprite[4] = 3;
  indexDataSprite[5] = 2;

  // --------------------------------------------------------------------------
  
  /// ------------------------ GE3 --------------------------   
  // キーボード入力情報取得開始
  BYTE key[256] = {};
  BYTE prekey[256] = {};

  MSG msg{};
  // ウィンドウの×ボタンが押されるまでループ
  while (msg.message != WM_QUIT) {
    // Windowにメッセージが来てたら最優先で処理される
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      /// ------------------------ GE3 --------------------------
      // キーボード情報の取得開始
      /*keyboard->Acquire();*/
      /*BYTE key[256] = {};*/
      // 前の入力を保存
      memcpy(prekey, key, 256);
      /*keyboard->GetDeviceState(sizeof(key), key);*/

       // ゲームの処理
      if (key[DIK_SPACE] && !prekey[DIK_SPACE]) {
        /*assert(false && "SPACEが押されたのが確認できました");*/
        OutputDebugStringA("Press Space\n");
      }

      // Sprite用のWorldViewProjectionMatrixを作る
      Matrix4x4 worldMatrixSprite =
          MakeAffineMatrix(transformSprite.scale, transformSprite.rotate,
                           transformSprite.translate);
      Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
      Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(
          0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
      Matrix4x4 worldViewProjectionMatrixSprite =
          Multiply(worldMatrixSprite,
                   Multiply(viewMatrixSprite, projectionMatrixSprite));
      *transformationMatrixDataSprite = worldViewProjectionMatrixSprite;

      // フレームが始まる旨を告げる
      ImGui_ImplDX12_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();

      transform.rotate.y += 0.03f;
      Matrix4x4 worldMatrix = MakeAffineMatrix(
          transform.scale, transform.rotate, transform.translate);
      Matrix4x4 cameraMatrix =
          MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate,
                           cameraTransform.translate);
      Matrix4x4 viewMatrix = Inverse(cameraMatrix);
      Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
          0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
      Matrix4x4 worldViewProjectionMatrix =
          Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
      *wvpData = worldViewProjectionMatrix;

      // 開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
      ImGui::ShowDemoWindow();

      ImGui::Begin("Settings");
      ImGui::ColorEdit4("material", &materialData->x,
                        ImGuiColorEditFlags_AlphaPreview); // RGBWの指定
      ImGui::DragFloat2("Sprite taransform", &transformSprite.translate.x,
                        1.0f);
      ImGui::DragFloat("rotate.y", &transform.rotate.y, 0.1f);
      ImGui::End();

      // ImGuiの内部コマンドを生成する
      ImGui::Render();

      // 更新処理をかく
      // これからもよろしくお願いします。
      UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

      // TransitionBarrierの設定
      D3D12_RESOURCE_BARRIER barrier{};
      // 今回のバリアはTransition
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      // Noneにしておく
      barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      // バリアを張る対象のリソース。現在のバックバッファに対して行う
      barrier.Transition.pResource = swapChainResources[backBufferIndex];
      // 遷移前（現在）のResourceState
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
      // 遷移後のResourceState
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
      // TransitionBarrierを張る
      commandList->ResourceBarrier(1, &barrier);

      // 描画先のRTVを設定する
      commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false,
                                      nullptr);
      // 指定した色で画面全体をクリアする
      float clearColor[] = {0.1f, 0.25f, 0.5f, 1.0f};
      commandList->ClearRenderTargetView(rtvHandles[backBufferIndex],
                                         clearColor, 0, nullptr);

      // 描画先のRTVとDSVを設定する
      D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
          dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
      commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false,
                                      &dsvHandle);
      // 指定した深度で画面全体をクリアする
      commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH,
                                         1.0f, 0, 0, nullptr);

      // 描画用のDescriptorHeapの設定
      ID3D12DescriptorHeap *descriptorHeaps[] = {srvDescriptorHeap};
      commandList->SetDescriptorHeaps(1, descriptorHeaps);

      // コマンドを積む
      commandList->RSSetViewports(1, &viewport);       // viewportを設定
      commandList->RSSetScissorRects(1, &scissorRect); // Scirssorを設定
      // RootSignatureを設定。PSOに設定しているけど別途設定が必要
      commandList->SetGraphicsRootSignature(rootSignature);
      commandList->SetPipelineState(graphicsPipelineState);     // PSOを設定
      commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定
      // 形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばいいい
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      // マテリアルCBufferの場所を設定
      commandList->SetGraphicsRootConstantBufferView(
          0, materialResource->GetGPUVirtualAddress());
      // wvp用のCBufferの場所を設定
      commandList->SetGraphicsRootConstantBufferView(
          1, wvpResource->GetGPUVirtualAddress());

      // SRVのDescriptorTableの先頭を設定。２はrootParameter[2]である。
      commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

      // インデックスを指定
      commandList->IASetIndexBuffer(&indexBufferViewSprite); // IBNを設定

      // 描画!(DrawCall/ドローコル）。３頂点で一つのインスタンス。インスタンスについては今後
      /*commandList->DrawInstanced(6, 1, 0, 0);*/

      // モデル描画
      commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

      //// Spriteの描画。変更が必要なものだけ変更
      // commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
      //// TransformationMatrixCBufferの場所を設定
      // commandList->SetGraphicsRootConstantBufferView(
      //     1, transformtionMatirxResourceSprite->GetGPUVirtualAddress());
      //// 描画！（DrawInstanced(DrawCall/ドローコル）
      // commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

      // 実際のcommandListのImGuiの描画コマンドを積む
      ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

      // 今回はRenderTargetからPresentにする
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
      // TransitionBarrierを張る
      commandList->ResourceBarrier(1, &barrier);

      // コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
      hr = commandList->Close();
      assert(SUCCEEDED(hr));

      // GPUにコマンドリストの実行を行わせる
      ID3D12CommandList *commandLists[] = {commandList};
      commandQueue->ExecuteCommandLists(1, commandLists);
      // GPUとOSに画面の交換を行うよう追加する
      swapChain->Present(1, 0);

      // Fenceの値を更新
      fenceValue++;

      // GPUがここまでたどり着いたときに、
      // Fenceの値を指定した値に代入するようにSignalを送る
      commandQueue->Signal(fence, fenceValue);

      // Fenceの値が指定したSignal値にたどり着いているか確認する
      // GetCompletdValueの初期化はFence作成時に渡した初期値
      if (fence->GetCompletedValue() < fenceValue) {
        // 指定したSignalにたどり着いいていないので、
        // たどり着くまで待つようにイベントを設定する
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        // イベントを待つ
        WaitForSingleObject(fenceEvent, INFINITE);
      }

      // 次のフレーム用のコマンドリストを準備
      hr = commandAllocator->Reset();
      assert(SUCCEEDED(hr));
      hr = commandList->Reset(commandAllocator, nullptr);
      assert(SUCCEEDED(hr));

      /*if (input->Pushkey[DIK_ESCAPE]) {
        OutputDebugStringA("Game Loop End\n");
        break;
      }*/
    }

#ifdef _DEBUG
    ID3D12InfoQueue *infoQueue = nullptr;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
      // やばいエラー時に止まる
      infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
      // エラー時に止まる
      infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
      // 警告時に止まる
      /*infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,
       * true);*/

      // 抑制するメッセージのID
      D3D12_MESSAGE_ID denyIds[] = {
          // Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用によるエラーメッセージ

          D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};

      // 抑制するレベル
      D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
      D3D12_INFO_QUEUE_FILTER filter{};
      filter.DenyList.NumIDs = _countof(denyIds);
      filter.DenyList.pIDList = denyIds;
      filter.DenyList.NumSeverities = _countof(severities);
      filter.DenyList.pSeverityList = severities;
      // 指定したメッセージの表示を抑制する
      infoQueue->PushStorageFilter(&filter);

      // 解放
      infoQueue->Release();
    }
#endif
  }

  CloseHandle(fenceEvent);

  textureResource->Release();

  // ImGuiの終了処理。詳細はさして重要ではないので解説は省略する。
  // こういうもんである。初期化と逆順に行う
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  indexResourceSprite->Release();
  vertexResourceSprite->Release();
  transformtionMatirxResourceSprite->Release();

  srvDescriptorHeap->Release();

  // 解放処理
  vertexResource->Release();
  graphicsPipelineState->Release();
  signatureBlob->Release();
  if (errorBlob) {
    errorBlob->Release();
  }
  rootSignature->Release();
  pixelShaderBlob->Release();
  vertexShaderBlob->Release();

  fence->Release();
  rtvDescriptorHeap->Release();
  swapChainResources[0]->Release();
  swapChainResources[1]->Release();
  swapChain->Release();
  commandList->Release();
  commandAllocator->Release();
  commandQueue->Release();
  device->Release();
  useAdapter->Release();
  dxgiFactory->Release();
#ifdef _DEBUG
  debugController->Release();
#endif

  wvpResource->Release();
  materialResource->Release();
  CloseWindow(hwnd);

  // リソースリークチェック
  IDXGIDebug1 *debug;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
    debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
    debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
    debug->Release();
  }

  CoUninitialize();

  return 0;
}