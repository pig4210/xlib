/**
  \file  xcrc.h
  \brief 定义了 CRC 算法模板。支持 crc16 、 crc32 、 crc64 、crcccitt 。

  \version    3.2.1.230307

  \author     triones
  \date       2013-03-19

  \section history 版本记录

  - 2013-03-19 新建 crc32 函数。 0.1 。
  - 2013-03-20 修正溢出 Bug 及优化。 0.2 。
  - 2013-11-30 新增 crc64 函数。 0.3 。
  - 2014-02-18 新增 crc16 函数。 0.4 。
  - 2016-12-16 适配 Linux g++ 。新增 crcccitt 函数。 1.0 。
  - 2017-07-24 改进函数定义。 1.1 。
  - 2019-09-19 修改为模板，使用了简单的单例。 2.0 。
  - 2019-09-29 引入新特性重构，解决线程安全的问题。 3.0 。
  - 2020-03-06 引入可变参数模板，表的生成重新设计。 3.1 。
  - 2020-05-09 扩大模板匹配，匹配多数顺序容器。优化接口 3.2 。
*/
#ifndef _XLIB_XCRC_H_
#define _XLIB_XCRC_H_

#include <array>
#include <climits>
#include <string>

namespace xlib {

/// 用于编译期计算 CRC 表单个值。
template <typename T, T N>
constexpr T inline XCrcTableValue(const T i) noexcept {
  T crc = i;
  for (size_t j = 0; j < CHAR_BIT; ++j) {
    crc = (crc >> 1) ^ ((crc & 1) ? N : 0);
  }
  return crc;
}

/// 用于编译期生成 CRC 表。
template <typename T, T N, std::size_t... I>
constexpr auto inline XCrcTable(std::index_sequence<I...>) noexcept {
  return std::array<T, sizeof...(I)>{XCrcTableValue<T, N>(I)...};
}

/// CRC 计算模板。
template <typename T, T N, T V, bool R>
T XCRC(const void* const data, const size_t size) {
  // 将在编译期生成 CRC 表。
  constexpr auto CrcTable = XCrcTable<T, N>(std::make_index_sequence<0x100>{});
  T ret = V;
  const size_t len = (nullptr == data) ? 0 : size;
  const uint8_t* const p = (const uint8_t*)data;
  for (size_t i = 0; i < len; ++i) {
    ret = CrcTable[(ret & 0xFF) ^ p[i]] ^ (ret >> 8);
  }
  return R ? ~ret : ret;
}

/**
  CRC 计算模板。用于 编译期计算。

  注意到，const T* const 模板无法生成 constexpr 结果。
*/
template <typename TC, size_t size, typename T, T N, T V, bool R> constexpr
T XCRC(TC const(&data)[size]) {
  // 将在编译期生成 CRC 表。
  constexpr auto CrcTable = XCrcTable<T, N>(std::make_index_sequence<0x100>{});
  constexpr auto st = sizeof(TC);
  T ret = V;
  const size_t len = (size - 1) * st;
  for (size_t i = 0; i < len; ++i) {
    const auto ch = data[i / st] >> ((i % st) * CHAR_BIT);
    ret = CrcTable[(ret & 0xFF) ^ (ch & 0xFF)] ^ (ret >> 8);
  }
  return R ? ~ret : ret;
}

//////////////////////////////////////////////////////////////////////////
/**
  生成指定数据的 crc 。
  \param    data    指定需要计算 crc 的数据。
  \param    size    指定需要计算 crc 的数据长度（以相应类型字计）。
  \return           返回 crc 值。

  \code
    // 接受指定长度数据。
    auto x = crc((void*)"12", 2);
    auto x = crc(L"12", 2);
    // 接受顺序容器。
    auto x = crc(std::string("12"));
    auto x = crc(std::array<char, 1>{'1'});
    // 接受字符串字面量。
    auto x = crc("12");
    auto x = crc(L"12");
  \endcode
*/
#define CRCX(FUNC, TT, NN, VV, RR)                                    \
  inline auto FUNC(const void* const data, const size_t size) {       \
    return XCRC<TT, NN, VV, RR>(data, size);                          \
  }                                                                   \
  template <typename T>                                               \
  inline auto FUNC(const T* const data, const size_t size) {          \
    return FUNC((const void*)data, size * sizeof(T));                 \
  }                                                                   \
  template <typename T>                                               \
  inline auto FUNC(const T& o)                                        \
      ->std::enable_if_t<std::is_pointer_v<decltype(o.data())>, TT> { \
    return FUNC(o.data(), o.size());                                  \
  }                                                                   \
  template <typename T, size_t size> constexpr                        \
  inline auto FUNC(T const(&data)[size]) {                            \
    return XCRC<T, size, TT, NN, VV, RR>(data);                       \
  }

CRCX(crc16,     uint16_t, 0xA001, 0, false);
CRCX(crc32,     uint32_t, 0xEDB88320, 0xFFFFFFFF, true);
CRCX(crc64,     uint64_t, 0xC96C5795D7870F42, 0xFFFFFFFFFFFFFFFF, true);
CRCX(crcccitt,  uint16_t, 0x8408, 0xFFFF, false);

#undef CRCX

}  // namespace xlib

#endif  // _XLIB_XCRC_H_