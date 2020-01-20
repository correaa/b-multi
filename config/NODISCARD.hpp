#ifdef COMPILATION_INSTRUCTIONS
(echo '#include"'$0'"'>$0.cpp)&&$CXX -D_TEST_MULTI_CONFIG_NODISCARD $0.cpp -o $0x &&$0x&&rm $0x $0.cpp;exit
#endif
// © Alfredo A. Correa 2019-2020

#ifndef MULTI_CONFIG_NODISCARD_HPP
#define MULTI_CONFIG_NODISCARD_HPP

#ifndef __has_cpp_attribute
#define __has_cpp_attribute(name) 0
#endif

#if (__has_cpp_attribute(nodiscard)) && (__cplusplus>=201703L)
	#define nodiscard_(MsG) [[nodiscard]]
	#define NODISCARD(MsG) [[nodiscard_(MsG)]]
	#if (__has_cpp_attribute(nodiscard)>=201907) && (__cplusplus>=201703L)
		#define nodiscard_(MsG) nodiscard(MsG)
		#define NODISCARD(MsG) [[nodiscard_(MsG)]]
	#endif
#elif __has_cpp_attribute(gnu::warn_unused_result)
	#define nodiscard_(MsG) gnu::warn_unused_result
	#define NODISCARD(MsG) [[nodiscard_(MsG)]]
#else
	#define nodiscard_(MsG)
	#define NODISCARD(MsG)
#endif

#ifdef _TEST_MULTI_CONFIG_NODISCARD

NODISCARD("because...") int f(){return 5;}
//[[nodiscard]] int g(){return 5;} // ok in g++ -std=c++14

int main(){
	int i; 
	i = f(); // ok
//	f();  // warning
	(void)i;
}
#endif
#endif


