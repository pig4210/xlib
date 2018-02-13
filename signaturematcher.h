/*!
  \file  signaturematcher.h
  \brief signaturematcher.h用于特征码定位

  - signaturematcher处理的全部为C格式ASCII字符串，要求以'\0'结尾

  \version    7.1.1709.1115
  \note       For All

  \author     triones
  \date       2011-06-16


  \b 特征码串由以下元素组合而成：

  \section Blanks 空白
  - 空白符由【空格】【制表符】【换行符】【回车符】组成
  - 如无特别说明，【空白】允许出现在特征码的任意处
  - 如无特别说明，【空白】在特征码解析过程中被忽略

  \section Quote 引用
  - 引用串以【"】号起始，【"】号结束
  - 引用串为Unicode格式时，需要在起始【"】前加【L】
  - 引用串内如果需要再包含【"】号，请使用【\\"】。
  - 引用串内将【空白】视作引用串的内容
  - 引用串内的【换行】与【回车】请使用【\\r】、【\\n】。支持转义字符。

  \code
  B8 "binsig_quote"  //相当于定位push XXXX, XXXX指向"binsig_quote"
  \endcode

  \section Record 特征点
  - 特征点以【<】号起始，【>】号结束
  - 特征点串第一个非空白符如果为【^】，则这是一个【偏移标记】
  - 【偏移标记】存在时，特征点值提取结果将减去匹配块的起始地址
  - 特征点串第一个非【偏移标记】、非空白符必须为下列字符的其中之一(无视大小写)
  - \b A 表示此点是一个地址，此点占用0 byte
  - \b F 表示此点是一个偏移，    占用4 byte
  - \b Q 表示此点是一个QWORD,    占用8 byte
  - \b D 表示此点是一个DWORD，   占用4 byte
  - \b W 表示此点是一个WORD，    占用2 byte
  - \b B 表示此点是一个BYTE，    占用1 byte
  - 特征点串第二个非空白符开始视作【特征点名称】，由【空白】结束，名称可以为空
  - 【特征点名称】允许的字符无特别限制，但实际应用中有限制
  - 【特征点名称】之后第一个非空白符至结束视作【特征点注释】
  - 【特征点注释】中【换行符】【回车符】将被替换为【空格】

  \code
  <A push_something> B8 78563412  //定位push 12345678之处
  \endcode

  \section Range 范围指示
  - 范围指示只能用于【点】【常量串】【组合】
  - 范围指示不能重复使用
  - 范围最小为0，最大为MAX，x86下为0x7FFFFFFF，x64下为0x7FFFFFFF_FFFFFFFF
  - 范围指示必须为以下几种类型
  - \b *      表示匹配范围从 0 到 MAX == {0, MAX} == { , }
  - \b +      表示匹配范围从 1 到 MAX == {1, MAX} == {1, }
  - \b ?      表示匹配范围从 0 到 1   == {0, 1}   == {, 1}
  - \b {N, M} 表示匹配范围从 N 到 M
  - \b {N}    表示匹配范围从 N 到 N
  - \b {, M}  表示匹配范围从 0 到 M
  - \b {N, }  表示匹配范围从 N 到 MAX
  - \b {, }   表示匹配范围从 0 到 MAX  ==　*
  - N与M间的分隔符允许为【,】【-】【~】【|】【:】，但一般为【,】
  - 注意N与M的不得超过范围,否则识别失败
  - 注意匹配模式是【非贪婪】的
  - 除分隔符外，{}间不得有除【空白】外的非十六进制字符，否则识别失败
  - N与M将以十六进制识别，允许N > M

  \code
  00? FF{8} //00匹配0次或1次，FF匹配8次
  \endcode

  \section String 字符串
  - 字符串以【'】号起始，【'】号结束
  - 字符串为Unicode格式时，需要在起始【'】前加【L】
  - 字符串如果需要再包含【'】号，请使用【\'】。
  - 字符串内将【空白】视作字符串的内容
  - 字符串内的【换行】与【回车】请使用【\r】、【\n】。支持转义字符。

  \code
  '123456789' //匹配字符串123456789
  \endcode

  \section MarkRef 附标记与附引用
  - 【附标记】由两个十六进制、【@】符号，【L】或【R】符号组成
  - 【附引用】由两个十六进制、【$】符号，[1-F]十六进制数索引，【L】或【R】符号组成
  - 【附标记】与【附引用】只标识一个十六进制数值，不能应用【范围】
  - 【附标记】与【附引用】可以同时存在，此时，无视书写顺序，一律认为【附标记】在【附引用】之前
  - 【L】或【R】可以同时存在，此时，无视书写顺序，一律认为【L】在【R】之前
  - 【附引用】的【$】后十六进制数可以缺省，缺省值为1
  - 【R】标记模糊处理0、1、2位。【L】标记模糊处理3、4、5位\n
  如50@R，匹配50-57。C8@L，匹配C8-F8\n
  模糊匹配的相关说明参考【常量串】
  - 【附标记】与【附引用】匹配使用，参考以下特征码

  \code
  B9@R 78563412     //匹配B8-B7，附标记
  8B 11@L$2R        //匹配00-3F，但低三位必须与上面标记位相同
  B9@R 78563412
  8B 11$R           //索引缺省
  \endcode


  \section Ucbit 常量串
  - 常量串由两个十六进制字符组成，允许中间有【空白】存在。
  - 常量串的表示也可以由【:】后紧接一个字符表示此字符
  - 注意当使用【:】表示字符时，紧接的【空白】不被忽略
  - 常量串可以通过以下操作符连接
  - \b - 连接操作符。00-FF 表示匹配从00到FF的任意值
  - \b | 或者操作符。00|FF 表示匹配00或者FF。
  - \b & 模糊操作符。注意，这个操作符是按位模糊处理，一般用于寄存器匹配\n
  处理的关键：maskA的位与maskB的位相异时，即此位0/1都能匹配。\n
  73&70 即0111 0011 & 0111 0000，匹配70、71、72、73\n
  7C&71 即0111 1100 & 0111 0001，匹配70、71、74、75、78、79、7C、7D\n
  其效果等同于7D&70\n
  半字节匹配例如7F&70，全字节匹配例如FF&00\n
  模糊操作符可接【R】【L】标记\n
  【R】标记模糊处理0、1、2位。【L】标记模糊处理3、4、5位
  - 常量串允许通过操作符无限次连接，但请注意值冲突。

  \code
  55 8BEC 83 10-20 //匹配push ebp\mov ebp, esp\sub esp, 10-20
  :A :B            //匹配A字符，匹配B字符，与'AB'同理
  B9&R             //匹配mov r32, xxx
  \endcode

  \section Collecion 组合
  - 组合由【[】号起始，【]】号结束
  - 组合的第一个非空白字符如果为【^】，匹配值组将被翻转
  - 组合内容由【常量串】组成
  - 除翻转操作外，组合一般可以由操作符连接的【常量串】就可以完成。
  - 组合内部的模糊操作符不可接【R】【L】标记
  - 组合的其它条件参考【常量串】说明。

  \code
  [^0A0D] //匹配除0A、0D外的任意值
  \endcode

  \section Dot 点
  - 点相当于00-FF，但任意匹配点匹配处理上有优化，效率较高。

  \code
  .... //匹配四个任意值，==.{4} ==00-FF{4} == [00&FF]{4}
  \endcode

  \section Note 注释
  - 注释以【#】号开始，直到行尾。
  - 注释在解析过程中被忽略

  \code
  <F show_msg> #bool __stdcall show_msg(char* msg);
  \endcode

  \section 额外说明
  - 如果特征码串中无特征点，自动在串头部添加<A>
  - 【特征点名称】允许重名，将判定重名特征点结果是否一致

  \section 特征码脚本格式说明
  - 【特征码】或【范围指示】用【/】标记起始与结束
  - 起始标记可省略；在脚本结尾，结束标记也可省略
  - 【范围指示】必须使用【@】或【$】作为起始非空白字符
  - 【范围指示】可以指示多块
  - 【范围指示】优先识别为模块，当模块不存在时，识别为【范围组】
  - 【范围组】必须成对出现，且不能有交集
  - 【范围指示】为标准PE文件时，【@】表示全映像，【$】表示代码段
  - 【@】与【$】应用于【范围组】意义相同

  \code
  /
  signature1
  /
  /
  signature2
  /
  #以上是完整的起始与结束标记
  signature3
  /
  signature4
  /
  signature5
  #以上省略起始标记，省略结尾结束标记，但是合法
  @xxx.dll  #指示一个模块
  @xxx.dll@yyy.dll@zzz.dll  #指示多个模块
  @400000@401000            #指示一个范围
  @400000@401000@401000@402000    #指示多个范围
  \endcode

*/
#ifndef _XLIB_SIGNATUREMATCHER_H_
#define _XLIB_SIGNATUREMATCHER_H_

#ifdef _WIN32

#include "vline.h"
#include "xblk.h"
#include "xlog.h"

#include <map>
#include <string>
#include <vector>

namespace signaturematcher
  {
  //! 特征码类型
  typedef const char* SIGNATURE;

  //! 特征值类型
  struct REPORT_VALUE
    {
    char t;   //!< qdwbp n(错误)
    union
      {
      uint64 q;
      uint32 d;
      uint16 w;
      uint8  b;
      void*  p;
      };
    };

  //! 特征点数据
  struct REPORT_NODE
    {
    REPORT_VALUE values;
    std::string  note;
    };

  //! 特征码词法基类预声明
  class LexicalBase;

  //! 词法识别，返回empty表示失败
  std::vector<LexicalBase*> create_lexer(SIGNATURE signature);

  //! 特征点数据组类型
  typedef std::map<std::string, REPORT_NODE> REPORT;

  //! 指定特征码串，生成中间原子
  vline create_atom(SIGNATURE signature);

  //! 特征点数据加入容器，数据存在时对比，不相同返回false，其余返回true并加入容器
  bool report_add(REPORT& report, std::string name, const REPORT_NODE& node);

  //! 指定内存范围，指定特征码词法，分析出结果（所有match的基础）
  REPORT match(const xblk& blk, const std::vector<LexicalBase*>& vec);

  //! 指定内存范围，指定特征码，分析出结果
  REPORT match(void* start, void* end, SIGNATURE signature);

  //! 指定内存范围，指定特征码中间原子，分析出结果
  REPORT match(void* start, void* end, vline& atom);

  //! 指定内存范围组，指定特征码，分析出结果
  REPORT match(const std::vector<xblk>& blks, SIGNATURE signature);

  //! 指定内存范围，指定特征码中间原子，分析出结果
  REPORT match(const std::vector<xblk>& blks, vline& atom);

  //! 指定内存范围，指定特征码，分析出结果并提取第一个结果的值
  REPORT_VALUE find(void* start, void* end, SIGNATURE signature);

  //! 获取或指定调试信息输出等级，默认只输出错误信息
  xlog::levels& log_level();

  //! 输出回调格式
  typedef void(*log_out_func)(const char* const buf);

  //! 设置输出回调
  void set_log_out(log_out_func func = nullptr);

  //! 脚本元素类型
  enum ScriptType : uint8
    {
    ST_Blk,           //!< 范围指示
    ST_Sign,          //!< 特征码串
    ST_Atom,          //!< 中间原子
    ST_Unknown,       //!< 未知脚本元素
    };

  //! 脚本结点
  struct ScriptNode
    {
    ScriptType        type;   //!< 脚本结点元素类型指示
    std::string       sig;    //!< 特征码串
    vline             atom;   //!< 中间原子
    };

  //! 脚本解析回调，返回false则阻止后继解析
  typedef bool(*analy_script_routine)(ScriptNode sn, void* lparam);

  //! 脚本解析
  bool analy_script(std::string& script, analy_script_routine asr, void* lparam = nullptr);

  //! 通过脚本生成中间原子
  vline create_atom_by_script(std::string& script);

  //! 给定范围指示，解析出范围组
  std::vector<xblk> analy_blk(std::string& blkstr);
  }

#define binsig_find signaturematcher::find

#endif  //_WIN32

#endif  // _XLIB_SIGNATUREMATCHER_H_