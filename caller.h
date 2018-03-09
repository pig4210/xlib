/*!
  \file  caller.h
  \brief caller.h用于灵活调用各类函数而无需关心函数类型等

  - 可自动识别应用于：对象调用普通函数　或　对象调用成员函数
  - 可完美应用于__stdcall、__cdcel与__thiscall、__fastcall，而无需关心堆栈平衡问题
  - x64下完美支持__fastcall。
  - 演化过程：ASM_thiscall >> ASM_thatcall >> mkCall >> SmartCall >> caller
  - 封装caller类，资源占用小，执行效率高（略低于原mkCall），不再要求DEP
  - 注意：caller类将原始堆栈使用调小（0x8个参数），寄存器参数不计入参数个数
  - 注意：caller必须初始化，请注意初始化的参数顺序，以及初始化设定的规则。
  - caller保证进入指定函数前，除指定寄存器外的所有寄存器一致。保证调用后除eax、edx(x86)外所有寄存器与调用caller前一致。caller返回64位值
  - caller支持远程调用：caller.request(...)，与普通调用一致。远程调用等待另一线程调用caller::execute_request()返回结果
  - 注意：Ring0下如果使用了远程调用，需要使用caller::freecallobj自行回收资源

  \version    9.0.1402.2611
  \note       For All

  \author     triones
  \date       2011-04-25
*/
#ifndef _XLIB_CALLER_H_
#define _XLIB_CALLER_H_

#include "xlib_base.h"

#ifdef _WIN32

#pragma warning(disable:4244) // warning C4244: 可能丢失数据。应用于int64转换

#ifndef _WIN64
typedef uint8 CallerArgType;
const CallerArgType arg_nul = 0;
const CallerArgType arg_eax = (1 << 0);
const CallerArgType arg_ecx = (1 << 1);
const CallerArgType arg_edx = (1 << 2);
const CallerArgType arg_ebx = (1 << 3);
//const CallerArgType arg_esp = (1 << 4);  //ESP不能被用作传参
const CallerArgType arg_ebp = (1 << 5);
const CallerArgType arg_esi = (1 << 6);
const CallerArgType arg_edi = (1 << 7);
#else  // _WIN64
typedef uint16 CallerArgType;
const CallerArgType arg_nul = 0;
const CallerArgType arg_rcx = (1 << 0);   //注意这样的设计才能使得传参顺序正确
const CallerArgType arg_rdx = (1 << 1);
const CallerArgType arg_r8 =  (1 << 2);
const CallerArgType arg_r9 =  (1 << 3);
const CallerArgType arg_rax = (1 << 4);
const CallerArgType arg_rbx = (1 << 5);
//const CallerArgType arg_rsp = (1<<6);   //RSP不能被用作传参
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

/*!

  \code
    caller tst(functions);
    tst(...);                                   // 纯函数调用
    caller tst(functions, 0, 0, 0, 0x10);
    tst(...);                                   // 纯函数调用，但扩展堆栈0x08+0x10==0x18
    caller tst(functions, this);
    tst(...);                                   // 纯this调用，this在初始化时传入
    caller tst(functions);
    tst[this](...);                             // 纯this调用，this在调用时指定
    caller tst(0xXXXX, this);
    tst(...);                                   // 纯this虚函数调用
    caller tst(functions, 0, 0, arg_eax);
    tst(eax_value, ...);                        // 普通调用，使用eax传递参数
    caller tst(functions, 0, 0, arg_eax | arg_esi);
    tst(eax_value, esi_value, ...);             // 普通调用，使用eax与esi传递参数(寄存器传参时，请注意顺序按照eax、ecx、edx...的顺序先后传递)
  \endcode
*/
class caller
  {
  public:
    //! 不定参调用
    uint64 operator()(...) const;
    //! 远程调用
    uint64 request(...) const;
    //! 允许临时改变this指针
    caller& changethis(void* lp_this);
    //! 重载操作允许在一个语句内完成this修改并调用
    caller& operator[](void* lp_this);
    //! 推荐使用这个模板构造函数初始化类。参数及意义、规则等参考init函数
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
    //! 移植fakevtcall(x86大约需要0xB0空间、x64大约需要0x120空间)
    /*!
      \param newmem    新fakevtcall位置，为nullptr时，还原原有位置
    */
    static void mov_call(void* newmem);
    //! 执行远程请求，用于执行请求队列中所有caller
    static void execute_request();
#ifdef FOR_RING0
    //! Ring0下用于回收资源
    static void freecallobj();
#endif

  protected:
    //! 初始化函数
    /*!
      \param func_vtno 函数地址或虚表偏移\n
                       <0x10000识别为偏移，否则识别为函数调用地址
      \param lp_this   对象this指针(默认为0)\n
                       ==arg_nul时，忽略thisregs
      \param this_regs this指针所使用的寄存器指示(只识别单个寄存器)\n
                       忽略reg_s中thisregs存在，即不能将this以参数形式传递，可以在初始化时指定，或使用changethis函数
      \param reg_s     可能存在的寄存器传参(可多个寄存器)
      \param expandargc  参数个数调整，正负数都可以，但如果将堆栈向下调整出界，则会重新使用默认参数个数
      \return          返回初始化成功与否
    */
    bool init(
      void*                 func_vtno,
      void*                 lp_this,
      const CallerArgType   this_regs,
      const CallerArgType   reg_s,
      const intptr_t        expandargc);
  private:
    //! 允许fakecall函数访问私有成员
#ifdef __INTEL_COMPILER
    friend uint64 fakecall(const caller* nowthis, void* argv);
#endif
  private:
    void*             caller_func;        //!< 函数地址，用以调用函数
    void*             caller_this;        //!< 对象指针，用以确定对象
    CallerArgType     caller_thisreg;     //!< __thiscall使用的寄存器
    CallerArgType     caller_regs;        //!< 寄存器传参指示
    uint8             caller_argc;        //!< 参数个数
  };

#endif  // _WIN32

#endif  // _XLIB_CALLER_H_