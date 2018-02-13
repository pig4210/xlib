/*!
  \file  singleton.h
  \brief singleton.h定义了单例模版类，来自于boost库，做了微小变动

  - 从boost中抄袭过来，做些微小变动，某些限制参考boost说明

  - \b 修改 by \b triones

  \version    1.0.1209.1809
  \note       Only For Ring3

  \author     boost
  \date       2012-09-18
*/
#ifndef _XLIB_SINGLETON_H_
#define _XLIB_SINGLETON_H_

#include "xlib_base.h"

template <typename T>
struct singleton_default
  {
  private:
    struct obj_creator
      {
      obj_creator() { singleton_default<T>::instance(); }
      inline void do_nothing() const { }
      };
    static obj_creator create_object;
    singleton_default();
  public:
    typedef T object_type;
    static object_type & instance()
      {
      static object_type obj;
      create_object.do_nothing();
      return obj;
      }
  };

template <typename T>
typename singleton_default<T>::obj_creator singleton_default<T>::create_object;

//! 模版函数使取实例操作更方便
template <typename T> T& singleton()
  {
  return singleton_default<T>::instance();
  }

//! 宏使得类初始化函数可以不为public，以避免误初始化
#define use_singleton(T) friend singleton_default<T>

#endif  // _XLIB_SINGLETON_H_