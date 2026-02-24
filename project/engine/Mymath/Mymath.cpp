#include "Mymath.h"
#include <cassert>

using namespace MyMath;

Matrix4x4 Math::MakeIdentity4x4() {
  Matrix4x4 identity{};

  identity.m[0][0] = 1.0f;
  identity.m[1][1] = 1.0f;
  identity.m[2][2] = 1.0f;
  identity.m[3][3] = 1.0f;

  return identity;
}

Matrix4x4 Math::Multiply(const Matrix4x4 &m1, const Matrix4x4 &m2) {
  Matrix4x4 result{};

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] +
                       m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
    }
  }

  return result;
}

Matrix4x4 Math::MakeRotateXMatrix(float radian) {
  Matrix4x4 result = MakeIdentity4x4();

  float c = std::cos(radian);
  float s = std::sin(radian);

  result.m[1][1] = c;
  result.m[1][2] = s;
  result.m[2][1] = -s;
  result.m[2][2] = c;

  return result;
}

Matrix4x4 Math::MakeRotateYMatrix(float radian) {
  Matrix4x4 result = MakeIdentity4x4();

  float c = std::cos(radian);
  float s = std::sin(radian);

  result.m[0][0] = c;
  result.m[0][2] = -s;
  result.m[2][0] = s;
  result.m[2][2] = c;

  return result;
}

Matrix4x4 Math::MakeRotateZMatrix(float radian) {
  Matrix4x4 result = MakeIdentity4x4();

  float c = std::cos(radian);
  float s = std::sin(radian);

  result.m[0][0] = c;
  result.m[0][1] = s;
  result.m[1][0] = -s;
  result.m[1][1] = c;

  return result;
}

Matrix4x4 Math::MakeAffineMatrix(const Vector3 &scale, const Vector3 &rotate,
                                 const Vector3 &translate) {

  Matrix4x4 rx = MakeRotateXMatrix(rotate.x);
  Matrix4x4 ry = MakeRotateYMatrix(rotate.y);
  Matrix4x4 rz = MakeRotateZMatrix(rotate.z);

  Matrix4x4 rot = Multiply(Multiply(rx, ry), rz);

  rot.m[0][0] *= scale.x;
  rot.m[0][1] *= scale.x;
  rot.m[0][2] *= scale.x;

  rot.m[1][0] *= scale.y;
  rot.m[1][1] *= scale.y;
  rot.m[1][2] *= scale.y;

  rot.m[2][0] *= scale.z;
  rot.m[2][1] *= scale.z;
  rot.m[2][2] *= scale.z;

  rot.m[3][0] = translate.x;
  rot.m[3][1] = translate.y;
  rot.m[3][2] = translate.z;
  rot.m[3][3] = 1.0f;

  return rot;
}

Matrix4x4 Math::MakePerspectiveFovMatrix(float fovY, float aspectRatio,
                                         float nearClip, float farClip) {

  Matrix4x4 result{};

  float cot = 1.0f / std::tan(fovY / 2.0f);

  result.m[0][0] = cot / aspectRatio;
  result.m[1][1] = cot;
  result.m[2][2] = farClip / (farClip - nearClip);
  result.m[2][3] = 1.0f;
  result.m[3][2] = -(nearClip * farClip) / (farClip - nearClip);

  return result;
}

Matrix4x4 Math::MakeOrthographicMatrix(float left, float top, float right,
                                       float bottom, float nearClip,
                                       float farClip) {

  Matrix4x4 result{};

  result.m[0][0] = 2.0f / (right - left);
  result.m[1][1] = 2.0f / (top - bottom);
  result.m[2][2] = 1.0f / (farClip - nearClip);

  result.m[3][0] = (left + right) / (left - right);
  result.m[3][1] = (top + bottom) / (bottom - top);
  result.m[3][2] = nearClip / (nearClip - farClip);
  result.m[3][3] = 1.0f;

  return result;
}

Matrix4x4 MyMath::Math::Inverse(const Matrix4x4 &m) {
  float determinant = +m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3] +
                      m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1] +
                      m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2];

  assert(determinant != 0.0f);

  Matrix4x4 result;
  float recpDeterminant = 1.0f / determinant;

  return result;
}
