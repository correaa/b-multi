#if COMPILATION_INSTRUCTIONS
(echo "#include\""$0"\"" > $0x.cpp) && c++ -O3 -std=c++14 -Wall -Wextra -Wfatal-errors -D_TEST_BOOST_MULTI_DETAIL_OPERATORS $0x.cpp -o $0x.x && time $0x.x $@ && rm -f $0x.x $0x.cpp; exit
#endif
#ifndef BOOST_MULTI_DETAIL_OPERATORS_HPP
#define BOOST_MULTI_DETAIL_OPERATORS_HPP

#include "layout.hpp"

//#include<algorithm> // copy_n // transform
//#include<array>
//#include<cassert>
//#include<tuple>
//#include<memory> // pointer_traits

//#include<boost/iterator/iterator_facade.hpp>
//#include<boost/operators.hpp>

namespace boost{
namespace multi{

struct empty_base{};

template<class T, class V, class B = empty_base> struct equality_comparable2;

template <class T, class B>
struct equality_comparable2<T, void, B> : B{
	template<class U, typename = std::enable_if_t<not std::is_same<U, T>{}> >
	friend bool operator==(const U& y, const T& x){return x == y;}
	template<class U, typename = std::enable_if_t<not std::is_same<U, T>{}> >
	friend bool operator!=(const U& y, const T& x){return not (x == y);}
	template<class U>
	friend bool operator!=(const T& y, const U& x){return not (y == x);}
};

template<class T, class V, class B = boost::operators_detail::empty_base<T>> struct partially_ordered2;

template <class T, class B>
struct partially_ordered2<T, void, B> : B{
	template<class U>
	friend bool operator<=(const T& x, const U& y){return (x < y) or (x == y);}
	template<class U>
	friend bool operator>=(const T& x, const U& y){return (x > y) or (x == y);}
	template<class U, typename = std::enable_if_t<not std::is_same<U, T>{}> >
	friend bool operator>(const U& x, const T& y){return y < x;}
	template<class U, typename = std::enable_if_t<not std::is_same<U, T>{}> >
	friend bool operator<(const U& x, const T& y){return y > x;}
	template<class U, typename = std::enable_if_t<not std::is_same<U, T>{}> >
	friend bool operator<=(const U& x, const T& y){return (y > x) or (y == x);}
	template<class U, typename = std::enable_if_t<not std::is_same<U, T>{}> >
	friend bool operator>=(const U& x, const T& y){return (y < x) or (y == x);}
};

template<class T, class B = boost::operators_detail::empty_base<T>>
struct random_iterable : B{
	auto cbegin() const{return typename T::const_iterator{static_cast<T const&>(*this).begin()};}
	auto cend()   const{return typename T::const_iterator{static_cast<T const*>(this)->end()};}
	template<class SS>//, typename = std::enable_if_t<std::is_same<std::decay_t<SS>, T>{}> >
	friend auto cbegin(SS const& s, T* = 0)->decltype(s.cbegin()){return s.cbegin();}
	template<class SS>//, typename = std::enable_if_t<std::is_same<std::decay_t<SS>, T>{}> >
	friend auto cend  (SS const& s, T* = 0)->decltype(s.cend()  ){return s.cend  ();}
	auto crbegin() const{return typename T::const_reverse_iterator{cend  ()};}
	auto crend  () const{return typename T::const_reverse_iterator{cbegin()};}
	friend auto begin(T const& t){return t.begin();}
	friend auto begin(T& t){return t.begin();}
	friend auto end(T const& t){return t.end();}
	friend auto end(T& t){return t.end();}
//	template<class S>//, typename = std::enable_if_t<std::is_same<std::decay_t<S>, T>{}> > 
//	friend auto  begin(S&& s, T* =0){return std::forward<S>(s). begin();}
//	template<class S>//, typename = std::enable_if_t<std::is_same<std::decay_t<S>, T>{}> > 
//	friend auto  end  (S&& s, T* =0){return std::forward<S>(s). end  ();}
	template<class S>//, typename = std::enable_if_t<std::is_same<std::decay_t<S>, T>{}> > 
	friend auto rbegin(S&& s, T* =0){return std::forward<S>(s).rbegin();}
	template<class S>//, typename = std::enable_if_t<std::is_same<std::decay_t<S>, T>{}> > 
	friend auto rend  (S&& s, T* =0){return std::forward<S>(s).rend  ();}
//	template<class S>
//	friend auto crbegin(S const& s, ...){return s.crbegin();}
//	template<class S>
//	friend auto crend  (S const& s, T* =0){return s.crend  ();}
	decltype(auto) cfront() const{return static_cast<T const&>(*this).front();}
	decltype(auto) cback()  const{return static_cast<T const&>(*this).back() ;}
	friend auto cfront(T const& s){return s.cfront();}
	friend auto cback (T const& s){return s.cback() ;}
	friend auto size(T const& self){return self.size();}
};

}}

#if _TEST_BOOST_MULTI_DETAIL_OPERATORS

#include<iostream>
using std::cout;
namespace multi = boost::multi;

int main(){}

#endif
#endif

