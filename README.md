# 兼容USB和PS2键盘驱动模拟方法
## 优点：
可以绕过模拟检测
## 技术原理：
### 1、键盘模拟
通过直接调用Kbdclass的回调函数KeyboardClassServiceCallback直接给上层发送KEYBOARD_INPUT_DATA键盘消息
### 2、鼠标模拟
驱动mouclass.sys中的MouseClassServiceCallback函数
通过直接调用mouclass.sys中的MouseClassServiceCallback函数，发送MOUSE_INPUT_DATA消息鼠标键盘