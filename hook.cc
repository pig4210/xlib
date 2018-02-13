#include "hook.h"

#ifdef _WIN32

#include <stdlib.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

#include "xlib_nt.h"
#include "xblk.h"
#include "xline.h"
#include "xmsg.h"
#include "hex_bin.h"
#include "xlog.h"
#include "syssnap.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! 全局HOOK错误码
static HookErrCode g_last_hook_err;

//! 设置HOOK错误码
static void SetLastHookErr(const HookErrCode new_errcode);

//! 最大实际修改字节。x86下6 byte，x64下12 byte
static const size_t gk_hook_max_byte2modify =
#ifdef _WIN64
sizeof(AddrDisp)+
#endif
1 + 1 + sizeof(void*);

//! 最大hooksize。真实指令最大长度15 byte
static const size_t gk_hook_max_byte2cover = 15 * 2;

//! 钩子结点结构
class HookNode
  {
  public:
    void*   mem;            //!< hook内存位置
    size_t  byte2cover;     //!< hook长度
    void*   routine;        //!< 回调地址
    void*   ip;             //!< eip/rip
    void*   lpshellcode;    //!< 指向shellcode
    uint8   oldcode[gk_hook_max_byte2modify];//!< 覆盖前的数据(用于卸载时还原判定)
    uint8   newcode[gk_hook_max_byte2modify];//!< 覆盖后的数据(用于卸载时覆盖判定)
    line    shellcode;      //!< 钩子代码
  };

//! 钩子链表
class HookList : public vector<HookNode*>
  {
  public:
    ~HookList();
  };

//!< 钩子链表
#ifndef FOR_RING0
//! RING3提供，ring3使用static类以实现自动初始化及自动卸载，故，不采用new HookList形式，
static HookList   g_hooklist;
static HookList*  g_hook_list = &g_hooklist;
#else
static HookList*  g_hook_list = nullptr;
#endif

//! 从链表中删除指定结点，无论链表中是否存在指定结点，都会删除。异常返回false，否则都true
static bool DeleteNode(HookNode* node);

//! 钩子覆盖判定，检查指定范围是否在链表中
static bool MemCanCover(void* mem, const size_t byte2cover);

//! 向链表中追加一个结点
static bool AddNode(HookNode* node);

#ifndef FOR_RING0       // Ring0不支持顶层异常处理
//! 存放旧UEF
static LPTOP_LEVEL_EXCEPTION_FILTER g_oldUEFHandling = LPTOP_LEVEL_EXCEPTION_FILTER(-1);

//! 顶层异常处理回调函数
static LONG WINAPI  HookUEFHandling(struct _EXCEPTION_POINTERS * ExceptionInfo);
#endif

//! 设置异常处理回调
static bool SetHookUEF(HookNode* node);

//! 清除异常处理回调
static bool ClearHookUEF();

//! 判定Hook长度是否符合要求
static bool Check_hooksize(const size_t hooksize);

//! 判定Hook地址是否可读可写
static bool Check_hookmem(void* hookmem);

//! 判定Routine是否有效
static bool Check_Routine(void* routine);

//! 初始化结点（普通版本）
static HookNode* MakeNode(void* hookmem, const size_t hooksize, void* routine);

//! 初始化结点（CallTable_Offset版本）
static HookNode* MakeNode(void* hookmem, void* routine, const bool calltable_offset);

//! 调整shellcode，主要是AntiDEP与设置指针
static bool FixShellCode(HookNode* node, void* p_shellcode);

//! 做普通hook shellcode
static bool MakeShellCode_Normal(HookNode* node, const bool routinefirst);

//! 做CallTable_Offset hook shellcode
static bool MakeShellCode_CtOff(HookNode* node, const bool routinefirst, const intptr_t expandargc);

//! 做普通jmpcode
static bool FixJmpCode_Normal(HookNode* node);

//! 做CallTable_Offset jmpcode
static bool FixJmpCode_CtOff(HookNode* node, const bool calltable_offset);

//! 正式修改并加入链表
static bool HookIn(HookNode* node);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#ifndef _WIN64

static const size_t hookshellcode_normal_prefix = 0x1C;
static const size_t hookshellcode_ctoff_prefix = 0x1D;
/*!
  要求前置shellcode如下

  \code
                                      --前置覆盖代码--
  $ ==>    >  68 XXXXXXXX            push    EIP
  $+5      >  68 XXXXXXXX            push    Routinue
  $+A      >  FF15 XXXXXXXX          call    dword ptr [gk_lp_HookShellCode_Normal]
                                      --后置覆盖代码--
  $+10     >  FF25 XXXXXXXX          jmp     [EIP]
  $+16
  \endcode
*/
static __declspec(naked) void HookShellCode_Normal()
  {
  __asm
    {
    push    dword ptr [esp + 4 * 2]       //参数EIP       ->ret routine
    pushfd
    pushad
    add     dword ptr [esp + 4 * 3], 4 * 5//修正esp       ->edi esi ebp  ->fd eip ret routine eip

    push    esp
    call    dword ptr [esp + 4 * 12]      //调用Routine   ->esp ad fd eip ret

    mov     eax, dword ptr [esp + 4 * 9]  //获取参数EIP   ->ad fd
    cmp     eax, dword ptr [esp + 4 * 12] //检测是否修改  ->ad fd eip ret routine
    mov     dword ptr [esp + 4 * 12], eax //修改EIP       ->ad fd eip ret routine

    popad

    jz      HookShellCode_Normal_Next
    popfd
    lea     esp, dword ptr [esp + 4 * 3]  //修改则跳过    ->eip ret routine
    retn

  HookShellCode_Normal_Next :
    popfd
    lea     esp, dword ptr [esp + 4 * 1]  //仅跳过EIP，返回执行可能存在的代码
    retn    4 * 2

    add     byte ptr [eax], al            //做0结尾
    }
  }

/*!
  要求前置shellcode如下

  \code
  $ ==>    >  68 XXXXXXXX            push    CoverCall
  $+5      >  68 XXXXXXXX            push    Routinue
  $+A      >  68 XXXXXXXX            push    HookArgc
  $+F      >  6A 00|01               push    routinefirst
  $+11     >  FF25 XXXXXXXX          jmp     dword ptr [gk_lp_HookShellCode_CtOff]
  $+17
  \endcode
*/
static __declspec(naked) void HookShellCode_CtOff()
  {
  __asm
    {
    pushfd

    xchg    eax, dword ptr [esp]          //eax为fd，原始值入栈
    xchg    eax, dword ptr [esp + 4 * 1]  //eax为routinefirst，fd向下移
    test    eax, eax                      //判定routinefirst

    pop     eax                           //还原eax

    jnz     HookShellCode_CtOff_Transit

    popfd

    push    0x01230814                    //做特征
    push    esp
    push    0x42104210
    mov     dword ptr [esp + 4 * 1], esp  //修正特征指向

    pushfd
    pushad                                //这里不用修正esp，修正也没意义

    mov     esi, esp

    mov     ecx, dword ptr [esp + 4 * 12] //获取argc       ->ad fd # esp #
    mov     edx, ecx
    shl     ecx, 2

    mov     ebx, dword ptr [esp + 4 * 14] //取得CoverCall  ->ad fd # esp # argc routine

    sub     esp, ecx                      //扩展局部栈
    mov     edi, esp

    xor     ecx, ecx
    mov     cl, 9                         //复制寄存器保护
    rep movsd

    mov     cl, 7                         //跳过           -># esp # argc ro cov ret
    rep lodsd

    mov     ecx, edx                      //参数复制
    rep movsd

    push    ebx
    pop     ebx                           //存放临时CoverCall

    popad
    popfd

    call    dword ptr [esp - 4 * 10]      //call CoverCall ->fd ad CoverCall

    push    edi
    push    esi
    push    ecx
    pushfd                                //环境保护

    mov     edi, esp                      //edi指向状态保护

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

    lea     esi, dword ptr [esp + 4 * 6]  //esi指向ret    -># esp # argc ro cov

    mov     ecx, dword ptr [esp + 4 * 3]  //提取argc      -># esp #
    shl     ecx, 2
    add     ecx, 4 * 11                   //计算位移      ->fd ecx esi edi # esp # argc ro cov ret

    lea     esp, dword ptr [edi + ecx]    //esp指向real_call_ret

    push    dword ptr [esi - 4 * 0]       //ret
    push    dword ptr [esi - 4 * 2]       //routine
    push    dword ptr [esi - 4 * 3]       //argc

    xchg    edi, esp
    popfd                                 //恢复环境
    pop     ecx
    pop     esi
    xchg    edi, esp
    mov     edi, dword ptr [edi]

    pushfd
  HookShellCode_CtOff_Base:
    popfd

    push    dword ptr [esp + 4 * 2]       //参数CoverCall ->argc routine
    pushfd
    pushad
    add     dword ptr [esp + 4 * 3], 4 * 5//修正esp       ->edi esi ebp ->fd eip argc routine eip

    push    esp
    call    dword ptr [esp + 4 * 12]      //调用Routine   ->esp ad fd eip argc

    popad
    popfd

    push    eax
    mov     eax, dword ptr [esp + 4 * 1]  //提取参数CoverCall
    mov     dword ptr [esp + 4 * 4], eax  //重写可能已被修改的CoverCall
    pop     eax

    lea     esp, dword ptr [esp + 4 * 3]  //跳过          ->CoverCall argc routine
    retn

    add     byte ptr [eax], al            //做0结尾
    }
  }

#else   // _WIN64

static const size_t hookshellcode_normal_prefix = 0x3C;
static const size_t hookshellcode_ctoff_prefix = 0x5E;
#ifndef __INTEL_COMPILER

#pragma message(" -- 不使用Intel C++ Compiler，HookShellCode版本可能较旧")
#pragma code_seg(".text")
__declspec(allocate(".text"))
static const uint8 HookShellCode_Normal[] = {"\
\xFF\x74\x24\x10\x9C\x41\x57\x41\x56\x41\x55\x41\x54\x41\x53\x41\
\x52\x57\x56\x55\x48\x8D\x6C\x24\x50\x48\x8D\x75\x20\x56\x53\x50\
\x41\x51\x41\x50\x52\x51\x48\x89\xE1\x41\x51\x41\x50\x52\x51\x48\
\x8B\x45\x10\x50\x58\xFF\x54\x24\xF8\x59\x5A\x41\x58\x41\x59\x59\
\x5A\x41\x58\x41\x59\x58\x5B\x5D\x5D\x5E\x5F\x41\x5A\x41\x5B\x41\
\x5C\x41\x5D\x41\x5E\x41\x5F\x48\x87\x44\x24\x08\x48\x3B\x44\x24\
\x20\x48\x89\x44\x24\x20\x48\x87\x44\x24\x08\x74\x07\x9D\x48\x8D\
\x64\x24\x18\xC3\x9D\x48\x8D\x64\x24\x08\xC2\x10\x00" };
__declspec(allocate(".text"))
static const uint8 HookShellCode_CtOff[] = {"\
\x9C\x48\x87\x04\x24\x48\x87\x44\x24\x08\x48\x85\xC0\x58\x75\x59\
\x9D\x68\x14\x08\x23\x01\x54\x68\x10\x42\x10\x42\x48\x89\x64\x24\
\x08\x9C\x57\x56\x52\x51\x50\x48\x8B\x44\x24\x58\x48\x8B\x4C\x24\
\x48\x50\x48\x89\xE6\x48\x89\xCA\x48\xC1\xE1\x03\x48\x2B\xE1\x48\
\x89\xE7\x48\x33\xC9\xB1\x07\xF3\x48\xA5\xB1\x07\xF3\x48\xAD\x48\
\x89\xD1\xF3\x48\xA5\x58\x58\x59\x5A\x5E\x5F\x9D\xFF\x54\x24\xC8\
\x57\x56\x51\x9C\x48\x89\xE7\xEB\x02\xEB\x4E\x48\x83\xC4\x08\x48\
\x81\x3C\x24\x10\x42\x10\x42\x75\xF2\x48\x39\x64\x24\x08\x75\xEB\
\x48\x81\x7C\x24\x10\x14\x08\x23\x01\x75\xE0\x48\x8D\x74\x24\x30\
\x48\x8B\x4C\x24\x18\x48\xC1\xE1\x03\x48\xC7\xC1\x58\x00\x00\x00\
\x48\x8D\x24\x0F\xFF\x36\xFF\x76\xF0\xFF\x76\xE8\x48\x87\xFC\x9D\
\x59\x5E\x48\x87\xFC\x48\x8B\x3F\x9C\x9D\xFF\x74\x24\x10\x9C\x41\
\x57\x41\x56\x41\x55\x41\x54\x41\x53\x41\x52\x57\x56\x55\x48\x8D\
\x6C\x24\x50\x48\x8D\x75\x20\x56\x53\x50\x41\x51\x41\x50\x52\x51\
\x48\x89\xE1\x41\x51\x41\x50\x52\x51\x48\x8B\x45\x10\x50\x58\xFF\
\x54\x24\xF8\x59\x5A\x41\x58\x41\x59\x59\x5A\x41\x58\x41\x59\x58\
\x5B\x5D\x5D\x5E\x5F\x41\x5A\x41\x5B\x41\x5C\x41\x5D\x41\x5E\x41\
\x5F\x9D\x50\x48\x8B\x44\x24\x08\x48\x89\x44\x24\x20\x58\x48\x8D\
\x64\x24\x18\xC3\x00"};
#pragma code_seg()

#else   // __INTEL_COMPILER

/*!
  要求前置shellcode如下

  \code
                                     --CoverCode--
  $ ==>    >  EB 18                  jmp     $+1A
  $+2      >  XXXXXXXXXXXXXXXX       [RIP]
  $+A      >  XXXXXXXXXXXXXXXX       [Routine]
  $+12     >  XXXXXXXXXXXXXXXX       [gk_lp_HookShellCode_Normal]
  $+1A     >  FF35 E2FFFFFF          push    qword ptr [RIP]
  $+20     >  FF35 E4FFFFFF          push    qword ptr [Routine]
  $+26     >  FF15 E6FFFFFF          call    qword ptr [gk_lp_HookShellCode_Normal]
                                     --CoverCode--
  $+2C     >  FF25 D0FFFFFF?         jmp     [eip]
  $+32
  \endcode
*/
static __declspec(naked) void HookShellCode_Normal()
  {
  __asm
    {
    push    qword ptr [rsp + 8 * 2]       //参数RIP       ->ret routine

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
    lea     rbp, qword ptr [rsp + 8 * 10] //指向参数RIP   ->rbp rsi rdi r10-15 fq
    lea     rsi, qword ptr [rbp + 8 * 4]  //指向ret       ->rip ret routin rip
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
    mov     rax, qword ptr [rbp + 8 * 2]  //提取Routine   ->rip ret
    push    rax
    pop     rax
    call    qword ptr [rsp - 8 * 1]       //调用Routine
    pop     rcx                           //弹出参数
    pop     rdx
    pop     r8
    pop     r9

    pop     rcx
    pop     rdx
    pop     r8
    pop     r9
    pop     rax
    pop     rbx
    pop     rbp                           //pop rsp
    pop     rbp
    pop     rsi
    pop     rdi
    pop     r10
    pop     r11
    pop     r12
    pop     r13
    pop     r14
    pop     r15

    xchg    rax, qword ptr [rsp + 8 * 1]  //取出参数RIP   ->fq
    cmp     rax, qword ptr [rsp + 8 * 4]  //检测是否修改  ->fq rip ret routine
    mov     qword ptr [rsp + 8 * 4], rax  //修改EIP       ->fq rip ret routine
    xchg    rax, qword ptr [rsp + 8 * 1]  //还原rax

    jz      HookShellCode_Normal_Next
    popfq
    lea     rsp, qword ptr [rsp + 8 * 3]  //修改则跳过    -> rip ret routine
    retn

  HookShellCode_Normal_Next :
    popfq
    lea     rsp, qword ptr [rsp + 8 * 1]  //仅跳过RIP，返回执行可能存在的代码
    retn    8 * 2

    add     byte ptr [rax], al            //做0结尾
    }
  }

/*!
  要求前置shellcode如下

  \code
  $ ==>    >  EB 28                  jmp     $+2A
  $+2      >  XXXXXXXXXXXXXXXX       [CoverCall]
  $+A      >  XXXXXXXXXXXXXXXX       [Routine]
  $+12     >  XXXXXXXXXXXXXXXX       [HookArgc]
  $+1A     >  XXXXXXXXXXXXXXXX       [routinefirst]
  $+22     >  XXXXXXXXXXXXXXXX       [gk_lp_HookShellCode_CtOff]
  $+2A     >  FF35 D2FFFFFF          push    qword ptr [CoverCall]
  $+30     >  FF35 D4FFFFFF          push    qword ptr [Routine]
  $+36     >  FF35 D6FFFFFF          push    qword ptr [HookArgc]
  $+3C     >  FF35 D8FFFFFF          push    qword ptr [routinefirst]
  $+42     >  FF35 DAFFFFFF          push    qword ptr [gk_lp_HookShellCode_CtOff]
  $+48     >  48 873C 24             xchg    rdi, qword ptr [rsp]
  $+4C     >  48 8B3F                mov     rdi, qword ptr [rdi]
  $+4F     >  48 873C 24             xchg    rdi, qword ptr [rsp]
  $+53     >  C3                     ret
  $+54
  \endcode
*/
static __declspec(naked) void HookShellCode_CtOff()
  {
  __asm
    {
    pushfq

    xchg    rax, qword ptr [rsp]          //rax为fq，原始值入栈
    xchg    rax, qword ptr [rsp + 8 * 1]  //rax为routinefirst，fq向下移
    test    rax, rax
    pop     rax

    jnz     HookShellCode_CtOff_Transit

    popfq
    push    0x01230814                    //push imm只允许32位，但入栈是64位
    push    rsp                           //注意后面会修正这个值，但不即时修正
    push    0x42104210                    //以上三个特征值为最后堆栈平衡设置
    mov     qword ptr [rsp + 8 * 1], rsp  //修正入栈rsp，以便最后栈平衡计算，rsp指向特征

    pushfq
    push    rdi
    push    rsi
    push    rdx
    push    rcx
    push    rax

    mov     rax, qword ptr [rsp + 8 * 11] //取得CoverCall ->rax rcx rdx rsi rdi fq # rsp # argc ro
    mov     rcx, qword ptr [rsp + 8 * 9]  //获取argc      ->rax rcx rdx rsi rdi fq # rsp #

    push    rax                           //push covercall

    mov     rsi, rsp
    mov     rdx, rcx
    shl     rcx, 3
    sub     rsp, rcx                      //扩展局部栈
    mov     rdi, rsp

    xor     rcx, rcx
    mov     cl, 7                         //复制寄存器保护->covercall rax rcx rdx rsi rdi fq
    rep movsq

    mov     cl, 7                         //跳过          -># rsp # argc ro cov ret
    rep lodsq

    mov     rcx, rdx                      //参数复制
    rep movsq

    pop     rax
    pop     rax
    pop     rcx
    pop     rdx
    pop     rsi
    pop     rdi
    popfq
    call    qword ptr [rsp - 8 * 7]      //call CoverCall ->covercall rax rcx rdx rsi rdi fq

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

    lea     rsi, qword ptr [rsp + 8 * 6]  //rsi指向ret    -># rsp # argc ro cov

    mov     rcx, qword ptr [rsp + 8 * 3]  //提取argc      -># rsp #
    shl     rcx, 3
    mov     rcx, 8 * 11                   //计算位移      ->rcx rsi rdi fd # rsp # argc ro cov ret
    lea     rsp, dword ptr [rdi + rcx]    //rsp指向real_call_ret

    push    qword ptr [rsi - 8 * 0]       //ret
    push    qword ptr [rsi - 8 * 2]       //routine
    push    qword ptr [rsi - 8 * 3]       //argc

    xchg    rdi, rsp
    popfq                                 //恢复环境
    pop     rcx
    pop     rsi
    xchg    rdi, rsp
    mov     rdi, qword ptr [rdi]

    pushfq

  HookShellCode_CtOff_Base:
    popfq

    push    qword ptr [rsp + 8 * 2]       //参数CoverCall ->argc routine
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
    lea     rbp, qword ptr [rsp + 8 * 10] //指向参数CoverCall ->rbp rsi rdi r10-15 fq
    lea     rsi, qword ptr [rbp + 8 * 4]  //指向ret       ->covercall argc routin covercall
    push    rsi                           //push rsp
    push    rbx
    push    rax
    push    r9
    push    r8
    push    rdx
    push    rcx

    mov     rcx, rsp
    push    rcx
    mov     rax, qword ptr [rbp + 8 * 2]  //提取Routine   ->covercall argc
    push    rax
    pop     rax
    call    qword ptr [rsp - 8 * 1]       //调用Routine
    pop     rcx                           //弹出参数

    pop     rcx
    pop     rdx
    pop     r8
    pop     r9
    pop     rax
    pop     rbx
    pop     rbp                           //pop rsp
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
    mov     rax, qword ptr [rsp + 8 * 1]  //提取参数CoverCall
    mov     qword ptr [rsp + 8 * 4], rax  //重写可能已被修改的CoverCall
    pop     rax

    lea     rsp, qword ptr [rsp + 8 * 3]  //跳过          ->Covercall argc routine
    retn

    add     byte ptr [rax], al             //做0结尾
    }
  }

#endif  // __INTEL_COMPILER

#endif  // _WIN64

//! 指针，用于shellcode
static const void* const gk_lp_HookShellCode_Normal = (void*)HookShellCode_Normal;
static const void* gk_lp_HookShellCode_CtOff = (void*)HookShellCode_CtOff;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SetLastHookErr(const HookErrCode new_errcode)
  {
  g_last_hook_err = new_errcode;
  }

HookErrCode GetLastHookErr()
  {
  return g_last_hook_err;
  }

bool Hookit(LPVOID mem, LPCVOID hookcode, const size_t len)
  {
  XLIB_TRY
    {
    uint8 tmp;
    memcpy(&tmp, mem, 1);
    }
  XLIB_CATCH
    {
    SetLastHookErr(HookErr_HookMem_Read_Fail);
    return false;
    }

#ifndef FOR_RING0       // Ring3下不采用WriteProcessMemory，而是采用修改页属性的方式
  LPVOID Mem = mem;
  ULONG_PTR Len = (ULONG_PTR)len;
  ULONG_PTR oap;

  if(STATUS_SUCCESS != ZwProtectVirtualMemory(
    GetCurrentProcess(), &Mem, &Len, PAGE_EXECUTE_READWRITE, &oap))
    {
    SetLastHookErr(HookErr_ProtectVirtualMemory_Fail);
    return false;
    }

  XLIB_TRY
    {
    RtlCopyMemory(mem, hookcode, len);
    }
  XLIB_CATCH
    {
    SetLastHookErr(HookErr_HookMem_Write_Fail);
    return false;
    }

  ZwProtectVirtualMemory(GetCurrentProcess(), &Mem, &Len, oap, &oap);
#else   // FOR_RING0      Ring0下不采用改变寄存器的方式
  PMDL pMDL = MmCreateMdl(nullptr, mem, len);
  if(pMDL == nullptr)
    {
    SetLastHookErr(HookErr_MmCreateMdl_Fail);
    return  false;
    }

  MmBuildMdlForNonPagedPool(pMDL);
  pMDL->MdlFlags = pMDL->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA | MDL_WRITE_OPERATION;

  PVOID Mem = MmMapLockedPages(pMDL, KernelMode);
  if(Mem == nullptr)
    {
    SetLastHookErr(HookErr_MmMapLockedPages_Fail);
    return false;
    }

  XLIB_TRY
    {
    RtlCopyMemory(Mem, hookcode, len);
    }
  XLIB_CATCH
    {
    SetLastHookErr(HookErr_HookMem_Write_Fail);
    return false;
    }

  MmUnmapLockedPages(Mem, pMDL);
  IoFreeMdl(pMDL);
#endif  // FOR_RING0
  SetLastHookErr(HookErr_Success);
  return true;
  }

bool Nopit(LPVOID mem, const size_t len)
  {
  const string str(len, '\x90');
  return Hookit(mem, str.c_str(), str.size());
  }

size_t CalcOffset(const void* mem, const void* dest)
  {
  return (size_t)dest - (size_t)mem - sizeof(AddrDisp);
  }

bool IsValidAddrDisp(const size_t addrdisp)
  {
#ifdef _WIN64
  if((intptr_t)addrdisp <= (intptr_t)(0x00000000FFFFFFFF) &&
     (intptr_t)addrdisp >= (intptr_t)(0xFFFFFFFF80000000))
    {
    return true;
    }
  return false;
#else
  UNREFERENCED_PARAMETER(addrdisp);
  return true;
#endif
  }

HookList::~HookList()
  {
  XLIB_TRY
    {
    if(!ClearHookUEF())  return;

    while(!empty())
      {
      UnHook(*begin(), false);
      }
    clear();
    }
  XLIB_CATCH
    {
    ;
    }
  }

bool DeleteNode(HookNode* node)
  {
  XLIB_TRY
    {
    if(node != nullptr)
      {
      delete node;

      if(g_hook_list != nullptr)
        {
        for(auto it = g_hook_list->begin(); it != g_hook_list->end(); ++it)
          {
          HookNode* now = *it;
          if(node == now)
            {
            g_hook_list->erase(it);
            break;
            }
          }
        }
      }
    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_DeleteNode_Fail);
  return false;
  }

bool MemCanCover(void* mem, const size_t byte2cover)
  {
  XLIB_TRY
    {
    if(g_hook_list != nullptr)
      {
      const xblk blkA(mem, byte2cover);
      for(const auto now : (*g_hook_list))
        {
        const xblk blkB(now->mem, now->byte2cover);
        if(blkB.checkin(blkA) != xblk::PD_NoIn)
          {
          SetLastHookErr(HookErr_MemCannotCover);
          return false;
          }
        }
      }
    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_MemCoverChk_Fail);
  return false;
  }

bool AddNode(HookNode* node)
  {
  XLIB_TRY
    {
    if(g_hook_list == nullptr)
      {
      g_hook_list = new HookList;
      }
    g_hook_list->push_back(node);
    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_AddNode_Fail);
  return false;
  }

#ifndef FOR_RING0

LONG WINAPI  HookUEFHandling(struct _EXCEPTION_POINTERS * ExceptionInfo)
  {
  if(ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT)
    {
    if(g_hook_list != nullptr)
      {
      for(const auto now : (*g_hook_list))
        {
#ifndef _WIN64
        if((DWORD)(now->mem) == ExceptionInfo->ContextRecord->Eip)
          {
          ExceptionInfo->ContextRecord->Eip = (DWORD)(now->lpshellcode);
          return  EXCEPTION_CONTINUE_EXECUTION;
          }
#else
        if((DWORD64)(now->mem) == ExceptionInfo->ContextRecord->Rip)
          {
          ExceptionInfo->ContextRecord->Rip = (DWORD64)(now->lpshellcode);
          return  EXCEPTION_CONTINUE_EXECUTION;
          }
#endif
        }
      }
    }
  if(g_oldUEFHandling == nullptr)  return  EXCEPTION_CONTINUE_SEARCH;
  return  g_oldUEFHandling(ExceptionInfo);
  }

#endif  // FOR_RING0

bool SetHookUEF(HookNode* node)
  {
#ifdef FOR_RING0
  UNREFERENCED_PARAMETER(node);
  SetLastHookErr(HookErr_Ring0_NoUEF);
  return false;
#else
  XLIB_TRY
    {
    if(!MemCanCover(node->mem, node->byte2cover)) return false;
    //注意先替换U.E.F！
    LPTOP_LEVEL_EXCEPTION_FILTER  olds = SetUnhandledExceptionFilter(HookUEFHandling);
    //考虑到U.E.F可能被反复下，只保存最后一次非CrackUEFHandling的原U.E.F
    if(olds != &HookUEFHandling) g_oldUEFHandling = olds;
    node->newcode[0] = 0xCC;
    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_SetUEF_Fail);
  return false;
#endif
  }

bool ClearHookUEF()
  {
#ifdef FOR_RING0
  SetLastHookErr(HookErr_Success);
  return true;
#else
  XLIB_TRY
    {
    if(g_oldUEFHandling != (LPTOP_LEVEL_EXCEPTION_FILTER)(-1))  //如果有改变顶层异常
      {
      LPTOP_LEVEL_EXCEPTION_FILTER  olds =
        SetUnhandledExceptionFilter(g_oldUEFHandling);  //尝试还原UEF
      if(olds != &HookUEFHandling)
        {
        SetLastHookErr(HookErr_ClearUEF_Cover);
        return false;
        }
      }
    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_ClearUEF_Fail);
  return false;
#endif
  }

bool Check_hooksize(const size_t hooksize)
  {
  if(hooksize > gk_hook_max_byte2cover)
    {
    SetLastHookErr(HookErr_HookSize_OverFlow);
    return false;
    }
  if(hooksize == 0)
    {
    SetLastHookErr(HookErr_HookSize_Zero);
    return false;
    }
  SetLastHookErr(HookErr_Success);
  return true;
  }

bool Check_hookmem(void* hookmem)
  {
  uint8 tmp[gk_hook_max_byte2modify];
  XLIB_TRY
    {
    memcpy(tmp, hookmem, sizeof(tmp));
    }
  XLIB_CATCH
    {
    SetLastHookErr(HookErr_HookMem_Read_Fail);
    return false;
    }
  return Hookit(hookmem, (LPCVOID)tmp, sizeof(tmp));
  }

bool Check_Routine(void* routine)
  {
  XLIB_TRY
    {
    uint8 tmp[2];
    memcpy(tmp, routine, sizeof(tmp));
    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_Routine_Illegal);
  return false;
  }

HookNode* MakeNode(void* hookmem, const size_t hooksize, void* routine)
  {
  XLIB_TRY
    {
    HookNode* node = nullptr;

    if(!Check_hooksize(hooksize) ||
       !Check_hookmem(hookmem) ||
       !Check_Routine(routine))
      {
      return nullptr;
      }

    node = new HookNode;
    node->mem = hookmem;
    node->byte2cover = hooksize;
    node->routine = routine;
    node->ip = ((uint8*)hookmem + hooksize);
    node->lpshellcode = nullptr;

    memcpy(node->oldcode, node->mem, sizeof(node->oldcode));
    memcpy(node->newcode, node->mem, sizeof(node->newcode));

    SetLastHookErr(HookErr_Success);
    return node;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_MakeNode_Fail);
  return nullptr;
  }

HookNode* MakeNode(void* hookmem, void* routine, const bool calltable_offset)
  {
  XLIB_TRY
    {
    HookNode* node = nullptr;

    if(!Check_hooksize(sizeof(void*)) ||
       !Check_hookmem(hookmem) ||
       !Check_Routine(routine))
      {
      return nullptr;
      }

    node = new HookNode;
    node->mem = hookmem;
    node->byte2cover = (calltable_offset) ? sizeof(void*): sizeof(AddrDisp);
    node->routine = routine;
    node->ip = (calltable_offset)
      ? (*(void**)(hookmem))
      : (void*)(*(AddrDisp*)(hookmem) + (size_t)hookmem + sizeof(AddrDisp));
    node->lpshellcode = nullptr;

    memcpy(node->oldcode, node->mem, sizeof(node->oldcode));
    memcpy(node->newcode, node->mem, sizeof(node->newcode));

    SetLastHookErr(HookErr_Success);
    return node;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_MakeNode_Fail);
  return nullptr;
  }

bool FixShellCode(HookNode* node, void* p_shellcode)
  {
  XLIB_TRY
    {
    //前缀的添加完全是为了x64。x86下不使用但不影响
    line& scs = node->shellcode;

    size_t off = 0;
    if(scs.c_str() != node->routine)  //如果不是Hook2Log的情况，lpshellcode需要修正
      {
      node->lpshellcode = (void*)scs.c_str();
      }
    else
      {
      off = (size_t)node->lpshellcode - (size_t)scs.c_str();
      }

#ifndef FOR_RING0
    LPVOID Mem = (LPVOID)scs.c_str();
    ULONG_PTR Len = (ULONG_PTR)scs.size();
    ULONG_PTR oap;
    if(STATUS_SUCCESS != ZwProtectVirtualMemory(
      GetCurrentProcess(), &Mem, &Len, PAGE_EXECUTE_READWRITE, &oap))
      {
      SetLastHookErr(HookErr_AntiShellCodeDEP_Fail);
      return false;
      }
#endif
    //如果指定了shellcode的空间，则转移之
    if(p_shellcode != nullptr)
      {
      if(!Hookit(p_shellcode, node->lpshellcode, scs.size() - off))
        return false;

      node->lpshellcode = p_shellcode;
#ifndef FOR_RING0
      LPVOID Mem = (LPVOID)node->lpshellcode;
      ULONG_PTR Len = (ULONG_PTR)(scs.size() - off);
      ULONG_PTR oap;
      if(STATUS_SUCCESS != ZwProtectVirtualMemory(
        GetCurrentProcess(), &Mem, &Len, PAGE_EXECUTE_READWRITE, &oap))
        {
        SetLastHookErr(HookErr_AntiShellCodeDEP_Fail);
        return false;
        }
#endif
      }
    *(void**)((uint8*)node->lpshellcode + 2) = node->lpshellcode;

    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_FixShellCode_Fail);
  return false;
  }

bool MakeShellCode_Normal(HookNode* node, const bool routinefirst)
  {
  XLIB_TRY
    {
    line& scs = node->shellcode;

    node->lpshellcode = (void*)(scs.c_str() + scs.size());    //对Hook2Log有效，对Hook无意义

    const uint8 head[1 + 1 + sizeof(void*)] =
      { (uint8)'\xEB', (uint8)sizeof(void*), 0 };
    scs.append(head, sizeof(head));

    if(!routinefirst)    //代码前行需要先写原始代码
      {
      scs.append((const uint8*)node->mem, node->byte2cover);
      }
#ifndef _WIN64
    scs << '\x68' << (AddrDisp)node->ip
      << '\x68' << (AddrDisp)node->routine
      << "\xFF\x15" << (AddrDisp)&gk_lp_HookShellCode_Normal;

    if(routinefirst)
      {
      scs.append((const uint8*)node->mem, node->byte2cover);  //代码后行后写原始代码
      }

    scs << "\xFF\x25" << (AddrDisp)&(node->ip);
#else   // _WIN64
    scs << "\xEB\x18"
      << node->ip << node->routine << (void*)gk_lp_HookShellCode_Normal
      << "\xFF\x35\xE2\xFF\xFF\xFF\xFF\x35\xE4\xFF\xFF\xFF\xFF\x15\xE6\xFF\xFF\xFF";

    if(routinefirst)
      {
      scs.append((const uint8*)node->mem, node->byte2cover);  //代码后行后写原始代码
      scs << "\xFF\x25" << (AddrDisp)(0xFFFFFFD0 - node->byte2cover);
      }
    else
      {
      scs << "\xFF\x25" << (AddrDisp)0xFFFFFFD0;
      }
#endif  // _WIN64
    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_MakShellCode_Fail);
  return false;
  }

bool MakeShellCode_CtOff(HookNode* node, const bool routinefirst, const intptr_t expandargc)
  {
  XLIB_TRY
    {
    line& scs = node->shellcode;

    node->lpshellcode = (void*)(scs.c_str() + scs.size());

    const uint8 head[1 + 1 + sizeof(void*)] =
      { (uint8)'\xEB', (uint8)sizeof(void*), 0 };
    scs.append(head, sizeof(head));

    static const intptr_t gk_hook_default_argc = 0x8;     //默认参数8个
    intptr_t hookargc = gk_hook_default_argc + expandargc;
    if(hookargc <= 0)  hookargc = gk_hook_default_argc;   //检测不让堆栈错误

#ifndef _WIN64
    scs << '\x68' << node->ip
      << '\x68' << node->routine
      << '\x68' << hookargc
      << '\x6A' << (uint8)routinefirst
      << "\xFF\x25" << (AddrDisp)&gk_lp_HookShellCode_CtOff;
#else
    scs << "\xEB\x28"
      << node->ip << node->routine << hookargc
      << (void*)routinefirst << (void*)&gk_lp_HookShellCode_CtOff
      << "\xFF\x35\xD2\xFF\xFF\xFF\xFF\x35\xD4\xFF\xFF\xFF\xFF\x35\xD6\xFF\xFF\xFF"
      << "\xFF\x35\xD8\xFF\xFF\xFF\xFF\x35\xDA\xFF\xFF\xFF"
      << "\x48\x87\x3C\x24\x48\x8B\x3F\x48\x87\x3C\x24\xC3";
#endif
    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_MakShellCode_Fail);
  return false;
  }

bool FixJmpCode_Normal(HookNode* node)
  {
  XLIB_TRY
    {
    switch(node->byte2cover)
      {
      case 1:  case 2: case 3: case 4:
        {
        if(!SetHookUEF(node)) return false;
        break;
        }
      case 5:
        {
        if(!MemCanCover(node->mem, node->byte2cover)) return false;
        node->newcode[0] = 0xE9;
        const size_t addrdisp = CalcOffset((uint8*)node->mem + 1, node->lpshellcode);
        if(!IsValidAddrDisp(addrdisp))
          {
          if(!SetHookUEF(node)) return false;
          break;
          }
        *(AddrDisp*)(&node->newcode[1]) = (AddrDisp)addrdisp;
        break;
        }
#ifndef _WIN64
      default:
        {
        node->newcode[0] = 0xFF;
        node->newcode[1] = 0x25;
        *(AddrDisp*)(&node->newcode[2]) = (AddrDisp)&node->lpshellcode;
        break;
        }
#else
      case 6: case 7: case 8: case 9: case 10: case 11: case 12: case 13:
        {
        node->newcode[0] = 0xFF;
        node->newcode[1] = 0x25;
        const size_t addrdisp = CalcOffset((uint8*)node->mem + 2,
                                           (uint8*)node->lpshellcode + 2);
        if(!IsValidAddrDisp(addrdisp))
          {
          if(!SetHookUEF(node)) return false;
          break;
          }
        *(AddrDisp*)(&node->newcode[2]) = (AddrDisp)addrdisp;
        break;
        }
      default:
        {
        node->newcode[0] = 0xFF;
        node->newcode[1] = 0x25;
        *(AddrDisp*)(&node->newcode[2]) = 0;
        *(uint8**)(&node->newcode[2 + sizeof(AddrDisp)]) = (uint8*)node->lpshellcode;
        break;
        }
#endif
      }
    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_FixJmpCode_Fail);
  return false;
  }

bool FixJmpCode_CtOff(HookNode* node, const bool calltable_offset)
  {
  XLIB_TRY
    {
    if(calltable_offset)
      {
      *(void**)&node->newcode = node->lpshellcode;
      }
    else
      {
      size_t addrdisp = CalcOffset(node->mem, node->lpshellcode);
      if(!IsValidAddrDisp(addrdisp))
        {
        SetLastHookErr(HookErr_AddDisp_Invalid);
        return false;
        }
      *(AddrDisp*)&node->newcode = (AddrDisp)addrdisp;
      }
    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_FixJmpCode_Fail);
  return false;
  }

bool HookIn(HookNode* node)
  {
  XLIB_TRY
    {
    const size_t thisbyte2cover =
    (node->byte2cover > sizeof(node->newcode))
    ? sizeof(node->newcode) : node->byte2cover;

    if(!Hookit(node->mem, node->newcode, thisbyte2cover)) return false;

    if(!AddNode(node))  return false;

    SetLastHookErr(HookErr_Success);
    return true;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_Hook_Fail);
  return false;
  }

HookNode* Hook(void*              hookmem,
               const size_t       hooksize,
               HookRoutine        routine,
               const bool         routinefirst,
               void*              p_shellcode)
  {
  //////////////////////////////////////////////////////////////////////////第一步：MakeNode
  HookNode* node = MakeNode(hookmem, hooksize, routine);
  if(node == nullptr) return nullptr;
  //////////////////////////////////////////////////////////////////////////第二步：MakeShellCode
  if(!MakeShellCode_Normal(node, routinefirst))
    {
    delete node;
    return nullptr;
    }
  if(!FixShellCode(node, p_shellcode))
    {
    delete node;
    return nullptr;
    }
  //////////////////////////////////////////////////////////////////////////第三步：FixJmpCode
  if(!FixJmpCode_Normal(node))
    {
    delete node;
    return nullptr;
    }
  //////////////////////////////////////////////////////////////////////////第四步：Hookin
  if(!HookIn(node))
    {
    delete node;
    return nullptr;
    }
  return node;
  }

HookNode* Hook(void*              hookmem,
               HookRoutine        routine,
               const bool         calltable_offset,
               const bool         routinefirst,
               void*              p_shellcode,
               const intptr_t     expandargc)
  {
  //////////////////////////////////////////////////////////////////////////第一步：MakeNode
  HookNode* node = MakeNode(hookmem, routine, calltable_offset);
  if(node == nullptr) return nullptr;
  //////////////////////////////////////////////////////////////////////////第二步：MakeShellCode
  if(!MakeShellCode_CtOff(node, routinefirst, expandargc))
    {
    delete node;
    return nullptr;
    }
  if(!FixShellCode(node, p_shellcode))
    {
    delete node;
    return nullptr;
    }
  //////////////////////////////////////////////////////////////////////////第三步：FixJmpCode
  if(!FixJmpCode_CtOff(node, calltable_offset))
    {
    delete node;
    return nullptr;
    }
  //////////////////////////////////////////////////////////////////////////第四步：Hookin
  if(!HookIn(node))
    {
    delete node;
    return nullptr;
    }
  return node;
  }

bool UnHook(HookNode* node, const bool errbreak)
  {
  XLIB_TRY
    {
    const size_t thisbyte2cover =
      (node->byte2cover > sizeof(node->newcode))
      ? sizeof(node->newcode) : node->byte2cover;

    if(memcmp(node->mem, node->oldcode, thisbyte2cover) == 0)
      {
      DeleteNode(node);
      SetLastHookErr(HookErr_UnHook_Restore);
      return false;
      }
    else
      {
      if(memcmp(node->mem, node->newcode, thisbyte2cover) == 0)
        {
        SetLastHookErr(HookErr_UnHook_BeCover);
        if(errbreak)  return false;
        }

      if(!Hookit(node->mem, node->oldcode, thisbyte2cover))
        {
        if(errbreak)  return false;
        }
      }
    return DeleteNode(node);
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_UnHook);
  return false;
  }

bool  HookClear()
  {
  XLIB_TRY
    {
    if(!ClearHookUEF())  return false;

    if(g_hook_list == nullptr)  return true;

    while(!g_hook_list->empty())
      {
      UnHook(*(g_hook_list->begin()), false);
      }
#ifdef FOR_RING0
    delete g_hook_list;
    g_hook_list = nullptr;
#endif
    return GetLastHookErr() == HookErr_Success;
    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_Clear);
  return false;
  }

bool MoveHookCallTableShellCode(void* mem)
  {
  XLIB_TRY
    {
    if(mem == nullptr)
      {
      gk_lp_HookShellCode_CtOff = (void*)HookShellCode_CtOff;
      SetLastHookErr(HookErr_Success);
      return true;
      }

    const uint8* lp = (uint8*)HookShellCode_CtOff;
    size_t len = 0;
    while(lp[len++]);

    Hookit(mem, lp, len);
    gk_lp_HookShellCode_CtOff = (void*)mem;

    }
  XLIB_CATCH
    {
    ;
    }
  SetLastHookErr(HookErr_MovShellCode_Fail);
  return false;
  }

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// 以下是附加的Hook to Log的代码
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//! 用于简化代码
#ifdef _WIN64
#   define fixx64  "\x48"
#else
#   define fixx64  ""
#endif

//!< 词法类型
enum LexicalType : uint8
  {
  LT_Error,
  LT_Value,
  LT_LeftParentheses,
  LT_RightParentheses,
  LT_LeftBracket,
  LT_RightBracket,
  LT_Len,
  LT_Mul,
  LT_Div,
  LT_Mod,
  LT_Add,
  LT_Sub,
  LT_Shl,
  LT_Shr,
  LT_And,
  LT_Xor,
  LT_Or,
  };

//! 词法
struct Lexical
  {
  LexicalType type;       //!< 词法类型
  string      code;       //!< 运算机器码
  };

//! 指定值，生成运算机器码
static string MakeValueCode(const size_t value)
  {
  string ret;
#ifndef _WIN64
  //push imm
  ret.push_back('\x68');
  ret.append((const char*)&value, sizeof(value));
#else
  //mov rax, imm   push rax
  ret.push_back('\x48');
  ret.push_back('\xB8');
  ret.append((const char*)&value, sizeof(value));
  ret.push_back('\x50');
#endif
  return ret;
  }

//! 指定寄存器名，生成运算机器码，如果指定的名字错误，返回nullptr
static const char* MakeRegs(const char* exp)
  {
  /*!
    寄存器名对应的运算机器码
    由于Ring0的SGI STL<map>不支持列表初始化，同时Ring0也不支持复杂全局对象初始化，故采用此列表格式
    特意去除\x00编码
  */
#ifndef _WIN64
  static const char* regsmap[] =
    {
    //push [ebp + XX]
    "eax", "\xFF\x75\x1C",
    "ecx", "\xFF\x75\x18",
    "edx", "\xFF\x75\x14",
    "ebx", "\xFF\x75\x10",
    "esp", "\xFF\x75\x0C",
    "ebp", "\xFF\x75\x08",
    "esi", "\xFF\x75\x04",
    //xor eax, eax   push [ebp + eax]
    "edi", "\x33\xC0\xFF\x34\x28",
    "efg", "\xFF\x75\x20",
    "eip", "\xFF\x75\x24",
    //movzx eax, byte ptr [ebp + XX]  push eax
    "al", "\x0F\xB6\x45\x1C\x50",
    "cl", "\x0F\xB6\x45\x18\x50",
    "dl", "\x0F\xB6\x45\x14\x50",
    "bl", "\x0F\xB6\x45\x10\x50",
    //movzx eax, byte ptr [ebp + XX + 1] push eax
    "ah", "\x0F\xB6\x45\x1D\x50",
    "ch", "\x0F\xB6\x45\x19\x50",
    "dh", "\x0F\xB6\x45\x15\x50",
    "bh", "\x0F\xB6\x45\x11\x50",
    //movzx eax, word ptr [ebp + XX]  push eax
    "ax", "\x0F\xB7\x45\x1C\x50",
    "cx", "\x0F\xB7\x45\x18\x50",
    "dx", "\x0F\xB7\x45\x14\x50",
    "bx", "\x0F\xB7\x45\x10\x50",
    "sp", "\x0F\xB7\x45\x0C\x50",
    "bp", "\x0F\xB7\x45\x08\x50",
    "si", "\x0F\xB7\x45\x04\x50",
    //xor eax, eax   movzx eax, word ptr [ebp + eax]   push eax
    "di", "\x33\xC0\x0F\xB7\x04\x28\x50",
    };
#else   // _WIN64
  static const char* regsmap[] =
    {
    //xor rax, rax  push [rbp + rax]
    "rcx", "\x48\x33\xC0\xFF\x34\x28",
    //push [rbp + XX]
    "rdx", "\xFF\x75\x08",
    "r8", "\xFF\x75\x10",
    "r9", "\xFF\x75\x18",
    "rax", "\xFF\x75\x20",
    "rbx", "\xFF\x75\x28",
    "rsp", "\xFF\x75\x30",
    "rbp", "\xFF\x75\x38",
    "rsi", "\xFF\x75\x40",
    "rdi", "\xFF\x75\x48",
    "r10", "\xFF\x75\x50",
    "r11", "\xFF\x75\x58",
    "r12", "\xFF\x75\x60",
    "r13", "\xFF\x75\x68",
    "r14", "\xFF\x75\x70",
    "r15", "\xFF\x75\x78",
    //xor rax, rax   mov al, XX   push [rbp + rax]
    "rfg", "\x48\x33\xC0\xB0\x80\xFF\x34\x28",
    "rip", "\x48\x33\xC0\xB0\x88\xFF\x34\x28",
    //xor rax, rax  dec rax  movzx rax, byte ptr [rbp+rax+1]   push rax
    "cl", "\x48\x33\xC0\x48\xFF\xC8\x48\x0F\xB6\x44\x05\x01\x50",
    //movzx rax, byte [rbp + XX]  push rax
    "dl", "\x48\x0F\xB6\x45\x08\x50",
    "r8b", "\x48\x0F\xB6\x45\x10\x50",
    "r9b", "\x48\x0F\xB6\x45\x18\x50",
    "al", "\x48\x0F\xB6\x45\x20\x50",
    "bl", "\x48\x0F\xB6\x45\x28\x50",
    "spl", "\x48\x0F\xB6\x45\x30\x50",
    "bpl", "\x48\x0F\xB6\x45\x38\x50",
    "sil", "\x48\x0F\xB6\x45\x40\x50",
    "dil", "\x48\x0F\xB6\x45\x48\x50",
    "r10b", "\x48\x0F\xB6\x45\x50\x50",
    "r11b", "\x48\x0F\xB6\x45\x58\x50",
    "r12b", "\x48\x0F\xB6\x45\x60\x50",
    "r13b", "\x48\x0F\xB6\x45\x68\x50",
    "r14b", "\x48\x0F\xB6\x45\x70\x50",
    "r15b", "\x48\x0F\xB6\x45\x78\x50",
    //movzx rax, byte ptr [rbp+ XX +1]   push rax
    "ch", "\x48\x0F\xB6\x45\x01\x50",
    "dh", "\x48\x0F\xB6\x45\x09\x50",
    "ah", "\x48\x0F\xB6\x45\x11\x50",
    "bh", "\x48\x0F\xB6\x45\x19\x50",
    //xor rax, rax  dec rax  movzx rax, word ptr [rbp+rax+1]   push rax
    "cx", "\x48\x33\xC0\x48\xFF\xC8\x48\x0F\xB7\x44\x05\x01\x50",
    //movzx rax, word ptr [rbp + XX]  push rax
    "dx", "\x48\x0F\xB7\x45\x08\x50",
    "r8w", "\x48\x0F\xB7\x45\x10\x50",
    "r9w", "\x48\x0F\xB7\x45\x18\x50",
    "ax", "\x48\x0F\xB7\x45\x20\x50",
    "bx", "\x48\x0F\xB7\x45\x28\x50",
    "sp", "\x48\x0F\xB7\x45\x30\x50",
    "bp", "\x48\x0F\xB7\x45\x38\x50",
    "si", "\x48\x0F\xB7\x45\x40\x50",
    "di", "\x48\x0F\xB7\x45\x48\x50",
    "r10w", "\x48\x0F\xB7\x45\x50\x50",
    "r11w", "\x48\x0F\xB7\x45\x58\x50",
    "r12w", "\x48\x0F\xB7\x45\x60\x50",
    "r13w", "\x48\x0F\xB7\x45\x68\x50",
    "r14w", "\x48\x0F\xB7\x45\x70\x50",
    "r15w", "\x48\x0F\xB7\x45\x78\x50",
    //xor rax, rax  xor rcx, rcx  dec rcx  mov eax, [rbp+rcx+1]  push rax
    "ecx", "\x48\x33\xC0\x48\x33\xC9\x48\xFF\xC9\x8B\x44\x0D\x01\x50",
    //xor rax, rax  mov eax, [rbp + 1C]  push rax
    "edx", "\x48\x33\xC0\x8B\x45\x08\x50",
    "r8d", "\x48\x33\xC0\x8B\x45\x10\x50",
    "r9d", "\x48\x33\xC0\x8B\x45\x18\x50",
    "eax", "\x48\x33\xC0\x8B\x45\x20\x50",
    "ebx", "\x48\x33\xC0\x8B\x45\x28\x50",
    "esp", "\x48\x33\xC0\x8B\x45\x30\x50",
    "ebp", "\x48\x33\xC0\x8B\x45\x38\x50",
    "esi", "\x48\x33\xC0\x8B\x45\x40\x50",
    "edi", "\x48\x33\xC0\x8B\x45\x48\x50",
    "r10d", "\x48\x33\xC0\x8B\x45\x50\x50",
    "r11d", "\x48\x33\xC0\x8B\x45\x58\x50",
    "r12d", "\x48\x33\xC0\x8B\x45\x60\x50",
    "r13d", "\x48\x33\xC0\x8B\x45\x68\x50",
    "r14d", "\x48\x33\xC0\x8B\x45\x70\x50",
    "r15d", "\x48\x33\xC0\x8B\x45\x78\x50",
    };
#endif  // _WIN64
  string ee(exp);
  for(auto& ch : ee)
    {
    ch = (char)tolower(ch);
    }
  size_t count = _countof(regsmap);
  for(size_t i = 0; i < count; i += 2)
    {
    if(0 == strcmp(regsmap[i], ee.c_str()))
      {
      return regsmap[i + 1];
      }
    }
  return nullptr;
  }

//! 指定表达式，解析成运算机器码，返回空串表示错误
static string MakeExpression(const char* exp)
  {
  if(exp == nullptr || strlen(exp) == 0)
    {
    SetLastHookErr(HookErr_EmptyExpression);
    return string();
    }
  //先判定是不是寄存器（小写），如果是，则直接返回
  auto code = MakeRegs(exp);
  if(code != nullptr)
    {
    return string(code);
    }
  //再判定是不是常量值，如果是，也返回
  size_t readlen = 0;
  size_t value = hex2value(string(exp), &readlen, 0, true);
  if(readlen != 0)
    {
    return MakeValueCode(value);
    }
#ifndef FOR_RING0
  //再解析是不是模块，先默认全部为模块名
  HMODULE mod = GetModuleHandleA(exp);
  if(mod != nullptr)
    {
    return MakeValueCode((size_t)mod);
    }
  //尝试提取模块名，以最后一个.为分隔
  const string expression(exp);
  auto itt = expression.end();
  for(auto it = expression.begin(); it != expression.end(); ++it)
    {
    if(*it == '.') itt = it;
    }
  //如果没有分隔符，全部模块名也不对，则此表达式无法解析
  if(itt == expression.end())
    {
    SetLastHookErr(HookErr_InvailExpression);
    return string();
    }
  string modname(expression.begin(), itt);
  if(modname.empty())
    {
    mod = GetModuleHandle(nullptr);
    }
  else
    {
    mod = GetModuleHandleA(modname.c_str());
    }
  if(mod == nullptr)
    {
    SetLastHookErr(HookErr_InvaildModuleName);
    return string();
    }
  //尝试提取offset或procname
  string offfun(itt + 1, expression.end());
  //如果为空，则直接返回模块基址
  if(offfun.empty())
    {
    return MakeValueCode((size_t)mod);
    }
  //尝试优先处理成offset
  readlen = 0;
  value = hex2value(offfun, &readlen, 0, true);
  if(readlen != 0)
    {
    return MakeValueCode(value + (size_t)mod);
    }
  value = (size_t)GetProcAddress(mod, offfun.c_str());
  if(value == 0)
    {
    SetLastHookErr(HookErr_InvaildProcAddress);
    return string();
    }
  return MakeValueCode(value);
#else   // FOR_RING0
  SysDriverSnap sds;
  //额外处理"."
  if(0 == strcmp(".", exp))
    {
    return MakeValueCode((size_t)sds.begin()->ImageBaseAddress);
    }
  for(const SYSTEM_MODULE& st : sds)
    {
    if(0 == strcmp((const char*)st.Name + st.NameOffset, exp))
      {
      return MakeValueCode((size_t)st.ImageBaseAddress);
      }
    }
  SetLastHookErr(HookErr_InvailExpression);
  return string();
#endif  // FOR_RING0
  }

//! 指定后缀，解析成运算机器码，如果指定的后缀错误，返回nullptr
static const char* MakeSufFix(const char suf)
  {
  switch(suf)
    {
    //pop eax   movsx eax, al  push eax
    //pop rax   movsx rax, al  push rax
    case 'b': return "\x58" fixx64 "\x0F\xBE\xC0\x50";
    //pop eax   movzx eax, al  push eax
    //pop rax   movzx rax, al  push rax
    case 'B': return "\x58" fixx64 "\x0F\xB6\xC0\x50";
    //pop eax   movsx eax, ax  push eax
    //pop rax   movsx rax, ax  push rax
    case 'w': return "\x58" fixx64 "\x0F\xBF\xC0\x50";
    //pop eax   movzx eax, ax  push eax
    //pop rax   movzx rax, ax  push rax
    case 'W': return "\x58" fixx64 "\x0F\xB7\xC0\x50";
#ifdef _WIN64
    case 'd': return "\x48\x0F\xBE\x44\x24\x03\x48\xC1\xEB\x20\x89\x44\x24\x04";
      //movsx rax, byte ptr [rsp+3]   shr rax, 20  mov dword ptr [rsp+4], eax
    case 'D': return "\x48\x33\xC0\x89\x44\x24\x04";
      //xor rax, rax  mov dword ptr [rsp+4], eax
#endif
    default:
      break;
    }
  return nullptr;
  }

//! 指定词组，生成运算机器码
static string DoLexical(vector<Lexical>& vec);

//! 指定词组，括号限定类型，聚合机器码
static bool DoFix(vector<Lexical>& vec, LexicalType left, LexicalType right)
  {
  while(true)
    {
    auto its = vec.begin();
    for(; its != vec.end(); ++its)
      {
      if(its->type == left)  break;
      }
    if(its == vec.end())  break;
    //如果存在左限定符，则开始寻找匹配的右限定符
    intptr_t leftfind = 0;
    auto ite = vec.end();
    for(auto it = its + 1; it != vec.end(); ++it)
      {
      if(it->type == left)
        {
        ++leftfind;
        continue;
        }
      if(it->type == right)
        {
        if(leftfind == 0)
          {
          ite = it;
          break;
          }
        --leftfind;
        }
      }
    if(ite == vec.end())
      {
      SetLastHookErr(HookErr_NoMatch);
      return false;
      }
    if(ite + 1 - its <= 2)
      {
      SetLastHookErr(HookErr_MatchEmpty);
      return false;
      }
    vector<Lexical> vv(its + 1, ite);   //提取括号中间的表达式
    Lexical lex = { LT_Value, DoLexical(vv) + ite->code };  //生成算式
    vec.insert(vec.erase(its, ite + 1), lex); //替换词组
    }
  return true;
  }

//! 指定词组，与需要处理的操作符，聚合机器码
static bool DoOperator(vector<Lexical>& vec, const LexicalType* op)
  {
  while(true)
    {
    auto it = vec.begin();
    for(; it != vec.end(); ++it)
      {
      bool find = false;
      for(const LexicalType* lt = op; *lt != LT_Error; ++lt)
        {
        if(*lt == it->type)
          {
          find = true;
          break;
          }
        }
      if(find)  break;
      }
    if(it == vec.end()) break;
    //操作符需要左右操作数
    auto itl = it - 1;
    if(it == vec.begin() || itl->type != LT_Value)
      {
      SetLastHookErr(HookErr_NeedLeftOp);
      return false;
      }
    auto itr = it + 1;
    if(itr == vec.end() || itr->type != LT_Value)
      {
      SetLastHookErr(HookErr_NeedRightOp);
      return false;
      }
    Lexical lex = { LT_Value, itl->code + itr->code + it->code }; //后缀式
    vec.insert(vec.erase(itl, itr + 1), lex); //替换词组
    }
  return true;
  }

//! 指定词组，生成最后的值算式
static string DoLexical(vector<Lexical>& vec)
  {
  //优先判定一下单值的情况
  if(vec.size() == 1)
    {
    const auto& vv = *vec.begin();
    if(vv.type != LT_Value)
      {
      SetLastHookErr(HookErr_NeedValue);
      return string();
      }
    return vv.code;
    }
  //扫描()
  if(!DoFix(vec, LT_LeftParentheses, LT_RightParentheses))  return string();
  //扫描[]
  if(!DoFix(vec, LT_LeftBracket, LT_RightBracket))  return string();
  //扫描#/##
  static const LexicalType lt0[] = { LT_Len, LT_Error };
  if(!DoOperator(vec, lt0))  return string();
  //扫描*/%
  static const LexicalType lt1[] = { LT_Mul, LT_Div, LT_Mod, LT_Error };
  if(!DoOperator(vec, lt1))  return string();
  //扫描+-
  static const LexicalType lt2[] = { LT_Add, LT_Sub, LT_Error };
  if(!DoOperator(vec, lt2))  return string();
  //扫描<< >>
  static const LexicalType lt3[] = { LT_Shl, LT_Shr, LT_Error };
  if(!DoOperator(vec, lt3))  return string();
  //扫描&
  static const LexicalType lt4[] = { LT_And, LT_Error };
  if(!DoOperator(vec, lt4))  return string();
  //扫描^
  static const LexicalType lt5[] = { LT_Xor, LT_Error };
  if(!DoOperator(vec, lt5))  return string();
  //扫描|
  static const LexicalType lt6[] = { LT_Or, LT_Error };
  if(!DoOperator(vec, lt6))  return string();
  //解析结束后，应该只剩一个值
  if(vec.size() != 1)
    {
    SetLastHookErr(HookErr_MoreExpression);
    return string();
    }
  const auto& vv = *vec.begin();
  if(vv.type != LT_Value)
    {
    SetLastHookErr(HookErr_InvailExpression);
    return string();
    }
  return vv.code;
  }

//! 指定数据描述，生成运算机器码，返回空串表示错误
static string MakeDescibe(const char* descibe)
  {
  if(descibe == nullptr)
    {
    SetLastHookErr(HookErr_EmptyExpression);
    return string();
    }
  //移除空白
  string desc;
  for(; *descibe != '\0'; ++descibe)
    {
    if(*descibe != ' ' && *descibe != '\t' && *descibe != '\r' && *descibe != '\n')
      desc.push_back(*descibe);
    }
  if(desc.empty())
    {
    SetLastHookErr(HookErr_EmptyExpression);
    return string();
    }
  //添加结尾0，方便后继处理
  desc.push_back('\0');
  vector<Lexical> vec;            //词法集合
  string exp;                     //表达式缓存
  for(auto it = desc.begin(); it != desc.end(); ++it)
    {
    const auto ch = *it;
    switch(ch)
      {
      case '(':   case ')':   case '[':   case ']':
      case '*':   case '/':   case '%':   case '+':
      case '-':   case '<':   case '>':   case '&':
      case '^':   case '|':   case '\0':
        if(!exp.empty())
          {
          Lexical lex = { LT_Value, MakeExpression(exp.c_str()) };
          vec.push_back(lex);
          exp.clear();
          }
        break;
      }
    if(ch == '\0')  break;
    Lexical lex;
    switch(ch)
      {
      case '+':
        //pop eax   add [esp], eax
        //pop rax   add [rsp], rax
        lex = { LT_Add, string("\x58" fixx64 "\x01\x04\x24") };
        break;
      case '*':
        //pop ecx pop eax cdq mul ecx push eax
        //pop rcx pop rax cdq mul rcx push rax
        lex = { LT_Mul, string("\x59\x58\x99" fixx64 "\xF7\xE1\x50") };
        break;
      case '[':
        lex = { LT_LeftBracket };
        break;
      case ']':
        {
        string ss("\x58" fixx64 "\x8B\x00\x50", (sizeof(size_t) == 8) ? 5 : 4);
        if(desc.end() != (it + 1))
          {
          auto code = MakeSufFix(*(it + 1));
          if(code != nullptr)
            {
            ++it;
            ss += code;
            }
          }
        lex = { LT_RightBracket, ss };
        break;
        }
      case '(':
        lex = { LT_LeftParentheses };
        break;
      case ')':
        {
        string code;
        if(desc.end() != (it + 1))
          {
          auto c = MakeSufFix(*(it + 1));
          if(c != nullptr)
            {
            code = string(c);
            ++it;
            }
          }
        lex = { LT_RightParentheses, code };
        break;
        }
      case '#':
        {
        //取长度操作，前置放一个伪造的空值，便于后继解析
        lex = { LT_Value };
        vec.push_back(lex);
        //xchg [esp], edi  pushfd  cld  xor eax, eax  xor ecx, ecx
        //xchg [rsp], rdi  pushfq  cld  xor rax, rax  xor rcx, rcx
        string code(fixx64 "\x87\x3C\x24\x9C\xFC" fixx64 "\x33\xC0" fixx64 "\x33\xC9");
#ifndef _WIN64
        //dec ecx
        code += "\x49\x66\xF2";
#else
        //dec rcx
        code += "\x48\xFF\xC9\xF2\x66";
#endif
        if(desc.end() != (it + 1) && *(it + 1) == '#')
          {
          //repne scasw
          ++it;
          code += "\xAF";
          }
        else
          {
          //repne scasb
          code += "\xAE";
          }
        //popfd  pop edi  not ecx  push ecx
        //popfq  pop rdi  not rcx  push rcx
        code += "\x9D\x5F" fixx64 "\xF7\xD1\x51";
        lex = { LT_Len, code };
        break;
        }
      case '/':
        //pop ecx pop eax cdq div ecx push eax
        //pop rcx pop rax cdq div rcx push rax
        lex = { LT_Div, string("\x59\x58\x99" fixx64 "\xF7\xF1\x50") };
        break;
      case '%':
        //pop ecx pop eax cdq div ecx push edx
        //pop rcx pop rax cdq div rcx push rdx
        lex = { LT_Mod, string("\x59\x58\x99" fixx64 "\xF7\xF1\x52") };
        break;
      case '-':
        //pop eax  sub [esp], eax
        //pop rax  sub [rsp], rax
        lex = { LT_Sub, string("\x58" fixx64 "\x29\x04\x24") };
        break;
      case '<':
        {
        ++it;
        if(desc.end() == it || *it != '<')
          {
          SetLastHookErr(HookErr_Shl);
          return string();
          }
        //pop ecx  pop eax  shl eax, cl  push eax
        //pop rcx  pop rax  shl rax, cl  push rax
        lex = { LT_Shl, string("\x59\x58" fixx64 "\xD3\xE0\x50") };
        break;
        }
      case '>':
        {
        ++it;
        if(desc.end() == it || *it != '>')
          {
          SetLastHookErr(HookErr_Shr);
          return string();
          }
        //pop ecx  pop eax  shr eax, cl  push eax
        //pop rcx  pop rax  shr rax, cl  push rax
        lex = { LT_Shr, string("\x59\x58" fixx64 "\xD3\xE8\x50") };
        break;
        }
      case '&':
        //pop eax  and [esp], eax
        //pop rax  and [rsp], rax
        lex = { LT_And, string("\x58" fixx64 "\x21\x04\x24") };
        break;
      case '^':
        //pop eax  xor [esp], eax
        //pop rax  xor [rsp], rax
        lex = { LT_Xor, string("\x58" fixx64 "\x31\x04\x24") };
        break;
      case '|':
        //pop eax  or [esp], eax
        //pop rax  or [rsp], rax
        lex = { LT_Or, string("\x58" fixx64 "\x09\x04\x24") };
        break;
      default:
        exp.push_back(ch);
        continue;
      }
    vec.push_back(lex);
    }
  const string code(DoLexical(vec));
  if(code.empty())  return string();
#ifndef _WIN64
  //push ebp   mov ebp, [rsp + 8]   ...  pop eax  pop ebp  retn
  return string("\x55\x8B\x6C\x24\x08") + code + string("\x58\x5D\xC3");
#else
  //push rbp   mov rbp, rcx  ...  pop rax  pop rbp  retn   注意x64的传参
  return string("\x55\x48\x89\xCD") + code + string("\x58\x5D\xC3");
#endif
  }

//! 默认的数据输出函数，只是简单的转换成Dump格式
static void DefaultHook2LogOut(const char* const   head,
                               const void*         buf,
                               const size_t        size)
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

//! Hook的输出处理封装，主要是引入异常处理，因为生成的硬编码在x64下难以引入异常处理
typedef size_t (__cdecl *DescibeFunction)(CPU_ST* lpcpu);
static void __cdecl Hook2LogRoutine(CPU_ST*             lpcpu,
                                    DescibeFunction     datas,
                                    DescibeFunction     lens,
                                    const char* const   head,
                                    hook2log_out_func   datafunc)
  {
  XLIB_TRY
    {
    const void* data = (const void*)datas(lpcpu);
    const size_t len = lens(lpcpu);
    if(datafunc == nullptr) DefaultHook2LogOut(head, data, len);
    else  datafunc(head, data, len);
    }
  XLIB_CATCH
    {
    xerr << "Hook2Log Routine Exception";
    }
  }

//! 指定数据描述、长度描述、头部数据说明、数据输出函数、头部
static string MakeCode(const char*        data_descibe,
                       const char*        len_descibe,
                       const char*        head,
                       hook2log_out_func  datafunc)
  {
  const string datas = MakeDescibe(data_descibe);
  if(datas.empty()) return string();

  const string lens = MakeDescibe(len_descibe);
  if(lens.empty()) return string();

  if(head == nullptr) head = "";
  const string heads(head);

  string code;
  //输出函数地址入栈
  code += MakeValueCode((size_t)datafunc);
  //头部说明入栈
  code.push_back('\xE8');
  AddrDisp size = (AddrDisp)heads.size() + sizeof(size_t);
  code.append((const char*)&size, sizeof(size));
  code += heads;
  code.append(sizeof(size_t), '\0');
  //长度函数入栈
  code.push_back('\xE8');
  size = (AddrDisp)lens.size();
  code.append((const char*)&size, sizeof(size));
  code += lens;
  //数据函数入栈
  code.push_back('\xE8');
  size = (AddrDisp)datas.size();
  code.append((const char*)&size, sizeof(size));
  code += datas;
#ifndef _WIN64
  //push [esp+14]
  code.append("\xFF\x74\x24\x14");
  //mov eax,Routine
  code.append("\xB8");
  const size_t lpRoutine = (size_t)Hook2LogRoutine;
  code.append((const char*)&lpRoutine, sizeof(lpRoutine));
  //call eax  add esp, 14   retn 4      x86的Hook Routine是__stdcall
  code.append("\xFF\xD0\x83\xC4\x14\xC2\x04\x00", 8);
#else
  //push [rsp + 28]  rcx rdx r8 r9    x64的调用约定需要额外做寄存器
  code.append("\x48\xFF\x74\x24\x28");
  code.append("\x48\x8B\x0C\x24");
  code.append("\x48\x8B\x54\x24\x08");
  code.append("\x4C\x8B\x44\x24\x10");
  code.append("\x4C\x8B\x4C\x24\x18");
  //mov rax, rRoutine
  code.append("\x48\xB8");
  const size_t lpRoutine = (size_t)Hook2LogRoutine;
  code.append((const char*)&lpRoutine, sizeof(lpRoutine));
  //call rax  add rsp, 28   retn   x64的Hook Routine遵循x64调用约定
  code.append("\xFF\xD0\x48\x83\xC4\x28\xC3");
#endif
  return code;
  }

HookNode* Hook2Log(void*              hookmem,
                   const size_t       hooksize,
                   const char*        data_descibe,
                   const char*        len_descibe,
                   const bool         logfirst,
                   const char*        head_msg,
                   hook2log_out_func  log_out_func,
                   void*              p_shellcode)
  {
  //先生成运算码
  string code(MakeCode(data_descibe, len_descibe, head_msg, log_out_func));
  if(code.empty())  return nullptr;

  //生成node，Routine此时胡乱指定，没有实际意义
  HookNode* node = MakeNode(hookmem, hooksize, (void*)code.c_str());
  if(node == nullptr) return nullptr;

  //复制运算码，增大shellcode对象容量，以避免在生成后面shellcode时，位置变化
  node->shellcode.assign((const uint8*)code.c_str(), code.size());
  node->shellcode.reserve(code.size() + hookshellcode_normal_prefix * 2);

  //写入真实的Routine地址，即生成的运算码
  node->routine = (void*)node->shellcode.c_str();

  if(!MakeShellCode_Normal(node, logfirst))
    {
    delete node;
    return nullptr;
    }
  if(!FixShellCode(node, p_shellcode))
    {
    delete node;
    return nullptr;
    }
  if(!FixJmpCode_Normal(node))
    {
    delete node;
    return nullptr;
    }

  if(!HookIn(node))
    {
    delete node;
    return nullptr;
    }
  return node;
  }

HookNode* Hook2Log(void*              hookmem,
                   const char*        data_descibe,
                   const char*        len_descibe,
                   const bool         calltable_offset,
                   const bool         logfirst,
                   const char*        head_msg,
                   hook2log_out_func  log_out_func,
                   void*              p_shellcode,
                   const intptr_t     expandargc)
  {
  string code(MakeCode(data_descibe, len_descibe, head_msg, log_out_func));
  if(code.empty())  return nullptr;

  HookNode* node = MakeNode(hookmem, (void*)code.c_str(), calltable_offset);
  if(node == nullptr) return nullptr;

  node->shellcode.assign((const uint8*)code.c_str(), code.size());
  node->shellcode.reserve(code.size() + hookshellcode_ctoff_prefix * 2);

  node->routine = (void*)node->shellcode.c_str();

  if(!MakeShellCode_CtOff(node, logfirst, expandargc))
    {
    delete node;
    return nullptr;
    }
  if(!FixShellCode(node, p_shellcode))
    {
    delete node;
    return nullptr;
    }
  if(!FixJmpCode_CtOff(node, calltable_offset))
    {
    delete node;
    return nullptr;
    }

  if(!HookIn(node))
    {
    delete node;
    return nullptr;
    }
  return node;
  }


#ifdef _XLIB_TEST_

#pragma code_seg(".text")
__declspec(allocate(".text"))
static const uint8 HookTestHook[] = {"\
\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\
\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\
\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\xC3" };
#pragma code_seg()

static void __stdcall HookTestRoutine(CPU_ST* lpcpu)
  {
#ifndef _WIN64
  lpcpu->regEax = 0x42104210;
  lpcpu->regEdx = 0;
#else
  lpcpu->regRax = 0x42104210;
  lpcpu->regRdx = 0;
#endif
  }

static void Hook2LogTestOut(const char* const ,
                            const void*,
                            const size_t )
  {
  cout << "\b\b\b\b";
  SHOW_TEST_RESULT(true);
  }

ADD_XLIB_TEST(HOOK)
  {
  SHOW_TEST_INIT;

  auto done = false;

  try
    {
    typedef size_t(*testfunc)();
    testfunc tf = (testfunc)&HookTestHook;

    SHOW_TEST_HEAD("hook");
    Hook((void*)&HookTestHook, 15, HookTestRoutine, true);
    done = (0x42104210 == tf());
    SHOW_TEST_RESULT(done);

    SHOW_TEST_HEAD("hook clear");
    HookClear();
    done = (0x42104210 != tf());
    SHOW_TEST_RESULT(done);

    SHOW_TEST_HEAD("hook2log");
    Hook2Log((void*)&HookTestHook, 15, "esp", "10", true, nullptr, Hook2LogTestOut);
    cout << "XXXX";
    tf();
    HookClear();
    }
  catch(const std::runtime_error& e)
    {
    cout << "Hook Exception : " << e.what() << endl;
    }
  catch(...)
    {
    cout << "Hook Exception!!!"<< endl;
    }
  }

#endif  // _XLIB_TEST_

#endif  // _WIN32