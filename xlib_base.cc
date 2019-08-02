#include "xlib_base.h"

#if defined(_WIN32) && defined(FOR_RING0)

// 注意 #undef POOL_TAGGING 无效，并不能还原 API 定义。
#ifdef POOL_TAGGING
  #undef ExAllocatePool
  #undef ExAllocatePoolWithQuota
  #undef ExFreePool
#endif

void* __cdecl operator new(size_t size)
  {
  return ExAllocatePool(NonPagedPool, size);
  }

void* __cdecl operator new(size_t size, POOL_TYPE pooltype)
  {
  return ExAllocatePool(pooltype, size);
  }

void* __cdecl operator new[](size_t size)
  {
  return ExAllocatePool(NonPagedPool, size);
  }

void* __cdecl operator new[](size_t size, POOL_TYPE pooltype)
  {
  return ExAllocatePool(pooltype, size);
  }

void __cdecl operator delete(void* mem)
  {
  if(mem == nullptr)  return;
  ExFreePool(mem);
  }

void __cdecl operator delete[](void* mem)
  {
  if(mem == nullptr)  return;
  ExFreePool(mem);
  }

#endif  // _WIN32 && FOR_RING0