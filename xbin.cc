#include "xbin.h"

#include "xlib_test.h"

#include "hexbin.h"

SHOW_TEST_INIT(XBIN)

lbin lb, aa;
gbin gb, bb;

SHOW_TEST_HEAD(<< void*);
lb.clear();
gb.clear();
lb << (void*)0x12345678;
gb << (void*)0;
done = lb.size() == sizeof(void*) &&
       gb.size() == sizeof(void*) &&
       *(void**)lb.c_str() == (void*)0x12345678 &&
       *(void**)gb.c_str() == (void*)0;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< T*);
lb.clear();
gb.clear();
lb << "123456";
gb << L"123456";
done = 0 == memcmp(lb.c_str(), "123456", 6) &&
       0 == memcmp(gb.c_str(), L"123456", 6 * sizeof(wchar_t));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< xbin);
lb.clear(); aa.clear();
gb.clear(); bb.clear();
aa << "123456"; lb << aa;
bb << "123456"; gb << bb;
done = lb.size() == sizeof(uint16_t) + 6 &&
       gb.size() == sizeof(uint16_t) + 7 &&
       *(uint16_t*)lb.c_str() == 0x0006 &&
       *(uint16_t*)gb.c_str() == 0x0700;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< basic_string);
lb.clear();
gb.clear();
lb << std::string("123456");
gb << std::wstring(L"123456");
done = 0 == memcmp(lb.c_str(), "123456", 6) &&
       0 == memcmp(gb.c_str(), L"123456", 6 * sizeof(wchar_t));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< T&);
lb.clear();
gb.clear();
lb << (uint32_t)0x12345678;
gb << (uint32_t)0x12345678;
done = lb.size() == sizeof(uint32_t) &&
       gb.size() == sizeof(uint32_t) &&
      *(uint32_t*)lb.c_str() == 0x12345678 &&
      *(uint32_t*)gb.c_str() == 0x78563412;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< mkhead);
lb.clear();
gb.clear();
lb << "123456"; lb.mkhead();
gb << "123456"; gb.mkhead();
done = lb.size() == sizeof(uint16_t) + 6 &&
        gb.size() == sizeof(uint16_t) + 7 &&
        *(uint16_t*)lb.c_str() == 0x0006 &&
        *(uint16_t*)gb.c_str() == 0x0700;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> void*);
lb.clear();
gb.clear();
lb << (void*)0x12345678 << "123456" << '\0';
gb << (void*)0x12345678 << "123456";
void* va;
void* vb;
lb >> va;
gb >> vb;
done = va == (void*)0x12345678 && vb == (void*)0x12345678;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> T*);
char abuf[0x10];
char bbuf[0x10];
lb >> (char*)abuf;
gb >> (char*)bbuf;
done = 0 == memcmp(abuf, "123456", 6) &&
       0 == memcmp(bbuf, "123456", 7);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> xbin);
lb.clear(); aa.clear();
gb.clear(); bb.clear();
aa << "123456"; aa.mkhead(); aa >> lb;
bb << "123456"; bb.mkhead(); bb >> gb;
done = 0 == memcmp(lb.c_str(), "123456", 6) &&
       0 == memcmp(gb.c_str(), "123456", 7);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> basic_string);
std::string ls, gs;
lb.clear();
gb.clear();
lb << "123456"; lb >> ls;
gb << "123456"; gb >> gs;
done = 0 == memcmp(ls.c_str(), "123456", 6) &&
       0 == memcmp(gs.c_str(), "123456", 7);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> T);
uint32_t li, gi;
lb.clear();
gb.clear();
lb << (uint32_t)0x12345678; lb >> li;
gb << (uint32_t)0x12345678; gb >> gi;
done = 0x12345678 == li && 0x12345678 == gi;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> T const);
lb.clear();
gb.clear();
lb << '\0' >> '\0';
gb << L'\0' >> L'\0';
done = lb.empty() && gb.empty();
SHOW_TEST_RESULT;

SHOW_TEST_DONE;