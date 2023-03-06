#include "xbin.h"

#include "xlib_test.h"

SHOW_TEST_INIT(xbin)

xlib::lbin lb, aa;
xlib::gbin gb, bb;
xlib::vbin vb, cc;

SHOW_TEST_HEAD(<< void*);
lb.clear(); gb.clear(); vb.clear();
lb << (void*)0x12345678;
gb << (const void*)0;
vb << (void*)12345678;
done = lb.size() == sizeof(void*) && *(void**)lb.data() == (void*)0x12345678 &&
       gb.size() == sizeof(void*) && *(void**)gb.data() == (void*)0 &&
       vb.size() == 4 && *(const uint32_t*)vb.data() == (uint32_t)0x05F1C2CE;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< bool);
lb.clear(); gb.clear(); vb.clear();
lb << true;
gb << false;
vb << true;
done = lb.size() == sizeof(bool) && *(bool*)lb.data() == true &&
       gb.size() == sizeof(bool) && *(bool*)gb.data() == false &&
       vb.size() == 1 && *(bool*)vb.data() == true;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< T*);
lb.clear(); gb.clear(); vb.clear();
lb << "123456";
gb << L"123456";
vb << (char*)"123456";
done = lb.size() == 6 && 0 == memcmp(lb.data(), "123456", 6) &&
       gb.size() == 7 * sizeof(wchar_t) && 0 == memcmp(gb.data(), L"123456", 6 * sizeof(wchar_t)) &&
       vb.size() == 6 && 0 == memcmp(vb.data(), "123456", 6);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< xbin);
lb.clear(); aa.clear();
gb.clear(); bb.clear();
vb.clear(); cc.clear();
aa << "123456"; lb << aa;
bb << "123456"; gb << bb;
cc << "123456"; vb << cc;
done = lb.size() == sizeof(uint16_t) + 6 && *(const uint16_t*)lb.data() == 0x0006 &&
       gb.size() == sizeof(uint16_t) + 7 && *(const uint16_t*)gb.data() == 0x0700 &&
       vb.size() == 1 + 6 && *(const uint8_t*)vb.data() == 0x06;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< basic_string);
lb.clear(); gb.clear(); vb.clear();
lb << std::string("123456");
gb << std::wstring(L"123456");
vb << std::string("123456");
done = lb.size() == 6 && 0 == memcmp(lb.data(), "123456", 6) &&
       gb.size() == 6 * sizeof(wchar_t) && 0 == memcmp(gb.data(), L"123456", 6 * sizeof(wchar_t)) &&
       vb.size() == 6 && 0 == memcmp(vb.data(), "123456", 6);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< unsigned);
lb.clear(); gb.clear(); vb.clear();
lb << (uint8_t)0x11;
gb << (uint32_t)0x11;
vb << (uint64_t)0x11;
done = lb.size() == 1 && *(uint8_t*)lb.data() == 0x11 &&
       gb.size() == sizeof(uint32_t) && *(uint32_t*)gb.data() == (uint32_t)0x11000000 &&
       vb.size() == 1 && *(uint8_t*)vb.data() == 0x11;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< signed);
lb.clear(); gb.clear(); vb.clear();
lb << (int8_t)0x11;
gb << (int32_t)0x11;
vb << (int64_t)0x11;
done = lb.size() == 1 && *(uint8_t*)lb.data() == 0x11 &&
       gb.size() == sizeof(uint32_t) && *(uint32_t*)gb.data() == (uint32_t)0x11000000 &&
       vb.size() == 1 && *(uint8_t*)vb.data() == 0x22;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< T&);
lb.clear(); gb.clear(); vb.clear();
lb << (uint32_t)0x12345678;
gb << (uint32_t)0x12345678;
vb << (uint32_t)0x12345678;
done = lb.size() == sizeof(uint32_t) && *(const uint32_t*)lb.data() == 0x12345678 &&
       gb.size() == sizeof(uint32_t) && *(const uint32_t*)gb.data() == 0x78563412 &&
       vb.size() == 5 && memcmp(aa.data(), "\xF8\xAC\xD1\x91\x01", 5);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(<< mkhead);
lb.clear(); gb.clear(); vb.clear();
lb << "123456"; lb.mkhead();
gb << "123456"; gb.mkhead();
vb << "123456"; vb.mkhead();
done = lb.size() == sizeof(uint16_t) + 6 && *(const uint16_t*)lb.data() == 0x0006 &&
       gb.size() == sizeof(uint16_t) + 7 && *(const uint16_t*)gb.data() == 0x0700 &&
       vb.size() == 7 && *(const uint8_t*)vb.data() == 0x06;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> void*);
lb.clear(); gb.clear(); vb.clear();
lb << (void*)0x12345678 << true  << (uint8_t)0x11  << (int8_t)0x11  << "123456" << '\0';
gb << (void*)0x12345678 << false << (uint32_t)0x11 << (int32_t)0x11 << "123456";
vb << (void*)0x12345678 << true  << (uint64_t)0x11 << (int64_t)0x11 << "123456" << '\0';
void* vva;
void* vvb;
void* vvc;
lb >> vva;
gb >> vvb;
vb >> vvc;
done = vva == (void*)0x12345678 && vvb == vva && vvc == vva;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> bool);
bool vba;
bool vbb;
bool vbc;
lb >> vba;
gb >> vbb;
vb >> vbc;
done = vba == true && vbb == false && vbc == true;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> unsigned);
uint8_t  vua;
uint32_t vub;
uint64_t vuc;
lb >> vua;
gb >> vub;
vb >> vuc;
done = vua == 0x11 && vub == 0x11 && vuc == 0x11;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> signed);
int8_t  via;
int32_t vib;
int64_t vic;
lb >> via;
gb >> vib;
vb >> vic;
done = via == 0x11 && vib == 0x11 && vic == 0x11;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> T*);
char abuf[0x10];
char bbuf[0x10];
char cbuf[0x10];
lb >> (char*)abuf >> '\0';
gb >> (char*)bbuf;
vb >> (char*)cbuf >> '\0';
done = lb.empty() && 0 == memcmp(abuf, "123456", 6) &&
       gb.empty() && 0 == memcmp(bbuf, "123456", 7) &&
       vb.empty() && 0 == memcmp(cbuf, "123456", 6);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> xbin);
lb.clear(); gb.clear(); vb.clear();
aa.clear(); bb.clear(); cc.clear();
lb << "123456"; lb.mkhead(); lb >> aa;
gb << "123456"; gb.mkhead(); gb >> bb;
vb << "123456"; vb.mkhead(); vb >> cc;
done = lb.empty() && 0 == memcmp(aa.data(), "123456", 6) &&
       gb.empty() && 0 == memcmp(bb.data(), "123456", 7) &&
       vb.empty() && 0 == memcmp(cc.data(), "123456", 6);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(>> basic_string);
std::string ls, gs, vs;
lb.clear(); gb.clear(); vb.clear();
lb << "123456"; lb >> ls;
gb << "123456"; gb >> gs;
vb << "123456"; vb >> vs;
done = lb.empty() && 0 == memcmp(ls.data(), "123456", 6) &&
       gb.empty() && 0 == memcmp(gs.data(), "123456", 7) &&
       vb.empty() && 0 == memcmp(vs.data(), "123456", 6);
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