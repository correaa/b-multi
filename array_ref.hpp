#if COMPILATION_INSTRUCTIONS
(echo "#include\""$0"\"" > $0x.cpp) && clang++ -O3 -std=c++17 -Wall -Wextra `#-Wfatal-errors` -D_TEST_BOOST_MULTI_ARRAY_REF $0x.cpp -o $0x.x && time $0x.x $@ && rm -f $0x.x $0x.cpp; exit
#endif
#ifndef BOOST_MULTI_ARRAY_REF_HPP
#define BOOST_MULTI_ARRAY_REF_HPP


#include "utility.hpp"

#include "detail/layout.hpp"
#include "detail/types.hpp" // dimensionality_type
#include "detail/operators.hpp" // random_iterable
#include "detail/memory.hpp" // pointer_traits

#include<algorithm> // copy_n

namespace boost{
namespace multi{

template<typename T, dimensionality_type D, typename ElementPtr, class Layout = layout_t<D>> 
struct basic_array;

template<typename T, dimensionality_type D, class Alloc = std::allocator<T>> 
struct array;

template<typename T, dimensionality_type D, typename ElementPtr, class Layout>
struct array_types : Layout{ //	template<class... As> array_types(As&&... as) : Layout{std::forward<As>(as)...}{}
	using element = T;
	constexpr static dimensionality_type dimensionality = D;
	using element_ptr = ElementPtr;
	using layout_t = Layout;
	using value_type = std::conditional_t<
		dimensionality!=1, 
		array<element, dimensionality-1>, 
		element
	>;
	using decay_type = array<element, dimensionality, typename pointer_traits<element_ptr>::default_allocator_type>;
	using difference_type = index;
	using reference       = std::conditional_t<
		dimensionality!=1, 
		basic_array<element, dimensionality-1, element_ptr>, 
		typename std::iterator_traits<element_ptr>::reference 	//	typename pointer_traits<element_ptr>::element_type&
	>;
	element_ptr        base() const{return base_;} //	element_const_ptr cbase() const{return base();}
	layout_t const& layout() const{return *this;}
	friend layout_t const& layout(array_types const& s){return s.layout();}
	element_ptr            origin() const{return base_+Layout::origin();} //	element_const_ptr     corigin() const{return origin();}
	friend decltype(auto)  origin(array_types const& s){return s.origin();} //	friend decltype(auto) corigin(array_types const& s){return s.corigin();}
protected:
	using derived = basic_array<T, D, ElementPtr, Layout>;
	element_ptr base_;
	array_types() = delete;
	array_types(std::nullptr_t np) : Layout{}, base_{np}{}
	array_types(array_types const&) = default; //	array_types(array_types const& o) : Layout{o}, base_{o.base_}{}
	array_types(layout_t l, element_ptr data) : Layout{l}, base_{data}{}
//	template<class ArrayTypes, typename = decltype(Layout{std::declval<ArrayTypes&&>().layout()}, element_ptr{std::declval<ArrayTypes&&>().base_})>
//	array_types(ArrayTypes&& other) : Layout{other.layout()}, base_{other.base_}{}
public://TODO find why this needs to be public and not protected or friend
	template<class ArrayTypes> array_types(ArrayTypes&& a) : Layout{a}, base_{a.base_}{}
	template<typename ElementPtr2>
	array_types(array_types<T, D, ElementPtr2, Layout> const& other) : Layout{other.layout()}, base_{other.base_}{}
	template<class T2, dimensionality_type D2, class E2, class L2> friend struct array_types;
};

template<class Ref, class Layout>
struct basic_array_ptr : 
	private Ref,
	boost::iterator_facade<
		basic_array_ptr<Ref, Layout>, void, std::random_access_iterator_tag, 
		Ref const&, typename Layout::difference_type
	>,
	boost::totally_ordered<basic_array_ptr<Ref, Layout>>
{
	using difference_type = typename Layout::difference_type;
	using value_type = typename Ref::decay_type;
	using pointer = Ref const*;
	using reference = Ref;//Ref const&;
	using element_type = typename Ref::value_type;
	using iterator_category = std::random_access_iterator_tag;
	basic_array_ptr(std::nullptr_t p = nullptr) : Ref{p}{}
	template<class, class> friend struct basic_array_ptr;
	template<class Other, typename = decltype(typename Ref::types::element_ptr{typename Other::element_ptr{}})> 
	basic_array_ptr(Other const& o) : Ref{o.layout(), o.base()}, stride_{o.stride_}{}
//	template<class Other>//, typename = decltype(typename Ref::types::element_ptr{typename Other::types::element_ptr{}})> 
//	basic_array_ptr(Other const& o) : Ref{o.base(), o.layout()}, stride_{o.stride_}{}
	basic_array_ptr& operator=(basic_array_ptr const& other){
		this->base_ = other.base_;
	//	layout = other.layout;
		static_cast<Layout&>(*this) = other; //impl_) = other.impl_;
		this->stride_ = other.stride_;
		return *this;
	}
//	explicit operator bool() const{return this->base_;}
	Ref const& operator*() const{/*assert(*this);*/ return *this;}
	Ref const* operator->() const{/*assert(*this);*/ return this;}
	Ref        operator[](difference_type n) const{return *(*this + n);}
	template<class O> bool operator==(O const& o) const{return equal(o);}
	bool operator<(basic_array_ptr const& o) const{return distance_to(o) > 0;}
	basic_array_ptr(typename Ref::element_ptr p, Layout l, index stride) : Ref{l, p}, stride_{stride}{}
	template<typename T, dimensionality_type D, typename ElementPtr, class LLayout>
	friend struct basic_array;
	auto stride() const{return stride_;}
	friend index stride(basic_array_ptr const& s){return s.stride();}
	auto base() const{return this->base_;}
	friend auto base(basic_array_ptr const& self){return self.base();}
private:
	index stride_ = {1}; // nice, non-zero default
	using Ref::layout;
	using Ref::base_;
	bool equal(basic_array_ptr const& o) const{return base_==o.base_ && stride_==o.stride_ && layout()==o.layout();}
	void increment(){base_ += stride_;}
	void decrement(){base_ -= stride_;}
	void advance(difference_type n){base_ += stride_*n;}
	difference_type distance_to(basic_array_ptr const& other) const{
		assert( stride_ == other.stride_ and stride_ != 0 and (other.base_ - base_)%stride_ == 0 and layout() == other.layout() );
		return (other.base_ - base_)/stride_;
	}
	friend class boost::iterator_core_access;
};

// TODO remove the T parameter?s
template<typename T, dimensionality_type D, typename ElementPtr, class Layout /*= layout_t<D>*/ >
struct basic_array : 
	boost::multi::partially_ordered2<basic_array<T, D, ElementPtr, Layout>, void>,
	boost::multi::random_iterable<basic_array<T, D, ElementPtr, Layout>>,
	array_types<T, D, ElementPtr, Layout>
{
	using types = array_types<T, D, ElementPtr, Layout>;
	friend struct basic_array<typename types::element, typename Layout::rank{} + 1, typename types::element_ptr >;
	friend struct basic_array<typename types::element, typename Layout::rank{} + 1, typename types::element_ptr&>;
//	friend struct basic_array<typename types::element, Layout::rank    , typename std::pointer_traits<typename types::element_ptr>::template rebind<typename types::element>>;
	using types::layout;
	decltype(auto) layout() const{return array_types<T, D, ElementPtr, Layout>::layout();}
protected:
	using types::types;
	template<typename, dimensionality_type, class Alloc> friend struct array;
public:
	template<class BasicArray, typename = decltype(types(std::declval<BasicArray&&>()))> 
	basic_array(BasicArray&& other) : types{std::forward<BasicArray>(other)}{}
	using decay_type = array<typename types::element, D, typename pointer_traits<typename types::element_ptr>::default_allocator_type>;
	decay_type decay() const{return *this;}
	friend auto decay(basic_array const& self){return self.decay();}
	typename types::reference operator[](index i) const{
		assert( this->extension().count(i) );
		return {sub, types::base_ + Layout::operator()(i)};
	}
	basic_array sliced(typename types::index first, typename types::index last) const{
		typename types::layout_t new_layout = *this;
		(new_layout.nelems_/=Layout::size())*=(last - first);
		return {new_layout, types::base_ + Layout::operator()(first)};
	}
	basic_array strided(typename types::index s) const{
		typename types::layout_t new_layout = *this; new_layout.stride_*=s;
		return {new_layout, types::base_ + types::nelems_};
	}
	basic_array sliced(typename types::index first, typename types::index last, typename types::index stride) const{
		return sliced(first, last).strided(stride);
	}
	auto range(typename types::index_range const& ir) const{
		return sliced(ir.front(), ir.front() + ir.size());
	}
	auto range(typename types::index_range const& ir, dimensionality_type n) const{
		return rotated(n).range(ir).rotated(-n);
	}
	auto rotated() const{
		typename types::layout_t new_layout = *this; 
		new_layout.rotate();
		return basic_array<T, D, ElementPtr>{new_layout, types::base_};
	}
	friend auto rotated(basic_array const& self){return self.rotated();}
	auto unrotated() const{
		typename types::layout_t new_layout = *this; 
		new_layout.unrotate();
		return basic_array<T, D, ElementPtr>{new_layout, types::base_};
	}
	friend auto unrotated(basic_array const& self){return self.unrotated();}
	auto rotated(dimensionality_type i) const{
		typename types::layout_t new_layout = *this; 
		new_layout.rotate(i);
		return basic_array<T, D, ElementPtr>{new_layout, types::base_};
	}
	basic_array const& operator()() const{return *this;}
	template<class... As>
	auto operator()(index_range a, As... as) const{return range(a).rotated()(as...).unrotated();}
	template<class... As>
	auto operator()(index i, As... as) const{return operator[](i)(as...);}
#define SARRAY2(A1, A2)	auto operator()(A1 a1, A2 a2) const{return operator()<A2>(a1, a2);}
	SARRAY2(index, index ); SARRAY2(irange, index );
	SARRAY2(index, irange); SARRAY2(irange, irange);
#undef SARRAY2
#define SARRAY3(A1, A2, A3) auto operator()(A1 a1, A2 a2, A3 a3) const{return operator()<A2, A3>(a1, a2, a3);}
	SARRAY3(index, index , index ); SARRAY3(irange, index , index );
	SARRAY3(index, index , irange); SARRAY3(irange, index , irange);
	SARRAY3(index, irange, index ); SARRAY3(irange, irange, index );
	SARRAY3(index, irange, irange); SARRAY3(irange, irange, irange);
#undef SARRAY3
#define SARRAY4(A1, A2, A3, A4) auto operator()(A1 a1, A2 a2, A3 a3, A4) const{return operator()<A2, A3, A4>(a1, a2, a3);}
	SARRAY4(index, index, index , index ); SARRAY4(index, irange, index , index );
	SARRAY4(index, index, index , irange); SARRAY4(index, irange, index , irange);
	SARRAY4(index, index, irange, index ); SARRAY4(index, irange, irange, index );
	SARRAY4(index, index, irange, irange); SARRAY4(index, irange, irange, irange);
	SARRAY4(irange, index, index , index ); SARRAY4(irange, irange, index , index );
	SARRAY4(irange, index, index , irange); SARRAY4(irange, irange, index , irange);
	SARRAY4(irange, index, irange, index ); SARRAY4(irange, irange, irange, index );
	SARRAY4(irange, index, irange, irange); SARRAY4(irange, irange, irange, irange);
#undef SARRAY4
private:
	using Layout::nelems_;
	using Layout::stride_;
	using Layout::sub;
public:
	using iterator = basic_array_ptr<typename types::reference, typename types::sub_t>;
private:
	template<class Iterator>
	struct basic_reverse_iterator : 
		std::reverse_iterator<Iterator>,
		boost::totally_ordered<basic_reverse_iterator<Iterator>>
	{
		template<class O, typename = decltype(std::reverse_iterator<Iterator>{base(std::declval<O const&>())})>
		basic_reverse_iterator(O const& o) : std::reverse_iterator<Iterator>{base(o)}{}
		basic_reverse_iterator() : std::reverse_iterator<Iterator>(){}
		explicit basic_reverse_iterator(Iterator it) : std::reverse_iterator<Iterator>(--it){}
		explicit operator Iterator() const{auto ret = this->base(); return ++ret;}
		explicit operator bool() const{auto ret = this->base(); ++ret; return static_cast<bool>(ret);}
		typename Iterator::reference operator*() const{return this->current;}
		typename Iterator::pointer operator->() const{return &this->current;}
		typename Iterator::reference operator[](typename Iterator::difference_type n) const{return *(this->current - n);}
		bool operator<(basic_reverse_iterator const& o) const{return o.base() < this->base();}
	};
public:
	using reverse_iterator = basic_reverse_iterator<iterator>;
	using ptr = basic_array_ptr<basic_array, Layout>;
	ptr operator&() const{return {this->base_, this->layout(), this->nelems_};}
//	friend auto strides(basic_array const& self){return self.strides();}
	iterator begin(index i) const{
		Layout l = static_cast<Layout const&>(*this); l.rotate(i);
		return {types::base_ + l(0       ), l.sub, l.stride_};
	}
	iterator end(index i) const{
		Layout l = static_cast<Layout const&>(*this); l.rotate(i);
		return {types::base_ + l(l.size()), l.sub, l.stride_};
	}
	iterator  begin() const{return {types::base_          , sub, stride_};}
	iterator  end  () const{return {types::base_ + nelems_, sub, stride_};}
//	typename types::reference front() const{return * begin();}
//	typename types::reference back()  const{return *this->rbegin();}
protected:
	template<class A>
	void intersection_assign_(A&& other) const{
		for(auto i : intersection(types::extension(), extension(other)))
			operator[](i).intersection_assign_(std::forward<A>(other)[i]);
	}
public:
	template<class It> void assign(It first, It last) const{
		using std::copy; using std::distance;
		assert( distance(first, last) == this->size() );
		copy(first, last, this->begin());
	}
	void assign(std::initializer_list<typename basic_array::value_type> il) const{assign(il.begin(), il.end());}
	template<class A, typename = std::enable_if_t<not std::is_base_of<basic_array, std::decay_t<A>>{}>>
	basic_array const& operator=(A&& o) const{
		using multi::extension;
		assert(extension(*this) == extension(o));
		using std::begin; using std::end;
		assign(begin(std::forward<A>(o)), end(std::forward<A>(o)));
		return *this;
	}
	basic_array const& operator=(basic_array const& o) const{
		using multi::extension;
		assert(extension(*this) == extension(o));
		using std::begin; using std::end;
		assign(begin(o), end(o));
		return *this;
	}
	template<class Array>
	void swap(Array&& o) const{
		using multi::extension; 
		using std::swap_ranges; 
		using std::begin;
		assert(this->extension() == extension(o));
		swap_ranges(this->begin(), this->end(), begin(std::forward<Array>(o)));
	}
	friend void swap(basic_array const& a1, basic_array const& a2){
		return a1.swap(a2);
	}
	template<class Array> void swap(basic_array const& a1, Array&& a2){return a1.swap(a2);}
	template<class Array> void swap(Array&& a1, basic_array const& a2){return a2.swap(a1);}
	template<class Array> 
	bool operator==(Array const& other) const{
		using multi::extension; using std::equal; using std::begin;
		if(this->extension() != extension(other)) return false;
		return equal(this->begin(), this->end(), begin(other));
	}
private:
	template<class A1, class A2>
	static auto lexicographical_compare(A1 const& a1, A2 const& a2){
		using multi::extension;
		if(extension(a1).first() > extension(a2).first()) return true;
		if(extension(a1).first() < extension(a2).first()) return false;
		using std::lexicographical_compare;
		using std::begin; using std::end;
		return lexicographical_compare(begin(a1), end(a1), begin(a2), end(a2));
	}
public:
	template<class O>
	bool operator<(O const& o) const{return lexicographical_compare(*this, o);}
	template<class O>
	bool operator>(O const& o) const{return lexicographical_compare(o, *this);}
};

template<class T2, class P2 = std::add_pointer_t<T2>, class T, dimensionality_type D, class P>
auto static_array_cast(basic_array<T, D, P> const& o){return basic_array<T2, D, P2>(o);} //need namespace

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template<class Element, dimensionality_type D, typename Ptr, class Ref = typename std::iterator_traits<Ptr>::reference>
struct array_iterator;

template<class Element, typename Ptr, typename Ref>
struct array_iterator<Element, 1, Ptr, Ref> : 
	boost::iterator_facade<
		array_iterator<Element, 1, Ptr, Ref>, 
		Element, std::random_access_iterator_tag, 
		Ref, multi::difference_type
	>
{
	template<class Other, typename = decltype(Ptr{typename Other::pointer{}})> 
	array_iterator(Other const& o) : data_{o.data_}, stride_{o.stride_}{}
	array_iterator(std::nullptr_t np = nullptr) : data_{np}, stride_{}{}
	explicit operator bool() const{return static_cast<bool>(this->data_);}
	Ref operator[](typename array_iterator::difference_type n) const{assert(*this); return *((*this) + n);}
private:
	array_iterator(Ptr d, typename basic_array<Element, 1, Ptr>::index s) : data_{d}, stride_{s}{}
	friend struct basic_array<Element, 1, Ptr>;
	Ptr data_;
	multi::index stride_;
//	typename types::index stride_;
	Ref dereference() const{/*assert(data_);*/ return *data_;}
	bool equal(array_iterator const& o) const{return data_ == o.data_ and stride_ == o.stride_;}
	void increment(){data_ += stride_;}
	void decrement(){data_ -= stride_;}
	void advance(typename array_iterator::difference_type n){data_ += stride_*n;}
	difference_type distance_to(array_iterator const& other) const{
		assert( stride_ == other.stride_ and stride_ != 0 and (other.data_ - data_)%stride_ == 0);
		return (other.data_ - data_)/stride_;
	}
    friend class boost::iterator_core_access;
    auto base() const{return data_;}
    friend auto base(array_iterator const& self){return self.base();}
    auto stride() const{return stride_;}
    friend auto stride(array_iterator const& self){return self.stride();}
};


//template<class Element, typename Ptr, class Ref>
//struct array_iterator<Element, 1, Ptr, Ref> :  basic_iterator<Element, Ptr, Ref>{
//	using basic_iterator<Element, Ptr, Ref>::basic_iterator;
//};

template<typename T, typename ElementPtr, class Layout>
struct basic_array<T, dimensionality_type{1}, ElementPtr, Layout> : 
	multi::partially_ordered2<basic_array<T, dimensionality_type(1), ElementPtr, Layout>, void>,
	multi::random_iterable<basic_array<T, dimensionality_type(1), ElementPtr, Layout> >,
	array_types<T, dimensionality_type(1), ElementPtr, Layout>
{
	using types = array_types<T, dimensionality_type{1}, ElementPtr, Layout>;
	using types::types;
	using decay_type = array<typename types::element, dimensionality_type{1}, typename pointer_traits<typename types::element_ptr>::default_allocator_type>;
	decay_type decay() const{return *this;}
	friend decay_type decay(basic_array const& self){return self.decay();}
protected:
	template<class A>
	void intersection_assign_(A&& other) const{
		for(auto i : intersection(types::extension(), extension(other)))
			operator[](i) = std::forward<A>(other)[i];
	}
protected:
//	friend struct basic_array<T, dimensionality_type{types::dimensionality + 1}, typename types::element_ptr>;
//	friend struct basic_array<T, dimensionality_type{types::dimensionality + 1}, typename types::element_ptr&>;
//	template<class T2, dimensionality_type D, class P2, class L> friend struct basic_array;
//	friend struct basic_array<T, dimensionality_type{types::dimensionality + 1}, typename std::pointer_traits<typename types::element_ptr>::template rebind<typename types::element>>;
	template<class TT, dimensionality_type DD, typename EP, class LLayout> friend struct basic_array;
	template<class TT, dimensionality_type DD, class Alloc> friend struct array;
	basic_array(basic_array const&) = default;
	template<class T2, class P2 = T2*>
	friend basic_array<T2, 1, P2, Layout> static_array_cast(basic_array const& o){return basic_array<T2, 1, P2, Layout>(o);}
//	friend basic_array<T, 1, double const*, Layout> f(basic_array const& o){return basic_array<T, 1, double const*, Layout>(o);}
public:
	template<class BasicArray, typename = decltype(types(std::declval<BasicArray&&>()))> 
	basic_array(BasicArray&& other) : types{std::forward<BasicArray>(other)}{}

	basic_array_ptr<basic_array, Layout> operator&() const{
		return {this->base_, this->layout(), this->nelems_};
	}
	template<class It> void assign(It first, It last) const{
		using std::distance;// assert(distance(first, last) == this->size());
		using std::copy; copy(first, last, this->begin());
	}
	template<class A, typename = std::enable_if_t<not std::is_base_of<basic_array, std::decay_t<A>>{}> >
	basic_array const& operator=(A&& o) const{
		using multi::extension;
		assert(this->extension() == extension(o));
		using std::begin; using std::end;
		this->assign(begin(std::forward<A>(o)), end(std::forward<A>(o)));
		return *this;
	}
	basic_array const& operator=(basic_array const& o) const{
		using multi::extension;
		assert(this->extension() == extension(o));
		using std::begin; using std::end;
		this->assign(begin(o), end(o));
		return *this;
	}
	typename types::reference operator[](typename types::index i) const{
		return *(types::base_ + Layout::operator()(i)); // types::base_[Layout::operator()(i)];
	}
	basic_array sliced(typename types::index first, typename types::index last) const{
		typename types::layout_t new_layout = *this; 
		(new_layout.nelems_/=Layout::size())*=(last - first);
		return {new_layout, types::base_ + Layout::operator()(first)};		
	}
	basic_array strided(typename types::index s) const{
		typename types::layout_t new_layout = *this;
		new_layout.stride_*=s;
		return {new_layout, types::base_ + Layout::operator()(this->extension().front())};		
	}
	basic_array sliced(typename types::index first, typename types::index last, typename types::index stride) const{
		return sliced(first, last).strided(stride);
	}
	auto range(index_range const& ir) const{return sliced(ir.front(), ir.last());}
	decltype(auto) operator()() const{return *this;}
	auto operator()(index_range const& ir) const{return range(ir);}
	auto operator()(typename types::index i) const{return operator[](i);}
	decltype(auto) rotated() const{return *this;}
	decltype(auto) unrotated() const{return *this;}	
	decltype(auto) rotated(dimensionality_type) const{return *this;}
public:
#if 0
	template<typename Ptr, typename Ref>
	struct basic_iterator : 
		boost::iterator_facade<
			basic_iterator<Ptr, Ref>, 
			typename types::element, std::random_access_iterator_tag, 
			Ref, typename types::difference_type
		>
	{
		template<class Other, typename = decltype(Ptr{typename Other::pointer{}})> 
		basic_iterator(Other const& o) : data_{o.data_}, stride_{o.stride_}{}
		basic_iterator(std::nullptr_t np = nullptr) : data_{np}, stride_{}{}
		explicit operator bool() const{return static_cast<bool>(this->data_);}
		Ref operator[](typename types::difference_type n) const{assert(*this); return *((*this) + n);}
	private:
		basic_iterator(Ptr d, typename basic_array::index s) : data_{d}, stride_{s}{}
		friend struct basic_array;
		Ptr data_;
		typename types::index stride_;
		Ref dereference() const{/*assert(data_);*/ return *data_;}
		bool equal(basic_iterator const& o) const{return data_ == o.data_ and stride_ == o.stride_;}
		void increment(){data_ += stride_;}
		void decrement(){data_ -= stride_;}
		void advance(typename types::difference_type n){data_ += stride_*n;}
		difference_type distance_to(basic_iterator const& other) const{
			assert( stride_ == other.stride_ and stride_ != 0 and (other.data_ - data_)%stride_ == 0);
			return (other.data_ - data_)/stride_;
		}
	    friend class boost::iterator_core_access;
	    auto base() const{return data_;}
	    friend auto base(basic_iterator const& self){return self.base();}
	    auto stride() const{return stride_;}
	    friend auto stride(basic_iterator const& self){return self.stride();}
	};
#endif
public:
//	using iterator = basic_array_ptr<typename types::element, typename types::sub_t>;
	using iterator = multi::array_iterator<typename types::element, 1, typename types::element_ptr, typename types::reference>;
//	using iterator = basic_iterator<typename types::element_ptr, typename types::reference>;//decltype(*std::declval<typename types::element_ptr>())>;
	using reverse_iterator = std::reverse_iterator<iterator>;

	friend size_type num_elements(basic_array const& self){return self.num_elements();}
	friend index_range extension(basic_array const& self){return self.extension();}

	iterator  begin() const{return {types::base_                 , Layout::stride_};}
	iterator  end  () const{return {types::base_ + types::nelems_, Layout::stride_};}
	auto     rbegin() const{return reverse_iterator{end()  };}
	auto     rend  () const{return reverse_iterator{begin()};}

//	typename types::reference front() const{return *begin();}
//	typename types::reference back () const{return *rbegin();}

	template<class Array> bool operator==(Array const& other) const{
		using multi::extension; using std::equal; using std::begin;
		if(this->extension() != extension(other)) return false;
		return equal(this->begin(), this->end(), begin(other));
	}
	bool operator<(basic_array const& o) const{return operator< <basic_array const&>(o);}
	template<class Array> void swap(Array&& o) const{
		using multi::extension; using std::swap_ranges; using std::begin;
		assert(this->extension() == extension(o));
		swap_ranges(this->begin(), this->end(), begin(std::forward<Array>(o)));
	}
//	friend void swap(basic_array&& a1, basic_array&& a2){return a1.swap(a2);}
	friend void swap(basic_array const& a1, basic_array const& a2){return a1.swap(a2);}
	template<class A> friend void swap(basic_array&& a1, A&& a2){return a1.swap(std::forward<A>(a2));}
	template<class A> friend void swap(A&& a1, basic_array&& a2){return a2.swap(std::forward<A>(a1));}
private:
	template<class A1, class A2>
	static auto lexicographical_compare(A1 const& a1, A2 const& a2){
		using multi::extension;
		if(extension(a1).first() > extension(a2).first()) return true;
		if(extension(a1).first() < extension(a2).first()) return false;
		using std::lexicographical_compare;
		using std::begin; using std::end;
		return lexicographical_compare(begin(a1), end(a1), begin(a2), end(a2));
	}
public:
	template<class O>
	bool operator<(O const& o) const{return lexicographical_compare(*this, o);}
	template<class O>
	bool operator>(O const& o) const{return lexicographical_compare(o, *this);}

};

template<typename T, dimensionality_type D, typename ElementPtr = T*>
struct array_ref : 
	boost::multi::partially_ordered2<array_ref<T, D, ElementPtr>, void>,
	basic_array<T, D, ElementPtr>
{	
protected:
	constexpr array_ref() noexcept
		: basic_array<T, D, ElementPtr>{typename array_ref::types::layout_t{}, nullptr}{}
public:
	array_ref(array_ref const&) = default;
	constexpr array_ref(typename array_ref::element_ptr p, typename array_ref::extensions_type e) noexcept
		: basic_array<T, D, ElementPtr>{typename array_ref::types::layout_t{e}, p}{}
	using basic_array<T, D, ElementPtr>::operator=;
	using basic_array<T, D, ElementPtr>::operator<;
	using basic_array<T, D, ElementPtr>::operator>;
	template<class ArrayRef> array_ref(ArrayRef&& a) : array_ref(a.data(), extensions(a)){} 
	array_ref& operator=(array_ref const&) & = delete;
	template<class A, typename = std::enable_if_t<not std::is_base_of<array_ref, std::decay_t<A>>{}> >
	array_ref const& operator=(A&& o) const{
		using multi::extension;
		assert(this->extension() == extension(o));
		using std::begin; using std::end;
		this->assign(begin(std::forward<A>(o)), end(std::forward<A>(o)));
		return *this;
	}
	array_ref const& operator=(array_ref const& o)&&{
		using multi::extension;
		assert(this->extension() == extension(o));
		using std::begin; using std::end;
		this->assign(begin(o), end(o));
		return *this;
	}
	typename array_ref::element_ptr data() const{return array_ref::base_;}
	friend typename array_ref::element_ptr data(array_ref const& self){return self.data();}

};

template<class T, dimensionality_type D> 
using array_cref = array_ref<T, D, T const*>;

template<dimensionality_type D, class P>
array_ref<typename std::iterator_traits<P>::value_type, D, P> 
make_array_ref(P p, index_extensions<D> x){return {p, x};}

template<class P> auto make_array_ref(P p, index_extensions<1> x){return make_array_ref<1>(p, x);}
template<class P> auto make_array_ref(P p, index_extensions<2> x){return make_array_ref<2>(p, x);}
template<class P> auto make_array_ref(P p, index_extensions<3> x){return make_array_ref<3>(p, x);}
template<class P> auto make_array_ref(P p, index_extensions<4> x){return make_array_ref<4>(p, x);}
template<class P> auto make_array_ref(P p, index_extensions<5> x){return make_array_ref<5>(p, x);}

//In ICC you need to specify the dimensionality in make_array_ref<D>
#if defined(__INTEL_COMPILER)
template<dimensionality_type D, class P> 
auto make_array_ref(P p, std::initializer_list<index_extension> il){return make_array_ref(p, detail::to_tuple<D, index_extension>(il));}
template<dimensionality_type D, class P> 
auto make_array_ref(P p, std::initializer_list<index> il){return make_array_ref(p, detail::to_tuple<D, index_extension>(il));}
#endif

#if __cpp_deduction_guides
template<class It> array_ref(It, typename detail::repeat<index_extension, 1>::type)->array_ref<typename std::iterator_traits<It>::value_type, 1, It>;
template<class It> array_ref(It, typename detail::repeat<index_extension, 2>::type)->array_ref<typename std::iterator_traits<It>::value_type, 2, It>;
template<class It> array_ref(It, typename detail::repeat<index_extension, 3>::type)->array_ref<typename std::iterator_traits<It>::value_type, 3, It>;
template<class It> array_ref(It, typename detail::repeat<index_extension, 4>::type)->array_ref<typename std::iterator_traits<It>::value_type, 4, It>;
template<class It> array_ref(It, typename detail::repeat<index_extension, 5>::type)->array_ref<typename std::iterator_traits<It>::value_type, 5, It>;
#endif

}}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


#if _TEST_BOOST_MULTI_ARRAY_REF

#include<cassert>
#include<numeric> // iota
#include<iostream>
#include<vector>

using std::cout;
namespace multi = boost::multi;


int main(){

	{
		double a[4][5] = {{1.,2.},{2.,3.}};
		multi::array_ref<double, 2> A(&a[0][0], {4, 5});
		multi::array_ref<double, 2, double const*> B(A);
		multi::array_ref<double const, 2> C(A);
		multi::array_cref<double, 2> D(A);
		A[1][1] = 2.;
//		A[1].cast<double const*>()[1] = 2.;
		double d[4][5] = {{1.,2.},{2.,3.}};
	//	typedef d45 = double const[4][5];
		auto dd = (double const(&)[4][5])(d);
		assert( &(dd[1][2]) == &(d[1][2]) );
		assert(( &multi::static_array_cast<double, double const*>(A[1])[1] == &A[1][1] ));
	}
	{
		double const d2D[4][5] = {{1.,2.},{2.,3.}};
		multi::array_ref<double, 2, const double*> d2Rce(&d2D[0][0], {4, 5});
		assert( &d2Rce[2][3] == &d2D[2][3] );
		assert( d2Rce.size() == 4 );
		assert( num_elements(d2Rce) == 20 );
	}
	{
		std::string const dc3D[4][2][3] = {
			{{"A0a", "A0b", "A0c"}, {"A1a", "A1b", "A1c"}},
			{{"B0a", "B0b", "B0c"}, {"B1a", "B1b", "B1c"}},
			{{"C0a", "C0b", "C0c"}, {"C1a", "C1b", "C1c"}}, 
			{{"D0a", "D0b", "D0c"}, {"D1a", "D1b", "D1c"}}, 
		};
		multi::array_cref<std::string, 3> A(&dc3D[0][0][0], {4, 2, 3});
		assert( num_elements(A) == 24 and A[2][1][1] == "C1b" );
		auto const& A2 = A.sliced(0, 3).rotated()[1].sliced(0, 2).unrotated();
		assert( multi::rank<std::decay_t<decltype(A2)>>{} == 2 and num_elements(A2) == 6 );
		assert( std::get<0>(sizes(A2)) == 3 and std::get<1>(sizes(A2)) == 2 );
		{auto x = extensions(A2);
		for(auto i : std::get<0>(x) ){
			for(auto j : std::get<1>(x) ) cout<< A2[i][j] <<' ';
			cout<<'\n';
		}}
		auto const& A3 = A({0, 3}, 1, {0, 2});
		assert( multi::rank<std::decay_t<decltype(A3)>>{} == 2 and num_elements(A3) == 6 );
		{
			auto x = extensions(A3);
			for(auto i : std::get<0>(x)){
				for(auto j : std::get<1>(x)) cout<< A3[i][j] <<' ';
				cout<<'\n';
			}
		}
	}
	return 0;
}
#endif
#endif

