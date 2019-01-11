#ifdef COMPILATION_INSTRUCTIONS
(echo "#include\""$0"\"" > $0x.cpp) && c++ `#-DNDEBUG` -std=c++17 -Wall -Wextra `#-Wfatal-errors` -D_TEST_BOOST_MULTI_ARRAY $0x.cpp -o $0x.x && time $0x.x $@ && rm -f $0x.x $0x.cpp; exit
#endif
#ifndef BOOST_MULTI_ARRAY_HPP
#define BOOST_MULTI_ARRAY_HPP

#include "./array_ref.hpp"

#include "detail/memory.hpp"

#include "utility.hpp"

namespace boost{
namespace multi{

template<int N> struct priority : priority<N-1>{};
template<> struct priority<0>{};

namespace detail{
    using std::begin;
    template<class C> auto maybestd_begin(C&& c) 
    ->decltype(begin(std::forward<C>(c))){
        return begin(std::forward<C>(c));}
	using std::end;
    template<class C> auto maybestd_end(C&& c) 
    ->decltype(end(std::forward<C>(c))){
        return end(std::forward<C>(c));}	
}

template<class C> auto maybestd_begin(C&& c)
->decltype(detail::maybestd_begin(std::forward<C>(c))){
    return detail::maybestd_begin(std::forward<C>(c));}
template<class C> auto maybestd_end(C&& c)
->decltype(detail::maybestd_end(std::forward<C>(c))){
    return detail::maybestd_end(std::forward<C>(c));}

template<class T, dimensionality_type D, class Alloc>
struct array : 
	private Alloc, 
	array_ref<T, D, typename std::allocator_traits<Alloc>::pointer>, 
	boost::multi::random_iterable<array<T, D, Alloc>>
{
	using allocator_type = Alloc;
	using ref = array_ref<T, D, typename std::allocator_traits<Alloc>::pointer>;
//	using add_lvalue_reference = array_ref<T, D, typename std::allocator_traits<Alloc>::pointer&>;
//	using value_type = typename std::conditional<
//		D == 1,
//		typename array::element,
//		array<typename array::element, D-1, allocator_type>
//	>::type;
private:
//	allocator_type allocator_;
	using alloc_traits = std::allocator_traits<allocator_type>;
	using typename ref::reference;
public:
	using typename ref::value_type;
	using ref::operator<;
	array() noexcept(noexcept(Alloc())) : Alloc{}, ref{}{}           //1a
	explicit array(Alloc const& a) : Alloc{a}, ref{}{}               //1b
	array(typename array::extensions_type x, typename array::element const& e, Alloc const& a = {})  //2
	:	Alloc{a}, ref(allocate(typename array::layout_t{x}.num_elements()), x){
		uninitialized_fill(e);
	}
	array(typename array::index_extension n, value_type const& v, Alloc const& a = {})
	: 	Alloc{a}, 
		ref{
			allocate(typename array::layout_t{tuple_cat(std::make_tuple(n), multi::extensions(v))}.num_elements()),
			tuple_cat(std::make_tuple(n), multi::extensions(v)),
		}
	{
		for(auto it = this->begin(); it != this->end(); ++it) *it = v;
	}
	array(typename array::size_type n, value_type const& v, Alloc const& a)
	: 	array(typename array::index_extension(n), v, a){}
	array(typename array::size_type n, value_type const& v)
	: 	array(typename array::index_extension(n), v){}

	explicit array(typename array::extensions_type x, Alloc const& a={}) //3
	:	Alloc{a}, ref{allocate(typename array::layout_t{x}.num_elements()), x}{
		uninitialized_value_construct();
	}
#if defined(__INTEL_COMPILER)
//	array(std::array<index, D> arr, Alloc const& a = {}) : 
//		array{multi::detail::to_tuple<typename array::index_extension>(il), arr}{}
	array(std::array<typename array::index_extension, D> arr, Alloc const& a = {}) : 
		array{multi::detail::to_tuple<typename array::index_extension>(arr), a}{}
#endif

	template<class It> static auto distance(It a, It b){using std::distance; return distance(a, b);}
	template<class It, typename=decltype(std::distance(std::declval<It>(), std::declval<It>()), *std::declval<It>())>      
	array(It first, It last, allocator_type const& a = {}) :                    //4
		Alloc{a}, 
		ref{}
	{
		auto const& f = *first;
		auto xx = extensions(f);
		layout_t<D>::operator=(typename array::layout_t{tuple_cat(std::make_tuple(index_extension{std::distance(first, last)}), xx)});
		this->base_ = allocate(typename array::layout_t{std::tuple_cat(std::make_tuple(index_extension{std::distance(first, last)}), multi::extensions(*first))}.num_elements());
		using std::next;
		using std::all_of;
		if(first!=last) assert( all_of(next(first), last, [x=multi::extensions(*first)](auto& e){return extensions(e)==x;}) );
		multi::uninitialized_copy<D>(first, last, ref::begin());
	}
	template<class Array, typename=std::enable_if_t</*not std::is_base_of<array, Array>{} and*/ multi::rank<std::remove_reference_t<Array>>{}>=1 > >//, typename=std::enable_if_t<std::rank<std::remove_reference_t<Array>>{}==D> >
	array(Array&& other, allocator_type const& a = {})
	:	Alloc{a}, ref{allocate(num_elements(other)), extensions(other)}{
		using std::begin; using std::end;
		multi::uninitialized_copy<D>(begin(other), end(other), ref::begin());
	}
	array(array const& other)                                                   // 5a
	:	Alloc{other}, ref{allocate(other.num_elements()), extensions(other)}{
		uninitialized_copy(other.data());
	}
	array(array const& other, allocator_type const& a)                      //5b
	:	Alloc{a}, ref{allocate(other.num_elements()), extensions(other)}{
		uninitialized_copy(other.data());
	}
	array(array&& other) noexcept                                           //6a
	:	Alloc{other.get_allocator()},
		ref{std::exchange(other.base_, nullptr), other.extensions()}
	{
		other.ref::layout_t::operator=({});
	}
	array(array&& other, allocator_type const& a)                           //6b
	:	Alloc{a},
		ref{std::exchange(other.base_, nullptr), other.extensions()}
	{
		//TODO
		other.ref::layout_t::operator=({});
	}
#if (not defined(__INTEL_COMPILER)) or (__GNUC >= 6)
	array(std::initializer_list<value_type> il, allocator_type const& a={}) 
	:	array(il.begin(), il.end(), a){}
#endif
	template<class A>
	array& operator=(A&& a){
		auto ext = extensions(a);
		if(ext==array::extensions()){
			ref::operator=(std::forward<A>(a));
		}else{
			clear();
			this->ref::layout_t::operator=(layout_t<D>{extensions(a)});
			this->base_ = allocate(this->num_elements());
		//	multi::uninitialized_copy<D>(maybestd_begin(std::forward<A>(a)), maybestd_end(std::forward<A>(a)), array::begin());
			multi::uninitialized_copy<D>(maybestd_begin(std::forward<A>(a)), maybestd_end(std::forward<A>(a)), array::begin());
		}
		return *this;
	}
	array& operator=(array const& other){return operator=<array const&>(other);}
	void swap(array& other) noexcept{
		using std::swap;
		swap(this->base_, other.base_);
		swap(static_cast<Alloc&>(*this), static_cast<Alloc&>(other));
		swap(
			static_cast<typename array_ref<T, D, typename std::allocator_traits<Alloc>::pointer>::layout_t&>(*this), 
			static_cast<typename array_ref<T, D, typename std::allocator_traits<Alloc>::pointer>::layout_t&>(other)
		);
	}
	friend void swap(array& a, array& b){a.swap(b);}
	array& operator=(array&& other){clear(); swap(other); return *this;}
	template<class It>
	array& assign(It first, It last){
		using std::next;
		using std::all_of;
		if(distance(first, last) == array::size() and multi::extensions(*first) == multi::extensions(*array::begin())){
			array_ref<T, D, typename std::allocator_traits<Alloc>::pointer>::assign(first, last);
		}else{
			clear();
			this->layout_t<D>::operator=(layout_t<D>{std::tuple_cat(std::make_tuple(index_extension{array::extension().front(), array::extension().front() + distance(first, last)}), multi::extensions(*first))});
			using std::next;
			using std::all_of;
			if(first!=last) assert( all_of(next(first), last, [x=multi::extensions(*first)](auto& e){return extensions(e)==x;}) );
			this->base_ = allocate();
			multi::uninitialized_copy<D>(first, last, array::begin());
		}
		return *this;
	}
	array& operator=(std::initializer_list<value_type> il){return assign(begin(il), end(il));}
	void reextent(typename array::extensions_type const& e, typename array::element const& v = {}){
		array tmp(e, v, static_cast<Alloc const&>(*this));
		tmp.intersection_assign_(*this);
		swap(tmp);
	}
	allocator_type get_allocator() const{return static_cast<allocator_type const&>(*this);}

	using element_const_ptr = typename std::pointer_traits<typename array::element_ptr>::template rebind<typename array::element const>;
	using const_reference = std::conditional_t<
		array::dimensionality!=1, 
		basic_array<typename array::element, array::dimensionality-1, element_const_ptr>, 
	//	typename std::iterator_traits<element_ptr>::reference const	//	
		typename pointer_traits<typename array::element_ptr>::element_type const&
	>;
	using const_iterator = typename std::conditional<
		array::dimensionality != 1,
		basic_array_ptr<const_reference, typename layout_t<D>::sub_t>,
		typename basic_array<typename array::element, dimensionality_type{1}, typename array::element_ptr>::template basic_iterator<element_const_ptr, const_reference>
	>::type;

	reference       operator[](index i)      {return ref::operator[](i);}
	const_reference operator[](index i) const{return ref::operator[](i);}

	typename array::element_ptr       data()      {return ref::data();}
	typename array::element_const_ptr data() const{return ref::data();}
	friend typename array::element_ptr       data(array&       s){return s.data();}
	friend typename array::element_const_ptr data(array const& s){return s.data();}

	typename array::element_ptr       origin()      {return ref::origin();}
	typename array::element_const_ptr origin() const{return ref::origin();}
	friend typename array::element_ptr       origin(array&       s){return s.origin();}
	friend typename array::element_const_ptr origin(array const& s){return s.origin();}

//	using const_reverse_iterator = basic_reverse_iterator<const_iterator>;

	typename array::iterator begin(){return ref::begin();}
	typename array::iterator end()  {return ref::end();}
//	typename array::iterator begin() &&{return ref::begin();}
//	typename array::iterator end()   &&{return ref::end();}

	typename array::iterator begin() const{return ref::begin();}
	typename array::iterator end()   const{return ref::end();}
#if 0
	friend auto begin(array&       s){return s.begin();}
	friend auto begin(array const& s){return s.begin();}
	friend auto end  (array&       s){return s.end();}
	friend auto end  (array const& s){return s.end();}

	auto cbegin() const{return begin();}
	auto cend()   const{return end();}
	friend auto cbegin(array const& s){return s.cbegin();}
	friend auto cend  (array const& s){return s.cend();}

	typename array::reverse_iterator rbegin(){return ref::rbegin();}
	typename array::reverse_iterator rend() {return ref::rend();}
	typename array::const_reverse_iterator rbegin() const{return ref::rbegin();}
	typename array::const_reverse_iterator rend() const{return ref::rend();}
#endif

#if 0
	friend auto begin(array const& s){return s.begin();}
	friend auto end(array const& s){return s.end();}
	friend auto begin(array& s){return s.begin();}
	friend auto end(array& s){return s.end();}
	typename array::const_reference front() const{assert(not this->empty()); return *this->cbegin();}
	typename array::const_reference back() const{assert(not this->empty()); return *(this->cbegin() + (this->size() - 1));}
	typename array::reference front(){return array_ref<T, D, typename std::allocator_traits<Alloc>::pointer>::front();}
	typename array::reference back(){return array_ref<T, D, typename std::allocator_traits<Alloc>::pointer>::back();}
#endif
	void clear() noexcept{
		destroy(); 
		deallocate();
		layout_t<D>::operator=({});
	}
	friend void clear(array& self) noexcept{self.clear();}
	~array() noexcept{clear();}	
private:
	void destroy(){destroy_n(this->data(), this->num_elements());}
	template<typename It>
	auto uninitialized_copy(It it){
		using std::uninitialized_copy_n;
		return uninitialized_copy_n(it, this->num_elements(), this->data());
	}
	auto uninitialized_default_construct(){
	//	using std::uninitialized_default_construct_n;
		return uninitialized_default_construct_n(this->base_, this->num_elements());
	}
	auto uninitialized_value_construct(){
	//	using std::uninitialized_value_construct_n;
		return uninitialized_value_construct_n(this->base_, this->num_elements());
	}
	auto uninitialized_fill(typename array::element const& el){
		using std::uninitialized_fill_n;
		return uninitialized_fill_n(this->base_, this->num_elements(), el);
	}
	typename array::element_ptr allocate(typename array::size_type n){
		return alloc_traits::allocate(*this, n);
	}
	auto allocate(){return allocate(this->num_elements());}
	void deallocate(){
		alloc_traits::deallocate(*this, this->base_, this->num_elements());
		this->base_ = nullptr;
	}
};

//template<class T, dimensionality_type D, class Alloc>
//array<T, D, Alloc>::array(size_type, array<T, D-1> const& arr, allocator_type const& a){}
//template<class T, class Alloc>
//array<T, 1u, Alloc>::array(size_type, T const& e, Alloc const& a){}

#if __cpp_deduction_guides
#define IL std::initializer_list
// clang cannot recognize templated-using, so don't replace IL<T>->IL1<T>
template<class T, class A=std::allocator<T>> array(IL<T>                , A={})->array<T,1,A>; 
template<class T, class A=std::allocator<T>> array(IL<IL<T>>            , A={})->array<T,2,A>;
template<class T, class A=std::allocator<T>> array(IL<IL<IL<T>>>        , A={})->array<T,3,A>; 
template<class T, class A=std::allocator<T>> array(IL<IL<IL<IL<T>>>>    , A={})->array<T,4,A>; 
template<class T, class A=std::allocator<T>> array(IL<IL<IL<IL<IL<T>>>>>, A={})->array<T,5,A>; 
#undef IL
#endif

}}

#if _TEST_BOOST_MULTI_ARRAY

#include<cassert>
#include<numeric> // iota
#include<iostream>
#include<algorithm>
#include<vector>

#include <random>
#include <boost/timer/timer.hpp>
#include<boost/multi_array.hpp>
using std::cout;
namespace multi = boost::multi;

#if 1
template<class Matrix, class Vector>
void solve(Matrix& m, Vector& y){
//	using std::size; // assert(size(m) == std::ptrdiff_t(size(y)));
	std::ptrdiff_t msize = size(m); 
	for(auto r = 0; r != msize; ++r){ //	auto mr = m[r]; //  auto const mrr = mr[r];// assert( mrr != 0 ); // m[r][r] = 1;
		auto mr = m[r];
		auto mrr = mr[r];
		for(auto c = r + 1; c != msize; ++c) mr[c] /= mrr;
		auto yr = (y[r] /= mrr);
		for(auto r2 = r + 1; r2 != msize; ++r2){ //	auto mr2 = m[r2]; //	auto const mr2r = mr2[r]; // m[r2][r] = 0;
			auto mr2 = m[r2];
			auto const& mr2r = mr2[r];
			auto const& mr = m[r];
			for(auto c = r + 1; c != msize; ++c) mr2[c] -= mr2r*mr[c];
			y[r2] -= mr2r*yr;
		}
	}
	for(auto r = msize - 1; r > 0; --r){ //	auto mtr = m.rotated(1)[r];
		auto const& yr = y[r];
		for(auto r2 = r-1; r2 >=0; --r2)
			y[r2] -= yr*m[r2][r];
	}
}
#endif

void f(boost::multi::array<double, 4> const& A){
	A[1][2];
	auto&& a = A[1][2]; (void)a; // careful, a is a reference here, don't use auto, 
	auto const& b = A[1][2]; (void)b; // use auto const& if possible
//	A[1][2][3][4] = 5; // fail, element is read-only
}

template<class C>
void set_99(C&& c){
	for(auto j : c.extension(0))
		for(auto k : c.extension(1))
				c[j][k] = 99.;
}

namespace multi = boost::multi;

template<class T> void fun(T const& t){
	std::cout << typeid(t).name() << std::endl;
}

template<class T> struct extension{};
template<class T> void gun(extension<T>){
	std::cout << typeid(T).name() << std::endl;
}

typedef double a1010[10][10];

struct A{
	double const* p;
	A(std::initializer_list<double> il){ p = &*(il.begin() + 1); };
};

double f(){return 5.;}
int main(){

#if __cpp_deduction_guides
{
	multi::array<double, 1> A1 = {1.,2.,3.}; 
	assert(A1.dimensionality==1 and A1.num_elements()==3);

	multi::array<double, 2> A2 {
		 {1.,2.,3.},
		 {4.,5.,6.}
	};
	*A2.begin()->begin() = 99;
	assert(A2[0][0] == 99 );
}
#endif

}
#endif
#endif

