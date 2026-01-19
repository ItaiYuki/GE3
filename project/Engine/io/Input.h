#pragma once
#include <Windows.h>
#include <wrl.h>

#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <dinput.h>

#include "engine/base/WinApp.h"

class Input {
public:
  // 初期化処理
  void Initialize(WinApp *winApp);

  // 更新処理
  void Update();

  // namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

  bool PushKey(BYTE keyNumber);
  bool TriggerKey(BYTE keyNumber);

private: // メンバ変数
  // キーボードのデバイス
  ComPtr<IDirectInputDevice8A>
      keyboard; // 修正: IDirectInputDevice8W -> IDirectInputDevice8A
  // DirectInputのインスタンス生成
  ComPtr<IDirectInput8A> directInput; // 修正: IDirectInput8W -> IDirectInput8A

  BYTE key[256] = {};
  BYTE keyPre[256] = {};

  // WindowsAPI
  WinApp *winApp_ = nullptr;
};