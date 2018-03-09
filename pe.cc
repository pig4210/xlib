#include "pe.h"

#ifdef _WIN32

#include "ws_s.h"
#include "syssnap.h"

using namespace std;

pe::pe(const HMODULE hMod)
:_hMod(hMod)
  {
  if(hMod != nullptr) return;
#ifndef FOR_RING0
  _hMod = GetModuleHandle(nullptr);
#else
  SysDriverSnap sds;
  for(const auto& st : sds)
    {
    _hMod = (HMODULE)st.ImageBaseAddress;
    break;
    }
#endif
  }

pe::pe(LPCTSTR name)
:_hMod(nullptr)
  {
#ifndef FOR_RING0
  _hMod = GetModuleHandle(name);
#else

#ifdef UNICODE
  const string s(ws2s(name));
  name = (LPCTSTR)s.c_str();
#endif

  SysDriverSnap sds;
  for(const auto& st : sds)
    {
    if(0 == _stricmp((const char*)name, (const char*)(st.Name + st.NameOffset)))
      {
      _hMod = (HMODULE)st.ImageBaseAddress;
      break;
      }
    }
#endif
  }

const IMAGE_DOS_HEADER* pe::GetDosHead() const
  {
  return (const IMAGE_DOS_HEADER*)_hMod;
  }

const IMAGE_NT_HEADERS* pe::GetPeHead() const
  {
  XLIB_TRY
    {
    auto doshead = GetDosHead();
    if(doshead == nullptr) return nullptr;
    return (const IMAGE_NT_HEADERS*)
      ((size_t)doshead + (size_t)doshead->e_lfanew);
    }
  XLIB_CATCH
    {
    return nullptr;
    }
  }

void* pe::EntryPoint() const
  {
  XLIB_TRY
    {
    auto pehead = GetPeHead();
    if(pehead == nullptr) return nullptr;
    return (void*)(pehead->OptionalHeader.AddressOfEntryPoint + 
      (size_t)GetDosHead());
    }
  XLIB_CATCH
    {
    return nullptr;
    }
  }

xblk pe::GetImage() const
  {
  XLIB_TRY
    {
    auto pehead = GetPeHead();
    if(pehead == nullptr) return xblk(nullptr, nullptr);
    return xblk((void*)GetDosHead(), pehead->OptionalHeader.SizeOfImage);
    }
  XLIB_CATCH
    {
    return xblk(nullptr, nullptr);
    }
  }

xblk pe::GetCode() const
  {
  XLIB_TRY
    {
    auto pehead = GetPeHead();
    if(pehead == nullptr) return xblk(nullptr, nullptr);
    return xblk(
      (void*)(pehead->OptionalHeader.BaseOfCode + (size_t)GetDosHead()),
      pehead->OptionalHeader.SizeOfCode);
    }
  XLIB_CATCH
    {
    return xblk(nullptr, nullptr);
    }
  }

HMODULE pe::Module() const
  {
  return _hMod;
  }

bool pe::IsPE() const
  {
  XLIB_TRY
    {
    auto doshead = GetDosHead();
    if(doshead->e_magic != 'ZM')  return false;
    auto pehead = GetPeHead();
    return pehead->Signature == 'EP';
    }
  XLIB_CATCH
    {
    return false;
    }
  }


#ifdef _XLIB_TEST_

ADD_XLIB_TEST(PE)
  {
  SHOW_TEST_INIT;

  auto done = false;

  const auto my = pe();

  SHOW_TEST_HEAD("GetDosHead");
  done = (my.GetDosHead() != nullptr);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("GetPeHead");
  done = (my.GetPeHead() != nullptr);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("EntryPoint");
  done = (my.EntryPoint() != nullptr);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("GetImage");
  done = (my.GetImage().size() != 0);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("GetCode");
  done = (my.GetCode().size() != 0);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("Module");
  done = (my.Module() != nullptr);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("IsPE");
  done = my.IsPE();
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_

#endif  // _WIN32