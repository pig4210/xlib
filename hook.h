/**
  \file  hook.h
  \brief hook.h用于执行指定hook、unhook操作

  \version    5.1.1.190614
  \note       For All

  \author     triones
  \date       2010-08-27
  
  \section more 额外说明

  - 当在 RING0 下，工程请关闭【C++异常】及【缓冲区检查】，否则将链接进VC运行时库导致驱动加载失败！
  - 由于链接器的【增量链接】可能会造成在使用 Hook 时出现一些莫名其妙的状况，请将之关闭！
  - Ring3 卸载时钩子自清除， Ring0 需要自行调用 HookClear() 。

  \section history 版本记录

  - 2010-07-05 为了增加对 Ring0 的支持，以及新加一些功能，重新改写这个 Crack 。 1.0 。
  - 2010-07-05 由于 Ring0 的局限性， vector 无法使用，重新启用 ver 1.0 的自建结构。 1.1 。
  - 2010-07-05 自建结构为单向链表。对自建结构做了一些改进。 1.2 。
  - 2010-07-05 由于驱动的局限性，不再实现 DLL 或 LIB ，直接写成代码，使用包含之。
  - 2010-07-06 初步完成，以后需要详细测试。
  - 2010-07-08 加进对跳转表下 HOOK 的功能。 1.3 。
  - 2010-08-20 新加一个标志： nounloadchk ，并作一些相应更改。 1.3.1 。
  - 2010-08-26 认识到 FixJMP XX 的覆盖危险性，调整默认为 JMPDD 。这样也与自动决策一致。 1.3.2 。
  - 2010-08-27 由于 Ring0 下的一些需求，如 SSDT hook 或 inline hook 等。再次改写这个 Crack 。 2.0 。
  - 2010-08-27 不再提供用户选择错误返回。有错误一律在 DebugView 输出。
  - 2010-08-27 Crack 函数返回值为 Cracknode 类。
  - 2010-08-27 提供 GetLastCrackError() 以返回最后一次错误信息，去除 IsCrackID 宏判断。
  - 2010-08-27 增加新功能：原始字节绕过。
  - 2010-08-27 增加新功能：钩子延迟执行。提供 CrackExec() 以驱动延迟钩子执行。但会做匹配判断，以免在此期间目标位置数据变化。
  - 2010-08-27 调动 nounloadchk 至全局标志。
  - 2010-08-27 加入：卸载时，钩子被外力还原，继续释放资源的判断。
  - 2010-08-27 如果钩子是延迟加载，则在使用 execute() 成员函数执行时，错误信息不会被输出，只能自行获取。
  - 2010-11-24 转移到 LIB ，没有根本改动 。
  - 2011-11-11 转移入 COMMON ，核心不变，外壳重新设计 Hook ，新加一些功能，删除一些冗余功能。 3.0 。
  - 2011-11-11 另外 Ring0 下的一些特殊操作还未添加。
  - 2012-03-23 修改 Hookit 的 Ring3 版本，不再调用 WriteProcessMemory 。 3.1 。
  - 2012-08-06 新增 ReplaceHook 函数以支持对跳转表的 Hook 。 3.2 。
  - 2012-09-25 一直有考虑开放 HookNode 定义，再次放弃，为记。 3.3 。
  - 2012-09-25 CheckMemFromList 引入 xblk 判定。 shellcode 组织调整。 ReplaceHook 改名 Hook 重载。
  - 2012-09-25 考虑增加相对偏移的 Hook ，但方案不成熟，暂不施行。
  - 2012-09-28 新增重载 Hook 用于跳转表、偏移的 Hook 。 3.4 。
  - 2012-10-08 优化原始 Hook 的 shellcode 。删除原跳转表 Hook ，由全新 Hook 代替。 3.5 。
  - 2013-04-12 新加 Nopit 函数 。
  - 2014-01-23 重新设计 Hook ，原打算分解模块，尝试不满意，还原之。内核重新设计。 3.6 。
  - 2014-01-25 x64 的支持初步完成。但实际操作过程不如意，有待完善。 3.7 。
  - 2014-02-11 内核再次重新设计，初步完成，通过基本测试。 4.0 。
  - 2014-08-26 修正 Ring3 钩子自动卸载不完全的 BUG 。 4.0.1 。
  - 2016-06-12 添加 Hook2Log 模块。 5.0 。
  - 2016-06-12 由于钩子自动卸载机制，无法将 Hook2Log 独立出来，因为可能会使资源重复回收致使崩溃。
  - 2017-07-27 改进 Hook2Log 的 ShellCode 写法。 5.1 。
  - 2019-06-14 添加跨平台寄存器宏。注意不使用 union 以保持代码清晰。 5.1.1 。
*/
#ifndef _XLIB_HOOK_H_
#define _XLIB_HOOK_H_

#ifdef _WIN32

#include "xlib_base.h"

#if defined(FOR_RING0) && defined(__EXCEPTIONS)
#   error "内核模式不支持C++异常，需关闭之"
#endif

/// Hook 错误码 。（错误码不添加预处理指令，会造成显示值与实际值不符。）
enum HookErrCode
  {
  HookErr_Success,                  ///< 成功完成。
  HookErr_HookMem_Read_Fail,        ///< 尝试读取 hookmem 失败（一般是地址非法）。
  HookErr_ProtectVirtualMemory_Fail,///< 尝试 ZwProtectVirtualMemory 出错。
  HookErr_MmCreateMdl_Fail,         ///< 尝试 MmCreateMdl 出错。
  HookErr_MmMapLockedPages_Fail,    ///< 尝试 MmMapLockedPages 出错。
  HookErr_HookMem_Write_Fail,       ///< 尝试写入失败（一般是权限不足）。
  HookErr_DeleteNode_Fail,          ///< 尝试删除结点失败（一般是资源不足）。
  HookErr_MemCannotCover,           ///< 地址无法再次覆盖（已存在不可覆盖的钩子）。
  HookErr_MemCoverChk_Fail,         ///< 检测地址覆盖出错（链表操作出错）。
  HookErr_AddNode_Fail,             ///< 加入结点失败（一般是资源不足）。
  HookErr_SetUEF_Fail,              ///< 设置异常处理回调失败。
  HookErr_ClearUEF_Fail,            ///< 清除异常处理回调失败。
  HookErr_ClearUEF_Cover,           ///< 清除异常处理回调被覆盖。
  HookErr_HookSize_OverFlow,        ///< hooksize 超限（限制最大为 15*2 ）。
  HookErr_HookSize_Zero,            ///< hooksize 为 0 。
  HookErr_Routine_Illegal,          ///< Routin 地址非法。
  HookErr_MakeNode_Fail,            ///< 生成 HookNode 失败（一般是资源不足）。
  HookErr_MakShellCode_Fail,        ///< 生成 shellcode 失败（一般是资源不足）。
  HookErr_AntiShellCodeDEP_Fail,    ///< 清除 shellcode 的 DEP 失败（ ZwProtectVirtualMemory 出错）。
  HookErr_FixShellCode_Fail,        ///< 调整 shellcode 失败。
  HookErr_Ring0_NoUEF,              ///< 内核无顶层异常。
  HookErr_AddDisp_Invalid,          ///< x64 下，偏移无效。
  HookErr_FixJmpCode_Fail,          ///< 设置 jmpcode 失败（一般是结点错误）。
  HookErr_Hook_Fail,                ///< HOOK 出错。
  HookErr_UnHook_Restore,           ///< 钩子已经被还原（钩子结点会被删除）。
  HookErr_UnHook_BeCover,           ///< 钩子位置被覆盖。
  HookErr_UnHook,                   ///< 还原钩子失败。
  HookErr_Clear,                    ///< 清除钩子链表失败。
  HookErr_MovShellCode_Fail,        ///< 转移 shellcode 失败（一般是无效地址）。
  // 以下是 Hook to Log 的错误信息。
  HookErr_Syntax,                   ///< 语法错误。
  HookErr_EmptyExpression,          ///< 空表达式。
  HookErr_InvaildModuleName,        ///< 无效模块名。
  HookErr_InvaildProcAddress,       ///< 无效模块函数名。
  HookErr_NoMatch,                  ///< 不匹配的 ()/[] 。
  HookErr_MatchEmpty,               ///< 空的 ()/[] 。
  HookErr_NeedLeftOp,               ///< 缺少左操作数。
  HookErr_NeedRightOp,              ///< 缺少右操作数。
  HookErr_InvailExpression,         ///< 非法表达式（非寄存器名或非模块名）。
  HookErr_NeedValue,                ///< 需要表达式值。
  HookErr_MoreExpression,           ///< 表达式未用操作符连接。
  HookErr_Shl,                      ///< 不匹配的 << 。
  HookErr_Shr,                      ///< 不匹配的 >> 。
  };

/**
  取得 Hook 执行错误码。
  \return   参考 HookErrCode 的说明。
*/
HookErrCode GetLastHookErr();

/**
  指定地址以及缓冲区，写入内存。
  \param  mem       指定地址。
  \param  hookcode  缓冲区。
  \param  len       缓冲区大小。
  \return           写操作的执行结果，返回 false 时，请使用 GetLastHookErr 查询。
*/
bool Hookit(LPVOID mem, LPCVOID hookcode, const size_t len);

/**
  指定地址以及大小， NOP 之。
  \param  mem       指定地址。
  \param  len       缓冲区大小。
  \return           写操作的执行结果，返回 false 时，请使用 GetLastHookErr 查询。
*/
bool Nopit(LPVOID mem, const size_t len);

/// 地址偏移， x86 与 x64 都是 4 byte 。
typedef uint32 AddrDisp;

/**
  偏移计算。
  \param  mem   需要偏移处地址。
  \param  dest  跳转目标处地址。
  \return       偏移值。
*/
size_t CalcOffset(const void* mem, const void* dest);

/// x64 检测是否有效的 AddrDisp ， x86 永远返回 true 。
bool IsValidAddrDisp(const size_t addrdisp);

/// 前置声明，不开放定义。
class HookNode;

/// 标准回调函数使用的 CPU 结构。
#pragma warning(push)
#pragma warning(disable:4510)  // C4510: 未能生成默认构造函数。
#pragma warning(disable:4512)  // C4512: 未能生成赋值运算符。
#pragma warning(disable:4610)  // C4610: 永远不能实例化。
struct CPU_ST
  {
#ifndef _WIN64
  DWORD         regEdi;
  DWORD         regEsi;
  DWORD         regEbp;
  const DWORD   regEsp;    ///< 注意 esp 不能修改，改了也没意义。
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
  };
#pragma warning(pop)

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
#else   // _WIN64
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
typedef void(__stdcall *HookRoutine)(CPU_ST* lpcpu);
#else
typedef void(*HookRoutine)(CPU_ST* lpcpu);
#endif

/**
  普通 Hook 函数。指定内存位置与回调函数，执行 Hook 操作。

  - x86 情况下：\n
    - 当 hooksize == 1 ~ 4 ：采用 UEF ，有强制钩子覆盖判定，不可下在已有钩子范围内；
    - 5 ：采用 jmp XXX ，有强制钩子覆盖判定，不可下在已有钩子范围内；
    - 6 以上：采用 jmp dword ptr [XXX] ，允许钩子覆盖。

  - x64 情况下：\n
    - 当 hooksize == 1 ~ 4 ：采用 UEF ，有强制钩子覆盖判定，不可下在已有钩子范围内；
    - 5 ：采用 jmp XXX ，偏移超出采用 UEF 。有强制钩子覆盖判定，不可下在已有钩子范围内；
    - 6-13 ：采用 jmp qword ptr [XXX] ，偏移超出采用 UEF 。采用 UEF 时，有覆盖判定；
    - 14 以上：采用 jmp qword ptr [XXX] 。

  \param  hookmem       指定 hook 内存的位置。
  \param  hooksize      hook 长度 [1, 30] 。
  \param  routine       指定的回调函数（声明请参考 HookRoutine ）
  \param  routinefirst  true ：回调先行， false ：覆盖代码先行。
  \param  p_shellcode   指定中转 shellcode 的存放位置。\n
                        x64 下用于避免偏移超出而自动采用 UEF 。\n
                        x86 下可忽略，为兼容而设计。但设置也不影响使用。\n
                        x86 请提供至少 0x1C + hooksize 大小的空间。\n
                        x64 请提供至少 0x3C + hooksize 大小的空间。
  \return               钩子结点指针，用于 UnHook 。\n
                        当执行失败时，返回 nullptr 。此时应调用 GetLastHookErr 得到失败原因。

  \code
    #include "hook.h"
    void __stdcall Routine(CPU_ST* lpcpu)
      {
      return;
      }
    // 在 0x401000 下 5 byte JMP XXX 钩子，代码先行。
    HookNode* node = Hook((void*)0x401000, 5, Routine, false);
    // 在 0x401000 下6 byte JMP [XXX] 钩子，回调先行。
    HookNode* node = Hook((void*)0x401000, 6, Routine, true);
    // 在 0x401000 下 1 byte UEF 钩子，回调先行。
    HookNode* node = Hook((void*)0x401000, 1, Routine, true);
    if(node == nullptr)
      {
      cout << "下钩子出错，错误码：" << GetLastHookErr();
      }
  \endcode

  \note
  - 在 hook 位置复用时，注意根据需要特别注意先后行的选择。
  - Routine 由于设计及安全上的考虑， Esp/Rsp 值的改变将被忽略。
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
HookNode* Hook(void*              hookmem,
               const size_t       hooksize,
               HookRoutine        routine,
               const bool         routinefirst,
               void*              p_shellcode = nullptr);

 /**
  指定跳转表或 call 偏移位置，执行 Hook 操作。

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
  \return               钩子结点指针，同样可以用 UnHook 卸载钩子。\n
                        当执行失败时，返回 nullptr 。此时应调用 GetLastHookErr 得到失败原因。

  \code
    #include "hook.h"
    int __stdcall Test(int a, int b)
      {
      return a+b;
      }
    typedef int (__stdcall *func)(int a, int b);
    func oldfunc = Test;
    void __stdcall Routine(CPU_ST* lpcpu)
      {
      ...
      }
    HookNode* node = Hook((void*)&oldfunc, Routine, true, false);
    if(node == nullptr)
      {
      cout << "下钩子出错，错误码：" << GetLastHookErr();
      }
  \endcode

  \note
  - 覆盖跳转表或 call 偏移位置时，无视覆盖函数的调用格式。
  - 注意， routinefirst == true 时， EIP/RIP 为覆盖函数地址， Routine 可选择改变之，但此时请特别注意原函数的调用格式，改变 EIP/RIP 可跳过原函数的执行。routinefirst == false 时， EIP/RIP 为返回地址， Routine 也可选择改变之。
  - routinefirst == true 时， Hook 保证寄存器前后一致。 routinefirst == false 时， Hook 保证除 Esp/Rsp 外的寄存器在原函数调用前一致，局部环境一致。 Routine 调用寄存器前后一致。出 Hook 时寄存器一致，局部环境一致。（不因 Hook 而造成任何寄存器变动及栈飘移。）
  - 使用 MoveHookCallTableShellCode() 伪造返回地址。
  - 未特殊说明的情况，参考 Hook 函数声明。
 */
 HookNode* Hook(void*             hookmem,
                HookRoutine       routine,
                const bool        calltable_offset,
                const bool        routinefirst,
                void*             p_shellcode = nullptr,
                const intptr_t    expandargc = 0);

/**
  UnHook 函数。\n
  指定 HookNode 结点指针，执行 UnHook 操作。\n
  因为卸载钩子会有一些异常情况，强制卸载极可能会造成崩溃，所以需要指定错误停止。

  \param  node      指定 HookNode 结点指针。
  \param  errbreak  指定在卸载钩子遭遇错误时，是否停止卸载。
  \return           当执行失败时，调用 GetLastHookErr 得到失败原因。
    - \b HookErr_Success　      操作成功完成。
    - \b HookErr_UnHook_Restore 钩子位置已经被还原（注意这只是一个 Warning 不影响卸载的安全性）。
    - \b HookErr_UnHook_BeCover 钩子位置被覆盖（强制卸载有风险，可停止卸载）。
    - \b HookErr_UnHook_Write   还原钩子时，写入失败（一般是权限不足）。
    - \b HookErr_Del_NoExist    钩子不存在于链表（注意这只是 Warning ，卸载已完成）。
    - \b HookErr_Unkown         未知异常（可能是个非法的 HookNode ）。

  \code
    #include "hook.h"
    HookNode* node = Hook((void*)0x401000, 5, Routin, false);
    if(node == nullptr) return;
    cout << "卸载钩子：" << UnHook(node, false);
  \endcode

*/
bool UnHook(HookNode* node, const bool errbreak);

/**
  还原全部钩子。 \n
  注意 HookClear 执行的是强制卸载操作，会尝试卸载链表中的每个钩子。\n
  一般应用于动态库卸载的清理流程中。\n
  Ring3 下，如果对卸载时机没有特殊要求，宿主卸载时，钩子链表会自动卸载。

  \return   当执行失败时，调用 GetLastHookErr 得到失败原因。
*/
bool HookClear();

/**
  用于伪造 call 先行时的返回地址。
  x86 情况下， mem 至少提供 B6 空间。 x64 情况下， mem 至少提供 126 空间。
  \param  mem       指定转移地址。当为 nullptr 时，还原。
  \return           当执行失败时，调用 GetLastHookErr 得到失败原因。
*/
bool MoveHookCallTableShellCode(void* mem);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// 以下是附加的 Hook to Log 。
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// 输出回调格式。
typedef void(*hook2log_out_func)(const char* const  head,
                                 const void*        buf,
                                 const size_t       size);

/**
  指定内存位置与信息输出描述，执行 Hook2Log 操作。\n
  其它未说明的参数请参考 Hook 。

  \param  data_descibe  数据指针描述。
  \param  len_descibe   数据长度描述。
  \param  head_msg      此 hook2log 的数据说明，允许为 nullptr 。
  \param  log_out_func  数据输出回调，允许为 nullptr ，此时使用默认的输出。
  \return               钩子结点指针，用于 UnHook 。

  \code
    #include "hook.h"
    void __stdcall logout(const char* const head, const void* buf, const size_t size)
      {
      XLIB_TRY
        {
        if(head != nullptr)
          {
          xlog() << head;
          }
        xlog() << showbin(buf, size);
        }
      XLIB_CATCH
        {
        xerr << xfunexpt;
        }
      }
    #ifdef _WIN64
    auto node = Hook2Log((void*)MessageBoxA, (size_t)8, "rdx", "10", true);
    #else
    auto node = Hook2Log((void*)MessageBoxA, (size_t)5, "[esp + 8]", "#[esp + 8]", true);
    #endif
    MessageBoxA(nullptr, "this is it", nullptr, MB_OK);
  \endcode

  \b 数据指针、数据长度，描述字符串由以下元素组合而成：

  \section Blanks 空白。
  - 空白符由【空格】【制表符】【换行符】【回车符】组成。
  - 如无特别说明，【空白】允许出现在任意处。
  - 如无特别说明，【空白】在解析过程中被忽略。

  \section Reg 寄存器标识。
  - 寄存器标识可由以下字符串表示（无视大小写）。
    eax  ecx  edx  ebx  esp  ebp  esi  edi
    eip  efg
    ah   ch   dh   bh
    al   cl   dl   bl
    ax   cx   dx   bx   sp   bp   si   di
  - x64 时，添加以下寄存器标识：
    rax  rcx  rdx  rbx  rsp  rbp  rsi  rdi
    r8   r9   r10  r11  r12  r13  r14  r15
    rip  rfg
    r8d  r9d  r10d r11d r12d r13d r14d r15d
    spl  bpl  sil  dil
    r8b  r9b  r10b r11b r12b r13b r14b r15b
    r8w  r9w  r10w r11w r12w r13w r14w r15w
  - x64 时，没有 x86 的以下寄存器。
    eip  efg
  - 寄存器取值一律视为无符号，如需要有符号取值，请使用 (reg)[bwd] 。

    \code
      eax
      eax + ecx
      [rax]
      al          // 取 al 无符号数。
      (al)B       // ==al ==(eax)B
      (al)b       // 取 al 有符号数。 ==(eax)b
    \endcode

  \section ConstHex 常数。
  - 常数为 1 - 8 个十六进制字符组成， x64 时为 1 - 16 个（无视大小写）。
    \code
      123 + 11
      AB * CD
    \endcode

  \section Operator 操作符。
  - 操作符有以下几种：
    + - * \ & | ^ % << >> # ##
  - 操作符的意义与优先级参考 C++ 。
  - 注意加入的 # ## ，为取长度值符，优先级高于其它操作符。
  - 注意 * 可能会溢出而丢失高位。
    \code
      eax * 4
      ecx & 1
      eax >> 2
      #eax          // 以 eax 的值为 ASCII 指针，计算以 '\0' 为结尾的字符串长度。
      ##eax         // 以 eax 的值为 UNICODE 指针，计算以 '\0\0' 结尾的字符串长度。
    \endcode

  \section PickValue 取值符。
  - 取值符以【[】号起始，【]】号结束
  - 取值符需要限定值时，允许在【]】后添加后缀【bwdBWD】分别表示 byte word dword 。
  - 默认取值 x86 为 dword 故不支持后缀【D】， x64 为 qword 。
  - 当后缀小写时，取符号值，大写时，取无符号值。

  \section Parentheses 小括号。
  - 小括号以【(】号起始，【)】号结束。
  - 小括号需要限定值时，允许在【)】后添加后缀【bwdBWD】。
  - 小括号用于提高运算优先级，其它说明参考【取值符】。

  \section Mod 模块限定。
  - 当数据非空白、寄存器标识、常数、操作符、取值符或小括号时，视为模块限定。
  - 允许 modname.offset/procname 格式。
  - 当 modename 为空时，取当前进程模块。
  - 优先尝试以 offset 读取。
  - 允许 offset/procname 为空，即 "." 也是一个合法表示，代表进程主模块。
  - Ring0 下，只支持为驱动名。 "." 表示第一个驱动模块，通常就是 ntosknl.exe。

  \code
    ntdll.1234     表示 ntdll 模块，偏移 1234 。
    ntdll.func     表示 ntdll 模块， func 导出函数。
    test.exe       当 test.exe 是一个模块时，表示一个模块。否则视 test 为模块名， exe 为 procname 。
  \endcode
*/
HookNode* Hook2Log(void*              hookmem,
                   const size_t       hooksize,
                   const char*        data_descibe,
                   const char*        len_descibe,
                   const bool         logfirst,
                   const char*        head_msg = nullptr,
                   hook2log_out_func  log_out_func = nullptr,
                   void*              p_shellcode = nullptr);
/**
  指定跳转表或 call 偏移位置，执行 Hook2Log 操作。\n
  未作说明的参数请参考 Hook 、 Hook2Log 。
 */
HookNode* Hook2Log(void*              hookmem,
                   const char*        data_descibe,
                   const char*        len_descibe,
                   const bool         calltable_offset,
                   const bool         logfirst,
                   const char*        head_msg = nullptr,
                   hook2log_out_func  log_out_func = nullptr,
                   void*              p_shellcode = nullptr,
                   const intptr_t     expandargc = 0);

#endif  // _WIN32

#endif  // _XLIB_HOOK_H_