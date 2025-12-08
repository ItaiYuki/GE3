#include "Input.h"
#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
  HRESULT result;

  // DirectInputオブジェクトの作成
  result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
                              (void **)&directInput, nullptr);
  assert(SUCCEEDED(result));

  // キーボードデバイスの作成
  result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
  assert(SUCCEEDED(result));

  // 入力データ形式のセット
  result = keyboard->SetDataFormat(&c_dfDIKeyboard);
  assert(SUCCEEDED(result));

  // 協調レベルの設定
  result = keyboard->SetCooperativeLevel(
      hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
  assert(SUCCEEDED(result));

  // 入力状態の初期化
  memset(key, 0, sizeof(key));
  memset(keyPre, 0, sizeof(keyPre));
}

void Input::Update() {
  HRESULT result;

  // 前回のキー状態を保存
  memcpy(keyPre, key, sizeof(key));

  // キーボード入力の取得開始
  result = keyboard->Acquire();
  if (FAILED(result)) {
    // フォーカスが外れているなどで失敗した場合は無視
    return;
  }

  // 現在の全キー状態を取得
  result = keyboard->GetDeviceState(sizeof(key), key);
  if (FAILED(result)) {
    // 取得失敗時も安全にスルー
    memset(key, 0, sizeof(key));
  }
}

bool Input::PushKey(BYTE keyNumber) {
  // キーが押されている間 true を返す
  return key[keyNumber] != 0;
}

bool Input::TriggerKey(BYTE keyNumber) {
  // 今フレーム押されていて、前フレームは押されていなかった場合 true
  return (key[keyNumber] != 0 && keyPre[keyNumber] == 0);
}