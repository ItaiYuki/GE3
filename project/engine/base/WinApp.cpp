#include "WinApp.h"
#include <cassert>

#include "externals/imgui/imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM,
                                                             LPARAM);

// #pragma comment(lib, "dinput8.lib")
// #pragma comment(lib, "dxguid.lib")

// ウィンドウプロシージャ
LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
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
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

void WinApp::Initialize() {
  // COMの初期化
  CoInitializeEx(0, COINIT_MULTITHREADED);

  // 出力ウィンドウへの文字入力
  OutputDebugStringA("Hello,DirectX!\n");

  wc = {};
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

  // ウィンドウサイズを表す構造体にクライアント領域を入れる
  RECT wrc = {0, 0, kClientWidth, kClientHeight};

  // クライアント領域をもとに実際のサイズにwrcを変更してもらう
  AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

  // ウィンドウの生成
  hwnd = CreateWindow(wc.lpszClassName, // 利用するクラス名
                      L"GE3",           // タイトルバーの文字
                      WS_OVERLAPPEDWINDOW, // よく見るウィンドウスタイル
                      CW_USEDEFAULT,       // 表示X座標(Windowsに任せる)
                      CW_USEDEFAULT, // 表示Y座標(WindowsOSに任せる)
                      wrc.right - wrc.left, // ウィンドウ横幅
                      wrc.bottom - wrc.top, // ウィンドウ縦幅
                      nullptr,              // 親ウィンドウハンドル
                      nullptr,              // メニューハンドル
                      wc.hInstance,         // インスタンスハンドル
                      nullptr               // オプション
  );

  // ウィンドウを表示する
  ShowWindow(hwnd, SW_SHOW);
}

void WinApp::Update() {}

void WinApp::Finalize() {
  CloseWindow(hwnd);
  CoUninitialize();
}

bool WinApp::ProcessMessage() {
  // メインループ
  MSG msg{};

  // Windowにメッセージが来てたら最優先で処理される
  if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  if (msg.message == WM_QUIT) {
    return true;
  }

  return false;
}