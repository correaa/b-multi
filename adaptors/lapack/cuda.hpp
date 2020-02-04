#ifdef COMPILATION_INSTRUCTIONS
(echo '#include"'$0'"'>$0.cpp)&&$CXX -Wall -Wextra -Wpedantic -Wfatal-errors -D_TEST_MULTI_ADAPTORS_LAPACK_CUDA $0.cpp -o $0x `pkg-config --libs blas` -lcudart -lcublas -lcusolver&&$0x&&rm $0x $0.cpp; exit
#endif
// © Alfredo A. Correa 2020

#ifndef MULTI_ADAPTORS_LAPACK_CUDA_HPP
#define MULTI_ADAPTORS_LAPACK_CUDA_HPP

#include "../../memory/adaptors/cuda/ptr.hpp"
#include "../../memory/adaptors/cuda/managed/ptr.hpp"
#include "../../memory/adaptors/cuda/managed/allocator.hpp"

#include "../../adaptors/cuda.hpp"

//#include<cublas_v2.h>
#include <cusolverDn.h>

//#include<iostream> // debug

#include <boost/log/trivial.hpp>

#include<complex>
#include<memory>

#include "../blas/filling.hpp"

namespace boost{
namespace multi{

namespace cusolver{

struct version_t{
	int major = -1, minor =-1, patch=-1;
	friend std::ostream& operator<<(std::ostream& os, version_t const& self){
		return os<< self.major <<'.'<< self.minor <<'.'<<self.patch <<'\n';
	}
};

auto version(){
	version_t ret;
	cusolverGetProperty(MAJOR_VERSION, &ret.major);
	cusolverGetProperty(MINOR_VERSION, &ret.minor);
	cusolverGetProperty(PATCH_LEVEL, &ret.patch);
	return ret;
}

namespace dense{

using blas::filling;

struct context : std::unique_ptr<std::decay_t<decltype(*cusolverDnHandle_t{})>, decltype(&cusolverDnDestroy)>{
	context()  : std::unique_ptr<std::decay_t<decltype(*cusolverDnHandle_t{})>, decltype(&cusolverDnDestroy)>{
		[]{
			cusolverDnHandle_t h; 
			auto s=cusolverDnCreate(&h); assert(CUSOLVER_STATUS_SUCCESS==s and h);
			return h;
		}(), &cusolverDnDestroy
	}{}
	template<class A> int potrf_buffer_size(filling uplo, A const& a);
};

template<typename T>
struct cusolverDn;//{
//	template<class... ArgsA3, class... ArgsB2>
//	static auto potrf_bufferSize(ArgsA3... argsa3, T* ptr, ArgsB2... argsb2);
//};

template<>
struct cusolverDn<float>{
	template<class... A3, class... B2>
	static auto potrf_bufferSize(A3... a3, double* ptr, B2... b2)
	->decltype(cusolverDnSpotrf_bufferSize(a3..., ptr, b2...)){
		return cusolverDnSpotrf_bufferSize(a3..., ptr, b2...);}
};

template<>
struct cusolverDn<double>{
	template<class... A3, class... B2>
	static auto potrf_bufferSize(A3... a3, double* ptr, B2... b2)
	->decltype(cusolverDnDpotrf_bufferSize(a3..., ptr, b2...)){
		return cusolverDnDpotrf_bufferSize(a3..., ptr, b2...);}
};

template<>
struct cusolverDn<std::complex<float>>{
	template<class... A3, class... B2>
	static auto potrf_bufferSize(A3... a3, std::complex<double>* ptr, B2... b2)
	->decltype(cusolverDnCpotrf_bufferSize(a3..., reinterpret_cast<cuComplex*>(ptr), b2...)){
		return cusolverDnCpotrf_bufferSize(a3..., reinterpret_cast<cuComplex*>(ptr), b2...);}
};

template<>
struct cusolverDn<std::complex<double>>{
	static auto translate(std::complex<double>* p){return reinterpret_cast<cuDoubleComplex*>(p);}
	template<class T> static auto translate(T t){return t;}
	template<class... A> static auto potrf_bufferSize(A... a)
	->decltype(cusolverDnZpotrf_bufferSize(translate(a)...)){
		return cusolverDnZpotrf_bufferSize(translate(a)...);}
	template<class... A> static auto potrf(A... a)
	->decltype(cusolverDnZpotrf(translate(a)...)){
		return cusolverDnZpotrf(translate(a)...);}
};

}
}

//namespace blas{
#if 0
class cublas_context{
protected:
	cublasHandle_t h_;
public:
	cublas_context(){
		cublasStatus_t s = cublasCreate(&h_); assert(s==CUBLAS_STATUS_SUCCESS);
	}
	int version() const{
		int ret;
		cublasStatus_t s = cublasGetVersion(h_, &ret); assert(s==CUBLAS_STATUS_SUCCESS);
		return ret;
	}
	~cublas_context() noexcept{cublasDestroy(h_);}
	//set_stream https://docs.nvidia.com/cuda/cublas/index.html#cublassetstream
	//get_stream https://docs.nvidia.com/cuda/cublas/index.html#cublasgetstream
	//get_pointer_mode https://docs.nvidia.com/cuda/cublas/index.html#cublasgetpointermode
	//set_pointer_mode https://docs.nvidia.com/cuda/cublas/index.html#cublasgetpointermode
};

template<class T> class cublas;

template<>
class cublas<double> : cublas_context{
public:
	template<class... Args>
	static auto gemm(Args... args){return cublasDgemm(args...);}
	template<class... Args>
	static auto scal(Args... args){return cublasDscal(args...);}
	template<class... Args>
	static auto syrk(Args&&... args){return cublasDsyrk(args...);}
	template<class... As> void copy (As... as){auto s=cublasDcopy (h_, as...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void iamax(As... as){auto s=cublasIdamax(h_, as...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void asum(As... as){auto s=cublasDasum(h_, as...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void trsm(As... as){auto s=cublasDtrsm(h_, as...); assert(s==CUBLAS_STATUS_SUCCESS);}
};

template<>
class cublas<float> : cublas_context{
public:
	template<class... Args>
	static auto gemm(Args... args){return cublasSgemm(args...);}
	template<class... Args>
	static auto scal(Args... args){return cublasSscal(args...);}
	template<class... Args>
	static auto syrk(Args&&... args){return cublasSsyrk(args...);}
	template<class... As> void copy (As... as){auto s=cublasScopy (h_, as...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void iamax(As... as){auto s=cublasIsamax(h_, as...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void asum(As... as){auto s=cublasSasum(h_, as...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void trsm(As... as){auto s=cublasStrsm(h_, as...); assert(s==CUBLAS_STATUS_SUCCESS);}
};

template<>
class cublas<std::complex<double>> : cublas_context{
	static_assert(sizeof(std::complex<double>)==sizeof(cuDoubleComplex), "!");
	template<class T> static decltype(auto) to_cu(T&& t){return std::forward<T>(t);}
	static decltype(auto) to_cu(std::complex<double> const* t){return reinterpret_cast<cuDoubleComplex const*>(t);}	
	static decltype(auto) to_cu(std::complex<double>* t){return reinterpret_cast<cuDoubleComplex*>(t);}	
public:
	template<class... Args>
	static auto gemm(Args&&... args){return cublasZgemm(to_cu(std::forward<Args>(args))...);}
	template<class... Args>
	static auto herk(Args&&... args){return cublasZherk(to_cu(std::forward<Args>(args))...);}
	template<class... Args>
	static auto scal(Args&&... args)
	->decltype(cublasZscal(to_cu(std::forward<Args>(args))...)){
		return cublasZscal(to_cu(std::forward<Args>(args))...);}
	template<class Handle, class Size, class... Args2>
	static auto scal(Handle h, Size s, double* alpha, Args2&&... args2){
		return cublasZdscal(h, s, alpha, to_cu(std::forward<Args2>(args2))...);
	}
	template<class... As> void copy (As... as){auto s=cublasZcopy (h_, to_cu(as)...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void iamax(As... as){auto s=cublasIzamax(h_, to_cu(as)...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void asum(As... as){auto s=cublasDzasum(h_, to_cu(as)...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void trsm(As... as){auto s=cublasZtrsm(h_, to_cu(as)...); assert(s==CUBLAS_STATUS_SUCCESS);}
};

template<>
class cublas<std::complex<float>> : cublas_context{
	static_assert(sizeof(std::complex<float>)==sizeof(cuComplex), "!");
	template<class T> static decltype(auto) to_cu(T&& t){return std::forward<T>(t);}
	static decltype(auto) to_cu(std::complex<float> const* t){return reinterpret_cast<cuComplex const*>(t);}	
	static decltype(auto) to_cu(std::complex<float>* t){return reinterpret_cast<cuComplex*>(t);}	
public:
	template<class... Args>
	static auto gemm(Args&&... args){return cublasCgemm(to_cu(std::forward<Args>(args))...);}
	template<class... Args>
	static auto herk(Args&&... args){return cublasCherk(to_cu(std::forward<Args>(args))...);}
	template<class... Args>
	static auto scal(Args&&... args)
	->decltype(cublasZscal(to_cu(std::forward<Args>(args))...)){
		return cublasZscal(to_cu(std::forward<Args>(args))...);}
	template<class Handle, class Size, class... Args2>
	static auto scal(Handle h, Size s, float* alpha, Args2&&... args2){
		return cublasZdscal(h, s, alpha, to_cu(std::forward<Args2>(args2))...);
	}
	template<class... As> void copy (As... as){auto s=cublasCcopy (h_, to_cu(as)...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void iamax(As... as){auto s=cublasIcamax(h_, to_cu(as)...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void asum(As... as){auto s=cublasScasum(h_, to_cu(as)...); assert(s==CUBLAS_STATUS_SUCCESS);}
	template<class... As> void trsm(As... as){auto s=cublasCtrsm(h_, to_cu(as)...); assert(s==CUBLAS_STATUS_SUCCESS);}
};

namespace memory{
namespace cuda{

template<class ComplexTconst, typename S>//, typename T = typename std::decay_t<ComplexTconst>::value_type>
auto asum(S n, cuda::ptr<ComplexTconst> x, S incx){
	decltype(std::abs(ComplexTconst{})) r; cublas<std::decay_t<ComplexTconst>>{}.asum(n, static_cast<ComplexTconst*>(x), incx, &r); return r;
}

//template<class Tconst, typename S>
//auto asum(S n, cuda::ptr<Tconst> x, S incx, void* = 0){
//	std::decay_t<Tconst> r; cublas<std::decay_t<Tconst>>{}.asum(n, static_cast<Tconst*>(x), incx, &r); return r;
//s}

template<class T, typename S>
S iamax(S n, cuda::ptr<T const> x, S incx){
	int r; cublas<T>{}.iamax(n, static_cast<T const*>(x), incx, &r); return r-1;
}

template<class T, class TA, class S> 
void scal(S n, TA a, multi::memory::cuda::ptr<T> x, S incx){
	cublasHandle_t handle;
	{cublasStatus_t s = cublasCreate(&handle); assert(s==CUBLAS_STATUS_SUCCESS);}
	cublasStatus_t s = cublas<T>::scal(handle, n, &a, static_cast<T*>(x), incx);
	if(s!=CUBLAS_STATUS_SUCCESS){
		std::cerr << [&](){switch(s){
			case CUBLAS_STATUS_SUCCESS: return "CUBLAS_STATUS_SUCCESS";
			case CUBLAS_STATUS_NOT_INITIALIZED: return "CUBLAS_STATUS_NOT_INITIALIZED";
			case CUBLAS_STATUS_ALLOC_FAILED: return "CUBLAS_STATUS_ALLOC_FAILED";
			case CUBLAS_STATUS_INVALID_VALUE: return "CUBLAS_STATUS_INVALID_VALUE";
			case CUBLAS_STATUS_ARCH_MISMATCH: return "CUBLAS_STATUS_ARCH_MISMATCH";
			case CUBLAS_STATUS_MAPPING_ERROR: return "CUBLAS_STATUS_MAPPING_ERROR";
			case CUBLAS_STATUS_EXECUTION_FAILED: return "CUBLAS_STATUS_EXECUTION_FAILED";
			case CUBLAS_STATUS_INTERNAL_ERROR: return "CUBLAS_STATUS_INTERNAL_ERROR";
			case CUBLAS_STATUS_NOT_SUPPORTED: return "CUBLAS_STATUS_NOT_SUPPORTED";
			case CUBLAS_STATUS_LICENSE_ERROR: return "CUBLAS_STATUS_LICENSE_ERROR";
		} return "<unknown>";}() << std::endl;
	}
	assert( s==CUBLAS_STATUS_SUCCESS ); (void)s;
	cublasDestroy(handle);
}

template<class Tconst, class T, class S> 
void copy(S n, cuda::ptr<Tconst> x, S incx, cuda::ptr<T> y, S incy){
	cublas<T>{}.copy(n, static_cast<T const*>(x), incx, static_cast<T*>(y), incy);
}

//template<class T, class S> 
//void copy(S n, multi::memory::cuda::ptr<T const> x, S incx, multi::memory::cuda::ptr<T> y, S incy){
//}

template<class Tconst, class T, class UL, class C, class S, class Real>
void syrk(UL ul, C transA, S n, S k, Real alpha, multi::memory::cuda::ptr<Tconst> A, S lda, Real beta, multi::memory::cuda::ptr<T> CC, S ldc){
	cublasHandle_t handle;
	{cublasStatus_t s = cublasCreate(&handle); assert(s==CUBLAS_STATUS_SUCCESS);}
	cublasFillMode_t uplo = [ul](){
		switch(ul){
			case 'U': return CUBLAS_FILL_MODE_UPPER;
			case 'L': return CUBLAS_FILL_MODE_LOWER;
		} assert(0); return CUBLAS_FILL_MODE_UPPER;
	}();
	cublasOperation_t cutransA = [transA](){
		switch(transA){
			case 'N': return CUBLAS_OP_N;
			case 'T': return CUBLAS_OP_T;
			case 'C': return CUBLAS_OP_C;
		} assert(0); return CUBLAS_OP_N;
	}();
	cublasStatus_t s = cublas<T>::syrk(handle, uplo, cutransA, n, k, &alpha, static_cast<T const*>(A), lda, &beta, static_cast<T*>(CC), ldc);
	if(s!=CUBLAS_STATUS_SUCCESS){
		std::cerr << [&](){switch(s){
			case CUBLAS_STATUS_SUCCESS: return "CUBLAS_STATUS_SUCCESS";
			case CUBLAS_STATUS_NOT_INITIALIZED: return "CUBLAS_STATUS_NOT_INITIALIZED";
			case CUBLAS_STATUS_ALLOC_FAILED: return "CUBLAS_STATUS_ALLOC_FAILED";
			case CUBLAS_STATUS_INVALID_VALUE: return "CUBLAS_STATUS_INVALID_VALUE";
			case CUBLAS_STATUS_ARCH_MISMATCH: return "CUBLAS_STATUS_ARCH_MISMATCH";
			case CUBLAS_STATUS_MAPPING_ERROR: return "CUBLAS_STATUS_MAPPING_ERROR";
			case CUBLAS_STATUS_EXECUTION_FAILED: return "CUBLAS_STATUS_EXECUTION_FAILED";
			case CUBLAS_STATUS_INTERNAL_ERROR: return "CUBLAS_STATUS_INTERNAL_ERROR";
			case CUBLAS_STATUS_NOT_SUPPORTED: return "CUBLAS_STATUS_NOT_SUPPORTED";
			case CUBLAS_STATUS_LICENSE_ERROR: return "CUBLAS_STATUS_LICENSE_ERROR";
		} return "<unknown>";}() << std::endl;
	}
	assert( s==CUBLAS_STATUS_SUCCESS ); (void)s;
	cublasDestroy(handle);
}

template<class Tconst, class T, class UL, class C, class S, class Real>
void herk(UL ul, C transA, S n, S k, Real alpha, multi::memory::cuda::ptr<Tconst> A, S lda, Real beta, multi::memory::cuda::ptr<T> CC, S ldc){
	cublasHandle_t handle;
	{cublasStatus_t s = cublasCreate(&handle); assert(s==CUBLAS_STATUS_SUCCESS);}
	cublasFillMode_t uplo = [ul](){
		switch(ul){
			case 'U': return CUBLAS_FILL_MODE_UPPER;
			case 'L': return CUBLAS_FILL_MODE_LOWER;
		} assert(0); return CUBLAS_FILL_MODE_UPPER;
	}();
	cublasOperation_t cutransA = [transA](){
		switch(transA){
			case 'N': return CUBLAS_OP_N;
			case 'T': return CUBLAS_OP_T;
			case 'C': return CUBLAS_OP_C;
		} assert(0); return CUBLAS_OP_N;
	}();
	cublasStatus_t s = cublas<T>::herk(handle, uplo, cutransA, n, k, &alpha, static_cast<T const*>(A), lda, &beta, static_cast<T*>(CC), ldc);
	if(s!=CUBLAS_STATUS_SUCCESS){
		std::cerr << [&](){switch(s){
			case CUBLAS_STATUS_SUCCESS: return "CUBLAS_STATUS_SUCCESS";
			case CUBLAS_STATUS_NOT_INITIALIZED: return "CUBLAS_STATUS_NOT_INITIALIZED";
			case CUBLAS_STATUS_ALLOC_FAILED: return "CUBLAS_STATUS_ALLOC_FAILED";
			case CUBLAS_STATUS_INVALID_VALUE: return "CUBLAS_STATUS_INVALID_VALUE";
			case CUBLAS_STATUS_ARCH_MISMATCH: return "CUBLAS_STATUS_ARCH_MISMATCH";
			case CUBLAS_STATUS_MAPPING_ERROR: return "CUBLAS_STATUS_MAPPING_ERROR";
			case CUBLAS_STATUS_EXECUTION_FAILED: return "CUBLAS_STATUS_EXECUTION_FAILED";
			case CUBLAS_STATUS_INTERNAL_ERROR: return "CUBLAS_STATUS_INTERNAL_ERROR";
			case CUBLAS_STATUS_NOT_SUPPORTED: return "CUBLAS_STATUS_NOT_SUPPORTED";
			case CUBLAS_STATUS_LICENSE_ERROR: return "CUBLAS_STATUS_LICENSE_ERROR";
		} return "<unknown>";}() << std::endl;
	}
	assert( s==CUBLAS_STATUS_SUCCESS ); (void)s;
	cublasDestroy(handle);
}

template<typename TconstA, typename TconstB, typename T, typename AA, typename BB, class C, typename S>
void gemm(C transA, C transB, S m, S n, S k, AA a, multi::memory::cuda::ptr<TconstA> A, S lda, multi::memory::cuda::ptr<TconstB> B, S ldb, BB beta, multi::memory::cuda::ptr<T> CC, S ldc){
	cublasHandle_t handle;
	{cublasStatus_t s = cublasCreate(&handle); assert(s==CUBLAS_STATUS_SUCCESS);}
	cublasOperation_t cutransA = [transA](){
		switch(transA){
			case 'N': return CUBLAS_OP_N;
			case 'T': return CUBLAS_OP_T;
			case 'C': return CUBLAS_OP_C;
		} assert(0); return CUBLAS_OP_N;
	}();
	cublasOperation_t cutransB = [transB](){
		switch(transB){
			case 'N': return CUBLAS_OP_N;
			case 'T': return CUBLAS_OP_T;
			case 'C': return CUBLAS_OP_C;
		} assert(0); return CUBLAS_OP_N;
	}();
	T Talpha{a};
	T Tbeta{beta};
	cublasStatus_t s = cublas<T>::gemm(handle, cutransA, cutransB, m, n, k, &Talpha, static_cast<T const*>(A), lda, static_cast<T const*>(B), ldb, &Tbeta, static_cast<T*>(CC), ldc);
	if(s!=CUBLAS_STATUS_SUCCESS){
		std::cerr << [&](){switch(s){
			case CUBLAS_STATUS_SUCCESS: return "CUBLAS_STATUS_SUCCESS";
			case CUBLAS_STATUS_NOT_INITIALIZED: return "CUBLAS_STATUS_NOT_INITIALIZED";
			case CUBLAS_STATUS_ALLOC_FAILED: return "CUBLAS_STATUS_ALLOC_FAILED";
			case CUBLAS_STATUS_INVALID_VALUE: return "CUBLAS_STATUS_INVALID_VALUE";
			case CUBLAS_STATUS_ARCH_MISMATCH: return "CUBLAS_STATUS_ARCH_MISMATCH";
			case CUBLAS_STATUS_MAPPING_ERROR: return "CUBLAS_STATUS_MAPPING_ERROR";
			case CUBLAS_STATUS_EXECUTION_FAILED: return "CUBLAS_STATUS_EXECUTION_FAILED";
			case CUBLAS_STATUS_INTERNAL_ERROR: return "CUBLAS_STATUS_INTERNAL_ERROR";
			case CUBLAS_STATUS_NOT_SUPPORTED: return "CUBLAS_STATUS_NOT_SUPPORTED";
			case CUBLAS_STATUS_LICENSE_ERROR: return "CUBLAS_STATUS_LICENSE_ERROR";
		} return "<unknown>";}() << std::endl;
	}
	assert( s==CUBLAS_STATUS_SUCCESS ); (void)s;
	cublasDestroy(handle);
}

template<class Side, class Fill, class Trans, class Diag, typename Size, class Tconst, class T>
void trsm(Side /*cublasSideMode_t*/ side, /*cublasFillMode_t*/ Fill uplo, /*cublasOperation_t*/ Trans trans, /*cublasDiagType_t*/ Diag diag,
                           Size m, Size n, T alpha, cuda::ptr<Tconst> A, Size lda, cuda::ptr<T> B, Size ldb){
	cublasOperation_t trans_cu = [&]{
		switch(trans){
			case 'N': return CUBLAS_OP_N;
			case 'T': return CUBLAS_OP_T;
			case 'C': return CUBLAS_OP_C;
		} __builtin_unreachable();
	}();
	cublas<T>{}.trsm(
		side=='L'?CUBLAS_SIDE_LEFT:CUBLAS_SIDE_RIGHT, uplo=='L'?CUBLAS_FILL_MODE_LOWER:CUBLAS_FILL_MODE_UPPER, trans_cu, diag=='N'?CUBLAS_DIAG_NON_UNIT:CUBLAS_DIAG_UNIT, m, n, &alpha, static_cast<Tconst*>(A), lda, static_cast<T*>(B), ldb);
}

}}}}

namespace boost{namespace multi{namespace memory{namespace cuda{namespace managed{//namespace boost::multi::memory::cuda::managed{

template<class Tconst, typename S>
auto asum(S n, cuda::managed::ptr<Tconst> x, S incx){
	return asum(n, cuda::ptr<Tconst>(x), incx);
}

template<class T, typename S>
S iamax(S n, cuda::managed::ptr<T const> x, S incx){
	return cuda::iamax(n, cuda::ptr<T const>(x), incx);
}

template<class T, class TA, class S> 
void scal(S n, TA a, multi::memory::cuda::managed::ptr<T> x, S incx){
	scal(n, a, multi::memory::cuda::ptr<T>(x), incx);
}

template<typename AA, typename BB, class S, class TconstA, class TconstB, class T>
void gemm(char transA, char transB, S m, S n, S k, AA const& a, multi::memory::cuda::managed::ptr<TconstA> A, S lda, multi::memory::cuda::managed::ptr<TconstB> B, S ldb, BB const& beta, multi::memory::cuda::managed::ptr<T> CC, S ldc){
	gemm(transA, transB, m, n, k, a, boost::multi::memory::cuda::ptr<TconstA>(A), lda, boost::multi::memory::cuda::ptr<TconstB>(B), ldb, beta, boost::multi::memory::cuda::ptr<T>(CC), ldc);
}

template<class Tconst, class T, class UL, class C, class S, class Real>
void herk(UL ul, C transA, S n, S k, Real alpha, multi::memory::cuda::managed::ptr<Tconst> A, S lda, Real beta, multi::memory::cuda::managed::ptr<T> CC, S ldc){
	herk(ul, transA, n, k, alpha, boost::multi::memory::cuda::ptr<Tconst>(A), lda, beta, boost::multi::memory::cuda::ptr<T>(CC), ldc);
}

template<class Tconst, class T, class UL, class C, class S, class Real>
void syrk(UL ul, C transA, S n, S k, Real alpha, multi::memory::cuda::managed::ptr<Tconst> A, S lda, Real beta, multi::memory::cuda::managed::ptr<T> CC, S ldc){
	syrk(ul, transA, n, k, alpha, boost::multi::memory::cuda::ptr<Tconst>(A), lda, beta, boost::multi::memory::cuda::ptr<T>(CC), ldc);
}

template<class Side, class Fill, class Trans, class Diag, typename Size, class Tconst, class T>
void trsm(Side /*cublasSideMode_t*/ side, /*cublasFillMode_t*/ Fill uplo, /*cublasOperation_t*/ Trans trans, /*cublasDiagType_t*/ Diag diag,
                           Size m, Size n, T alpha, cuda::managed::ptr<Tconst> A, Size lda, cuda::managed::ptr<T> B, Size ldb){
	return trsm(side, uplo, trans, diag, m, n, alpha, cuda::ptr<Tconst>(A), lda, cuda::ptr<T>(B), ldb);
}

}}}
#endif

namespace memory{
namespace cuda{

template<class UL, class S, class PtrT, typename T = typename std::pointer_traits<PtrT>::element_type>
void potrf(UL ul, S n, PtrT A, S incx, int& info){
	boost::multi::cusolver::dense::context ctx; BOOST_LOG_TRIVIAL(trace)<<"cuda::potrf called on size/stride "<< n <<' '<< incx <<'\n';
	int lwork = -1;
	{
		auto s = cusolver::dense::cusolverDn<T>::potrf_bufferSize(ctx.get(), ul=='U'?CUBLAS_FILL_MODE_UPPER:CUBLAS_FILL_MODE_LOWER, n, raw_pointer_cast(A), incx, &lwork);
		assert(s == CUSOLVER_STATUS_SUCCESS); assert(lwork >= 0);
	}
	multi::cuda::array<T, 1> work(lwork);
	multi::cuda::static_array<int, 0> devInfo;
	auto s = cusolver::dense::cusolverDn<T>::potrf(ctx.get(), ul=='U'?CUBLAS_FILL_MODE_UPPER:CUBLAS_FILL_MODE_LOWER, n, raw_pointer_cast(A), incx, raw_pointer_cast(base(work)), lwork, raw_pointer_cast(base(devInfo)) );
	assert(s == CUSOLVER_STATUS_SUCCESS);
	cudaDeviceSynchronize();
	info = devInfo();
}

namespace managed{
	template<class UL, class S, class PtrT, typename T = typename std::pointer_traits<PtrT>::element_type>
	auto potrf(UL ul, S n, PtrT A, S incx, int& info)
	->decltype(cuda::potrf(ul, n, cuda::ptr<T>(A), incx, info)){
		return cuda::potrf(ul, n, cuda::ptr<T>(A), incx, info);}
}

}
}

}}

///////////////////////////////////////////////////////////////////////////////

#if _TEST_MULTI_ADAPTORS_LAPACK_CUDA

#include "../../array.hpp"
#include "../../utility.hpp"
#include<cassert>

namespace multi = boost::multi;

int main(){
	std::cout << "cusolver version " << multi::cusolver::version() << std::endl;
	multi::cusolver::dense::context c;
//	multi::cublas_context c;
//	assert( c.version() >= 10100 );
}

#endif
#endif
