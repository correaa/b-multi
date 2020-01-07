#ifdef COMPILATION_INSTRUCTIONS
$CXX -std=c++17 -O3 -Wall -Wextra -Wfatal-errors $0 -lboost_unit_test_framework -o $0x -lstdc++fs -lboost_serialization -lboost_iostreams -lcudart&&$0x $@&&rm $0x;exit
#endif

#define BOOST_TEST_MODULE "C++ Unit Tests for Multi fill"
#define BOOST_TEST_DYN_LINK
#include<boost/test/unit_test.hpp>

#include "../array.hpp"

#include "../adaptors/cuda.hpp"

#include<boost/archive/xml_oarchive.hpp>
#include<boost/archive/xml_iarchive.hpp>
#include<boost/archive/text_oarchive.hpp>
#include<boost/archive/text_iarchive.hpp>
#include<boost/archive/binary_oarchive.hpp>
#include<boost/archive/binary_iarchive.hpp>

#include<boost/serialization/nvp.hpp>
#include<boost/serialization/complex.hpp>

#include<boost/iostreams/filtering_stream.hpp>
#include<boost/iostreams/filter/gzip.hpp>

#include<fstream>
#include<filesystem>

#include<chrono>
#include<iostream>
#include<random>

namespace multi = boost::multi;

struct watch{
	std::string name_;
	decltype(std::chrono::high_resolution_clock::now()) start_;
	watch(std::string name = "") : name_(name), start_(std::chrono::high_resolution_clock::now()){}
	~watch(){
		std::cerr<< name_ <<": "<< std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_).count() <<" sec"<<std::endl;
	}
};

BOOST_AUTO_TEST_CASE(multi_serialization){

	{
		multi::static_array<double, 2> d2D({10, 10});
		auto gen = [d = std::uniform_real_distribution<double>{-1, 1}, e = std::mt19937{std::random_device{}()}]() mutable{return d(e);};
		std::for_each(
			begin(d2D), end(d2D), 
			[&](auto&& r){std::generate(begin(r), end(r), gen);}
		);
		std::ofstream ofs{"serialization-static.xml"}; assert(ofs);
		boost::archive::xml_oarchive{ofs} << BOOST_SERIALIZATION_NVP(d2D);
	}
	{
		multi::static_array<double, 0> d0D = 12.;
		std::ofstream ofs{"serialization-static_0D.xml"}; assert(ofs);
		boost::archive::xml_oarchive{ofs} << BOOST_SERIALIZATION_NVP(d0D);
	}

	using complex = std::complex<float>;

	multi::array<complex, 2> d2D = {
		{150., 16., 17., 18., 19.}, 
		{  5.,  5.,  5.,  5.,  5.}, 
		{100., 11., 12., 13., 14.}, 
		{ 50.,  6.,  7.,  8.,  9.}  
	};
	d2D.reextent({2000, 2000});

	auto gen = [d = std::uniform_real_distribution<double>{-1, 1}, e = std::mt19937{std::random_device{}()}]() mutable{return std::complex{d(e), d(e)};};
	std::for_each(
		begin(d2D), end(d2D), 
		[&](auto&& r){std::generate(begin(r), end(r), gen);}
	);
	{
		multi::cuda::managed::array<complex, 2> cud2D({2000, 2000});
		[&, _=watch("cuda binary write")]{
			std::ofstream ofs{"serialization.bin"}; assert(ofs);
			boost::archive::binary_oarchive{ofs} << cud2D;
		}();
		std::cerr<<"size "<< (std::filesystem::file_size("serialization.bin")/1e6) <<"MB\n";
	}
	{
		[&, _ = watch("text write")]{
			std::ofstream ofs{"serialization.txt"}; assert(ofs);
			boost::archive::text_oarchive{ofs} << d2D;
		}();
		std::cerr<<"size "<< (std::filesystem::file_size("serialization.txt")/1e6) <<"MB\n";
	}
	{
		multi::array<complex, 2> d2D_copy;//(extensions(d2D), 9999.);
		[&, _ = watch("text read")]{
			std::ifstream ifs{"serialization.txt"}; assert(ifs);
			boost::archive::text_iarchive{ifs} >> d2D_copy;
		}();
		BOOST_REQUIRE( d2D_copy == d2D );
	}
	{
		[&, _=watch("binary write")]{
			std::ofstream ofs{"serialization.bin"}; assert(ofs);
			boost::archive::binary_oarchive{ofs} << d2D;
		}();
		std::cerr<<"size "<< (std::filesystem::file_size("serialization.bin")/1e6) <<"MB\n";
	}
	{
		multi::array<complex, 2> d2D_copy;//(extensions(d2D), 9999.);
		[&, _=watch("binary read")]{
			std::ifstream ifs{"serialization.bin"}; assert(ifs);
			boost::archive::binary_iarchive{ifs} >> d2D_copy;
		}();
		BOOST_REQUIRE( d2D_copy == d2D );
	}
	{
		[&, _=watch("binary compressed write")]{
			std::ofstream ofs{"serialization_compressed.bin.gz"};
			{
				boost::iostreams::filtering_stream<boost::iostreams::output> f;
				f.push(boost::iostreams::gzip_compressor());
				f.push(ofs);
				boost::archive::binary_oarchive{f} << d2D;
			}
		}();
		std::cerr<<"size "<< (std::filesystem::file_size("serialization.bin.gz")/1e6) <<"MB\n";
	}
	{
		[&, _ = watch("xml write")]{
			std::ofstream ofs{"serialization.xml"}; assert(ofs);
			boost::archive::xml_oarchive{ofs} << BOOST_SERIALIZATION_NVP(d2D);
		}();
		std::cerr<<"size "<< (std::filesystem::file_size("serialization.xml")/1e6) <<"MB\n";
	}
	{
		[&, _ = watch("compressed xml write")]{
			std::ofstream ofs{"serialization.xml.gz"}; assert(ofs);
			{
				boost::iostreams::filtering_stream<boost::iostreams::output> f;
				f.push(boost::iostreams::gzip_compressor());
				f.push(ofs);
				boost::archive::xml_oarchive{f} << BOOST_SERIALIZATION_NVP(d2D);
			}
		}();
		std::cerr<<"size "<< (std::filesystem::file_size("serialization.xml.gz")/1e6) <<"MB\n";
	}
	{
		multi::array<complex, 2> d2D_copy;//(extensions(d2D), 9999.);
		[&, _ = watch("xml read")]{
			std::ifstream ifs{"serialization.xml"}; assert(ifs);
			boost::archive::xml_iarchive{ifs} >> BOOST_SERIALIZATION_NVP(d2D_copy);
		}();
		BOOST_REQUIRE( d2D_copy == d2D );
	}
}
