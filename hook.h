/**
  \file  hook.h
  \brief hook.h用于执行指定hook、unhook操作

  \version    5.1.1.190614
  \note       For All

  \author     triones
  \date       2010-08-27

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

class hook
{
////////////////////////////////////////////////////////////////
public:
/// Hook 错误码。
enum ErrorCode
  {
  EC_Success,                  ///< 成功完成。
  EC_HookMem_Read,             ///< 尝试读取 hookmem 失败（一般是地址非法）。
  EC_ProtectVirtualMemory,     ///< 尝试 ZwProtectVirtualMemory 出错。
  EC_MmCreateMdl,              ///< 尝试 MmCreateMdl 出错。
  EC_MmMapLockedPages,         ///< 尝试 MmMapLockedPages 出错。
  EC_HookMem_Write,            ///< 尝试写入失败（一般是权限不足）。
  EC_DeleteNode,               ///< 尝试删除结点失败（一般是资源不足）。
  EC_MemCannotCover,           ///< 地址无法再次覆盖（已存在不可覆盖的钩子）。
  EC_MemCoverChk,              ///< 检测地址覆盖出错（链表操作出错）。
  EC_AddNode,                  ///< 加入结点失败（一般是资源不足）。
  EC_SetUEF,                   ///< 设置异常处理回调失败。
  EC_ClearUEF,                 ///< 清除异常处理回调失败。
  EC_ClearUEF_Cover,           ///< 清除异常处理回调被覆盖。
  EC_HookSize_OverFlow,        ///< hooksize 超限（限制最大为 15*2 ）。
  EC_HookSize_Zero,            ///< hooksize 为 0 。
  EC_Routine_Illegal,          ///< Routin 地址非法。
  EC_MakeNode,                 ///< 生成 HookNode 失败（一般是资源不足）。
  EC_MakShellCode,             ///< 生成 shellcode 失败（一般是资源不足）。
  EC_AntiShellCodeDEP,         ///< 清除 shellcode 的 DEP 失败（ ZwProtectVirtualMemory 出错）。
  EC_FixShellCode,             ///< 调整 shellcode 失败。
  EC_Ring0_NoUEF,              ///< 内核无顶层异常。
  EC_AddDisp_Invalid,          ///< x64 下，偏移无效。
  EC_FixJmpCode,               ///< 设置 jmpcode 失败（一般是结点错误）。
  EC_Hook,                     ///< HOOK 出错。
  EC_UnHook_Restore,           ///< 钩子已经被还原（钩子结点会被删除）。
  EC_UnHook_BeCover,           ///< 钩子位置被覆盖。
  EC_UnHook,                   ///< 还原钩子失败。
  EC_Clear,                    ///< 清除钩子链表失败。
  EC_MovShellCode,             ///< 转移 shellcode 失败（一般是无效地址）。
  };
public:
static const ErrorCode error_code = EC_Success;
#pragma code_seg(".text")
__declspec(allocate(".text"))
static inline const uint8_t HookShellCode_Normal[] = {"\
\xFF\x74\x24\x10\x9C\x41\x57\x41\x56\x41\x55\x41\x54\x41\x53\x41\
\x52\x57\x56\x55\x48\x8D\x6C\x24\x50\x48\x8D\x75\x20\x56\x53\x50\
\x41\x51\x41\x50\x52\x51\x48\x89\xE1\x41\x51\x41\x50\x52\x51\x48\
\x8B\x45\x10\x50\x58\xFF\x54\x24\xF8\x59\x5A\x41\x58\x41\x59\x59\
\x5A\x41\x58\x41\x59\x58\x5B\x5D\x5D\x5E\x5F\x41\x5A\x41\x5B\x41\
\x5C\x41\x5D\x41\x5E\x41\x5F\x48\x87\x44\x24\x08\x48\x3B\x44\x24\
\x20\x48\x89\x44\x24\x20\x48\x87\x44\x24\x08\x74\x07\x9D\x48\x8D\
\x64\x24\x18\xC3\x9D\x48\x8D\x64\x24\x08\xC2\x10\x00" };
#pragma code_seg()
////////////////////////////////////////////////////////////////
};

#endif  // _XLIB_HOOK_H_