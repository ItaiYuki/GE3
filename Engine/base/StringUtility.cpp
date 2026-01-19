#include "StringUtility.h"
#include <Windows.h>

namespace StringUtility {
std::wstring ConvertString(const std::string &str) {
  if (str.empty()) {
    return std::wstring();
  }

  int sizeNeeded = MultiByteToWideChar(
      CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

  if (sizeNeeded == 0) {
    return std::wstring();
  }

  std::wstring result(sizeNeeded, 0);

  MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
                      result.data(), sizeNeeded);

  return result; // 修正: 正しい値を返す
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
} // namespace StringUtility