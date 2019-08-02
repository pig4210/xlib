/**
  \file  caller.h
  \brief 用于灵活调用各类函数而无需关心函数参数、类型等。

  \version    1.7.2.150721
  \note       For All

  \author     triones
  \date       2011-04-25

  \section more 额外说明

  - 可自动识别应用于：对象调用普通函数　或　对象调用成员函数。
  - 可完美应用于__stdcall、__cdcel与__thiscall、__fastcall，无需关心堆栈平衡问题。
  - x64 下完美支持 __fastcall 。
  - 演化过程： ASM_thiscall >> ASM_thatcall >> mkCall >> SmartCall >> caller 。
  - 封装 caller 类，资源占用小，执行效率高（略低于原 mkCall ），不再要求 DEP 。
  - 注意： caller 类将原始堆栈使用调小（ 0x8 个参数），寄存器参数不计入参数个数。
  - 注意： caller 必须初始化，请注意初始化的参数顺序，以及初始化设定的规则。
  - caller 保证进入指定函数前，除指定寄存器外的所有寄存器一致。保证调用后除 eax/rax、edx(x86) 外所有寄存器与调用 caller 前一致。 caller 返回 64 位值。
  - caller 支持远程调用： caller.request(...) ，与普通调用一致。远程调用等待另一线程调用 caller::execute_request() 返回结果。
  - 注意： Ring0 下如果使用了远程调用，需要使用 caller::freecallobj 自行回收资源。

  \section history 版本记录

  - 2010-07-29 发现 BUG ，动用约定寄存器可能影响运行环境，所以保护之。并进一步优化保护。 0.1 。
  - 2010-10-06 去除对首次包含需要定义宏的要求，把函数定义为 static 。 0.2 。
  - 2010-10-06 也去除了对参数容量的动态控制。如果对参数容量有更大要求，需要自行修改程序。
  - 2010-10-06 不过， 0x10 个参数，应该够多了。另外，加入 thiscall 的包含。
  - 2010-11-05 移植入 LIB ，做相应改造。对常用情况做常用处理。 0.3 。
  - 2010-11-06 套用 netline ，提升成 mkCall ，重新恢复对参数容量的动态控制。 0.4 。
  - 2010-11-30 源码作一些改动，但不会影响实际效果。 0.4.1 。
  - 2010-12-10 改造 mkcallflags 为 mkmkCallFlag 。 0.5 。
  - 2011-02-19 改造 mkCall 内部使用 line 类（原使用 netline 类）。 0.6 。
  - 2011-04-22 新增 SmartCall 类。 0.7 。
  - 2011-04-23 新增 caller 类。 0.8 。
  - 2011-04-23 原 SmartCall 类内存资源占用较大，但执行效率高。
  - 2011-04-23 caller 类内存资源占用小，但执行效率较低。（与 mkCall 基本一致）
  - 2011-04-23 预备将 ASM_Call 完全提升为 caller ，暂时共存。过段时间 caller 经求大量测试无误后，将完全替代原 ASM_Call 。
  - 2011-06-25 正式移除 ASM_Call 、 SmartCall 。将全文件改名 caller 。 1.0 。
  - 2011-06-25 调整 caller_regs 为 unsigned char 以减少 caller 大小。
  - 2011-06-25 调整 arg_R32 类型为 unsigned char 。
  - 2011-06-26 新增 arg_nul 常量。大幅修改 fakevtcall 函数，不再要求预先写入 this 指针。 1.1 。
  - 2011-06-26 新增 changethis 函数，允许临时改变 this 指针。
  - 2011-06-26 但是特别注意的是：这次修改需要运行时读取虚成员函数偏移，再次把崩溃风险引 fakevtcall 。
  - 2011-06-26 这次改动，使寄存器参数不再计入参数个数范围。
  - 2011-09-24 新增 [] 操作符用以改变 this 指针。这样使得临时改变 this 并调用函数变成： xx[this](...); 。 1.2 。
  - 2011-09-27 发现上一版本产生 00 编码，今已去除。同时加上 00 编码结尾，以方便移植计算。 1.2.1 。
  - 2011-09-27 新增 mov_vt 函数，用以移植。引入 offseter 与 objer 类。 1.3 。
  - 2011-10-22 修正上上版本升级时，类初始化未去除强制要求 this 指针的判定。 1.3.1 。
  - 2011-12-14 移入 COMMON ，去除 offseter 与 objer 类。把类 static 变量转移入 CPP 。 1.4 。
  - 2012-05-22 小动作，把改变虚表的操作封装成 mkvt 函数。 1.4.1 。
  - 2012-10-09 重新设计 caller 的 shellcode ，寄存器保护更加严格。
  - 2012-10-09 使 caller 返回值为 uint64 。 1.5 。
  - 2013-03-02 去除虚函数表的修改。去除默认构造，强制 caller 构造。
  - 2013-03-02 不定参调用不再使用虚函数。新加远程调用的支持。 1.6 。
  - 2013-12-09 发现远程调用时，寄存器参数个数没有实现不计，修正之。 1.6.1 。
  - 2014-02-26 完成 xlib 移植， shellcode 作了一些优化。 1.7 。
  - 2014-08-26 shellcode 修正。 1.7.1 。
  - 2015-07-21 发现 x64 下回调函数内部可能利用多余参数空间，故修改 x64 的 shellcode 以适应。 1.7.2 。
*/
#ifndef _XLIB_CALLER_H_
#define _XLIB_CALLER_H_

#include "xlib_base.h"

#ifdef _WIN32

#pragma warning(disable:4244) // warning C4244: 可能丢失数据。应用于 int64 转换。

#ifndef _WIN64
typedef uint8 CallerArgType;
const CallerArgType arg_nul = 0;
const CallerArgType arg_eax = (1 << 0);
const CallerArgType arg_ecx = (1 << 1);
const CallerArgType arg_edx = (1 << 2);
const CallerArgType arg_ebx = (1 << 3);
//const CallerArgType arg_esp = (1 << 4);  // ESP 不能被用作传参。
const CallerArgType arg_ebp = (1 << 5);
const CallerArgType arg_esi = (1 << 6);
const CallerArgType arg_edi = (1 << 7);
#else  // _WIN64
typedef uint16 CallerArgType;
const CallerArgType arg_nul = 0;
const CallerArgType arg_rcx = (1 << 0);   // 注意这样的设计才能使得传参顺序正确。
const CallerArgType arg_rdx = (1 << 1);
const CallerArgType arg_r8 =  (1 << 2);
const CallerArgType arg_r9 =  (1 << 3);
const CallerArgType arg_rax = (1 << 4);
const CallerArgType arg_rbx = (1 << 5);
//const CallerArgType arg_rsp = (1<<6);   // RSP 不能被用作传参。
const CallerArgType arg_rbp = (1 << 7);
const CallerArgType arg_rsi = (1 << 8);
const CallerArgType arg_rdi = (1 << 9);
const CallerArgType arg_r10 = (1 << 10);
const CallerArgType arg_r11 = (1 << 11);
const CallerArgType arg_r12 = (1 << 12);
const CallerArgType arg_r13 = (1 << 13);
const CallerArgType arg_r14 = (1 << 14);
const CallerArgType arg_r15 = (1 << 15);
const CallerArgType arg_x64 = arg_rcx | arg_rdx | arg_r8 | arg_r9;
#endif  // _WIN64

/**

  \code
    caller tst(functions);
    tst(...);                                   // 纯函数调用。
    caller tst(functions, 0, 0, 0, 0x10);
    tst(...);                                   // 纯函数调用，但扩展堆栈 0x08 + 0x10 == 0x18 。
    caller tst(functions, this);
    tst(...);                                   // 纯 this 调用， this 在初始化时传入。
    caller tst(functions);
    tst[this](...);                             // 纯 this 调用， this 在调用时指定。
    caller tst(0xXXXX, this);
    tst(...);                                   // 纯 this 虚函数调用。
    caller tst(functions, 0, 0, arg_eax);
    tst(eax_value, ...);                        // 普通调用，使用 eax 传递参数。
    caller tst(functions, 0, 0, arg_eax | arg_esi);
    tst(eax_value, esi_value, ...);             // 普通调用，使用 eax 与 esi 传递参数。（寄存器传参时，请注意顺序按照 eax 、 ecx 、 edx ... 的顺序先后传递。）
  \endcode
*/
class caller
  {
  public:
    /// 不定参调用。
    uint64 operator()(...) const;
    /// 远程调用。
    uint64 request(...) const;
    /// 允许临时改变 this 指针。
    caller& changethis(void* lp_this);
    /// 重载操作允许在一个语句内完成 this 修改并调用。
    caller& operator[](void* lp_this);
    /// 推荐使用这个模板构造函数初始化类。参数及意义、规则等参考 init 函数。
    template<typename T> caller(
      const T&              func_vtno,
      void* const           lp_this       = 0,

#ifndef _WIN64
      const CallerArgType   this_regs     = arg_ecx,
      const CallerArgType   reg_s         = arg_nul,
#else
      const CallerArgType   this_regs     = arg_nul,
      const CallerArgType   reg_s         = arg_x64,
#endif
      const intptr_t        expandargc    = 0)
      {
      init((void*)func_vtno, lp_this, this_regs, reg_s, expandargc);
      }
  public:
    /// 移植 fakevtcall （ x86 大约需要 0xB0 空间、 x64 大约需要 0x120 空间）。
    /**
      \param newmem    新 fakevtcall 位置，为 nullptr 时，还原原有位置。
    */
    static void mov_call(void* newmem);
    /// 执行远程请求，用于执行请求队列中所有 caller 。
    static void execute_request();
#ifdef FOR_RING0
    /// Ring0 下用于回收资源。
    static void freecallobj();
#endif

  protected:
    /// 初始化函数。
    /**
      \param func_vtno 函数地址或虚表偏移。\n
                       < 0x10000 识别为偏移，否则识别为函数调用地址。
      \param lp_this   对象 this 指针（默认为 0 ）。\n
                       == arg_nul 时，忽略 thisregs 。
      \param this_regs this 指针所使用的寄存器指示（只识别单个寄存器）。\n
                       忽略 reg_s 中 thisregs 存在，即不能将 this 以参数形式传递，可以在初始化时指定，或使用 changethis 函数。
      \param reg_s     可能存在的寄存器传参（可多个寄存器）。
      \param expandargc  参数个数调整，正负数都可以，但如果将堆栈向下调整出界，则会重新使用默认参数个数。
      \return          返回初始化成功与否。
    */
    bool init(
      void*                 func_vtno,
      void*                 lp_this,
      const CallerArgType   this_regs,
      const CallerArgType   reg_s,
      const intptr_t        expandargc);
  private:
    /// 允许 fakecall 函数访问私有成员。
#ifdef __INTEL_COMPILER
    friend uint64 fakecall(const caller* nowthis, void* argv);
#endif
  private:
    void*             caller_func;        ///< 函数地址，用以调用函数。
    void*             caller_this;        ///< 对象指针，用以确定对象。
    CallerArgType     caller_thisreg;     ///< __thiscall 使用的寄存器。
    CallerArgType     caller_regs;        ///< 寄存器传参指示。
    uint8             caller_argc;        ///< 参数个数。
  };

#endif  // _WIN32

#endif  // _XLIB_CALLER_H_