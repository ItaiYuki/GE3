#include "StringUtility.h"
#include <windows.h>



namespace StringUtility {
std::wstring StringUtility::ConvertString(const std::string &str) {
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

std::string StringUtility::ConvertString(const std::wstring &str) {
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
}; 