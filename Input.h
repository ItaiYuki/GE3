#pragma once
#include <windows.h>
#include <wrl.h>

#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <dinput.h>

class Input {
public:
  /// <summary>
  /// 初期化
  /// </summary>
	
  void Initialize(HINSTANCE hInstance, HWND hwnd);
  /// <summary>
  // 更新
  /// </summary>
  void Update();

// namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

  bool PushKey(BYTE keyNumber); 
  bool TriggerKey(BYTE keyNumber);

private:
  /// directInput

  /// keyboardDevice
  ComPtr<IDirectInputDevice8> keyboard;
  ComPtr<IDirectInput8> directInput;
  /// 各キーの入力状態

  // iframe前の各キーの入力状態
  BYTE key[256] = {};
  BYTE keyPre[256] = {};

};
