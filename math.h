#pragma once
#include <cmath>
#include <string>
#include <vector>

namespace math {

struct Vector2 {
  float x;
  float y;
};

struct Vector3 {
  float x;
  float y;
  float z;
};

struct Vector4 {
  float x;
  float y;
  float z;
  float w;
};

struct Matrix4x4 {
  float m[4][4];
};

struct Transform {
  Vector3 scale;
  Vector3 rotate;
  Vector3 translate;
};

class Math {
public:
  // 単位行列
  static Matrix4x4 MakeIdentity4x4();

  // 行列乗算
  static Matrix4x4 Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2);

  // 回転
  static Matrix4x4 MakeRotateXMatrix(float radian);
  static Matrix4x4 MakeRotateYMatrix(float radian);
  static Matrix4x4 MakeRotateZMatrix(float radian);

  // アフィン変換
  static Matrix4x4 MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate,
                                    const Vector3 &translate);

  // 射影変換
  static Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                            float nearClip, float farClip);

  static Matrix4x4 MakeOrthographicMatrix(float left, float top, float right,
                                          float bottom, float nearClip,
                                          float farClip);

  // 逆行列
  static Matrix4x4 Inverse(const Matrix4x4 &m);
};

} 