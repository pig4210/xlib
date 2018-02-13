#include "caller.h"

#ifdef _WIN32

#include <intrin.h>
#include <vector>

#include "hook.h"
#include "xevent.h"

using namespace std;

//处理普通call
#ifndef _WIN64

#pragma warning(push)
#pragma warning(disable:4100) //warning C4100: 未引用的形参
__declspec(naked) uint64 fakecall(const caller* nowthis, void* argv)
  {
  __asm
    {
    push    0x01230814
    push    esp                           //注意后面会修正这个值，但不即时修正
    push    0x42104210                    //以上三个特征值为最后堆栈平衡设置
    mov     dword ptr [esp + 4 * 1], esp  //修正特征指向
    pushfd
    pushad

    mov     esi, esp                      //●esi指向寄存器保护
    mov     ebx, dword ptr [esp + 4 * 13] //●ebx为this对象指针 ->ad fd # esp # ret
    movzx   edx, [ebx].caller_argc        //edx为参数个数
    add     edx, 10                       //              ->ad fd func
    shl     edx, 2
    sub     esp, edx                      //扩展局部堆栈
    mov     edi, esp                      //●edi保存局部堆栈指针

    xor     ecx, ecx
    mov     cl, 9                         //              ->ad fd
    rep movsd                             //复制原始寄存器保护
    mov     esi, dword ptr [esi + 4 * 5]  //●esi指向参数 -># esp # ret this
    
    ////////////////////////开始修改可能存在的对象指针寄存器
    mov     ebp, [ebx].caller_this        //●ebp为对象指针，注意后继可能用于成员函数的调用
    test    ebp, ebp                      //对象指针存在才修改
    jz      fakecall_copyregs
    movzx   edx, [ebx].caller_thisreg     //edx为对象指针寄存器指示
    mov     cl, 8                         //八个寄存器  开始遍历
fakecall_copythisregs:
    dec     ecx
    js      fakecall_copyregs             //八轮遍历
    shr     edx, 1                        //检测对象指针寄存器指示
    jnc     fakecall_copythisregs
    mov     dword ptr [esp + ecx * 4], ebp //copy对象指针  对象指针只有一个，所以只修改一次

    ////////////////////////开始修改可能存在的寄存器传参。注意之前esi已经指向参数
fakecall_copyregs:
    movzx   edx, [ebx].caller_regs        //edx为寄存器传参指示
    mov     cl, 8                         //注意没有清零，类初始化有检测，所以故意不清零
fakecall_loop_copyregs:
    dec     ecx
    js      fakecall_regs_ok
    shr     edx, 1                        //依次检测寄存器参数指示
    jnc     fakecall_loop_copyregs
    lodsd                                 //促使esi跳过寄存器参数
    mov     dword ptr [esp + ecx * 4], eax //如果需要此寄存器，改动之
    jmp     fakecall_loop_copyregs        //八个寄存器都要过一遍
fakecall_regs_ok:

    ////////////////////////开始计算并压入函数地址
    mov     eax, [ebx].caller_func        //读取函数地址（或虚成员函数偏移）
    mov     edx, eax
    shr     edx, 16
    jnz     fakecall_func                 //判定是函数地址，或者是虚成员函数偏移
    mov     edx, dword ptr [edx + ebp]    //读取虚函数表，注意不用mov edx, [ebp]是为了不产生00编码，注意[ebp+edx]也不行
    mov     eax, dword ptr [eax + edx]    //读取虚成员函数
fakecall_func:
    stosd                                 //压入函数地址
    
    ////////////////////////开始复制剩余参数（除去寄存器参数）。注意在前面esi已经跳过可能的寄存器参数了。而edi已经很早就准备好了
    movzx   ecx, [ebx].caller_argc        //ecx为参数个数
    rep movsd                             //复制剩余参数

    ////////////////////////所有工作结束，弹出修改过的寄存器组，开始正式进入函数调用
    popad                                 //弹出保存的寄存器，当然，某些值可能已经被修改
    popfd
    lea     esp, dword ptr [esp + 4]      //再次虚弹，但这次只能操作esp
    call    dword ptr [esp - 4]           //万事俱备，调用已经准备的函数

    ////////////////////////函数调用结束，开始验证特征值以平衡堆栈
    push    eax                           //环境保护
    mov     eax, esp  
fakecall_findrealesp:
    add     esp, 4
    cmp     dword ptr [esp], 0x42104210
    jnz     fakecall_findrealesp
    cmp     dword ptr [esp + 4], esp
    jnz     fakecall_findrealesp
    cmp     dword ptr [esp + 8], 0x01230814
    jnz     fakecall_findrealesp

    sub     esp, 4                        //跳过状态
    push    dword ptr [eax]               //修改eax
    sub     esp, 4                        //跳过ecx
    push    edx                           //修改edx
    sub     esp, 4 * 5                    //提升栈

    popad                                 //恢复原始状态
    popfd
    lea     esp, dword ptr[esp + 4 * 3]   //最后一次平衡栈
    retn                                  //搞定收功

    add     byte ptr [eax], al            //用于标记函数结尾00
    }
  }
#pragma warning(pop)

#else   // _WIN64

#ifndef __INTEL_COMPILER

#pragma message(" -- 不使用Intel C++ Compiler，CallerShellCode版本可能较旧")
#pragma code_seg(".text")
__declspec(allocate(".text"))
static const uint8 fakecall[] = { "\
\x68\x14\x08\x23\x01\x54\x68\x10\x42\x10\x42\x48\x89\x64\x24\x08\
\x9C\x51\x52\x41\x50\x41\x51\x50\x53\x48\x89\xCB\x54\x55\x48\x89\
\xD5\x56\x57\x41\x52\x41\x53\x41\x54\x41\x55\x41\x56\x41\x57\x48\
\x89\xE6\x48\x33\xC9\xB1\x11\x48\x0F\xB6\x53\x14\x48\x03\xD1\x48\
\xC1\xE2\x03\x48\x2B\xE2\x48\x89\xE7\xF3\x48\xA5\x48\x89\xEE\x48\
\x8B\x6B\x08\x48\x85\xED\x74\x15\x48\x0F\xB7\x53\x10\xB1\x10\x48\
\xFF\xC9\x78\x09\x48\xD1\xEA\x73\xF6\x48\x89\x2C\xCC\x48\x0F\xB7\
\x53\x12\xB1\x10\x48\xFF\xC9\x78\x0D\x48\xD1\xEA\x73\xF6\x48\xAD\
\x48\x89\x04\xCC\xEB\xEE\x48\x8B\x03\x48\x89\xC2\x48\xC1\xEA\x10\
\x75\x08\x48\x8B\x14\x2A\x48\x8B\x04\x10\x48\x89\x44\x24\x48\x48\
\x0F\xB6\x4B\x14\xF3\x48\xA5\x41\x5F\x41\x5E\x41\x5D\x41\x5C\x41\
\x5B\x41\x5A\x5F\x5E\x5D\x5B\x5B\x58\x41\x59\x41\x58\x5A\x59\x9D\
\x41\x51\x41\x50\x52\x51\xFF\x54\x24\xE0\x50\x48\x83\xC4\x08\x48\
\x81\x3C\x24\x10\x42\x10\x42\x75\xF2\x48\x39\x64\x24\x08\x75\xEB\
\x48\x81\x7C\x24\x10\x14\x08\x23\x01\x75\xE0\x48\x33\xC9\xB1\x11\
\x48\xC1\xE1\x03\x48\x2B\xE1\x41\x5F\x41\x5E\x41\x5D\x41\x5C\x41\
\x5B\x41\x5A\x5F\x5E\x5D\x5B\x5B\x41\x59\x41\x59\x41\x58\x5A\x59\
\x9D\x48\x8D\x64\x24\x18\xC3\x00"};
#pragma code_seg()

#else   // __INTEL_COMPILER

#pragma warning(push)
#pragma warning(disable:4100) //warning C4100: 未引用的形参
//补充说明：虽然x64下只有fastcall调用约定，但存在Varargs、非原型函数等，所以还是可能存在除4个默认寄存器外的其它寄存器传参，所以设计这个fakecall与x86兼容
__declspec(naked) uint64 fakecall(const caller* nowthis, void* argv)
  {
  __asm
    {
    push    0x01230814                    //push imm只允许32位，但入栈是64位
    push    rsp                           //注意后面会修正这个值，但不即时修正
    push    0x42104210                    //以上三个特征值为最后堆栈平衡设置
    mov     qword ptr [rsp + 8 * 1], rsp  //修正入栈rsp，以便最后栈平衡计算，rsp指向特征
    pushfq
    
    push    rcx
    push    rdx
    push    r8
    push    r9
    push    rax
    push    rbx
    mov     rbx, rcx                      //●rbx为this
    push    rsp
    push    rbp
    mov     rbp, rdx                      //●rbp为argv
    push    rsi
    push    rdi
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rsi, rsp                      //●rsi指向寄存器保护
    xor     rcx, rcx
    mov     cl, 16 + 1

    movzx   rdx, [rbx].caller_argc
    add     rdx, rcx
    shl     rdx, 3
    sub     rsp, rdx                      //扩展局部堆栈
    mov     rdi, rsp                      //●rdi保存局部堆栈指针

    rep movsq                             //复制原始寄存器保护

    mov     rsi, rbp                      //●rsi指向参数

    ////////////////////////开始修改可能存在的对象指针寄存器
    mov     rbp, [rbx].caller_this        //●rbp为对象指针，注意后继可能用于成员函数的调用
    test    rbp, rbp
    jz      fakecall_copyregs
    movzx   rdx, [rbx].caller_thisreg     //rdx为对象指针寄存器指示
    mov     cl, 16                        //16个寄存器　开始遍历。不清零
fakecall_copythisregs:
    dec     rcx
    js      fakecall_copyregs             //16轮遍历
    shr     rdx, 1                        //检测对象指针寄存器指示
    jnc     fakecall_copythisregs
    mov     qword ptr [rsp + rcx * 8], rbp

    ////////////////////////开始修改可能存在的r16寄存器传参。注意之前rsi已经指向参数
fakecall_copyregs:
    movzx   rdx, [rbx].caller_regs        //rdx为寄存器传参指示
    mov     cl, 16                        //注意没有清零，类初始化有检测对象，保证此时rcx>=0
fakevall_loop_copyregs:
    dec     rcx
    js      fakecall_regs_ok
    shr     rdx, 1
    jnc     fakevall_loop_copyregs
    lodsq
    mov     qword ptr [rsp + rcx * 8], rax//如果需要此寄存器，改动之
    jmp     fakevall_loop_copyregs
fakecall_regs_ok:

    ////////////////////////开始计算并压入函数地址
    mov     rax, [rbx].caller_func        //读取函数地址（或虚成员函数偏移）
    mov     rdx, rax
    shr     rdx, 16
    jnz     fakecall_func                 //判定是函数地址，或者是虚成员函数偏移
    mov     rdx, qword ptr [rdx + rbp]    //读取虚函数表，注意不用mov rdx, [rbp]是为了不产生00编码，注意[rbp+rdx]也不行
    mov     rax, qword ptr [rax + rdx]    //读取虚成员函数
fakecall_func:
    mov     [rsp + 8 * 9], rax            //压入函数地址，修改是rsp位 ->r15~r10 rdi rsi rbp
      
    ////////////////////////开始复制剩余参数（除去寄存器参数）。注意在前面rsi已经跳过可能的寄存器参数了。而rdi已经很早就准备好了
    movzx   rcx, [rbx].caller_argc
    rep movsq                             //复制剩余参数

    ////////////////////////所有工作结束，弹出修改过的寄存器组，开始正式进入函数调用
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     rdi
    pop     rsi
    pop     rbp
    pop     rbx                            //pop rsp == func
    pop     rbx
    pop     rax
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    popfq

    push    r9
    push    r8
    push    rdx
    push    rcx
    call    dword ptr [rsp - 8 * 4]       //万事俱备，调用已经准备的函数
    
    ////////////////////////函数调用结束，开始验证特征值以平衡堆栈
    push    rax           //环境保护
fakecall_findrealesp:
    add     rsp, 8
    cmp     qword ptr [rsp], 0x42104210
    jnz     fakecall_findrealesp
    cmp     qword ptr [rsp + 8], rsp
    jnz     fakecall_findrealesp
    cmp     qword ptr [rsp + 16], 0x01230814
    jnz     fakecall_findrealesp

    xor     rcx, rcx
    mov     cl, 16 + 1
    shl     rcx, 3
    sub     rsp, rcx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     rdi
    pop     rsi
    pop     rbp
    pop     rbx                           //pop rsp
    pop     rbx
    pop     r9                            //pop rax
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    popfq

    lea     rsp, qword ptr [rsp + 8 * 3]  //最后一次平衡栈
    retn                                  //搞定收功
    add     byte ptr [rax], al            //用于标记函数结尾00
    }
  }
#pragma warning(pop)

#endif  // __INTEL_COMPILER

#endif  // _WIN64

static uint64(*g_lp_fakecall)(const caller* nowthis, void* argv) =
(uint64(*)(const caller *, void *))&fakecall;  //指向伪call，用于伪call代码的转移

uint64 caller::operator()(...) const
  {
  void** ap = (void**)_AddressOfReturnAddress();
  ++ap;
#ifndef _WIN64            // x86虽然可能默认ecx传递this，但也可能存在this入栈的情况
  if(*ap == this) ++ap;
#else                     // x64下，this一律入栈
  ++ap;
#endif
  return g_lp_fakecall(this, (void*)ap);
  }

static const intptr_t gk_default_argc = 0x8; //默认操作参数个数为0x8个

caller& caller::changethis(void* const lp_this)
  {
  caller_this = lp_this;
  return *this;
  }

caller& caller::operator[](void* const lp_this)
  {
  return changethis(lp_this);
  }

bool caller::init(void* const         func_vtno,
                  void* const         lp_this,
                  const CallerArgType this_regs,
                  const CallerArgType reg_s,
                  const intptr_t      expandargc)
  {
  if(lp_this && (this_regs==arg_nul))  return false;//如存在this，却没寄存器指示，出错

  caller_func = func_vtno;
  caller_this = lp_this;
  caller_thisreg = this_regs;

  caller_regs = (reg_s & (~this_regs));  //当指针寄存器指示存在时，不允许该寄存器传参
  intptr_t count = gk_default_argc + expandargc;
  if(count <= 0)  count = gk_default_argc;  //检测不让堆栈错误

#if defined(_WIN64) && !defined(__INTEL_COMPILER)
  //MS编译器好像存在有诡异的堆栈对齐假定，所以需要使其64位对齐
  if((count % 8) == 0) ++count;
#endif

  caller_argc = (CallerArgType)count;
  return true;
  }

void caller::mov_call(void* newmem)
  {
  if(newmem == NULL)
    {
    g_lp_fakecall = (uint64(*)(const caller *, void *))&fakecall;
    return;
    }

  const uint8* lp = (const uint8*)&fakecall;
  size_t len = 0;
  while(lp[len++]);
  Hookit(newmem, lp, len);

  g_lp_fakecall = (uint64(*)(const caller *, void *))newmem;
  }

//远程caller所需参数
struct caller_request
  {
  const caller*     requester;        //申请者
  void*             request_arg;      //参数指针
  xevent*           request_event;    //申请者通知
  uint64            return_value;     //申请结果
  bool              except_append;    //执行过程中出现异常
  };

typedef vector<caller_request*> caller_request_queue;    //申请队列类型

//针对Ring0与Ring3，接口相同，方法不同
#ifdef FOR_RING0
static caller_request_queue* g_crq = nullptr;

static caller_request_queue& caller_queue()
  {
  if(g_crq == nullptr)
    {
    g_crq = new caller_request_queue;
    }
  return *g_crq;
  }

static xevent* g_ce = nullptr;

static xevent& caller_event()
  {
  if(g_ce == nullptr)
    {
    g_ce = new xevent(FALSE, TRUE);
    }
  return *g_ce;
  }

void caller::freecallobj()
  {
  if(g_crq != nullptr)
    {
    delete g_crq;
    g_crq = nullptr;
    }
  if(g_ce != nullptr)
    {
    delete g_ce;
    g_ce = nullptr;
    }
  }

#else   // FOR_RING0

//远程请求队列
static caller_request_queue& caller_queue()
  {
  static caller_request_queue g_crq;
  return g_crq;
  }

//远程请求锁
static xevent& caller_event()
  {
  static xevent g_ce(FALSE, TRUE);
  return g_ce;
  }

#endif                                  // FOR_RING0

//发起远程申请
uint64 fakerequest(const caller* nowthis, void* argv)
  {
  xevent xe;
  caller_request req = {nowthis, argv, &xe, 0, false};
  caller_event().wait();      //等待可以申请
  caller_queue().push_back(&req);   //写入申请
  caller_event().set();       //释放申请
  xe.wait();                  //等待申请返回
  if(req.except_append)
    {
    //xerr << xfunexpt;
    return 0;
    }
  return req.return_value;
  }

uint64 caller::request(...) const
  {
  void** ap = (void**)_AddressOfReturnAddress();
  ++ap;
#ifndef _WIN64
  if(*ap == this) ++ap;
#else
  ++ap;
#endif
  return fakerequest(this, (void*)ap);
  }

//执行远程申请
static void caller_request_exec(caller_request& cr)
  {
  XLIB_TRY
    {
    cr.return_value = g_lp_fakecall(cr.requester, cr.request_arg);
    }
  XLIB_CATCH
    {
    cr.except_append = true;
    }
  }

//处理远程申请队列
void caller::execute_request()
  {
  if(caller_queue().empty())  return;   //申请队列为空，直接返回
  caller_event().wait();                //锁定申请

  for(caller_request* cr : caller_queue())
    {
    caller_request_exec(*cr);
    (*(cr->request_event)).set();
    }

  caller_queue().clear();               //清空队列
  caller_event().set();                 //解锁申请
  }

#ifdef _XLIB_TEST_

static void CallerTest()
  {
  cout << "\b\b\b\b";
  SHOW_TEST_RESULT(true);
  }

ADD_XLIB_TEST(CALLER)
  {
  SHOW_TEST_INIT;

  try
    {
    SHOW_TEST_HEAD("caller");
    caller func(&CallerTest);
    cout << "XXXX";
    func();
    }
  catch(const std::runtime_error& e)
    {
    cout << "Caller Exception : " << e.what() << endl;
    }
  catch(...)
    {
    cout << "Caller Exception!!!" << endl;
    }
  }

#endif  // _XLIB_TEST_

#endif  // _WIN32