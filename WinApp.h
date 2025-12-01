#pragma once
#include <Windows.h>
#include <wrl.h>

#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <dinput.h>

#include <cstdint>

class WinApp {
public:
  // 初期化処理
  void Initialize();

  // 更新処理
  void Update();

  // 終了
  void Finalize();

  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam);

  static const int32_t kClientWidth = 1280;
  static const int32_t kClientHeight = 720;

  // ウィンドウハンドル
  HWND hwnd = nullptr;
  // getter
  HWND GetHwnd() const { return hwnd; }
  HINSTANCE GetHInstance() const { return wc.hInstance; }

  // メッセージの処理
  bool ProcessMessage();

private: // メンバ変数
  // ウィンドウクラスの設定
  WNDCLASS wc{};
};