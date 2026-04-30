/**
  \file  xcrc.h
  \brief 定义了 CRC 算法模板。支持 crc16 、 crc32 、 crc64 、crcccitt 。

  \version    4.0.0.260430

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
  - 2026-04-30 替换宏，升级定义。 4.0 。
*/
#ifndef _xlib_xcrc_H_
#define _xlib_xcrc_H_

#include <climits>
#include <cstdint>
#include <cstddef>
#include <array>
#include <type_traits>
#include <utility>

namespace xlib {

//////////////////////////////////////////////////////////////// CRC Table 实现。
/// 用于编译期计算 CRC 表单个值。
template <typename T, T N>
inline constexpr T XCrcTableValue(const T i) noexcept {
  T crc = i;
  for (std::size_t j = 0; j < CHAR_BIT; ++j) {
    crc = (crc >> 1) ^ ((crc & 1) ? N : 0);
  }
  return crc;
}

/// 用于编译期生成 CRC 表。
template <typename T, T N, std::size_t... I>
inline constexpr auto XCrcTable(std::index_sequence<I...>) noexcept {
  return std::array<T, sizeof...(I)>{XCrcTableValue<T, N>(I)...};
}

//////////////////////////////////////////////////////////////// CRC 通用计算。
/// CRC 计算模板。
template <typename T, T N, T V, bool R>
T XCRC(const void* const data, const std::size_t size) {
  // 将在编译期生成 CRC 表。
  // 这里将访问全局变量，没有局部变量复制。对于 crc 场景，相对复制整个全局变量后计算，直接访问更高效。
  // 如果不加 static ，则会创建局部变量，从全局变量中复制一份表，多此一举。
  static constexpr auto CrcTable = XCrcTable<T, N>(std::make_index_sequence<0x100>{});
  T ret = V;
  const std::size_t len = (nullptr == data) ? 0 : size;
  const auto p = reinterpret_cast<const std::uint8_t*>(data);
  for (std::size_t i = 0; i < len; ++i) {
    ret = CrcTable[(ret & 0xFF) ^ p[i]] ^ (ret >> 8);
  }
  return R ? ~ret : ret;
}

//////////////////////////////////////////////////////////////// CRC 字符串字面量编译期计算。
template <typename T> struct IsCrcChar  : std::false_type {};
template <> struct IsCrcChar<char>      : std::true_type  {};
template <> struct IsCrcChar<wchar_t>   : std::true_type  {};
template <> struct IsCrcChar<char16_t>  : std::true_type  {};
template <> struct IsCrcChar<char32_t>  : std::true_type  {};
#ifdef __cpp_char8_t
template <> struct IsCrcChar<char8_t>   : std::true_type  {};
#else
// 兼容 xlib 自适应的 char8_t 。
template <> struct IsCrcChar<unsigned char> : std::true_type {};
#endif

/**
  CRC 计算模板。用于 字符串字面量编译期计算。

  - data 必须是字符串字面量，且最后一个字符必须是 '\0' 。
  - 计算忽略最后一个 '\0' 。
  - 小端序。

  注意到，const T* const 模板无法生成 constexpr 结果。
*/
template <typename TC, std::size_t size, typename T, T N, T V, bool R>
constexpr std::enable_if_t<IsCrcChar<std::remove_cv_t<TC>>::value, T>
XCRC(TC const(&data)[size]) {
  // 将在编译期生成 CRC 表。
  // 这里因为需要整个函数可以编译期计算，所以不能使用 static 引入存储。
  // 而且因为模板是肯定在编译期计算的，所以无需计较局部变量。
  // data 必须直接访问，不能通过指针间接访问，否则 无法编译期计算。
  constexpr auto CrcTable = XCrcTable<T, N>(std::make_index_sequence<0x100>{});
  constexpr auto st = sizeof(TC);
  T ret = V;
  const std::size_t len = (size - 1) * st;  // 忽略最后一个 '\0' 。
  for (std::size_t i = 0; i < len; ++i) {
    const auto u = static_cast<std::make_unsigned_t<std::remove_cv_t<TC>>>(data[i / st]);
    const auto ch = u >> ((i % st) * CHAR_BIT);
    ret = CrcTable[(ret & 0xFF) ^ (ch & 0xFF)] ^ (ret >> 8);
  }
  return R ? ~ret : ret;
}

//////////////////////////////////////////////////////////////// CRC 接口内核。
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

template <typename TT, TT NN, TT VV, bool RR>
class XCRCCore {
 private:
  static TT bytes(const void* data, std::size_t size) {
    return XCRC<TT, NN, VV, RR>(data, size);
  }

  template <typename T>
  static TT bytes(const T* data, std::size_t size) {
    return bytes(static_cast<const void*>(data), size * sizeof(T));
  }

  template <typename T,
            typename = decltype(std::declval<T>().data(), std::declval<T>().size())>
  static auto bytes(const T& o) -> TT {
    return bytes(o.data(), o.size());
  }

  template <typename T, std::size_t N>
  static constexpr TT bytes(T const (&data)[N]) {
    return XCRC<T, N, TT, NN, VV, RR>(data);
  }

 public:
  template <typename... Args>
  constexpr auto operator()(Args&&... args) const {
    return bytes(std::forward<Args>(args)...);
  }
};

//////////////////////////////////////////////////////////////// crc16 、 crc32 、 crc64 、crcccitt
constexpr XCRCCore<uint16_t, 0xA001,             0,                  false> crc16{};
constexpr XCRCCore<uint32_t, 0xEDB88320,         0xFFFFFFFF,         true>  crc32{};
constexpr XCRCCore<uint64_t, 0xC96C5795D7870F42, 0xFFFFFFFFFFFFFFFF, true>  crc64{};
constexpr XCRCCore<uint16_t, 0x8408,             0xFFFF,             false> crcccitt{};

}  // namespace xlib

#endif  // _xlib_xcrc_H_