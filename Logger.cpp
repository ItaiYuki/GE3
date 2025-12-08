namespace Logger {
 void Log(const std::string &message) {
  // 出力ウィンドウに文字列を表示
  OutputDebugStringA(message.c_str());
 }
} 