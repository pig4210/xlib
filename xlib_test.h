/**
  \file  xlib_test.h
  \brief 定义了 xlib 测试环境。

  \version    1.0.0.161115
  \note       Only For Ring3

  \author     triones
  \date       2013-03-19
*/
#ifndef _XLIB_TEST_H_
#define _XLIB_TEST_H_

#ifdef _XLIB_TEST_

#ifdef FOR_RING0
#   pragma message("RING0 下不能使用 _XLIB_TEST_")
#   error "不兼容的编译选项"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>
#include <iomanip>

using std::string;
using std::cout;
using std::endl;
using std::setw;
using std::setiosflags;
using std::ios;

typedef void (*xlib_test_routine)(void);

bool Add_XLIB_TEST_ROUTINE(const char* const name, xlib_test_routine func);

#define ADD_XLIB_TEST(name) \
                      static void name##_TEST();\
                      static const auto btest = Add_XLIB_TEST_ROUTINE(#name, name##_TEST);\
                      static void name##_TEST()
#define SHOW_TEST_INIT cout << "======== " << __FUNCTION__ << " ========" << endl;
#define SHOW_TEST_HEAD(head) cout << setiosflags(ios::left) << setw(41) << head;
#define SHOW_TEST_RESULT(done)  cout << " : " << (done ? "ok" : "fail !!!") << endl;

#endif  // _XLIB_TEST_

#endif  // _XLIB_TEST_H_