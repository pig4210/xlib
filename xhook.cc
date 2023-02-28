
#include "xlib_test.h"
#include "xhook.h"


#ifdef _WIN32
#pragma code_seg(".text")
__declspec(allocate(".text"))
static const uint8_t hook_test_shellcode[] = {
    "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
    "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
    "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
    "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
    "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
    "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
    "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
    "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
    "\x33\xC0\xC3"};
#pragma code_seg()

using hook_test_function = int(*)(void);
// 注意，不能设置成 const ，编译器会优化不访问，致使测试出现错误结果。
static auto hook_test = (hook_test_function)&hook_test_shellcode;

static void HookCalling Routine(xlib::CPU_ST* lpcpu) {
  lpcpu->regXax += 1;
}

SHOW_TEST_INIT(XHOOK)

hook_test = (hook_test_function)&hook_test_shellcode;

SHOW_TEST_HEAD(Crack);
done = 0 == hook_test();
xlib::Crack(hook_test_shellcode, "\x33\xC0\xFE\xC0\xC3");
done = done && (1 == hook_test());
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(shellcodes);
xlib::shellcodes shellcode("\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x33\xC0\xFE\xC0\xC3");
done = 1 == ((hook_test_function)shellcode.data())();
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hook 2 byte);
xlib::Crack(hook_test_shellcode, "\x33\xC0\xC3");
auto h2 = new xlib::xHook(&hook_test_shellcode, 2, &Routine, false);
done = 1 == hook_test();
delete h2;
done = done && (0 == hook_test());
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hook 5 byte);
xlib::Crack(hook_test_shellcode, "\x33\xC0\xFE\xC0\x90\xC3");
auto h5 = new xlib::xHook(&hook_test_shellcode, 5, &Routine, false);
done = 2 == hook_test();
delete h5;
done = done && (1 == hook_test());
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hook 6 byte);
xlib::Crack(hook_test_shellcode, "\x33\xC0\xFE\xC0\xFE\xC0\xC3");
auto h6 = new xlib::xHook(&hook_test_shellcode, 6, &Routine, false);
done = 3 == hook_test();
delete h6;
done = done && (2 == hook_test());
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hook 15 byte);
xlib::Crack(hook_test_shellcode, "\x33\xC0\xFE\xC0\xFE\xC0\xFE\xC0\x90\x90\x90\x90\x90\x90\x90\x90\xC3");
auto h15 = new xlib::xHook(&hook_test_shellcode, 15, &Routine, false);
done = 4 == hook_test();
delete h15;
done = done && (3 == hook_test());
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hook call table);
xlib::Crack(hook_test_shellcode, "\x33\xC0\xFE\xC0\xC3");
auto hc = new xlib::xHook(&hook_test, &Routine, true, false);
done = 2 == hook_test();
delete hc;
done = done && (1 == hook_test());
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hook offset);
xlib::Crack(hook_test_shellcode, "\xE8\x01\x00\x00\x00\xC3\x33\xC0\xFE\xC0\xC3", 11);
#ifndef _WIN64
auto ho = new xlib::xHook(&hook_test_shellcode[1], &Routine, false, false);
#else
auto ho = new xlib::xHook(&hook_test_shellcode[1], &Routine, false, false, (void*)&hook_test_shellcode[0x10]);
#endif
done = 2 == hook_test();
delete ho;
done = done && (1 == hook_test());
SHOW_TEST_RESULT;

SHOW_TEST_DONE;

#endif  //_WIN32