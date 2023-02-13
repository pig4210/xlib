#include "xlib_test.h"

static int g_exception_count = 0;

bool xlib_test(xlib_test_routine routine)
  {
  try
    {
    routine();
    return true;
    }
  catch(...)
    {
    ++g_exception_count;
    std::cerr << std::endl << "======== exception !!!" << std::endl;
    return false;
    }
  }

#ifdef _WIN32
#include <tchar.h>
int _tmain(int , _TCHAR*)
#else
int main()
#endif
  {
  std::cout << std::endl << "xlib test done." << std::endl;
  return g_exception_count;
  }


#define xsig_need_debug
#include "xsig.h"

xsig operator"" _sig(const char* signature, std::size_t) {
  xsig s;
  s.make_lexs(signature);
  return s;
}

static const auto gkb = []{
  xsig::dbglog = true;

const std::string ssss = std::string(0x500, '0') + "1";
const std::regex rrr("0.{200,600}1");
auto itt = std::cregex_iterator(ssss.data(), ssss.data() + ssss.size(), rrr);
if(itt == std::cregex_iterator()) {
  xerr << "nnnnnnn";
} else {
  xdbg << "xxx" << itt->position(0);
}


const auto sigs = xsig::read_sig("/\n12345\r\n/\r\n6789\n/\n1111\n/22222\n/\n   \n/\n\t\t\n/\n33333");

auto xsigs = xsig::read_sig_file(TEXT("xsig.sig"));

auto hmod = LoadLibrary(TEXT("D:/share/APPS/JD/pop_dd_workbench/jdwb_core.dll"));
xdbg << "Load : " << hmod;
if(nullptr == hmod) return false;

const auto xblks = xsig::check_blk(xblk((void*)hmod, (int)0x5D2000));
for(const auto& b : xblks) {
  xdbg << "~~ " << b.start << " ~ " << b.end;
}
for(auto& s : xsigs) {
  for(const auto& b : xblks) {
    const auto reps = s.match_core(b);
    xdbg << "match : " << !reps.empty();
    for(const auto& v : reps) {
      xdbg << "  " << v.second.q << " : " << v.first;
    }
    if(!reps.empty()) break;
  }
}

if(nullptr != hmod) return false;

const std::string ss(hex2bin(std::string(
"0000000000000000"
"64 A3 00000000"
"FF 15 11223344"
"64 A3 00000000"
"FF 15 11223344"
"6A 55"
"E8 16000000"
"8B F0"
"89 75 00"
"68 1122 0000"
"C7 45 FC 00000000"
"E8 00000000"
"83 C4 08"
"89 45 00"
"8B C8"
"C6 45 FC 01"
"0000000000000000"
)));

xsig sig = 
"64 A3 00{ 1, 4 }"
"FF 15 <D core@HttpClient::curlGlobalInit>"
"6A <B sizeof(JDWBCore)>"
"E8 <^F core@operator_new>"
"8B F0"
"89 75 ."
"68 <W sizeof(CoreContext)>0000"
"C7 45 FC 00000000"
"E8 <^F core@operator_new>"
"83 C4 08"
"89 45 ."
"8B C8"
"C6 45 FC 01 <A>...."_sig;

auto s = (size_t)GetModuleHandle(nullptr);
s -= 0x100;
const auto blks = xsig::check_blk(xblk((void*)s, 0x80000));
for(const auto& v : blks) {
  xdbg << v.start << " - " << v.end;
}
xdbg << showbin(ss);

const auto reps = sig.match({xblk(ss.data(), ss.size())});

for(const auto& v : reps) {
  xdbg << v.second.p << " : " << v.first;
}

  return true;
  }();

