#include <Windows.h>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

#include <dinput.h>
#include <dxcapi.h>
#include <dxgidebug.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "dinput8.lib")

#include "DirectXCommon.h"
#include "Input.h"
#include "WinApp.h"
#include "externals/DirectXTex/DirectXTex.h"

#include <wrl.h>
using namespace Microsoft::WRL;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

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

struct VertexData {
  Vector4 position;
  Vector2 texcoord;
};

struct Matrix4x4 {
  float m[4][4];
};

struct Transform {
  Vector3 scale;
  Vector3 rotate;
  Vector3 translate;
};

struct MaterialData {
  std::string textureFilePath;
};

struct ModelData {
  std::vector<VertexData> vertices;
  MaterialData material;
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

// std::wstring ConvertString(const std::string& str)
//{
//	if (str.empty())
//	{
//		return std::wstring();
//	}
//	auto sizeNeebed =
//		MultiByteToWideChar
//		(
//			CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]),
//			static_cast<int>(str.size()), NULL, 0
//		);
//	if (sizeNeebed == 0)
//	{
//		return std::wstring();
//	}
//	std::wstring result(sizeNeebed, 0);
//	MultiByteToWideChar
//	(
//		CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]),
//		static_cast<int>(str.size()), &result[0], sizeNeebed
//	);
//	return result;
// }
//
////１Textureデータを読む
// DirectX::ScratchImage LoadTexture(const std::string& filePath)
//{
//	//テクスチャファイルを読んでプログラムで扱えるようにする
//	DirectX::ScratchImage image{};
//	std::wstring filePathW = ConvertString(filePath);
//	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(),
// DirectX::WIC_FLAGS_FORCE_RGB, nullptr, image); 	assert(SUCCEEDED(hr));
//
//	//ミップマップの作成
//	DirectX::ScratchImage mipImages{};
//	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
// image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
//	assert(SUCCEEDED(hr));
//
//	//ミップマップ付きのデータを返す
//	return mipImages;
// }

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

  ModelData modelData;
  std::vector<Vector4> positions;
  std::vector<Vector3> normals;
  std::vector<Vector2> texcoords;
  std::string line;

  std::ifstream file(directoryPath + "/" + filename);
  assert(file.is_open());

  while (std::getline(file, line)) {

    std::string identifier;
    std::istringstream s(line);
    s >> identifier;

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

      for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {

        std::string vertexDefinition;
        s >> vertexDefinition;

        std::istringstream v(vertexDefinition);
        uint32_t elementIndices[3];

        for (int32_t element = 0; element < 3; ++element) {
          std::string index;
          std::getline(v, index, '/');
          elementIndices[element] = std::stoi(index);
        }

        Vector4 position = positions[elementIndices[0] - 1];
        position.x *= -1.0f;

        Vector2 texcoord = texcoords[elementIndices[1] - 1];
        texcoord.y = 1.0f - texcoord.y;

        triangle[faceVertex] = {position, texcoord};
      }

      modelData.vertices.push_back(triangle[2]);
      modelData.vertices.push_back(triangle[1]);
      modelData.vertices.push_back(triangle[0]);
    } else if (identifier == "mtllib") {

      std::string materialFilename;
      s >> materialFilename;

      modelData.material =
          LoadMaterialTemplateFile(directoryPath, materialFilename);
    }
  }

  return modelData;
}

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

// ウィンメイン
// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  // ポインタ
  WinApp *winApp = nullptr;
  // WindowsAPIの初期化
  winApp = new WinApp();
  winApp->Initialize();

  // ポインタ
  DirectXCommon *dxCommon = nullptr;
  // DirectXの初期化
  dxCommon = new DirectXCommon();
  dxCommon->Initialize(winApp);

  ID3D12Device *device = dxCommon->GetDevice();

  // ======================
  // DXC初期化
  // ======================
  ComPtr<IDxcUtils> dxcUtils;
  ComPtr<IDxcCompiler3> dxcCompiler;
  ComPtr<IDxcIncludeHandler> includeHandler;

  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
  DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
  dxcUtils->CreateDefaultIncludeHandler(&includeHandler);

  ComPtr<IDxcBlob> vertexShaderBlob;
  ComPtr<IDxcBlob> pixelShaderBlob;

  ID3D12GraphicsCommandList *commandList = dxCommon->GetCommandList();

#ifdef _DEBUG
  /*ID3D12Debug1* debugController = nullptr;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
          debugController->EnableDebugLayer();

          debugController->SetEnableGPUBasedValidation(TRUE);
  }*/
#endif

  // Log(ConvertString(std::format(L"WSTRING{}\n", L"abc")));

  //=========================================//
  //         各種デスクリプタヒープ          //↓↓↓始まり↓↓↓
  //=========================================//

  ////RTV用のヒープでデイスクリプタの数は２。RTVはShader内で触るものではないので、ShaderVisibleはfasle
  // ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(device,
  // D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

  ////SRV用のヒープでデイスクリプタの数は128。SRVはShader内で触るものなので、ShaderVIsibleはture
  // ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(device,
  // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

  //=========================================//
  //         各種デスクリプタヒープ          //↑↑↑終わり↑↑↑
  //=========================================//

  // RootSignature作成
  D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
  descriptionRootSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  // b0 (WVP用CBV)
  D3D12_ROOT_PARAMETER rootParameters[1]{};
  rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
  rootParameters[0].Descriptor.ShaderRegister = 0;

  descriptionRootSignature.pParameters = rootParameters;
  descriptionRootSignature.NumParameters = 1;

  // シリアライズ
  ComPtr<ID3DBlob> signatureBlob;
  ComPtr<ID3DBlob> errorBlob;
  HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
                                           D3D_ROOT_SIGNATURE_VERSION_1,
                                           &signatureBlob, &errorBlob);
  assert(SUCCEEDED(hr));

  // 作成
  ComPtr<ID3D12RootSignature> rootSignature;
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
  // すべての色要素を書き込む
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;

  // RasiterZerStateの設定
  D3D12_RASTERIZER_DESC rasterizeDesc{};
  // 裏面（時計周り）を表示しない
  rasterizeDesc.CullMode = D3D12_CULL_MODE_BACK;
  // 三角形の中を塗りつぶす
  rasterizeDesc.FillMode = D3D12_FILL_MODE_SOLID;
  // 裏面表示
  rasterizeDesc.CullMode = D3D12_CULL_MODE_NONE;

  // shaderをコンバイルする
  vertexShaderBlob = dxCommon->CompileShader(
      L"resources/shaders/hlsli/Object3d.VS.hlsl", L"vs_6_0");

  pixelShaderBlob = dxCommon->CompileShader(
      L"resources/shaders/hlsli/Object3d.PS.hlsl", L"ps_6_0");

  assert(vertexShaderBlob != nullptr);
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

  ////DepthstencilStateの設定
  // D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  ////Depthの機能を有効化する
  // depthStencilDesc.DepthEnable = true;
  ////書き込みします
  // depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  ////比較関数はLessEqual。つまり、近ければ描画される
  // depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  ////DepthStencilの設定
  // graphicsPipelinStateDesc.DepthStencilState = depthStencilDesc;
  // graphicsPipelinStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  // 実際に生成
  ID3D12PipelineState *graphicsPipelineState = nullptr;
  hr = device->CreateGraphicsPipelineState(
      &graphicsPipelinStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
  assert(SUCCEEDED(hr));

  // ポインタ
  Input *input = nullptr;
  // 入力の初期化
  input = new Input();
  input->Initialize(winApp);
  // 入力の更新
  input->Update();

  // モデル読み込み
  /*ModelData modelData = LoadObjFile("resources", "plane.obj");*/
  ModelData modelData = LoadObjFile("resources", "axis.obj");

  // 頂点リソースを作る
  ComPtr<ID3D12Resource> vertexResource = dxCommon->CreateBufferResource(
      sizeof(VertexData) * modelData.vertices.size());
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

  ////ビューボート
  // D3D12_VIEWPORT viewport{};
  ////クライアント領域のサイズと一緒にして画面全体に表示
  // viewport.Width = WinApp::kClientWidth;
  // viewport.Height = WinApp::kClientHeight;
  // viewport.TopLeftX = 0;
  // viewport.TopLeftY = 0;
  // viewport.MinDepth = 0.0f;
  // viewport.MaxDepth = 1.0f;

  ////シザー矩形
  // D3D12_RECT scissorRect{};
  ////基本的にビューボートと同じ矩形が構成されるようにする
  // scissorRect.left = 0;
  // scissorRect.right = WinApp::kClientWidth;
  // scissorRect.top = 0;
  // scissorRect.bottom = WinApp::kClientHeight;

  // マテリアル用のリソースを作る。今回はcolor１つ分のサイズを用意する
  ComPtr<ID3D12Resource> materialResource =
      dxCommon->CreateBufferResource(sizeof(Vector4));
  ////マテリアルにデータを書き込む
  // Vector4* materialData = nullptr;
  ////書き込むためのアドレスを取得
  // materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
  ////今回は白を書き込んでみる
  //*materialData = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

  // wvp用のリソースを作る。matrix4x4 1つ分のサイズを用意する
  ComPtr<ID3D12Resource> wvpResource =
      dxCommon->CreateBufferResource(sizeof(Matrix4x4));
  // データを書き込む
  Matrix4x4 *wvpData = nullptr;
  // 書き込むためのアドレスを取得
  wvpResource->Map(0, nullptr, reinterpret_cast<void **>(&wvpData));
  // 単位行列を書き込んでおく
  *wvpData = MakeIdentity4x4();

  // Transform transform{
  //   {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
  // Transform cameraTransform{
  //	{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -5.0f} };

  ////ImGuiの初期化。詳細はさして重要ではないので解説は省略する。
  ////こういうもんである
  // IMGUI_CHECKVERSION();
  // ImGui::CreateContext();
  // ImGui::StyleColorsDark();
  // ImGui_ImplWin32_Init(winApp->GetHwnd());
  // ImGui_ImplDX12_Init(device,
  //	swapChainDesc.BufferCount,
  //	rtvDesc.Format,
  //	srvDescriptorHeap,
  //	srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
  //	srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

  ////Textueを読んで転送する
  ////DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
  // DirectX::ScratchImage mipImages =
  // LoadTexture(modelData.material.textureFilePath); const
  // DirectX::TexMetadata& metadata = mipImages.GetMetadata(); ID3D12Resource*
  // textureResource = CreateTextureResource(device, metadata);
  // UploadTextureData(textureResource, mipImages);

  ////metaDataを基にSRVの設定
  // D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  // srvDesc.Format = metadata.format;
  // srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  // srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//２Dテクスチャ
  // srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

  ////SRVを作成するDescriptorHeapの場所を決める
  // D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU =
  // srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
  // D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU =
  // srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
  ////先頭はImGuiが使っているのでその次を使う
  // textureSrvHandleCPU.ptr +=
  // device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  // textureSrvHandleGPU.ptr +=
  // device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  ////SRVの生成
  // device->CreateShaderResourceView(textureResource, &srvDesc,
  // textureSrvHandleCPU);

  ////DepthStencilTextureをウィンドウのサイズで作成
  // ID3D12Resource* depthStencilResource =
  // CreateDepthStencilTextureResource(device, WinApp::kClientWidth,
  // WinApp::kClientHeight);

  ////DSV用のヒープでデイスクリプタの数は１．DSVはShader内で触るものではないので、ShaderVisibleはfalse
  // ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device,
  // D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

  ////DSVの設定
  // D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
  // dsvDesc.Format =
  // DXGI_FORMAT_D24_UNORM_S8_UINT;//Format.基本的にはResourceに合わせる
  // dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  ////DSVHeapの先頭にDSVをつくる
  // device->CreateDepthStencilView(depthStencilResource, &dsvDesc,
  // dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

  ////Sprite用の頂点リソースを作る
  // ID3D12Resource* vertexResourceSprite = CreateBufferResource(device,
  // sizeof(VertexData) * 6);

  ////頂点バッファビューを作成する
  // D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
  ////リソースの先頭のアドレスから使う
  // vertexBufferViewSprite.BufferLocation =
  // vertexResourceSprite->GetGPUVirtualAddress();
  ////使用するリソースのサイズは頂点６つ分のサイズ
  // vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;
  ////１頂点あたりのサイズ
  // vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

  // VertexData* vertexDataSprite = nullptr;
  // vertexResourceSprite->Map(0, nullptr,
  // reinterpret_cast<void**>(&vertexDataSprite));
  ////１枚目の三角形
  // vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };//左下
  // vertexDataSprite[0].texcoord = { 0.0f,1.0f };
  // vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };//左上
  // vertexDataSprite[1].texcoord = { 0.0f,0.0f };
  // vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };//右下
  // vertexDataSprite[2].texcoord = { 1.0f,1.0f };
  // vertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f };//右上
  // vertexDataSprite[3].texcoord = { 1.0f,0.0f };
  // ２枚目の三角形
  // vertexDataSprite[3].position = { 0.0f,0.0f,0.0f,1.0f };//左上
  // vertexDataSprite[3].texcoord = { 0.0f,0.0f };
  // vertexDataSprite[5].position = { 640.0f,360.0f,0.0f,1.0f };//右下
  // vertexDataSprite[5].texcoord = { 1.0f,1.0f };

  ////Sparite用のTransformationMatrix用のリソースを作る。Matrix4x4
  /// １つ分のサイズを用意する
  // ID3D12Resource* transformtionMatirxResourceSprite =
  // CreateBufferResource(device, sizeof(Matrix4x4));
  ////データを書き込む
  // Matrix4x4* transformationMatrixDataSprite = nullptr;
  ////書き込むためのアドレスを取得
  // transformtionMatirxResourceSprite->Map(0, nullptr,
  // reinterpret_cast<void**>(&transformationMatrixDataSprite));
  ////単位行列を書きこんでおく
  //*transformationMatrixDataSprite = MakeIdentity4x4();

  ////CPUで動かす用のTransformを作る
  // Transform transformSprite{ {1.0f,1.0f,1.0f},{ 0.0f,0.0f,0.0f
  // },{0.0f,0.0f,0.0f} };

  ////頂点インデックス
  // ID3D12Resource* indexResourceSprite = CreateBufferResource(device,
  // sizeof(uint32_t) * 6);

  // D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
  ////リソースの先頭のアドレスから使う
  // indexBufferViewSprite.BufferLocation =
  // indexResourceSprite->GetGPUVirtualAddress();
  ////使用するリソースのサイズはインデックス６つ分のサイズ
  // indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
  ////インデックスはuint32_tとする
  // indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

  ////インデックスリソースにデータを書き込む
  // uint32_t* indexDataSprite = nullptr;
  // indexResourceSprite->Map(0, nullptr,
  // reinterpret_cast<void**>(&indexDataSprite)); indexDataSprite[0] = 0;
  // indexDataSprite[1] = 1; indexDataSprite[2] = 2; indexDataSprite[3] = 1;
  // indexDataSprite[4] = 3; indexDataSprite[5] = 2;

  // BYTE key[256]{};
  // BYTE prekey[256]{};

  //------------------------------------------------------------------------------------------------------------------------------

  // ウィンドウの×ボタンが押されるまでループ
  while (true) {
    // コマンドリスト取得
    ID3D12GraphicsCommandList *commandList = dxCommon->GetCommandList();

    // 描画前準備
    dxCommon->PreDraw();

    // ビューポート
    D3D12_VIEWPORT viewport{};
    viewport.Width = float(WinApp::kClientWidth);
    viewport.Height = float(WinApp::kClientHeight);
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    commandList->RSSetViewports(1, &viewport);

    // シザー
    D3D12_RECT scissorRect{};
    scissorRect.left = 0;
    scissorRect.right = WinApp::kClientWidth;
    scissorRect.top = 0;
    scissorRect.bottom = WinApp::kClientHeight;
    commandList->RSSetScissorRects(1, &scissorRect);

    // PSO
    commandList->SetPipelineState(graphicsPipelineState);

    // RootSignature
    commandList->SetGraphicsRootSignature(rootSignature.Get());

    // VB
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // WVP
    commandList->SetGraphicsRootConstantBufferView(
        0, wvpResource->GetGPUVirtualAddress());

    // Draw
    commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

    // 描画後
    dxCommon->PostDraw();

    ////windowsのメッセージ処理
    if (winApp->ProcessMessage()) {
      break;
    }

    Matrix4x4 world = MakeIdentity4x4();
    Matrix4x4 view = MakeIdentity4x4();
    Matrix4x4 projection = MakePerspectiveFovMatrix(
        0.45f, float(WinApp::kClientWidth) / WinApp::kClientHeight, 0.1f,
        100.0f);

    Matrix4x4 wvp = Multiply(world, Multiply(view, projection));
    *wvpData = wvp;

    //
    //	//数字の0キーが押されていたら
    //	if (input->PushKey(DIK_0))
    //	{
    //		OutputDebugStringA("Hit 0\n");
    //	}

    //	//ゲームの処理

    //	//Sprite用のWorldViewProjectionMatrixを作る
    //	Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale,
    // transformSprite.rotate, transformSprite.translate); 	Matrix4x4
    // viewMatrixSprite = MakeIdentity4x4(); 	Matrix4x4 projectionMatrixSprite
    // = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth),
    // float(WinApp::kClientHeight), 0.0f, 100.0f); 	Matrix4x4
    // worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite,
    // Multiply(viewMatrixSprite, projectionMatrixSprite));
    //	*transformationMatrixDataSprite = worldViewProjectionMatrixSprite;

    //	//フレームが始まる旨を告げる
    //	ImGui_ImplDX12_NewFrame();
    //	ImGui_ImplWin32_NewFrame();
    //	ImGui::NewFrame();

    //	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale,
    // transform.rotate, transform.translate); 	Matrix4x4 cameraMatrix =
    // MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate,
    // cameraTransform.translate); 	Matrix4x4 viewMatrix =
    // Inverse(cameraMatrix); 	Matrix4x4 projectionMatrix =
    // MakePerspectiveFovMatrix(0.45f, float(WinApp::kClientWidth) /
    // float(WinApp::kClientHeight), 0.1f, 100.0f); 	Matrix4x4
    // worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix,
    // projectionMatrix)); 	*wvpData = worldViewProjectionMatrix;

    //	//開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
    //	ImGui::ShowDemoWindow();

    //	ImGui::Begin("Settings");
    //	ImGui::ColorEdit4("material", &materialData->x,
    // ImGuiColorEditFlags_AlphaPreview);//RGBWの指定
    //	ImGui::DragFloat("rotate.y", &transform.rotate.y, 0.1f);
    //	ImGui::DragFloat3("transform", &transform.translate.x, 0.1f);
    //	ImGui::DragFloat2("Sprite transform",
    //&transformSprite.translate.x, 1.0f); 	ImGui::End();

    //	//transform.rotate.y += 0.03f;

    //	//ImGuiの内部コマンドを生成する
    //	ImGui::Render();

    //	//更新処理をかく
    //	//これからもよろしくお願いします。
    //	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    // TransitionBarrierの設定
    //	D3D12_RESOURCE_BARRIER barrier{};
    //	// 今回のバリアはTransition
    //	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    //	// Noneにしておく
    //	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    //	// バリアを張る対象のリソース。現在のバックバッファに対して行う
    //	barrier.Transition.pResource = swapChainResources[backBufferIndex];
    //	// 遷移前（現在）のResourceState
    //	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    //	// 遷移後のResourceState
    //	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    //	// TransitionBarrierを張る
    //	commandList->ResourceBarrier(1, &barrier);

    ////描画先のRTVを設定する
    // commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false,
    // nullptr);
    ////指定した色で画面全体をクリアする
    // float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
    // commandList->ClearRenderTargetView(rtvHandles[backBufferIndex],
    // clearColor, 0, nullptr);

    //	//描画先のRTVとDSVを設定する
    //	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
    // dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    //	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false,
    //&dsvHandle);
    //	//指定した深度で画面全体をクリアする
    //	commandList->ClearDepthStencilView(dsvHandle,
    // D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    //	//描画用のDescriptorHeapの設定
    //	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
    //	commandList->SetDescriptorHeaps(1, descriptorHeaps);

    //	//コマンドを積む
    //	commandList->RSSetViewports(1, &viewport);//viewportを設定
    //	commandList->RSSetScissorRects(1, &scissorRect);//Scirssorを設定
    //	//RootSignatureを設定。PSOに設定しているけど別途設定が必要
    //	commandList->SetGraphicsRootSignature(rootSignature);
    //	commandList->SetPipelineState(graphicsPipelineState);//PSOを設定
    //	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);//VBVを設定
    //	//形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばいいい
    //	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //	//マテリアルCBufferの場所を設定
    //	commandList->SetGraphicsRootConstantBufferView(0,
    // materialResource->GetGPUVirtualAddress());
    //	// wvp用のCBufferの場所を設定
    //	commandList->SetGraphicsRootConstantBufferView(1,
    // wvpResource->GetGPUVirtualAddress());

    //	//SRVのDescriptorTableの先頭を設定。２はrootParameter[2]である。
    //	commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

    //	//インデックスを指定
    //	commandList->IASetIndexBuffer(&indexBufferViewSprite);//IBNを設定

    //	//描画!(DrawCall/ドローコル）。３頂点で一つのインスタンス。インスタンスについては今後
    //	commandList->DrawInstanced(6, 1, 0, 0);

    //	//モデル描画
    //	commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

    //	//--------------------------------------

    //	//Spriteの描画。変更が必要なものだけ変更
    //	commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
    //	//TransformationMatrixCBufferの場所を設定
    //	commandList->SetGraphicsRootConstantBufferView(1,
    // transformtionMatirxResourceSprite->GetGPUVirtualAddress());
    //	//描画！（DrawInstanced(DrawCall/ドローコル）
    //	commandList->DrawInstanced(6, 1, 0, 0);
    //	//描画!(DrawCall/ドローコル）６個のインデックスを使用し１つのインスタンスを描画。その他は当面０で良い
    //	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    //	//実際のcommandListのImGuiの描画コマンドを積む
    //	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

    //	// 今回はRenderTargetからPresentにする
    //	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    //	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    //	// TransitionBarrierを張る
    //	commandList->ResourceBarrier(1, &barrier);

    // コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
    /*hr = commandList->Close();
    assert(SUCCEEDED(hr));*/

    ////GPUにコマンドリストの実行を行わせる
    // ID3D12CommandList* commandLists[] = { commandList };
    // commandQueue->ExecuteCommandLists(1, commandLists);
    ////GPUとOSに画面の交換を行うよう追加する
    // swapChain->Present(1, 0);

    //	// Fenceの値を更新
    //	fenceValue++;

    //	// GPUがここまでたどり着いたときに、
    //	// Fenceの値を指定した値に代入するようにSignalを送る
    //	commandQueue->Signal(fence, fenceValue);

    //	// Fenceの値が指定したSignal値にたどり着いているか確認する
    //	// GetCompletdValueの初期化はFence作成時に渡した初期値
    //	if (fence->GetCompletedValue() < fenceValue) {
    //		// 指定したSignalにたどり着いいていないので、
    //		// たどり着くまで待つようにイベントを設定する
    //		fence->SetEventOnCompletion(fenceValue, fenceEvent);
    //		// イベントを待つ
    //		WaitForSingleObject(fenceEvent, INFINITE);
    //	}

    ////次のフレーム用のコマンドリストを準備
    // hr = commandAllocator->Reset();
    // assert(SUCCEEDED(hr));
    // hr = commandList->Reset(commandAllocator, nullptr);
    // assert(SUCCEEDED(hr));

#ifdef _DEBUG
    // ID3D12InfoQueue* infoQueue = nullptr;
    // if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    //	// やばいエラー時に止まる
    //	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    //	// エラー時に止まる
    //	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    //	// 警告時に止まる
    ///*infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);*/

    //	// 抑制するメッセージのID
    //	D3D12_MESSAGE_ID denyIds[] = {
    //		//
    // Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用によるエラーメッセージ

    //		D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
    //};

    //	// 抑制するレベル
    //	D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
    //	D3D12_INFO_QUEUE_FILTER filter{};
    //	filter.DenyList.NumIDs = _countof(denyIds);
    //	filter.DenyList.pIDList = denyIds;
    //	filter.DenyList.NumSeverities = _countof(severities);
    //	filter.DenyList.pSeverityList = severities;
    //	// 指定したメッセージの表示を抑制する
    //	infoQueue->PushStorageFilter(&filter);

    //	// 解放
    //	infoQueue->Release();

    //}
#endif
  }

  // CloseHandle(fenceEvent);

  // ImGuiの終了処理。詳細はさして重要ではないので解説は省略する。
  // こういうもんである。初期化と逆順に行う
  /*ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();*/

  // indexResourceSprite->Release();
  // dsvDescriptorHeap->Release();
  // depthStencilResource->Release();

  // textureResource->Release();

  // srvDescriptorHeap->Release();

  ////解放処理
  // vertexResource->Release();
  // graphicsPipelineState->Release();
  // signatureBlob->Release();
  // if (errorBlob)
  //{
  //	errorBlob->Release();
  // }
  // rootSignature->Release();
  // pixelShaderBlob->Release();
  // vertexShaderBlob->Release();

  // fence->Release();
  // rtvDescriptorHeap->Release();
  // swapChainResources[0]->Release();
  // swapChainResources[1]->Release();
  // swapChain->Release();
  // commandList->Release();
  // commandAllocator->Release();
  // commandQueue->Release();
  // device->Release();
  // useAdapter->Release();
  // dxgiFactory->Release();

  // 入力解放
  delete input;

  // WindowsAPIの終了処理
  winApp->Finalize();

  // WindowsAPI解放
  delete winApp;
  winApp = nullptr;

  // DirectX解放
  delete dxCommon;

#ifdef _DEBUG
  // debugController->Release();
#endif

  // wvpResource->Release();
  // materialResource->Release();

  //// リソースリークチェック
  // IDXGIDebug1* debug;
  // if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
  //{
  //	debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
  //	debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
  //	debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
  //	debug->Release();
  // }

  // CoUninitialize();

  return 0;
}