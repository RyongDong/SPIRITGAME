#include <boost/test/unit_test.hpp>
#include <limits>

#include "bignum.h"
#include "util.h"

BOOST_AUTO_TEST_SUITE(bignum_tests)

// Unfortunately there's no standard way of preventing a function from being
// inlined, so we define a macro for it.
//
// You should use it like this:
//   NOINLINE void function() {...}
#if defined(__GNUC__)
// This also works and will be defined for any compiler implementing gcc
// extensions, such as clang and icc.
#define NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
// We give out a warning because it impacts the correctness of one bignum test.
#warning You should define NOINLINE for your compiler.
#define NOINLINE
#endif

// For the following test case, it is useful to use additional tools.
//
// The simplest one to use is the compiler flag -ftrapv, which detects integer
// overflows and similar errors. However, due to optimizations and compilers
// taking advantage of undefined behavior sometimes it may not actually detect
// anything.
//
// You can also use compiler-based stack protection to possibly detect possible
// stack buffer overruns.
//
// For more accurate diagnostics, you can use an undefined arithmetic operation
// detector such as the clang-based tool:
//
// "IOC: An Integer Overflow Checker for C/C++"
//
// Available at: http://embed.cs.utah.edu/ioc/
//
// It might also be useful to use Google's AddressSanitizer to detect
// stack buffer overruns, which valgrind can't currently detect.

// Let's force this code not to be inlined, in order to actually
// test a generic version of the function. This increases the chance
// that -ftrapv will detect overflows.
NOINLINE void mysetint64(CBigNum& num, int64 n)
{
	num.setint64(n);
}

// For each number, we do 2 tests: one with inline code, then we reset the
// value to 0, then the second one with a non-inlined functio