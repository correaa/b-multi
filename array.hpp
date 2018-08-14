#ifdef COMPILATION_INSTRUCTIONS
(echo "#include\""$0"\"" > $0x.cpp) && clang++ -Ofast `#-DNDEBUG` -std=c++14 -Wall -Wextra -Wfatal-errors -lboost_timer -I${HOME}/prj -D_TEST_BOOST_MULTI_ARRAY $0x.cpp -o $0x.x && time $0x.x $@ && rm -f $0x.x $0x.cpp; exit
#endif
#ifndef BOOST_MULTI_ARRAY_HPP
#define BOOST_MULTI_ARRAY_HPP

#include "../multi/array_ref.hpp"

#include<iostream> // cerr
#include<algorithm>
#include<numeric>

namespace boost{
namespace multi{

using std::cerr;

template<class TT> auto list_extensions(std::initializer_list<TT> const& il){
	return std::array<index_extension, 1>{index_extension{0, size_type(il.size())}};
}
template<class TT> auto list_extensions(std::initializer_list<std::initializer_list<TT>> il){
	return std::array<index_extension, 2>{
		index_extension{0, size_type(il.size())}, index_extension{0, size_type(il.begin()->size())} 
	};
}
template<class TT> auto list_extensions(std::initializer_list<std::initializer_list<std::initializer_list<TT>>> il){
	return std::array<index_extension, 3>{
		index_extension{0, size_type(il.size())}, index_extension{0, size_type(il.begin()->size())}, index_extension{0, size_type(il.begin()->begin()->size())} 
	};
}

template<class T, dimensionality_type D, class Allocator>
class array : 
	public array_ref<T, D, typename std::allocator_traits<Allocator>::pointer>
{
	using extensions_type = std::array<index_extension, D>;
public:
	using allocator_type = Allocator;
private:
	allocator_type allocator_;
	using alloc_traits = std::allocator_traits<allocator_type>;
public:
	template<class Array, typename = decltype(std::declval<Array>().extensions())>
	array(Array&& other) : array(other.extensions()){
		array::operator=(std::forward<Array>(other));
	}
	array(extensions_type e = {}, Allocator alloc = Allocator{}) : 
		array_ref<T, D, typename array::element_ptr>(nullptr, e),
		allocator_(std::move(alloc))
	{
		this->data_ = alloc_traits::allocate(allocator_, array::num_elements());
		uninitialized_construct();
	}
	array(extensions_type e, typename array::element const& el, Allocator alloc = Allocator{}) : 
		array_ref<T, D, typename array::element_ptr>(nullptr, e),
		allocator_(std::move(alloc))
	{
		this->data_ = alloc_traits::allocate(allocator_, array::num_elements());
		uninitialized_construct(el);
	}
/*	array(std::initializer_list<typename array::value_type> il, Allocator alloc = Allocator{}) : array(list_extensions<typename array::element>(il), alloc){
		this->recursive_assign_(il.begin(), il.end());
	}*/
	array(typename array::initializer_list il, Allocator alloc = Allocator{}) : array(list_extensions<typename array::element>(il), alloc){
		this->recursive_assign_(il.begin(), il.end());
	}
	array& operator=(array const& other){
		array tmp(other.extensions());
		for(auto i : tmp.extension()) tmp[i] = other[i];
		swap(tmp);
		return *this;
	}
	template<class Array>
	array& operator=(Array const& a){
		array tmp(a.extensions());
		tmp.array_ref<T, D, typename std::allocator_traits<Allocator>::pointer>::operator=(a);
		swap(tmp);
		return *this;
	}
	void swap(array& other) noexcept{
		using std::swap;
		swap(this->data_, other.data_);
		using layout_t = std::decay_t<decltype(this->layout())>;
		swap(static_cast<layout_t&>(*this), static_cast<layout_t&>(other));
	}
	friend void swap(array& self, array& other){self.swap(other);}
	//! Destructs the array
	~array(){
		destroy();
		allocator_.deallocate(this->data(), this->num_elements());
	}
private:
	void destroy(){
	//	destroy(this->data() + this->num_elements());
	//	for(auto cur = this->data(); cur != last; ++cur)
	///		alloc_traits::destroy(allocator_, std::addressof(*cur));
		using std::for_each;
		for_each(
			this->data(), this->data() + this->num_elements(), 
			[&](auto&& e){alloc_traits::destroy(allocator_, std::addressof(e));}
		);
	}
	void destroy(typename array::element_ptr last){
		for(auto cur = this->data(); cur != last; ++cur)
			alloc_traits::destroy(allocator_, std::addressof(*cur));
	}
	template<class... Args>
	auto uninitialized_construct(Args&&... args){
		typename array::element_ptr cur = this->data();
		try{
			using std::for_each;
			for_each(
				this->data(), this->data() + this->num_elements(), 
				[&](auto&& e){
					alloc_traits::construct(
						allocator_, std::addressof(e), 
						std::forward<Args>(args)...
					);
				}
			);
		//	for(size_type n = this->num_elements(); n > 0; --n, ++cur)
		//		alloc_traits::construct(allocator_, std::addressof(*cur), std::forward<Args>(args)...);
		//	return cur;
		}catch(...){destroy(cur); throw;}
	}
};

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

int main(){
	
	assert(( boost::multi::list_extensions({1.,2.,3.})[0] == boost::multi::index_range{0, 3} ));
	assert(( boost::multi::list_extensions(
		{{1.,2.,3.}, {1.,2.,3.}}
	)[0] == boost::multi::index_range{0, 2} ));
	assert(( boost::multi::list_extensions(
		{{1.,2.,3.}, {1.,2.,3.}}
	)[1] == boost::multi::index_range{0, 3} ));

//	assert(( boost::multi::list_extensions(
//		{{1.,2.,3.}, {1.,2.,3.}}
//	)[1] == boost::multi::index_range{0, 3} ));

//	assert((boost::multi::extensions<1>({1.,2.})[0] == boost::multi::index_range{0,2}));
//	extensions<double>( {{1.,2.},{3.,4.}} );


//	assert( MA0[1][2] == 12. );

	multi::array<double, 2> MA0 = {
		{0.1, 01., 02.},
		{10., 11., 12.},
		{20., 21., 22.}
	};
	multi::array<double, 1> VV0 = {1.,2.,3.};

	return 0;
/*	for(auto i : MA0.extension(0)){
		for(auto j : MA0.extension(1))
			cout << MA0[i][j] << '\t';
		cout <<"\t|"<< VV0[i] <<'\n';
	}
	cout << "--\n";*/

/*	double MA00[3][3] = {
		{0.1, 01., 02.},
		{10., 11., 12.},
		{20., 21., 22.}
	};*/
	multi::array<double, 2> MA00 = {
		{0.1, 01., 02.},
		{10., 11., 12.},
		{20., 21., 22.}
	};
	std::vector<double> VV00 = {1.,2.,3.};
	solve(MA00, VV00);

	for(int i = 0; i != int(VV00.size()); ++i){//VV0.extension(0)){
	//	for(auto j : MA0.extension(1))
	//		cout << MA0[i][j] << '\t';
		cout <<"\t|"<< VV00[i] << std::endl;
	}
	for(auto n = 1; n < 10000; n = n*2)
	{
		multi::array<double, 2> MA( {n, n} );
//		boost::multi_array<double, 2> MA(boost::extents[3000][3000]);
		std::vector<double> V(n);
    	std::mt19937 rng;
		std::generate_n(MA.data(), MA.num_elements(), [&]{return rng();});
		std::generate_n(V.data(), V.size(), [&]{return rng();});
		{
			auto MA2 = MA;
			auto V2 = V;
			solve(MA2, V2);
			boost::timer::cpu_timer timer;
	//		boost::timer::auto_cpu_timer t;
			solve(MA, V);
			cout << n <<'\t' << timer.elapsed().user << std::endl;
		}
//		cout << "some " << V[13] << std::endl;
	}

	return 0;

	multi::array<double, 2> MA({2, 3});
	MA[1][1] = 11.;
	assert( MA[1][1] == 11.);
	multi::array<double, 2> MA2({4, 5});
	using std::swap;
	swap(MA, MA2);
	assert( MA.size() == 4 );
	assert( MA2.size() == 2 );
	cout << MA2[1][1] << std::endl;
	assert( MA2[1][1] == 11. );
	multi::array<double, 2> MA3 = MA2;//({2, 3});
//	MA3 = MA2;
	cout << MA3[1][1] << std::endl;
	assert(MA3[1][1] == 11.);
#if 0
	multi::array<double, 2> MAswap({4, 5});
	multi::array<double, 1> MA1({3});
	using std::swap;
	swap(MA, MAswap);
	assert(MA[2][2] == 0.);
	MA[1][3] = 7.1;
	assert(MA[1][3] == 7.1);
	cout << MA.stride() << '\n';	
	cout << MA.strides()[0] << '\n';
	cout << MA.strides()[1] << '\n';
	cout << "s = " << MA.size() << std::endl;
	assert( MA.size() == 4 );
	assert( MA.size(0) == 4 );
	assert( MA.size(1) == 5 );

	double d2D[4][5] {
		{ 0,  1,  2,  3,  4}, 
		{ 5,  6,  7,  8,  9}, 
		{10, 11, 12, 13, 14}, 
		{15, 16, 17, 18, 19}
	};
	multi::array_ref<double, 2> d2D_ref{&d2D[0][0], {4, 5}};
	d2D_ref[1][1] = 8.1;
	multi::array<double, 2> MA2;
	MA2 = d2D_ref;
	d2D_ref[1][1] = 8.2;
	assert(MA2.extensions() == d2D_ref.extensions());
	cout << MA2[1][1] << std::endl;
	assert(MA2[1][1] == 8.2);
#endif
}
#endif
#endif

