/**
  \file  xhook.h
  \brief 用于 windows hook 。

  \version    0.0.1.230228
  \note       only for windows .

  \author     triones
  \date       2022-06-02

  \section more 额外说明

  - 由于链接器的【增量链接】可能会造成在使用 Hook 时出现一些莫名其妙的状况，请将之关闭！

  \section history 版本记录

  - 2022-06-02 新建 xhook 。
  - 2023-02-27 修正 UEF 在 x86/x64 下的错误。
  - 2023-02-28 修正 x64 下 hook offset 时的 shellcode 错误。
*/
#ifndef _XLIB_XHOOK_H_
#define _XLIB_XHOOK_H_

#ifdef _WIN32

#include <memory>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN

namespace xlib {

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/// Hook 错误码。
enum HOOK_ERROR_ENUM {
  XHE_NoError,
  XHE_ReCoverProtect,  //< Crack 还原页属性失败。
  XHE_OK,
  XHE_ModifyProtect,  //< Crack 修改页属性失败。
  XHE_Write,          //< Crack 修改内存失败。
  XHE_0Size,          //< hooksize 0 大小。
  XHE_BadHookMem,     //< hookmem 指向内存不可读。
  XHE_BadRoutine,     //< routine 指向内存不可读。
  XHE_UEFCover,       //< UEF 回调被覆盖。
  XHE_Restored,       //< Hook 已被第三方还原。
  XHE_BeCovered,      //< Hook 已被第三方覆盖。
  XHE_UnHooked,       //< 已 UnHook 。
  XHE_Move,           //< 转移 shellcode 时写入失败。
  XHE_AddDisp,        //< x64 下，偏移无效。
};

/// 判定 Hook 错误码是否成功。（有些错误对成功不造成影响）
inline bool IsHookOK(const HOOK_ERROR_ENUM e) { return XHE_OK >= e; }

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/**
  指定内存位置，写入数据。
  \param    mem   需要写入数据的内存位置。
  \param    code  写入数据指针。
  \param    size  写入数据大小。

  \code
    // 接受指定长度数据。
    auto e = Crack(0, (void*)"\x90\x90", 2);
    auto e = Crack((void*)0, u8"\x90\x90", 2);
    // 接受顺序容器。
    auto e = Crack(nullptr, std::string("\x90\x90"));
    auto e = Crack(0, std::array<char, 2>{'\x90', '\x90'});
    // 接受字符串字面量。
    auto e = Crack(0, "\x90\x90");
    auto e = Crack(0, u8"\x90\x90");
  \endcode
*/
template <typename P, typename T>
HOOK_ERROR_ENUM Crack(P mem, const T* code, const size_t size) {
  auto m = (LPVOID)mem;
  auto c = (const void*)code;
  auto s = sizeof(T) * size;
  DWORD oldprotect;
  if (FALSE == VirtualProtect(m, s, PAGE_EXECUTE_READWRITE, &oldprotect)) {
    return XHE_ModifyProtect;
  }
  bool ok = true;
  try {
    memcpy(m, c, s);
  } catch (...) {
    ok = false;
  }
  // 不管成功与否，都应该还原页属性。
  if (FALSE == VirtualProtect(m, s, oldprotect, &oldprotect)) {
    if (ok) {
      return XHE_ReCoverProtect;
    }
  }
  return ok ? XHE_NoError : XHE_Write;
}
template <typename P>
auto Crack(P mem, const void* code, const size_t size) {
  return Crack(mem, (const char*)code, size);
}
template <typename P, class T>
auto Crack(P mem, const T& o)
    -> std::enable_if_t<std::is_pointer_v<decltype(o.data())>, HOOK_ERROR_ENUM> {
  return Crack(mem, o.data(), o.size());
}
template <typename P, typename T, size_t size>
auto Crack(P mem, T const (&data)[size]) {
  return Crack(mem, data, size - 1);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/**·
  CPU 状态标志寄存器
  英特尔 64 和 IA-32 架构软件 developer’s 手册，第 1 卷、基本架构 [PDF] 。
  文件名： 25366521.pdf ，大小： 3170961 字节，日期： 2006 年 9 月， Vol.1，3-21 & 3-24 。
  - O  one
  - Z  zero
  - S  Indicates a Status Flag
  - C  Indicates a Control Flag
  - X  Indicates a System Flag
*/
struct CPU_FLAGS {
  size_t CF : 1;  //< S  Carry Flag
  size_t O1 : 1;
  size_t PF : 1;  //< S  Parity Flag
  size_t Z3 : 1;

  size_t AF : 1;  //< S  Auxiliary Carry Flag
  size_t Z5 : 1;
  size_t ZF : 1;  //< S  Zero Flag
  size_t SF : 1;  //< S  Sign Flag

  size_t TF : 1;  //< X  Trap Flag
  size_t IF : 1;  //< X  Interrupt Enable Flag
  size_t DF : 1;  //< C  Direction Flag
  size_t OF : 1;  //< S  Overflow Flag

  size_t IOPL : 2;  //< X  I/O Privilege Level
  size_t NT : 1;    //< X  Nested Task (NT)
  size_t Z15 : 1;

  size_t RF : 1;   //< X  Resume Flag
  size_t VM : 1;   //< X  Virtual-8086 Mode
  size_t AC : 1;   //< X  Alignment Check
  size_t VIF : 1;  //< X  Virtual Interrupt Flag

  size_t VIP : 1;  //< X  Virtual Interrupt Pending
  size_t ID : 1;   //< X  ID Flag
  size_t Z22 : 1;
  size_t Z23 : 1;

  size_t Z24 : 1;
  size_t Z25 : 1;
  size_t Z26 : 1;
  size_t Z27 : 1;

  size_t Z28 : 1;
  size_t Z29 : 1;
  size_t Z30 : 1;
  size_t Z31 : 1;

#ifdef _WIN64
  size_t ZReserved : 32;  //< x64 下高 32 位保留 。
#endif
};

/// 标准回调函数使用的 CPU 结构。
struct CPU_ST {
#ifndef _WIN64
  DWORD         regEdi;
  DWORD         regEsi;
  DWORD         regEbp;
  const DWORD   regEsp;     //< 注意 esp 不能修改，改了也没意义。
  DWORD         regEbx;
  DWORD         regEdx;
  DWORD         regEcx;
  DWORD         regEax;
  CPU_FLAGS     regflag;
  DWORD         regEip;
#else   // _WIN64
  DWORD64       regRcx;
  DWORD64       regRdx;
  DWORD64       regR8;
  DWORD64       regR9;
  DWORD64       regRax;
  DWORD64       regRbx;
  const DWORD64 regRsp;
  DWORD64       regRbp;
  DWORD64       regRsi;
  DWORD64       regRdi;
  DWORD64       regR10;
  DWORD64       regR11;
  DWORD64       regR12;
  DWORD64       regR13;
  DWORD64       regR14;
  DWORD64       regR15;
  CPU_FLAGS     regflag;
  DWORD64       regRip;
#endif  // _WIN64
  CPU_ST() = delete;
  CPU_ST(const CPU_ST&) = delete;
  CPU_ST& operator=(const CPU_ST&) = delete;
};

// 跨平台寄存器宏。
#ifndef _WIN64
#define regXdi regEdi
#define regXsi regEsi
#define regXbp regEbp
#define regXsp regEsp
#define regXbx regEbx
#define regXdx regEdx
#define regXcx regEcx
#define regXax regEax
#define regXip regEip
#else  // _WIN64
#define regXcx regRcx
#define regXdx regRdx
#define regXax regRax
#define regXbx regRbx
#define regXsp regRsp
#define regXbp regRbp
#define regXsi regRsi
#define regXdi regRdi
#define regXip regRip
#endif  // _WIN64

/// 回调函数格式。
#ifndef _WIN64
#define HookCalling __stdcall
#else
#define HookCalling
#endif
using HookRoutine = void(HookCalling*)(CPU_ST* lpcpu);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/**
  shellcode 缓冲分配器。

  \warning
    注意到：当 size() < 0x10 时，不触发，不分配空间。
*/
template <class T>
struct ShellCodeAllocator {
  using value_type = T;
  ShellCodeAllocator() = default;
  template <class U>
  constexpr ShellCodeAllocator(const ShellCodeAllocator<U>&) noexcept {}
  T* allocate(std::size_t n) {
    auto p = VirtualAlloc(nullptr, n * sizeof(T), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (nullptr != p) return (T*)p;
    throw std::bad_alloc();
  }
  void deallocate(T* p, std::size_t) noexcept {
    VirtualFree(p, 0, MEM_RELEASE);
  }
};

template <class T, class U>
bool operator==(const ShellCodeAllocator<T>&, const ShellCodeAllocator<U>&) {
  return true;
}
template <class T, class U>
bool operator!=(const ShellCodeAllocator<T>&, const ShellCodeAllocator<U>&) {
  return false;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/**
  相当于给 std::string 增加了三种 << 操作。

  \code
    // 接受顺序容器。
    Opcodes << std::string("\x90");
    Opcodes << std::array<char, 2>{'\x90', '\x90'});
    // 接受 数值、指针等。
    Opcodes << 0;
    Opcodes << (void*)0;
    // 接受字符串字面量。
    Opcodes << "\x90\x90"
  \endcode
*/
template <class Allocator>
class Opcodes
    : public std::basic_string<char, std::char_traits<char>, Allocator> {
 public:
  using std::basic_string<char, std::char_traits<char>, Allocator>::basic_string;

 public:
  template <class T>
  auto operator<<(const T& v)
      -> std::enable_if_t<std::is_pointer_v<decltype(v.data())>, Opcodes&> {
    this->append((const char*)v.data(), v.size() * sizeof(T::value_type));
    return *this;
  }
  template <class T>
  std::enable_if_t<!std::is_class_v<T>, Opcodes&> operator<<(const T& v) {
    this->append((const char*)&v, sizeof(v));
    return *this;
  }
  template <typename T, size_t size>
  Opcodes& operator<<(T const (&data)[size]) {
    this->append((const char*)data, sizeof(T) * (size - 1));
    return *this;
  }
};

using opcodes = Opcodes<std::allocator<char>>;
using shellcodes = Opcodes<ShellCodeAllocator<char>>;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/// xHook 用于指定 hook 位置 与 回调，生成对应钩子。
class xHook {
 public:
  /// 地址偏移， x86 与 x64 都是 4 byte 。
  using AddrDisp = uint32_t;
  using UEFRoutine = LPTOP_LEVEL_EXCEPTION_FILTER;

 public:
  /// 判定 HOOK 对象是否有效。
  bool IsOK() const { return IsHookOK(_e); }
  operator bool() const { return IsOK(); }
  /// 因为卸载钩子会有一些异常情况，强制卸载极可能会造成崩溃，所以允许指定错误时停止。
  HOOK_ERROR_ENUM UnHook(const bool safe) {
    // 无效的对象，不执行卸载流程。
    if (!IsOK()) return _e;

    // 尝试清除 UEF 。
    if ((UEFRoutine)-1 != _oldUEFHandling) {
      auto nowuef = SetUnhandledExceptionFilter(_oldUEFHandling);
      if (safe && (UEFRoutine)_uefshellcode.data() != nowuef) {
        SetUnhandledExceptionFilter(nowuef);
        return XHE_UEFCover;
      }
    }

    if (safe) {
      if (0 == memcmp(_hookmem, _oldcode.data(), _oldcode.size())) {
        return XHE_Restored;
      }
      if (0 != memcmp(_hookmem, _hookcode.data(), _hookcode.size())) {
        return XHE_BeCovered;
      }
    }

    _e = Crack(_hookmem, _oldcode);
    if (!IsOK()) return _e;

    // 正确卸载时，写入错误码，避免析构时重复卸载。
    return _e = XHE_UnHooked;
  }

 protected:
  /// 检测 写入位置。
  bool check_hookmem(void* hookmem, const size_t hooksize) {
    if (0 == hooksize) {
      _e = XHE_0Size;
      return false;
    }

    if (TRUE == IsBadReadPtr(hookmem, hooksize)) {
      _e = XHE_BadHookMem;
      return false;
    }

    _hookmem = hookmem;
    _hooksize = hooksize;

    _oldcode.assign((const char*)hookmem, hooksize);

    return true;
  }
  /// 检测 回调。
  bool check_routine(HookRoutine routine) {
    if (TRUE == IsBadReadPtr(routine, 1)) {
      _e = XHE_BadRoutine;
      return false;
    }
    _routine = routine;
    return true;
  }
  /// 调整 shellcode 。
  bool fix_shellcode(void* p_shellcode) {
    _lpshellcode = _shellcode.data();
    *(const void**)(_shellcode.data() + 2) = _lpshellcode;
    // 如果指定了 shellcode 的空间，则转移之。
    if (nullptr != p_shellcode) {
      *(const void**)(_shellcode.data() + 2) = p_shellcode;
      auto ret = Crack(p_shellcode, _shellcode);
      if (!IsHookOK(ret)) {
        _e = XHE_Move;
        return false;
      }
      _lpshellcode = p_shellcode;
    }
    return true;
  }
  /// UEF 回调。
  static inline LONG __cdecl UEFHandling(_EXCEPTION_POINTERS* ExceptionInfo,
                                         const xHook* o) {
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) {
#ifndef _WIN64
      if ((DWORD)o->_hookmem == ExceptionInfo->ContextRecord->Eip) {
        ExceptionInfo->ContextRecord->Eip = (DWORD)(o->_lpshellcode);
        return EXCEPTION_CONTINUE_EXECUTION;
      }
#else
      if ((DWORD64)o->_hookmem == ExceptionInfo->ContextRecord->Rip) {
        ExceptionInfo->ContextRecord->Rip = (DWORD64)(o->_lpshellcode);
        return EXCEPTION_CONTINUE_EXECUTION;
      }
#endif
    }
    if (nullptr == o->_oldUEFHandling) return EXCEPTION_CONTINUE_SEARCH;
    return o->_oldUEFHandling(ExceptionInfo);
  }
  /// 实现 UEF shellcode 。
  void mkuef() {
    _uefshellcode.clear();
    _hookcode.clear();
#ifndef _WIN64
    /*
    mov ecx, [esp + 4]
    mov edx, this
    mov eax, UEFHandling
    push edx
    push ecx
    call eax
    pop ecx
    pop edx
    retn 4
    */
    _uefshellcode
        << "\x8B\x4C\x24\x04"
        << '\xBA' << this
        << "\xB8" << &UEFHandling
        << "\x52\x51\xFF\xD0\x59\x5A\xC2\x04\x00";
#else
    /*
    // rcx 已经准备好。
    mov rdx, this
    mov rax, UEFHandling
    jmp rax
    // 注意到，仿造 x86 的 push ，UEFHandling 内部会出现奇怪的异常。
    */
    _uefshellcode
        << "\x48\xBA" << this
        << "\x48\xB8" << &UEFHandling
        << "\xFF\xE0";
#endif
    _oldUEFHandling = SetUnhandledExceptionFilter((UEFRoutine)_uefshellcode.data());
    _hookcode << '\xCC';
  }

 public:
  xHook() = delete;
  xHook(const xHook&) = delete;
  xHook& operator=(const xHook&) = delete;
  ~xHook() { UnHook(false); }
  /**
    普通 Hook 函数。指定 内存位置 与 回调函数，执行 Hook 操作。

    - x86 情况下：\n
      - 当 hooksize == 1 ~ 4 ：采用 UEF ，钩子不能被覆盖；
      - 5 ：采用 jmp XXX ，钩子不能被覆盖；
      - 6 以上：采用 jmp dword ptr [XXX] ，允许钩子覆盖。

    - x64 情况下：\n
      - 当 hooksize == 1 ~ 4 ：采用 UEF ，钩子不能被覆盖；
      - 5 ：采用 jmp XXX ，偏移超出采用 UEF 。钩子不能被覆盖；
      - 6-13 ：采用 jmp qword ptr [XXX] ，允许钩子覆盖。偏移超出采用 UEF
    ，钩子不能被覆盖；
      - 14 以上：采用 jmp qword ptr [XXX] ，允许钩子覆盖。

    \param  hookmem       指定 hook 内存的位置。
    \param  hooksize      hook 长度 [1, 30] 。
    \param  routine       指定的回调函数（声明请参考 HookRoutine ）
    \param  routinefirst  true ：回调先行， false ：覆盖代码先行。
    \param  p_shellcode   指定中转 shellcode 的存放位置。\n
                          x64 下用于避免偏移超出而自动采用 UEF 。\n
                          x86 下可忽略，为兼容而设计。但设置也不影响使用。\n
                          x86 请提供至少 0x1C + hooksize 大小的空间。\n
                          x64 请提供至少 0x3C + hooksize 大小的空间。

    \code
      #include "hook.h"
      void HookCalling Routine(CPU_ST* lpcpu)
        {
        return;
        }
      // 在 0x401000 下 5 byte JMP XXX 钩子，代码先行。
      xHook node(0x401000, 5, Routine, false);
      // 在 0x401000 下6 byte JMP [XXX] 钩子，回调先行。
      xHook node(0x401000, 6, Routine, true);
      // 在 0x401000 下 1 byte UEF 钩子，回调先行。
      xHook node(0x401000, 1, Routine, true);
      if(node)
        {
        cout << "下钩子出错，错误码：" << node._e;
        }
    \endcode

    \note
    - 在 hook 位置复用时，注意根据需要特别注意先后行的选择。
    - Routine 由于设计及安全上的考虑， Esp/Rsp 值不可改变（强改也失效）。
      其它改变都是允许的，但需要保证程序的继续运行，否则进程崩溃。
    - 在 Routine 内部期望修改 EIP/RIP 以达到流程控制效果时，谨慎选择前后行，
      推荐使用 "代码先行" ，否则在 EIP/RIP 被修改的情况下覆盖代码将不被执行。
    - 采用异常处理实现存在一定风险。在于：当顶层异常处理被其它处理覆盖时，
      钩子可能失效，并且这时恢复钩子，将破坏当前处理。(当然，概率较小)
    - 罕见使用：当希望使用 UEF 却又需要覆盖 hooksize >= 5 时，
      请在回调函数中设置 EIP/RIP 一样可以实现效果，而不能使 hooksize >= 5 实现 UEF 。
    - 由于没有添加偏移修正， Hook 范围内不能存在偏移地址，否则旧代码执行出错。
    - Hook 保证所有寄存器前后一致，除非回调内部特意修改。
  */
  xHook(void*         hookmem,
        const size_t  hooksize,
        HookRoutine   routine,
        const bool    routinefirst,
        void*         p_shellcode = nullptr) {
    //////////////////////////////////////////////////////////////// 基本检查 及
    ///初始化。
    if (!check_hookmem(hookmem, hooksize)) return;
    if (!check_routine(routine)) return;

    _ip = (const void*)((const char*)hookmem + hooksize);
    _lpshellcode = nullptr;
    _lpfixshellcode = (void*)ShellCodeNormal.data();
    _oldUEFHandling = (UEFRoutine)-1;
    //////////////////////////////////////////////////////////////// 设计
    ///shellcode 。
    _shellcode << '\xEB' << (char)sizeof(void*) << (void*)nullptr;

    if (!routinefirst) {
      _shellcode << _oldcode; // 代码前行需要先写原始代码。
    }
#ifndef _WIN64
    _shellcode
        << '\x68' << (AddrDisp)_ip
        << '\x68' << (AddrDisp)_routine
        << "\xFF\x15" << (AddrDisp)&_lpfixshellcode;

    if (routinefirst) {
      _shellcode << _oldcode;  // 代码后行后写原始代码。
    }

    _shellcode << "\xFF\x25" << (AddrDisp) & (_ip);
#else   // _WIN64
    _shellcode
        << '\xEB' << '\x18' << _ip << _routine << _lpfixshellcode
        << "\xFF\x35\xE2\xFF\xFF\xFF\xFF\x35\xE4\xFF\xFF\xFF\xFF\x15\xE6\xFF\xFF\xFF";

    if (routinefirst) {
      _shellcode << _oldcode;  // 代码后行后写原始代码。
      _shellcode << "\xFF\x25" << (AddrDisp)(0xFFFFFFD0 - hooksize);
    } else {
      _shellcode << "\xFF\x25" << (AddrDisp)0xFFFFFFD0;
    }
#endif  // _WIN64
    //////////////////////////////////////////////////////////////// 调整
    ///shellcode 。
    if (!fix_shellcode(p_shellcode)) return;
    //////////////////////////////////////////////////////////////// 计算
    ///hookcode 。
    switch (_hooksize) {
      case 1:      case 2:      case 3:      case 4:
        mkuef(); break;
      case 5: {
        const size_t addrdisp =
            CalcOffset((const char*)_hookmem + 1, _lpshellcode);
        if (!IsValidAddrDisp(addrdisp)) {
          mkuef();
          break;
        }
        _hookcode << '\xE9' << addrdisp;
        break;
      }
#ifndef _WIN64
      default:
        _hookcode << "\xFF\x25" << (AddrDisp)&_lpshellcode;
        break;
#else
      case 6:      case 7:      case 8:      case 9:
      case 10:     case 11:     case 12:     case 13: {
        const size_t addrdisp = CalcOffset((const char*)_hookmem + 2,
                                           (const char*)_lpshellcode + 2);
        if (!IsValidAddrDisp(addrdisp)) {
          mkuef();
          break;
        }
        _hookcode << "\xFF\x25" << (AddrDisp)addrdisp;
        break;
      }
      default:
        _hookcode << "\xFF\x25" << (AddrDisp)0 << _lpshellcode;
        break;
#endif
    }
    ////////////////////////////////////////////////////////////////
    _e = Crack(_hookmem, _hookcode);
  }
  template <typename T>
  xHook(T             hookmem,
        const size_t  hooksize,
        HookRoutine   routine,
        const bool    routinefirst,
        void*         p_shellcode = nullptr)
      : xHook((void*)hookmem, hooksize, routine, routinefirst, p_shellcode) {
    // 注意，不能在这里 xHook 。
  }
  /**
    指定 跳转表 或 call 偏移位置，执行 Hook 操作。

    \param  hookmem       跳转表位置或 call 偏移位置
    \param  routine       指定的回调函数（声明请参考 HookRoutine ）。
    \param  calltable_offset    指明是跳转表或 call 偏移。
    \param  routinefirst  true： 回调先行， false： 覆盖函数先行。
    \param  p_shellcode   指定中转 shellcode 的存放位置。\n
                          x64 下用于避免偏移超出而无法 HOOK 。\n
                          x86 下可忽略，为兼容而设计。但设置也不影响使用。\n
                          x86 请提供至少 0x1D 空间。\n
                          x64 请提供至少 0x5E 空间。
    \param  expandargc    覆盖函数可能存在参数过多的现象，以调整栈平衡。

    \code
      int __stdcall Test(int a, int b)
        {
        return a+b;
        }
      using func = int (__stdcall *)(int a, int b);
      func oldfunc = Test;
      void __stdcall Routine(CPU_ST* lpcpu)
        {
        ...
        }
      auto x = xHook((void*)&oldfunc, Routine, true, false);
      if(!x.IsOK())
        {
        cout << "下钩子出错，错误码：" << x._e;
        }
    \endcode

    \note
    - 覆盖跳转表或 call 偏移位置时，无视覆盖函数的调用格式。
    - 注意， routinefirst == true 时， EIP/RIP 为覆盖函数地址， Routine 可选择改变之，但此时请特别注意原函数的调用格式，改变 EIP/RIP 可跳过原函数的执行。routinefirst == false 时， EIP/RIP 为返回地址， Routine 也可选择改变之。
    - routinefirst == true 时， Hook 保证寄存器前后一致。 routinefirst == false 时， Hook 保证除 Esp/Rsp 外的寄存器在原函数调用前一致，局部环境一致。 Routine 调用寄存器前后一致。出 Hook 时寄存器一致，局部环境一致。（不因 Hook 而造成任何寄存器变动及栈飘移。）
    - 使用 MoveHookCallTableShellCode() 伪造返回地址。
    - 未特殊说明的情况，参考 Hook 函数声明。
  */
  xHook(void*           hookmem,
        HookRoutine     routine,
        const bool      calltable_offset,
        const bool      routinefirst,
        void*           p_shellcode = nullptr,
        const intptr_t  expandargc = 0) {
    //////////////////////////////////////////////////////////////// 基本检查 及
    ///初始化。
    const size_t hooksize =
        calltable_offset ? sizeof(hookmem) : sizeof(AddrDisp);
    if (!check_hookmem(hookmem, hooksize)) return;
    if (!check_routine(routine)) return;

    _ip = calltable_offset ? *(void**)hookmem
                           : (void*)(*(AddrDisp*)(hookmem) + (size_t)hookmem + sizeof(AddrDisp));
    _lpshellcode = nullptr;
    _lpfixshellcode = (void*)ShellCodeCtOff.data();
    _oldUEFHandling = (UEFRoutine)-1;
    //////////////////////////////////////////////////////////////// 设计
    ///shellcode 。
    constexpr intptr_t default_argc = 0x8;  // 默认参数 8 个。
    intptr_t hookargc = default_argc + expandargc;
    if (hookargc <= 0) hookargc = default_argc;  // 检测不让堆栈错误。

    _shellcode << '\xEB' << (char)sizeof(void*) << (void*)nullptr;

#ifndef _WIN64
    _shellcode
        << '\x68' << _ip
        << '\x68' << _routine
        << '\x68' << hookargc
        << '\x6A' << (uint8_t)routinefirst
        << "\xFF\x25" << (AddrDisp)&_lpfixshellcode;
#else
    _shellcode
        << "\xEB\x28"
        << _ip << _routine << hookargc
        << (void*)routinefirst << (void*)&_lpfixshellcode
        << "\xFF\x35\xD2\xFF\xFF\xFF\xFF\x35\xD4\xFF\xFF\xFF\xFF\x35\xD6\xFF\xFF\xFF"
           "\xFF\x35\xD8\xFF\xFF\xFF\xFF\x35\xDA\xFF\xFF\xFF"
           "\x48\x87\x3C\x24\x48\x8B\x3F\x48\x87\x3C\x24\xC3";
#endif
    //////////////////////////////////////////////////////////////// 调整
    ///shellcode 。
    if (!fix_shellcode(p_shellcode)) return;
    //////////////////////////////////////////////////////////////// 计算
    ///hookcode 。
    if (calltable_offset) {
      _hookcode << _lpshellcode;
    } else {
      const size_t addrdisp = CalcOffset(_hookmem, _lpshellcode);
      if (!IsValidAddrDisp(addrdisp)) {
        _e = XHE_AddDisp;
        return;
      }
      _hookcode << (AddrDisp)addrdisp;
    }
    ////////////////////////////////////////////////////////////////
    _e = Crack(_hookmem, _hookcode);
  }
  template <typename T>
  xHook(T               hookmem,
        HookRoutine     routine,
        const bool      calltable_offset,
        const bool      routinefirst,
        void*           p_shellcode = nullptr,
        const intptr_t  expandargc = 0)
      : xHook((void*)hookmem, routine, calltable_offset, routinefirst, p_shellcode, expandargc) {}

 public:
  /// 偏移计算。
  static inline size_t CalcOffset(const void* mem, const void* dest) {
    return (size_t)dest - (size_t)mem - sizeof(AddrDisp);
  }
  /// x64 检测是否有效的 AddrDisp ， x86 永远返回 true 。
  static inline bool IsValidAddrDisp(const size_t addrdisp) {
#ifdef _WIN64
    if ((intptr_t)addrdisp <= (intptr_t)(0x00000000FFFFFFFF) &&
        (intptr_t)addrdisp >= (intptr_t)(0xFFFFFFFF80000000)) {
      return true;
    }
    return false;
#else
    UNREFERENCED_PARAMETER(addrdisp);
    return true;
#endif
  }

 public:
  HOOK_ERROR_ENUM   _e;               //< 错误码。
  void*             _hookmem;         //< hook 内存位置。
  size_t            _hooksize;        //< hook 写入长度。
  HookRoutine       _routine;         //< 回调地址。
  const void*       _ip;              //< eip/rip 。
  const void*       _lpshellcode;     //< 指向 shellcode 。
  const void*       _lpfixshellcode;  //< 指向 fixshellcode 。
  shellcodes        _shellcode;       //< shellcode 缓冲。
  opcodes           _oldcode;         //< 覆盖前的数据（用于卸载时还原判定）。
  opcodes           _hookcode;        //< 覆盖后的数据（用于卸载时覆盖判定）。
  UEFRoutine        _oldUEFHandling;  //< UEF 旧回调。
  shellcodes        _uefshellcode;    //< UEF shellcode 。
 private:
#ifndef _WIN64
  static inline constexpr size_t ShellcodeNormalPrefix = 0x1C;
  static inline constexpr size_t ShellcodeCtOffPrefix = 0x1D;
  /**
    要求前置 shellcode 如下：

    \code
                                        --前置覆盖代码--
    $ ==>    >  68 XXXXXXXX            push    EIP
    $+5      >  68 XXXXXXXX            push    Routinue
    $+A      >  FF15 XXXXXXXX          call    dword ptr [ShellCodeNormal]
                                        --后置覆盖代码--
    $+10     >  FF25 XXXXXXXX          jmp     [EIP]
    $+16
    \endcode

    ```c++
    static __declspec(naked) void ShellCodeNormal()
      {
      __asm
        {
        push    dword ptr [esp + 4 * 2]       // 参数 EIP 。      ->ret routine
        pushfd
        pushad
        add     dword ptr [esp + 4 * 3], 4 * 5// 修正 esp 。      ->edi esi ebp ->fd eip ret routine eip

        push    esp
        call    dword ptr [esp + 4 * 12]      // 调用 Routine 。  ->esp ad fd eip ret

        mov     eax, dword ptr [esp + 4 * 9]  // 获取参数 EIP 。  ->ad fd
        cmp     eax, dword ptr [esp + 4 * 12] // 检测是否修改。   ->ad fd eip ret routine
        mov     dword ptr [esp + 4 * 12], eax // 修改 EIP 。     ->ad fd eip ret routine

        popad

        jz      HookShellCode_Normal_Next
        popfd
        lea     esp, dword ptr [esp + 4 * 3]  // 修改则跳过。     ->eip ret
    routine retn

      HookShellCode_Normal_Next :
        popfd
        lea     esp, dword ptr [esp + 4 * 1]  // 仅跳过 EIP ，返回执行可能存在的代码。
        retn    4 * 2

        add     byte ptr [eax], al            // 做 0 结尾。
        }
      }
    ```
  */
  static inline const shellcodes ShellCodeNormal{
      "\xFF\x74\x24\x08\x9C\x60\x83\x44\x24\x0C\x14\x54\xFF\x54\x24\x30"
      "\x8B\x44\x24\x24\x3B\x44\x24\x30\x89\x44\x24\x30\x61\x74\x06\x9D"
      "\x8D\x64\x24\x0C\xC3\x9D\x8D\x64\x24\x04\xC2\x08"};
  /**
    要求前置 shellcode 如下：

    \code
    $ ==>    >  68 XXXXXXXX            push    CoverCall
    $+5      >  68 XXXXXXXX            push    Routinue
    $+A      >  68 XXXXXXXX            push    HookArgc
    $+F      >  6A 00|01               push    routinefirst
    $+11     >  FF25 XXXXXXXX          jmp     dword ptr [ShellCodeCtOff]
    $+17
    \endcode

    ```c++
    static __declspec(naked) void ShellCodeCtOff()
      {
      __asm
        {
        pushfd

        xchg    eax, dword ptr [esp]          // eax 为 fd ，原始值入栈。
        xchg    eax, dword ptr [esp + 4 * 1]  // eax 为 routinefirst ， fd 向下移。
        test    eax, eax                      // 判定 routinefirst 。

        pop     eax                           // 还原 eax 。

        jnz     HookShellCode_CtOff_Transit

        popfd

        push    0x01230814                    // 做特征。
        push    esp
        push    0x42104210
        mov     dword ptr [esp + 4 * 1], esp  // 修正特征指向。

        pushfd
        pushad                                // 这里不用修正 esp ，修正也没意义。

        mov     esi, esp

        mov     ecx, dword ptr [esp + 4 * 12] // 获取 argc 。     ->ad fd # esp #
        mov     edx, ecx shl     ecx, 2

        mov     ebx, dword ptr [esp + 4 * 14] // 取得 CoverCall 。->ad fd # esp # argc routine

        sub     esp, ecx                      // 扩展局部栈。
        mov     edi, esp

        xor     ecx, ecx
        mov     cl, 9                         // 复制寄存器保护。
        rep movsd

        mov     cl, 7                         // 跳过。           -># esp # argc ro cov ret rep lodsd

        mov     ecx, edx                      // 参数复制。
        rep movsd

        push    ebx
        pop     ebx                           // 存放临时 CoverCall 。

        popad
        popfd

        call    dword ptr [esp - 4 * 10]      // call CoverCall   ->fd ad CoverCall

        push    edi
        push    esi
        push    ecx
        pushfd                                // 环境保护。

        mov     edi, esp                      // edi 指向状态保护。

        jmp     HookShellCode_CtOff_Chk
      HookShellCode_CtOff_Transit:
        jmp     HookShellCode_CtOff_Base

      HookShellCode_CtOff_Chk:
        add     esp, 4
        cmp     dword ptr [esp], 0x42104210
        jnz     HookShellCode_CtOff_Chk
        cmp     dword ptr [esp + 4], esp
        jnz     HookShellCode_CtOff_Chk
        cmp     dword ptr [esp + 8], 0x01230814
        jnz     HookShellCode_CtOff_Chk

        lea     esi, dword ptr [esp + 4 * 6]  // esi 指向 ret 。  -># esp # argc ro cov

        mov     ecx, dword ptr [esp + 4 * 3]  // 提取 argc 。     -># esp #
        shl     ecx, 2
        add     ecx, 4 * 11                   // 计算位移。       ->fd ecx esi edi # esp # argc ro cov ret

        lea     esp, dword ptr [edi + ecx]    // esp 指向 real_call_ret 。

        push    dword ptr [esi - 4 * 0]       // ret 。
        push    dword ptr [esi - 4 * 2]       // routine 。
        push    dword ptr [esi - 4 * 3]       // argc 。

        xchg    edi, esp
        popfd                                 // 恢复环境。
        pop     ecx
        pop     esi
        xchg    edi, esp
        mov     edi, dword ptr [edi]

        pushfd
      HookShellCode_CtOff_Base:
        popfd

        push    dword ptr [esp + 4 * 2]       // 参数 CoverCall 。->argc routine
        pushfd
        pushad
        add     dword ptr [esp + 4 * 3], 4 * 5// 修正 esp 。      ->edi esi ebp ->fd eip argc routine eip

        push    esp
        call    dword ptr [esp + 4 * 12]      // 调用 Routine 。  ->esp ad fd eip argc

        popad
        popfd

        push    eax
        mov     eax, dword ptr [esp + 4 * 1]  // 提取参数 CoverCall 。
        mov     dword ptr [esp + 4 * 4], eax  // 重写可能已被修改的 CoverCall 。
        pop     eax

        lea     esp, dword ptr [esp + 4 * 3]  // 跳过。           ->CoverCall argc routine
        retn

        add     byte ptr [eax], al            // 做 0 结尾。
        }
      }
    ```
  */
  static inline const shellcodes ShellCodeCtOff{
      "\x9C\x87\x04\x24\x87\x44\x24\x04\x85\xC0\x58\x75\x43\x9D\x68\x14"
      "\x08\x23\x01\x54\x68\x10\x42\x10\x42\x89\x64\x24\x04\x9C\x60\x8B"
      "\xF4\x8B\x4C\x24\x30\x8B\xD1\xC1\xE1\x02\x8B\x5C\x24\x38\x2B\xE1"
      "\x8B\xFC\x33\xC9\xB1\x09\xF3\xA5\xB1\x07\xF3\xAD\x8B\xCA\xF3\xA5"
      "\x53\x5B\x61\x9D\xFF\x54\x24\xD8\x57\x56\x51\x9C\x8B\xFC\xEB\x02"
      "\xEB\x3F\x83\xC4\x04\x81\x3C\x24\x10\x42\x10\x42\x75\xF4\x39\x64"
      "\x24\x04\x75\xEE\x81\x7C\x24\x08\x14\x08\x23\x01\x75\xE4\x8D\x74"
      "\x24\x18\x8B\x4C\x24\x0C\xC1\xE1\x02\x83\xC1\x2C\x8D\x24\x0F\xFF"
      "\x36\xFF\x76\xF8\xFF\x76\xF4\x87\xFC\x9D\x59\x5E\x87\xFC\x8B\x3F"
      "\x9C\x9D\xFF\x74\x24\x08\x9C\x60\x83\x44\x24\x0C\x14\x54\xFF\x54"
      "\x24\x30\x61\x9D\x50\x8B\x44\x24\x04\x89\x44\x24\x10\x58\x8D\x64"
      "\x24\x0C\xC3"};
#else
  static inline constexpr size_t ShellcodeNormalPrefix = 0x3C;
  static inline constexpr size_t ShellcodeCtOffPrefix = 0x5E;
  /**
    要求前置 shellcode 如下：

    \code
                                      --CoverCode--
    $ ==>    >  EB 18                  jmp     $+1A
    $+2      >  XXXXXXXXXXXXXXXX       [RIP]
    $+A      >  XXXXXXXXXXXXXXXX       [Routine]
    $+12     >  XXXXXXXXXXXXXXXX       [ShellCodeNormal]
    $+1A     >  FF35 E2FFFFFF          push    qword ptr [RIP]
    $+20     >  FF35 E4FFFFFF          push    qword ptr [Routine]
    $+26     >  FF15 E6FFFFFF          call    qword ptr [HookShellCodeNormal]
                                      --CoverCode--
    $+2C     >  FF25 D0FFFFFF?         jmp     [eip]
    $+32
    \endcode

    ```c++
      static __declspec(naked) void ShellCodeNormal()
        {
        __asm
          {
          push    qword ptr [rsp + 8 * 2]       // 参数 RIP 。      ->ret
    routine

          pushfq
          push    r15
          push    r14
          push    r13
          push    r12
          push    r11
          push    r10
          push    rdi
          push    rsi
          push    rbp
          lea     rbp, qword ptr [rsp + 8 * 10] // 指向参数 RIP 。  ->rbp rsi rdi r10-15 fq
          lea     rsi, qword ptr [rbp + 8 * 4]  // 指向 ret 。      ->rip ret routin rip
          push    rsi
          push    rbx
          push    rax
          push    r9
          push    r8
          push    rdx
          push    rcx

          mov     rcx, rsp
          push    r9
          push    r8
          push    rdx
          push    rcx
          mov     rax, qword ptr [rbp + 8 * 2]  // 提取 Routine 。  ->rip ret
          push    rax
          pop     rax
          call    qword ptr [rsp - 8 * 1]       // 调用 Routine 。
          pop     rcx                           // 弹出参数。
          pop     rdx
          pop     r8
          pop     r9

          pop     rcx
          pop     rdx
          pop     r8
          pop     r9
          pop     rax
          pop     rbx
          pop     rbp                           // pop rsp
          pop     rbp
          pop     rsi
          pop     rdi
          pop     r10
          pop     r11
          pop     r12
          pop     r13
          pop     r14
          pop     r15

          xchg    rax, qword ptr [rsp + 8 * 1]  // 取出参数 RIP 。  ->fq
          cmp     rax, qword ptr [rsp + 8 * 4]  // 检测是否修改。   ->fq rip ret routine
          mov     qword ptr [rsp + 8 * 4], rax  // 修改 EIP 。      ->fq rip ret routine
          xchg    rax, qword ptr [rsp + 8 * 1]  // 还原 rax 。

          jz      HookShellCode_Normal_Next
          popfq
          lea     rsp, qword ptr [rsp + 8 * 3]  // 修改则跳过。     -> rip ret routine retn

        HookShellCode_Normal_Next :
          popfq
          lea     rsp, qword ptr [rsp + 8 * 1]  // 仅跳过 RIP ，返回执行可能存在的代码。
          retn    8 * 2

          add     byte ptr [rax], al            // 做 0 结尾。
          }
        }
    ```
  */
  static inline const shellcodes ShellCodeNormal{
      "\xFF\x74\x24\x10\x9C\x41\x57\x41\x56\x41\x55\x41\x54\x41\x53\x41"
      "\x52\x57\x56\x55\x48\x8D\x6C\x24\x50\x48\x8D\x75\x20\x56\x53\x50"
      "\x41\x51\x41\x50\x52\x51\x48\x89\xE1\x41\x51\x41\x50\x52\x51\x48"
      "\x8B\x45\x10\x50\x58\xFF\x54\x24\xF8\x59\x5A\x41\x58\x41\x59\x59"
      "\x5A\x41\x58\x41\x59\x58\x5B\x5D\x5D\x5E\x5F\x41\x5A\x41\x5B\x41"
      "\x5C\x41\x5D\x41\x5E\x41\x5F\x48\x87\x44\x24\x08\x48\x3B\x44\x24"
      "\x20\x48\x89\x44\x24\x20\x48\x87\x44\x24\x08\x74\x07\x9D\x48\x8D"
      "\x64\x24\x18\xC3\x9D\x48\x8D\x64\x24\x08\xC2\x10\x00"};
  /**
    要求前置 shellcode 如下：

    \code
    $ ==>    >  EB 28                  jmp     $+2A
    $+2      >  XXXXXXXXXXXXXXXX       [CoverCall]
    $+A      >  XXXXXXXXXXXXXXXX       [Routine]
    $+12     >  XXXXXXXXXXXXXXXX       [HookArgc]
    $+1A     >  XXXXXXXXXXXXXXXX       [routinefirst]
    $+22     >  XXXXXXXXXXXXXXXX       [ShellCodeCtOff]
    $+2A     >  FF35 D2FFFFFF          push    qword ptr [CoverCall]
    $+30     >  FF35 D4FFFFFF          push    qword ptr [Routine]
    $+36     >  FF35 D6FFFFFF          push    qword ptr [HookArgc]
    $+3C     >  FF35 D8FFFFFF          push    qword ptr [routinefirst]
    $+42     >  FF35 DAFFFFFF          push    qword ptr [ShellCodeCtOff]
    $+48     >  48 873C 24             xchg    rdi, qword ptr [rsp]
    $+4C     >  48 8B3F                mov     rdi, qword ptr [rdi]
    $+4F     >  48 873C 24             xchg    rdi, qword ptr [rsp]
    $+53     >  C3                     ret
    $+54
    \endcode

    ```c++
    static __declspec(naked) void ShellCodeCtOff()
      {
      __asm
        {
        pushfq

        xchg    rax, qword ptr [rsp]          // rax 为 fq ，原始值入栈。
        xchg    rax, qword ptr [rsp + 8 * 1]  // rax 为 routinefirst ， fq 向下移。
        test    rax, rax pop     rax

        jnz     HookShellCode_CtOff_Transit

        popfq
        push    0x01230814                    // push imm 只允许 32 位，但入栈是 64 位。
        push    rsp                           // 注意后面会修正这个值，但不即时修正。
        push    0x42104210                    // 以上三个特征值为最后堆栈平衡设置。
        mov     qword ptr [rsp + 8 * 1], rsp  // 修正入栈 rsp ，以便最后栈平衡计算， rsp 指向特征。

        pushfq
        push    rdi
        push    rsi
        push    rdx
        push    rcx
        push    rax

        mov     rax, qword ptr [rsp + 8 * 11] // 取得 CoverCall 。->rax rcx rdx rsi rdi fq # rsp # argc ro
        mov     rcx, qword ptr [rsp + 8 * 9]  // 获取 argc 。     ->rax rcx rdx rsi rdi fq # rsp #

        push    rax                           // push covercall 。

        mov     rsi, rsp
        mov     rdx, rcx
        shl     rcx, 3
        sub     rsp, rcx                      // 扩展局部栈。
        mov     rdi, rsp

        xor     rcx, rcx
        mov     cl, 7                         // 复制寄存器保护。 ->covercall rax rcx rdx rsi rdi fq
        rep movsq

        mov     cl, 7                         // 跳过。           -># rsp # argc ro cov ret
        rep lodsq

        mov     rcx, rdx                      // 参数复制。
        rep movsq

        pop     rax
        pop     rax
        pop     rcx
        pop     rdx
        pop     rsi
        pop     rdi
        popfq
        call    qword ptr [rsp - 8 * 7]      // call CoverCall 。 ->covercall rax rcx rdx rsi rdi fq

        push    rdi
        push    rsi
        push    rcx
        pushfq
        mov     rdi, rsp

        jmp     HookShellCode_CtOff_Chk

      HookShellCode_CtOff_Transit:
        jmp     HookShellCode_CtOff_Base

      HookShellCode_CtOff_Chk:
        add     rsp, 8
        cmp     qword ptr [rsp], 0x42104210
        jnz     HookShellCode_CtOff_Chk
        cmp     qword ptr [rsp + 8], rsp
        jnz     HookShellCode_CtOff_Chk
        cmp     qword ptr [rsp + 8 * 2], 0x01230814
        jnz     HookShellCode_CtOff_Chk

        lea     rsi, qword ptr [rsp + 8 * 6]  // rsi 指向 ret 。  -># rsp # argc ro cov

        mov     rcx, qword ptr [rsp + 8 * 3]  // 提取 argc 。     -># rsp #
        shl     rcx, 3
        add     rcx, 8 * 11                   // 计算位移。       ->rcx rsi rdi fd # rsp # argc ro cov ret
        lea     rsp, dword ptr [rdi + rcx]    // rsp 指向 real_call_ret 。

        push    qword ptr [rsi - 8 * 0]       // ret 。
        push    qword ptr [rsi - 8 * 2]       // routine 。
        push    qword ptr [rsi - 8 * 3]       // argc 。

        xchg    rdi, rsp
        popfq                                 // 恢复环境。
        pop     rcx
        pop     rsi
        xchg    rdi, rsp
        mov     rdi, qword ptr [rdi]

        pushfq

      HookShellCode_CtOff_Base:
        popfq

        push    qword ptr [rsp + 8 * 2]       // 参数 CoverCall 。->argc routine
        pushfq
        push    r15
        push    r14
        push    r13
        push    r12
        push    r11
        push    r10
        push    rdi
        push    rsi
        push    rbp
        lea     rbp, qword ptr [rsp + 8 * 10] // 指向参数 CoverCall 。->rbp rsi rdi r10-15 fq
        lea     rsi, qword ptr [rbp + 8 * 4]  // 指向 ret 。  ->covercall argc routin covercall
        push    rsi                           // push rsp 。
        push    rbx
        push    rax
        push    r9
        push    r8
        push    rdx
        push    rcx

        mov     rcx, rsp
        push    rcx
        mov     rax, qword ptr [rbp + 8 * 2]  // 提取 Routine 。  ->covercall argc
        push    rax
        pop     rax
        call    qword ptr [rsp - 8 * 1]       // 调用 Routine 。
        pop     rcx                           // 弹出参数。

        pop     rcx
        pop     rdx
        pop     r8
        pop     r9
        pop     rax
        pop     rbx
        pop     rbp                           // pop rsp 。
        pop     rbp
        pop     rsi
        pop     rdi
        pop     r10
        pop     r11
        pop     r12
        pop     r13
        pop     r14
        pop     r15
        popfq

        push    rax
        mov     rax, qword ptr [rsp + 8 * 1]  // 提取参数 CoverCall 。
        mov     qword ptr [rsp + 8 * 4], rax  // 重写可能已被修改的 CoverCall 。
        pop     rax

        lea     rsp, qword ptr [rsp + 8 * 3]  // 跳过。           ->Covercall argc routine
        retn

        add     byte ptr [rax], al             // 做 0 结尾。
        }
      }
    ```
  */
  static inline const shellcodes ShellCodeCtOff{
      "\x9C\x48\x87\x04\x24\x48\x87\x44\x24\x08\x48\x85\xC0\x58\x75\x59"
      "\x9D\x68\x14\x08\x23\x01\x54\x68\x10\x42\x10\x42\x48\x89\x64\x24"
      "\x08\x9C\x57\x56\x52\x51\x50\x48\x8B\x44\x24\x58\x48\x8B\x4C\x24"
      "\x48\x50\x48\x89\xE6\x48\x89\xCA\x48\xC1\xE1\x03\x48\x2B\xE1\x48"
      "\x89\xE7\x48\x33\xC9\xB1\x07\xF3\x48\xA5\xB1\x07\xF3\x48\xAD\x48"
      "\x89\xD1\xF3\x48\xA5\x58\x58\x59\x5A\x5E\x5F\x9D\xFF\x54\x24\xC8"
      "\x57\x56\x51\x9C\x48\x89\xE7\xEB\x02\xEB\x4E\x48\x83\xC4\x08\x48"
      "\x81\x3C\x24\x10\x42\x10\x42\x75\xF2\x48\x39\x64\x24\x08\x75\xEB"
      "\x48\x81\x7C\x24\x10\x14\x08\x23\x01\x75\xE0\x48\x8D\x74\x24\x30"
      "\x48\x8B\x4C\x24\x18\x48\xC1\xE1\x03\x48\x83\xC1\x58\x48\x8D\x24"
      "\x0F\xFF\x36\xFF\x76\xF0\xFF\x76\xE8\x48\x87\xFC\x9D\x59\x5E\x48"
      "\x87\xFC\x48\x8B\x3F\x9C\x9D\xFF\x74\x24\x10\x9C\x41\x57\x41\x56"
      "\x41\x55\x41\x54\x41\x53\x41\x52\x57\x56\x55\x48\x8D\x6C\x24\x50"
      "\x48\x8D\x75\x20\x56\x53\x50\x41\x51\x41\x50\x52\x51\x48\x89\xE1"
      "\x41\x51\x41\x50\x52\x51\x48\x8B\x45\x10\x50\x58\xFF\x54\x24\xF8"
      "\x59\x5A\x41\x58\x41\x59\x59\x5A\x41\x58\x41\x59\x58\x5B\x5D\x5D"
      "\x5E\x5F\x41\x5A\x41\x5B\x41\x5C\x41\x5D\x41\x5E\x41\x5F\x9D\x50"
      "\x48\x8B\x44\x24\x08\x48\x89\x44\x24\x20\x58\x48\x8D\x64\x24\x18"
      "\xC3\x00"};
#endif
};

}  // namespace xlib

#endif  //_WIN32

#endif  //_XHOOK_H_