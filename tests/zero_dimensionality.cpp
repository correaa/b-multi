#ifdef COMPILATION_INSTRUCTIONS
clang++ -Wno-deprecated-declarations -Wall -Wextra -Wfatal-errors $0 -o$0x -lboost_unit_test_framework -lcudart&&$0x&&rm $0x;exit
#endif

#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE "C++ Unit Tests for Multi legacy adaptor example"
#include<boost/test/unit_test.hpp>

#include<iostream>

#include "../array.hpp"

#include "../adaptors/cuda.hpp"

#include<iostream>
#include<vector>
#include<complex>

namespace multi = boost::multi;

BOOST_AUTO_TEST_CASE(zero_dimensionality){
	{
		std::vector<double> v1 = {1., 2., 3.};

		multi::array_ref<double, 1> m1(v1.data(), 3);
		BOOST_REQUIRE( size(m1) == 3 );
		BOOST_REQUIRE( &m1[1] == &v1[1] );
		BOOST_REQUIRE( num_elements(m1) == 3 );

		multi::array_ref<double, 0> m0(v1.data());
		BOOST_REQUIRE( &m0 == v1.data() );
		BOOST_REQUIRE( data_elements(m0) == v1.data() );
		BOOST_REQUIRE( num_elements(m0) == 1 );

		m0 = 5.1;
		BOOST_REQUIRE( v1[0] == 5.1 );

		double d = m0;
		BOOST_REQUIRE( d == 5.1 );
	}
	{
		multi::static_array<double, 0> a0 = 45.;
		BOOST_REQUIRE( num_elements(a0) == 1 );
		BOOST_REQUIRE( a0 == 45. );

		a0 = 60.;
	//	BOOST_REQUIRE( a0 == 60. );
	}
	{
		std::allocator<double> alloc;
		multi::static_array<double, 0> a0(45., alloc);
		BOOST_REQUIRE( num_elements(a0) == 1 );
		BOOST_REQUIRE( a0 == 45. );

		a0 = 60.;
		BOOST_REQUIRE( a0 == 60. );
	}
	{
		multi::array<std::complex<double>, 2> a({1, 2}, std::allocator<std::complex<double>>{});
		assert( size(a) == 1 );
	}
	{
		multi::array<std::complex<double>, 0> a = std::complex<double>{1., 2.};
		assert( num_elements(a) == 1 );
	}
	{
		multi::cuda::managed::array<double, 0> a0 = 45.;
		BOOST_REQUIRE( a0 == 45. );
	}
}

