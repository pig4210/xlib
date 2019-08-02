/*!
  \file  signaturematcher.h
  \brief 用于特征码定位。

  \version    1.5.1.190716
  \note       For All

  \author     triones
  \date       2011-06-16

  \details signaturematcher 特征说明请参考 signaturematcher.md 。

  \section history 版本记录

  - 2011-06-18 新建 binsig 。0.1 。
  - 2012-12-03 重构。 0.2 。
  - 2012-12-06 重构版本与旧设计共存，大量测试无误后，替代之。
  - 2012-12-26 修正范围匹配达到最大范围时无法返回递进的 BUG 。 0.2.1 。
  - 2013-03-20 通过实测，库中移除旧设计。
  - 2014-04-01 改进 binarysign 为 signaturematcher 。 1.0 。
  - 2014-05-01 新加脚本处理。 1.1 。
  - 2014-05-05 处理了解析 note 词法的 BUG 。 1.1.1 。
  - 2014-06-03 处理了解析 ##&L 词法的 BUG 。 1.1.2 。
  - 2014-06-06 处理了匹配 mark_ref 的 BUG 、及代码一些小调整。 mark_ref 的定义微调。 1.1.3 。
  - 2014-07-29 code review 中发现处理范围的一个失误，中间原子匹配时资源未回收，修正之。 1.1.4 。
  - 2016-01-08 修正因使用基类改进，而带进的范围处理始终失败的 BUG 。1.2 。
  - 2016-03-21 修正 match_markref 的一个 BUG ，此 BUG 导致 Ring0 下链接失败。 1.2.1 。
  - 2016-03-31 改进防止内存不可读时产生异常。 Ring0 下异常无法捕获导致重启。 1.3 。
  - 2016-04-06 修正 x64 下偏移计算错误的 Bug 。 1.3.1 。
  - 2016-10-18 改进模块代码段的起始地址获取，避免取偏移计算造成不必要的偏差。 1.3.2 。
  - 2017-06-06 修正引用校验失败后，索引不正确导致 match 失败的 BUG 。 1.3.3 。
  - 2017-06-28 修正 ConstHex 后，递进匹配错误的 BUG。 1.3.4 。
  - 2017-07-21 重新修正引用校验失败后，不能正确回退导致 match 失败的 BUG 。 1.3.5 。
  - 2017-09-06 引入 vline ，做适应性更改，使得 atom 能够简单压缩。 1.4 。
  - 2017-09-11 修正引入 vline 时产生的错误，修正范围识别的 BUG 。 1.4.1 。
  - 2019-06-19 转换 log 为静态。调整特征部分语法。 1.5 。
  - 2019-07-16 修正 1.5 修改范围识别时引入的 BUG 。 1.5.1 。
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
  /// 特征码类型。
  typedef const char* SIGNATURE;

  /// 特征值类型。
  struct REPORT_VALUE
    {
    char t;   ///< qdwbp n（错误）。
    union
      {
      uint64 q;
      uint32 d;
      uint16 w;
      uint8  b;
      void*  p;
      };
    };

  /// 特征码词法基类预声明。
  class LexicalBase;

  /// 词法识别，返回 empty 表示失败。
  std::vector<LexicalBase*> create_lexer(SIGNATURE signature);

  /// 特征点数据组类型。
  typedef std::map<std::string, REPORT_VALUE> REPORT;

  /// 指定特征码串，生成中间原子。
  vline create_atom(SIGNATURE signature);

  /// 特征点数据加入容器，数据存在时对比，不相同返回 false ，其余返回 true 并加入容器。
  bool report_add(REPORT& report, std::string name, const REPORT_VALUE& node);

  /// 指定内存范围，指定特征码词法，分析出结果（所有 match 的基础）。
  REPORT match(const xblk& blk, const std::vector<LexicalBase*>& vec);

  /// 指定内存范围，指定特征码，分析出结果。
  REPORT match(void* start, void* end, SIGNATURE signature);

  /// 指定内存范围，指定特征码中间原子，分析出结果。
  REPORT match(void* start, void* end, vline& atom);

  /// 指定内存范围组，指定特征码，分析出结果。
  REPORT match(const std::vector<xblk>& blks, SIGNATURE signature);

  /// 指定内存范围，指定特征码中间原子，分析出结果。
  REPORT match(const std::vector<xblk>& blks, vline& atom);

  /// 指定内存范围，指定特征码，分析出结果并提取第一个结果的值。
  REPORT_VALUE find(void* start, void* end, SIGNATURE signature);

  /// 输出回调格式。
  typedef void(*log_out_func)(const char* const buf);

  /// 设置输出回调。
  void set_log_out(log_out_func func = nullptr);

  /// 脚本元素类型。
  enum ScriptType : uint8
    {
    ST_Blk,           ///< 范围指示。
    ST_Sign,          ///< 特征码串。
    ST_Atom,          ///< 中间原子。
    ST_Unknown,       ///< 未知脚本元素。
    };

  /// 脚本结点。
  struct ScriptNode
    {
    ScriptType        type;   ///< 脚本结点元素类型指示。
    std::string       sig;    ///< 特征码串。
    vline             atom;   ///< 中间原子。
    };

  /// 脚本解析回调，返回 false 则阻止后继解析。
  typedef bool(*analy_script_routine)(ScriptNode sn, void* lparam);

  /// 脚本解析。
  bool analy_script(std::string& script, analy_script_routine asr, void* lparam = nullptr);

  /// 通过脚本生成中间原子。
  vline create_atom_by_script(std::string& script);

  /// 给定范围指示，解析出范围组。
  std::vector<xblk> analy_blk(std::string& blkstr);
  }

#define binsig_find signaturematcher::find

#endif  //_WIN32

#endif  // _XLIB_SIGNATUREMATCHER_H_