﻿#include "xbin.h"

#include "xlib_test.h"

SHOW_TEST_INIT(XBIN)

lbin lb, aa;
gbin gb, bb;
vbin vb, cc;

SHOW_TEST_HEAD(<< void*);
lb.clear(); gb.clear(); vb.clear();
lb << (void*)0x12345678;
gb << (void*)0;
vb << (void*)12345678;
done = lb.size() == sizeof(void*) && *(void**)lb.c_str() == (void*)0x12345678 &&
       gb.size() == sizeof(void*) && *(void**)gb.c_str() == (void*)0 &&
       vb.size() == 4 && *(const uint32_t*)vb.c_str() == (uint32_t)0x05F1C2CE;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< T*);
lb.clear(); gb.clear(); vb.clear();
lb << "123456";
gb << L"123456";
vb << "123456";
done = 0 == memcmp(lb.c_str(), "123456", 6) &&
       0 == memcmp(gb.c_str(), L"123456", 6 * sizeof(wchar_t)) &&
       0 == memcmp(vb.c_str(), "123456", 6);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< xbin);
lb.clear(); aa.clear();
gb.clear(); bb.clear();
vb.clear(); cc.clear();
aa << "123456"; lb << aa;
bb << "123456"; gb << bb;
cc << "123456"; vb << cc;
done = lb.size() == sizeof(uint16_t) + 6 && *(const uint16_t*)lb.c_str() == 0x0006 &&
       gb.size() == sizeof(uint16_t) + 7 && *(const uint16_t*)gb.c_str() == 0x0700 &&
       vb.size() == 1 + 6 && *(const uint8_t*)vb.c_str() == 0x06;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< basic_string);
lb.clear(); gb.clear(); vb.clear();
lb << std::string("123456");
gb << std::wstring(L"123456");
vb << std::string("123456");
done = 0 == memcmp(lb.c_str(), "123456", 6) &&
       0 == memcmp(gb.c_str(), L"123456", 6 * sizeof(wchar_t)) &&
       0 == memcmp(vb.c_str(), "123456", 6);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< T&);
lb.clear(); gb.clear(); vb.clear();
lb << (uint32_t)0x12345678;
gb << (uint32_t)0x12345678;
vb << (uint32_t)0x12345678;
done = lb.size() == sizeof(uint32_t) && *(const uint32_t*)lb.c_str() == 0x12345678 &&
       gb.size() == sizeof(uint32_t) && *(const uint32_t*)gb.c_str() == 0x78563412 &&
       vb.size() == 5 && memcmp(aa.c_str(), "\xF8\xAC\xD1\x91\x01", 5);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< mkhead);
lb.clear(); gb.clear(); vb.clear();
lb << "123456"; lb.mkhead();
gb << "123456"; gb.mkhead();
vb << "123456"; vb.mkhead();
done = lb.size() == sizeof(uint16_t) + 6 && *(const uint16_t*)lb.c_str() == 0x0006 &&
       gb.size() == sizeof(uint16_t) + 7 && *(const uint16_t*)gb.c_str() == 0x0700 &&
       vb.size() == 7 && *(const uint8_t*)vb.c_str() == 0x06;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> void*);
lb.clear(); gb.clear(); vb.clear();
lb << (void*)0x12345678 << "123456" << '\0';
gb << (void*)0x12345678 << "123456";
vb << (void*)0x12345678 << "123456" << '\0';
void* vva;
void* vvb;
void* vvc;
lb >> vva;
gb >> vvb;
vb >> vvc;
done = vva == (void*)0x12345678 && vvb == vva && vvc == vva;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> T*);
char abuf[0x10];
char bbuf[0x10];
char cbuf[0x10];
lb >> (char*)abuf;
gb >> (char*)bbuf;
vb >> (char*)cbuf;
done = 0 == memcmp(abuf, "123456", 6) &&
       0 == memcmp(bbuf, "123456", 7) &&
       0 == memcmp(cbuf, "123456", 6);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> xbin);
lb.clear(); gb.clear(); vb.clear();
aa.clear(); bb.clear(); cc.clear();
aa << "123456"; aa.mkhead(); aa >> lb;
bb << "123456"; bb.mkhead(); bb >> gb;
cc << "123456"; cc.mkhead(); cc >> vb;
done = 0 == memcmp(lb.c_str(), "123456", 6) &&
       0 == memcmp(gb.c_str(), "123456", 7) &&
       0 == memcmp(vb.c_str(), "123456", 6);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> basic_string);
std::string ls, gs, vs;
lb.clear(); gb.clear(); vb.clear();
lb << "123456"; lb >> ls;
gb << "123456"; gb >> gs;
vb << "123456"; vb >> vs;
done = 0 == memcmp(ls.c_str(), "123456", 6) &&
       0 == memcmp(gs.c_str(), "123456", 7) &&
       0 == memcmp(vs.c_str(), "123456", 6);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> T);
uint32_t li, gi, vi;
lb.clear(); gb.clear(); vb.clear();
lb << (uint32_t)0x12345678; lb >> li;
gb << (uint32_t)0x12345678; gb >> gi;
vb << (uint32_t)0x12345678; vb >> vi;
done = 0x12345678 == li && 0x12345678 == gi && 0x12345678 == vi;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> const T);
lb.clear(); gb.clear(); vb.clear();
lb << '\0' >> '\0';
gb << L'\0' >> L'\0';
vb << L'\0' >> L'\0';
done = lb.empty() && gb.empty() && vb.empty();
SHOW_TEST_RESULT;

SHOW_TEST_DONE;