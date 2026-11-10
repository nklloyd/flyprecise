/**
 * @example get_touched_key.ino
 * @brief 获取当前的按键值并打印
 */

// 包含头文件matrix_keyboard.h
#include "matrix_keyboard.h"

MatrixKeyboard key_board;

void setup() {
  Serial.begin(115200);
  key_board.Setup();  // 在setup函数内调用MatrixKeyboard::Setup();
  Serial.println("setup done");
}

void loop() {
  const char key = key_board.GetTouchedKey();  // 获取当前按下的按键值

  // 如果当前按键值不为'\0'则打印出按键值
  if (key != '\0') {
    Serial.println(key);

    // 可以对不同的按键值分别处理
    switch (key) {
      case '1':
        // do something
        break;
      case '2':
        // do something
        break;
      case '3':
        // do something
        break;
      case 'A':
        // do something
        break;
      case '4':
        // do something
        break;
      case '5':
        // do something
        break;
      case '6':
        // do something
        break;
      case '7':
        // do something
        break;
      case '8':
        // do something
        break;
      case '9':
        // do something
        break;
      case 'C':
        // do something
        break;
      case '*':
        // do something
        break;
      case '0':
        // do something
        break;
      case '#':
        // do something
        break;
      case 'D':
        // do something
        break;
      default:
        // do nothing
        break;
    }
  }
}