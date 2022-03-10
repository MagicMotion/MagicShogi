/*******************************************************************************
 * Copyright (c) 2008-2016 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
 * KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
 * SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
 *    https://www.khronos.org/registry/
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 ******************************************************************************/

/*! \file
 *
 *   \brief C++ bindings for OpenCL 1.0 (rev 48), OpenCL 1.1 (rev 33),
 *       OpenCL 1.2 (rev 15) and OpenCL 2.0 (rev 29)
 *   \author Lee Howes and Bruce Merry
 *
 *   Derived from the OpenCL 1.x C++ bindings written by
 *   Benedict R. Gaster, Laurent Morichetti and Lee Howes
 *   With additions and fixes from:
 *       Brian Cole, March 3rd 2010 and April 2012
 *       Matt Gruenke, April 2012.
 *       Bruce Merry, February 2013.
 *       Tom Deakin and Simon McIntosh-Smith, July 2013
 *       James Price, 2015-
 *
 *   \version 2.0.10
 *   \date 2016-07-20
 *
 *   Optional extension support
 *
 *         cl_ext_device_fission
 *         #define CL_HPP_USE_CL_DEVICE_FISSION
 *         cl_khr_d3d10_sharing
 *         #define CL_HPP_USE_DX_INTEROP
 *         cl_khr_sub_groups
 *         #define CL_HPP_USE_CL_SUB_GROUPS_KHR
 *         cl_khr_image2d_from_buffer
 *         #define CL_HPP_USE_CL_IMAGE2D_FROM_BUFFER_KHR
 *
 *   Doxygen documentation for this header is available here:
 *
 *       http://khronosgroup.github.io/OpenCL-CLHPP/
 *
 *   The latest version of this header can be found on the GitHub releases page:
 *
 *       https://github.com/KhronosGroup/OpenCL-CLHPP/releases
 *
 *   Bugs and patches can be submitted to the GitHub repository:
 *
 *       https://github.com/KhronosGroup/OpenCL-CLHPP
 */

/*! \mainpage
 * \section intro Introduction
 * For many large applications C++ is the language of choice and so it seems
 * reasonable to define C++ bindings for OpenCL.
 *
 * The interface is contained with a single C++ header file \em cl2.hpp and all
 * definitions are contained within the namespace \em cl. There is no additional
 * requirement to include \em cl.h and to use either the C++ or original C
 * bindings; it is enough to simply include \em cl2.hpp.
 *
 * The bindings themselves are lightweight and correspond closely to the
 * underlying C API. Using the C++ bindings introduces no additional execution
 * overhead.
 *
 * There are numerous compatibility, portability and memory management
 * fixes in the new header as well as additional OpenCL 2.0 features.
 * As a result the header is not directly backward compatible and for this
 * reason we release it as cl2.hpp rather than a new version of cl.hpp.
 * 
 *
 * \section compatibility Compatibility
 * Due to the evolution of the underlying OpenCL API the 2.0 C++ bindings
 * include an updated approach to defining supported feature versions
 * and the range of valid underlying OpenCL runtime versions supported.
 *
 * The combination of preprocessor macros CL_HPP_TARGET_OPENCL_VERSION and 
 * CL_HPP_MINIMUM_OPENCL_VERSION control this range. These are three digit
 * decimal values representing OpenCL runime versions. The default for 
 * the target is 200, representing OpenCL 2.0 and the minimum is also 
 * defined as 200. These settings would use 2.0 API calls only.
 * If backward compatibility with a 1.2 runtime is required, the minimum
 * version may be set to 120.
 *
 * Note that this is a compile-time setting, and so affects linking against
 * a particular SDK version rather than the versioning of the loaded runtime.
 *
 * The earlier versions of the header included basic vector and string 
 * classes based loosely on STL versions. These were difficult to 
 * maintain and very rarely used. For the 2.0 header we now assume
 * the presence of the standard library unless requested otherwise.
 * We use std::array, std::vector, std::shared_ptr and std::string 
 * throughout to safely manage memory and reduce the chance of a 
 * recurrance of earlier memory management bugs.
 *
 * These classes are used through typedefs in the cl namespace: 
 * cl::array, cl::vector, cl::pointer and cl::string.
 * In addition cl::allocate_pointer forwards to std::allocate_shared
 * by default.
 * In all cases these standard library classes can be replaced with 
 * custom interface-compatible versions using the CL_HPP_NO_STD_ARRAY, 
 * CL_HPP_NO_STD_VECTOR, CL_HPP_NO_STD_UNIQUE_PTR and 
 * CL_HPP_NO_STD_STRING macros.
 *
 * The OpenCL 1.x versions of the C++ bindings included a size_t wrapper
 * class to interface with kernel enqueue. This caused unpleasant interactions
 * with the standard size_t declaration and led to namespacing bugs.
 * In the 2.0 version we have replaced this with a std::array-based interface.
 * However, the old behaviour can be regained for backward compatibility
 * using the CL_HPP_ENABLE_SIZE_T_COMPATIBILITY macro.
 *
 * Finally, the program construction interface used a clumsy vector-of-pairs
 * design in the earlier versions. We have replaced that with a cleaner 
 * vector-of-vectors and vector-of-strings design. However, for backward 
 * compatibility old behaviour can be regained with the
 * CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY macro.
 * 
 * In OpenCL 2.0 OpenCL C is not entirely backward compatibility with 
 * earlier versions. As a result a flag must be passed to the OpenCL C
 * compiled to request OpenCL 2.0 compilation of kernels with 1.2 as
 * the default in the absence of the flag.
 * In some cases the C++ bindings automatically compile code for ease.
 * For those cases the compilation defaults to OpenCL C 2.0.
 * If this is not wanted, the CL_HPP_CL_1_2_DEFAULT_BUILD macro may
 * be specified to assume 1.2 compilation.
 * If more fine-grained decisions on a per-kernel bases are required
 * then explicit build operations that take the flag should be used.
 *
 *
 * \section parameterization Parameters
 * This header may be parameterized by a set of preprocessor macros.
 *
 * - CL_HPP_TARGET_OPENCL_VERSION
 *
 *   Defines the target OpenCL runtime version to build the header
 *   against. Defaults to 200, representing OpenCL 2.0.
 *
 * - CL_HPP_NO_STD_STRING
 *
 *   Do not use the standard library string class. cl::string is not
 *   defined and may be defined by the user before cl2.hpp is
 *   included.
 *
 * - CL_HPP_NO_STD_VECTOR
 *
 *   Do not use the standard library vector class. cl::vector is not
 *   defined and may be defined by the user before cl2.hpp is
 *   included.
 *
 * - CL_HPP_NO_STD_ARRAY
 *
 *   Do not use the standard library array class. cl::array is not
 *   defined and may be defined by the user before cl2.hpp is
 *   included.
 *
 * - CL_HPP_NO_STD_UNIQUE_PTR
 *
 *   Do not use the standard library unique_ptr class. cl::pointer and
 *   the cl::allocate_pointer functions are not defined and may be
 *   defined by the user before cl2.hpp is included.
 *
 * - CL_HPP_ENABLE_DEVICE_FISSION
 *
 *   Enables device fission for OpenCL 1.2 platforms.
 *
 * - CL_HPP_ENABLE_EXCEPTIONS
 *
 *   Enable exceptions for use in the C++ bindings header. This is the
 *   preferred error handling mechanism but is not required.
 *
 * - CL_HPP_ENABLE_SIZE_T_COMPATIBILITY
 *
 *   Backward compatibility option to support cl.hpp-style size_t
 *   class.  Replaces the updated std::array derived version and
 *   removal of size_t from the namespace. Note that in this case the
 *   new size_t class is placed in the cl::compatibility namespace and
 *   thus requires an additional using declaration for direct backward
 *   compatibility.
 *
 * - CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY
 *
 *   Enable older vector of pairs interface for construction of
 *   programs.
 *
 * - CL_HPP_CL_1_2_DEFAULT_BUILD
 *
 *   Default to OpenCL C 1.2 compilation rather than OpenCL C 2.0
 *   applies to use of cl::Program construction and other program
 *   build variants.
 *
 *
 * \section example Example
 *
 * The following example shows a general use case for the C++
 * bindings, including support for the optional exception feature and
 * also the supplied vector and string classes, see following sections for
 * decriptions of these features.
 *
 * \code
    #define CL_HPP_ENABLE_EXCEPTIONS
    #define CL_HPP_TARGET_OPENCL_VERSION 200

    #include <CL/cl2.hpp>
    #include <iostream>
    #include <vector>
    #include <memory>
    #include <algorithm>

    const int numElements = 32;

    int main(void)
    {
        // Filter for a 2.0 platform and set it as the default
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        cl::Platform plat;
        for (auto &p : platforms) {
            std::string platver = p.getInfo<CL_PLATFORM_VERSION>();
            if (platver.find("OpenCL 2.") != std::string::npos) {
                plat = p;
            }
        }
        if (plat() == 0)  {
            std::cout << "No OpenCL 2.0 platform found.";
            return -1;
        }

        cl::Platform newP = cl::Platform::setDefault(plat);
        if (newP != plat) {
            std::cout << "Error setting default platform.";
            return -1;
        }

        // Use C++11 raw string literals for kernel source code
        std::string kernel1{R"CLC(
            global int globalA;
            kernel void updateGlobal()
            {
              globalA = 75;
            }
        )CLC"};
        std::string kernel2{R"CLC(
            typedef struct { global int *bar; } Foo;
            kernel void vectorAdd(global const Foo* aNum, global const int *inputA, global const int *inputB,
                                  global int *output, int val, write_only pipe int outPipe, queue_t childQueue)
            {
              output[get_global_id(0)] = inputA[get_global_id(0)] + inputB[get_global_id(0)] + val + *(aNum->bar);
              write_pipe(outPipe, &val);
              queue_t default_queue = get_default_queue();
              ndrange_t ndrange = ndrange_1D(get_global_size(0)/2, get_global_size(0)/2);

              // Have a child kernel write into third quarter of output
              enqueue_kernel(default_queue, CLK_ENQUEUE_FLAGS_WAIT_KERNEL, ndrange,
                ^{
                    output[get_global_size(0)*2 + get_global_id(0)] =
                      inputA[get_global_size(0)*2 + get_global_id(0)] + inputB[get_global_size(0)*2 + get_global_id(0)] + globalA;
                });

              // Have a child kernel write into last quarter of output
              enqueue_kernel(childQueue, CLK_ENQUEUE_FLAGS_WAIT_KERNEL, ndrange,
                ^{
                    output[get_global_size(0)*3 + get_global_id(0)] =
                      inputA[get_global_size(0)*3 + get_global_id(0)] + inputB[get_global_size(0)*3 + get_global_id(0)] + globalA + 2;
                });
            }
        )CLC"};

        // New simpler string interface style
        std::vector<std::string> programStrings {kernel1, kernel2};

        cl::Program vectorAddProgram(programStrings);
        try {
            vectorAddProgram.build("-cl-std=CL2.0");
        }
        catch (...) {
            // Print build info for all devices
            cl_int buildErr = CL_SUCCESS;
            auto buildInfo = vectorAddProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);
            for (auto &pair : buildInfo) {
                std::cerr << pair.second << std::endl << std::endl;
            }

            return 1;
        }

        typedef struct { int *bar; } Foo;

        // Get and run kernel that initializes the program-scope global
        // A test for kernels that take no arguments
        auto program2Kernel =
            cl::KernelFunctor<>(vectorAddProgram, "updateGlobal");
        program2Kernel(
            cl::EnqueueArgs(
            cl::NDRange(1)));

        //////////////////
        // SVM allocations

        auto anSVMInt = cl::allocate_svm<int, cl::SVMTraitCoarse<>>();
        *anSVMInt = 5;
        cl::SVMAllocator<Foo, cl::SVMTraitCoarse<cl::SVMTraitReadOnly<>>> svmAllocReadOnly;
        auto fooPointer = cl::allocate_pointer<Foo>(svmAllocReadOnly);
        fooPointer->bar = anSVMInt.get();
        cl::SVMAllocator<int, cl::SVMTraitCoarse<>> svmAlloc;
        std::vector<int, cl::SVMAllocator<int, cl::SVMTraitCoarse<>>> inputA(numElements, 1, svmAlloc);
        cl::coarse_svm_vector<int> inputB(numElements, 2, svmAlloc);

        //
        //////////////

        // Traditional cl_mem allocations
        std::vector<int> output(numElements, 0xdeadbeef);
        cl::Buffer outputBuffer(begin(output), end(output), false);
        cl::Pipe aPipe(sizeof(cl_int), numElements / 2);

        // Default command queue, also passed in as a parameter
        cl::DeviceCommandQueue defaultDeviceQueue = cl::DeviceCommandQueue::makeDefault(
            cl::Context::getDefault(), cl::Device::getDefault());

        auto vectorAddKernel =
            cl::KernelFunctor<
                decltype(fooPointer)&,
                int*,
                cl::coarse_svm_vector<int>&,
                cl::Buffer,
                int,
                cl::Pipe&,
                cl::DeviceCommandQueue
                >(vectorAddProgram, "vectorAdd");

        // Ensure that the additional SVM pointer is available to the kernel
        // This one was not passed as a parameter
        vectorAddKernel.setSVMPointers(anSVMInt);

        // Hand control of coarse allocations to runtime
        cl::enqueueUnmapSVM(anSVMInt);
        cl::enqueueUnmapSVM(fooPointer);
        cl::unmapSVM(inputB);
        cl::unmapSVM(output2);

	    cl_int error;
	    vectorAddKernel(
            cl::EnqueueArgs(
                cl::NDRange(numElements/2),
                cl::NDRange(numElements/2)),
            fooPointer,
            inputA.data(),
            inputB,
            outputBuffer,
            3,
            aPipe,
            defaultDeviceQueue,
		    error
            );

        cl::copy(outputBuffer, begin(output), end(output));
        // Grab the SVM output vector using a map
        cl::mapSVM(output2);

        cl::Device d = cl::Device::getDefault();

        std::cout << "Output:\n";
        for (int i = 1; i < numElements; ++i) {
            std::cout << "\t" << output[i] << "\n";
        }
        std::cout << "\n\n";

        return 0;
    }
 *
 * \endcode
 *
 */
#ifndef CL_HPP_
#define CL_HPP_

/* Handle deprecated preprocessor definitions. In each case, we only check for
 * the old name if the new name is not defined, so that user code can define
 * both and hence work with either version of the bindings.
 */
#if !defined(CL_HPP_USE_DX_INTEROP) && defined(USE_DX_INTEROP)
# pragma message("cl2.hpp: USE_DX_INTEROP is deprecated. Define CL_HPP_USE_DX_INTEROP instead")
# define CL_HPP_USE_DX_INTEROP
#endif
#if !defined(CL_HPP_USE_CL_DEVICE_FISSION) && defined(USE_CL_DEVICE_FISSION)
# pragma message("cl2.hpp: USE_CL_DEVICE_FISSION is deprecated. Define CL_HPP_USE_CL_DEVICE_FISSION instead")
# define CL_HPP_USE_CL_DEVICE_FISSION
#endif
#if !defined(CL_HPP_ENABLE_EXCEPTIONS) && defined(__CL_ENABLE_EXCEPTIONS)
# pragma message("cl2.hpp: __CL_ENABLE_EXCEPTIONS is deprecated. Define CL_HPP_ENABLE_EXCEPTIONS instead")
# define CL_HPP_ENABLE_EXCEPTIONS
#endif
#if !defined(CL_HPP_NO_STD_VECTOR) && defined(__NO_STD_VECTOR)
# pragma message("cl2.hpp: __NO_STD_VECTOR is deprecated. Define CL_HPP_NO_STD_VECTOR instead")
# define CL_HPP_NO_STD_VECTOR
#endif
#if !defined(CL_HPP_NO_STD_STRING) && defined(__NO_STD_STRING)
# pragma message("cl2.hpp: __NO_STD_STRING is deprecated. Define CL_HPP_NO_STD_STRING instead")
# define CL_HPP_NO_STD_STRING
#endif
#if defined(VECTOR_CLASS)
# pragma message("cl2.hpp: VECTOR_CLASS is deprecated. Alias cl::vector instead")
#endif
#if defined(STRING_CLASS)
# pragma message("cl2.hpp: STRING_CLASS is deprecated. Alias cl::string instead.")
#endif
#if !defined(CL_HPP_USER_OVERRIDE_ERROR_STRINGS) && defined(__CL_USER_OVERRIDE_ERROR_STRINGS)
# pragma message("cl2.hpp: __CL_USER_OVERRIDE_ERROR_STRINGS is deprecated. Define CL_HPP_USER_OVERRIDE_ERROR_STRINGS instead")
# define CL_HPP_USER_OVERRIDE_ERROR_STRINGS
#endif

/* Warn about features that are no longer supported
 */
#if defined(__USE_DEV_VECTOR)
# pragma message("cl2.hpp: __USE_DEV_VECTOR is no longer supported. Expect compilation errors")
#endif
#if defined(__USE_DEV_STRING)
# pragma message("cl2.hpp: __USE_DEV_STRING is no longer supported. Expect compilation errors")
#endif

/* Detect which version to target */
#if !defined(CL_HPP_TARGET_OPENCL_VERSION)
# pragma message("cl2.hpp: CL_HPP_TARGET_OPENCL_VERSION is not defined. It will default to 200 (OpenCL 2.0)")
# define CL_HPP_TARGET_OPENCL_VERSION 200
#endif
#if CL_HPP_TARGET_OPENCL_VERSION != 100 && CL_HPP_TARGET_OPENCL_VERSION != 110 && CL_HPP_TARGET_OPENCL_VERSION != 120 && CL_HPP_TARGET_OPENCL_VERSION != 200
# pragma message("cl2.hpp: CL_HPP_TARGET_OPENCL_VERSION is not a valid value (100, 110, 120 or 200). It will be set to 200")
# undef CL_HPP_TARGET_OPENCL_VERSION
# define CL_HPP_TARGET_OPENCL_VERSION 200
#endif
/* Forward target OpenCL version to C headers */
#define CL_TARGET_OPENCL_VERSION CL_HPP_TARGET_OPENCL_VERSION

#if !defined(CL_HPP_MINIMUM_OPENCL_VERSION)
# define CL_HPP_MINIMUM_OPENCL_VERSION 200
#endif
#if CL_HPP_MINIMUM_OPENCL_VERSION != 100 && CL_HPP_MINIMUM_OPENCL_VERSION != 110 && CL_HPP_MINIMUM_OPENCL_VERSION != 120 && CL_HPP_MINIMUM_OPENCL_VERSION != 200
# pragma message("cl2.hpp: CL_HPP_MINIMUM_OPENCL_VERSION is not a valid value (100, 110, 120 or 200). It will be set to 100")
# undef CL_HPP_MINIMUM_OPENCL_VERSION
# define CL_HPP_MINIMUM_OPENCL_VERSION 100
#endif
#if CL_HPP_MINIMUM_OPENCL_VERSION > CL_HPP_TARGET_OPENCL_VERSION
# error "CL_HPP_MINIMUM_OPENCL_VERSION must not be greater than CL_HPP_TARGET_OPENCL_VERSION"
#endif

#if CL_HPP_MINIMUM_OPENCL_VERSION <= 100 && !defined(CL_USE_DEPRECATED_OPENCL_1_0_APIS)
# define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#endif
#if CL_HPP_MINIMUM_OPENCL_VERSION <= 110 && !defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)
# define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#endif
#if CL_HPP_MINIMUM_OPENCL_VERSION <= 120 && !defined(CL_USE_DEPRECATED_OPENCL_1_2_APIS)
# define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#endif
#if CL_HPP_MINIMUM_OPENCL_VERSION <= 200 && !defined(CL_USE_DEPRECATED_OPENCL_2_0_APIS)
# define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#endif

#ifdef _WIN32

#include <malloc.h>

#if defined(CL_HPP_USE_DX_INTEROP)
#include <CL/cl_d3d10.h>
#include <CL/cl_dx9_media_sharing.h>
#endif
#endif // _WIN32

#if defined(_MSC_VER)
#include <intrin.h>
#endif // _MSC_VER 
 
 // Check for a valid C++ version

// Need to do both tests here because for some reason __cplusplus is not 
// updated in visual studio
#if (!defined(_MSC_VER) && __cplusplus < 201103L) || (defined(_MSC_VER) && _MSC_VER < 1700)
#error Visual studio 2013 or another C++11-supporting compiler required
#endif

// 
#if defined(CL_HPP_USE_CL_DEVICE_FISSION) || defined(CL_HPP_USE_CL_SUB_GROUPS_KHR)
#include <CL/cl_ext.h>
#endif

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif // !__APPLE__

#if (__cplusplus >= 201103L)
#define CL_HPP_NOEXCEPT_ noexcept
#else
#define CL_HPP_NOEXCEPT_
#endif

#if defined(_MSC_VER)
# define CL_HPP_DEFINE_STATIC_MEMBER_ __declspec(selectany)
#else
# define CL_HPP_DEFINE_STATIC_MEMBER_ __attribute__((weak))
#endif // !_MSC_VER

// Define deprecated prefixes and suffixes to ensure compilation
// in case they are not pre-defined
#if !defined(CL_EXT_PREFIX__VERSION_1_1_DEPRECATED)
#define CL_EXT_PREFIX__VERSION_1_1_DEPRECATED  
#endif // #if !defined(CL_EXT_PREFIX__VERSION_1_1_DEPRECATED)
#if !defined(CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED)
#define CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
#endif // #if !defined(CL_EXT_PREFIX__VERSION_1_1_DEPRECATED)

#if !defined(CL_EXT_PREFIX__VERSION_1_2_DEPRECATED)
#define CL_EXT_PREFIX__VERSION_1_2_DEPRECATED  
#endif // #if !defined(CL_EXT_PREFIX__VERSION_1_2_DEPRECATED)
#if !defined(CL_EXT_SUFFIX__VERSION_1_2_DEPRECATED)
#define CL_EXT_SUFFIX__VERSION_1_2_DEPRECATED
#endif // #if !defined(CL_EXT_PREFIX__VERSION_1_2_DEPRECATED)

#if !defined(CL_CALLBACK)
#define CL_CALLBACK
#endif //CL_CALLBACK

#include <utility>
#include <limits>
#include <iterator>
#include <mutex>
#include <cstring>
#include <functional>


// Define a size_type to represent a correctly resolved size_t
#if defined(CL_HPP_ENABLE_SIZE_T_COMPATIBILITY)
namespace cl {
    using size_type = ::size_t;
} // namespace cl
#else // #if defined(CL_HPP_ENABLE_SIZE_T_COMPATIBILITY)
namespace cl {
    using size_type = size_t;
} // namespace cl
#endif // #if defined(CL_HPP_ENABLE_SIZE_T_COMPATIBILITY)


#if defined(CL_HPP_ENABLE_EXCEPTIONS)
#include <exception>
#endif // #if defined(CL_HPP_ENABLE_EXCEPTIONS)

#if !defined(CL_HPP_NO_STD_VECTOR)
#include <vector>
namespace cl {
    template < class T, class Alloc = std::allocator<T> >
    using vector = std::vector<T, Alloc>;
} // namespace cl
#endif // #if !defined(CL_HPP_NO_STD_VECTOR)

#if !defined(CL_HPP_NO_STD_STRING)
#include <string>
namespace cl {
    using string = std::string;
} // namespace cl
#endif // #if !defined(CL_HPP_NO_STD_STRING)

#if CL_HPP_TARGET_OPENCL_VERSION >= 200

#if !defined(CL_HPP_NO_STD_UNIQUE_PTR)
#include <memory>
namespace cl {
    // Replace unique_ptr and allocate_pointer for internal use
    // to allow user to replace them
    template<class T, class D>
    using pointer = std::unique_ptr<T, D>;
} // namespace cl
#endif 
#endif // #if CL_HPP_TARGET_OPENCL_VERSION >= 200
#if !defined(CL_HPP_NO_STD_ARRAY)
#include <array>
namespace cl {
    template < class T, size_type N >
    using array = std::array<T, N>;
} // namespace cl
#endif // #if !defined(CL_HPP_NO_STD_ARRAY)

// Define size_type appropriately to allow backward-compatibility
// use of the old size_t interface class
#if defined(CL_HPP_ENABLE_SIZE_T_COMPATIBILITY)
namespace cl {
    namespace compatibility {
        /*! \brief class used to interface between C++ and
        *  OpenCL C calls that require arrays of size_t values, whose
        *  size is known statically.
        */
        template <int N>
        class size_t
        {
        private:
            size_type data_[N];

        public:
            //! \brief Initialize size_t to all 0s
            size_t()
            {
                for (int i = 0; i < N; ++i) {
                    data_[i] = 0;
                }
            }

            size_t(const array<size_type, N> &rhs)
            {
                for (int i = 0; i < N; ++i) {
                    data_[i] = rhs[i];
                }
            }

            size_type& operator[](int index)
            {
                return data_[index];
            }

            const size_type& operator[](int index) const
            {
                return data_[index];
            }

            //! \brief Conversion operator to T*.
            operator size_type* ()             { return data_; }

            //! \brief Conversion operator to const T*.
            operator const size_type* () const { return data_; }

            operator array<size_type, N>() const
            {
                array<size_type, N> ret;

                for (int i = 0; i < N; ++i) {
                    ret[i] = data_[i];
                }
                return ret;
            }
        };
    } // namespace compatibility

    template<int N>
    using size_t = compatibility::size_t<N>;
} // namespace cl
#endif // #if defined(CL_HPP_ENABLE_SIZE_T_COMPATIBILITY)

// Helper alias to avoid confusing the macros
namespace cl {
    namespace detail {
        using size_t_array = array<size_type, 3>;
    } // namespace detail
} // namespace cl


/*! \namespace cl
 *
 * \brief The OpenCL C++ bindings are defined within this namespace.
 *
 */
namespace cl {
    class Memory;

#define CL_HPP_INIT_CL_EXT_FCN_PTR_(name) \
    if (!pfn_##name) {    \
    pfn_##name = (PFN_##name) \
    clGetExtensionFunctionAddress(#name); \
    if (!pfn_##name) {    \
    } \
    }

#define CL_HPP_INIT_CL_EXT_FCN_PTR_PLATFORM_(platform, name) \
    if (!pfn_##name) {    \
    pfn_##name = (PFN_##name) \
    clGetExtensionFunctionAddressForPlatform(platform, #name); \
    if (!pfn_##name) {    \
    } \
    }

    class Program;
    class Device;
    class Context;
    class CommandQueue;
    class DeviceCommandQueue;
    class Memory;
    class Buffer;
    class Pipe;

#if defined(CL_HPP_ENABLE_EXCEPTIONS)
    /*! \brief Exception class 
     * 
     *  This may be thrown by API functions when CL_HPP_ENABLE_EXCEPTIONS is defined.
     */
    class Error : public std::exception
    {
    private:
        cl_int err_;
        const char * errStr_;
    public:
        /*! \brief Create a new CL error exception for a given error code
         *  and corresponding message.
         * 
         *  \param err error code value.
         *
         *  \param errStr a descriptive string that must remain in scope until
         *                handling of the exception has concluded.  If set, it
         *                will be returned by what().
         */
        Error(cl_int err, const char * errStr = NULL) : err_(err), errStr_(errStr)
        {}

        ~Error() throw() {}

        /*! \brief Get error string associated with exception
         *
         * \return A memory pointer to the error message string.
         */
        virtual const char * what() const throw ()
        {
            if (errStr_ == NULL) {
                return "empty";
            }
            else {
                return errStr_;
            }
        }

        /*! \brief Get error code associated with exception
         *
         *  \return The error code.
         */
        cl_int err(void) const { return err_; }
    };
#define CL_HPP_ERR_STR_(x) #x
#else
#define CL_HPP_ERR_STR_(x) NULL
#endif // CL_HPP_ENABLE_EXCEPTIONS


namespace detail
{
#if defined(CL_HPP_ENABLE_EXCEPTIONS)
static inline cl_int errHandler (
    cl_int err,
    const char * errStr = NULL)
{
    if (err != CL_SUCCESS) {
        throw Error(err, errStr);
    }
    return err;
}
#else
static inline cl_int errHandler (cl_int err, const char * errStr = NULL)
{
    (void) errStr; // suppress unused variable warning
    return err;
}
#endif // CL_HPP_ENABLE_EXCEPTIONS
}



//! \cond DOXYGEN_DETAIL
#if !defined(CL_HPP_USER_OVERRIDE_ERROR_STRINGS)
#define __GET_DEVICE_INFO_ERR               CL_HPP_ERR_STR_(clGetDeviceInfo)
#define __GET_PLATFORM_INFO_ERR             CL_HPP_ERR_STR_(clGetPlatformInfo)
#define __GET_DEVICE_IDS_ERR                CL_HPP_ERR_STR_(clGetDeviceIDs)
#define __GET_PLATFORM_IDS_ERR              CL_HPP_ERR_STR_(clGetPlatformIDs)
#define __GET_CONTEXT_INFO_ERR              CL_HPP_ERR_STR_(clGetContextInfo)
#define __GET_EVENT_INFO_ERR                CL_HPP_ERR_STR_(clGetEventInfo)
#define __GET_EVENT_PROFILE_INFO_ERR        CL_HPP_ERR_STR_(clGetEventProfileInfo)
#define __GET_MEM_OBJECT_INFO_ERR           CL_HPP_ERR_STR_(clGetMemObjectInfo)
#define __GET_IMAGE_INFO_ERR                CL_HPP_ERR_STR_(clGetImageInfo)
#define __GET_SAMPLER_INFO_ERR              CL_HPP_ERR_STR_(clGetSamplerInfo)
#define __GET_KERNEL_INFO_ERR               CL_HPP_ERR_STR_(clGetKernelInfo)
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __GET_KERNEL_ARG_INFO_ERR           CL_HPP_ERR_STR_(clGetKernelArgInfo)
#endif // CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __GET_KERNEL_WORK_GROUP_INFO_ERR    CL_HPP_ERR_STR_(clGetKernelWorkGroupInfo)
#define __GET_PROGRAM_INFO_ERR              CL_HPP_ERR_STR_(clGetProgramInfo)
#define __GET_PROGRAM_BUILD_INFO_ERR        CL_HPP_ERR_STR_(clGetProgramBuildInfo)
#define __GET_COMMAND_QUEUE_INFO_ERR        CL_HPP_ERR_STR_(clGetCommandQueueInfo)

#define __CREATE_CONTEXT_ERR                CL_HPP_ERR_STR_(clCreateContext)
#define __CREATE_CONTEXT_FROM_TYPE_ERR      CL_HPP_ERR_STR_(clCreateContextFromType)
#define __GET_SUPPORTED_IMAGE_FORMATS_ERR   CL_HPP_ERR_STR_(clGetSupportedImageFormats)

#define __CREATE_BUFFER_ERR                 CL_HPP_ERR_STR_(clCreateBuffer)
#define __COPY_ERR                          CL_HPP_ERR_STR_(cl::copy)
#define __CREATE_SUBBUFFER_ERR              CL_HPP_ERR_STR_(clCreateSubBuffer)
#define __CREATE_GL_BUFFER_ERR              CL_HPP_ERR_STR_(clCreateFromGLBuffer)
#define __CREATE_GL_RENDER_BUFFER_ERR       CL_HPP_ERR_STR_(clCreateFromGLBuffer)
#define __GET_GL_OBJECT_INFO_ERR            CL_HPP_ERR_STR_(clGetGLObjectInfo)
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __CREATE_IMAGE_ERR                  CL_HPP_ERR_STR_(clCreateImage)
#define __CREATE_GL_TEXTURE_ERR             CL_HPP_ERR_STR_(clCreateFromGLTexture)
#define __IMAGE_DIMENSION_ERR               CL_HPP_ERR_STR_(Incorrect image dimensions)
#endif // CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __SET_MEM_OBJECT_DESTRUCTOR_CALLBACK_ERR CL_HPP_ERR_STR_(clSetMemObjectDestructorCallback)

#define __CREATE_USER_EVENT_ERR             CL_HPP_ERR_STR_(clCreateUserEvent)
#define __SET_USER_EVENT_STATUS_ERR         CL_HPP_ERR_STR_(clSetUserEventStatus)
#define __SET_EVENT_CALLBACK_ERR            CL_HPP_ERR_STR_(clSetEventCallback)
#define __WAIT_FOR_EVENTS_ERR               CL_HPP_ERR_STR_(clWaitForEvents)

#define __CREATE_KERNEL_ERR                 CL_HPP_ERR_STR_(clCreateKernel)
#define __SET_KERNEL_ARGS_ERR               CL_HPP_ERR_STR_(clSetKernelArg)
#define __CREATE_PROGRAM_WITH_SOURCE_ERR    CL_HPP_ERR_STR_(clCreateProgramWithSource)
#define __CREATE_PROGRAM_WITH_BINARY_ERR    CL_HPP_ERR_STR_(clCreateProgramWithBinary)
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __CREATE_PROGRAM_WITH_BUILT_IN_KERNELS_ERR    CL_HPP_ERR_STR_(clCreateProgramWithBuiltInKernels)
#endif // CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __BUILD_PROGRAM_ERR                 CL_HPP_ERR_STR_(clBuildProgram)
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __COMPILE_PROGRAM_ERR               CL_HPP_ERR_STR_(clCompileProgram)
#define __LINK_PROGRAM_ERR                  CL_HPP_ERR_STR_(clLinkProgram)
#endif // CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __CREATE_KERNELS_IN_PROGRAM_ERR     CL_HPP_ERR_STR_(clCreateKernelsInProgram)

#if CL_HPP_TARGET_OPENCL_VERSION >= 200
#define __CREATE_COMMAND_QUEUE_WITH_PROPERTIES_ERR          CL_HPP_ERR_STR_(clCreateCommandQueueWithProperties)
#define __CREATE_SAMPLER_WITH_PROPERTIES_ERR                CL_HPP_ERR_STR_(clCreateSamplerWithProperties)
#endif // CL_HPP_TARGET_OPENCL_VERSION >= 200
#define __SET_COMMAND_QUEUE_PROPERTY_ERR    CL_HPP_ERR_STR_(clSetCommandQueueProperty)
#define __ENQUEUE_READ_BUFFER_ERR           CL_HPP_ERR_STR_(clEnqueueReadBuffer)
#define __ENQUEUE_READ_BUFFER_RECT_ERR      CL_HPP_ERR_STR_(clEnqueueReadBufferRect)
#define __ENQUEUE_WRITE_BUFFER_ERR          CL_HPP_ERR_STR_(clEnqueueWriteBuffer)
#define __ENQUEUE_WRITE_BUFFER_RECT_ERR     CL_HPP_ERR_STR_(clEnqueueWriteBufferRect)
#define __ENQEUE_COPY_BUFFER_ERR            CL_HPP_ERR_STR_(clEnqueueCopyBuffer)
#define __ENQEUE_COPY_BUFFER_RECT_ERR       CL_HPP_ERR_STR_(clEnqueueCopyBufferRect)
#define __ENQUEUE_FILL_BUFFER_ERR           CL_HPP_ERR_STR_(clEnqueueFillBuffer)
#define __ENQUEUE_READ_IMAGE_ERR            CL_HPP_ERR_STR_(clEnqueueReadImage)
#define __ENQUEUE_WRITE_IMAGE_ERR           CL_HPP_ERR_STR_(clEnqueueWriteImage)
#define __ENQUEUE_COPY_IMAGE_ERR            CL_HPP_ERR_STR_(clEnqueueCopyImage)
#define __ENQUEUE_FILL_IMAGE_ERR            CL_HPP_ERR_STR_(clEnqueueFillImage)
#define __ENQUEUE_COPY_IMAGE_TO_BUFFER_ERR  CL_HPP_ERR_STR_(clEnqueueCopyImageToBuffer)
#define __ENQUEUE_COPY_BUFFER_TO_IMAGE_ERR  CL_HPP_ERR_STR_(clEnqueueCopyBufferToImage)
#define __ENQUEUE_MAP_BUFFER_ERR            CL_HPP_ERR_STR_(clEnqueueMapBuffer)
#define __ENQUEUE_MAP_IMAGE_ERR             CL_HPP_ERR_STR_(clEnqueueMapImage)
#define __ENQUEUE_UNMAP_MEM_OBJECT_ERR      CL_HPP_ERR_STR_(clEnqueueUnMapMemObject)
#define __ENQUEUE_NDRANGE_KERNEL_ERR        CL_HPP_ERR_STR_(clEnqueueNDRangeKernel)
#define __ENQUEUE_NATIVE_KERNEL             CL_HPP_ERR_STR_(clEnqueueNativeKernel)
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __ENQUEUE_MIGRATE_MEM_OBJECTS_ERR   CL_HPP_ERR_STR_(clEnqueueMigrateMemObjects)
#endif // CL_HPP_TARGET_OPENCL_VERSION >= 120

#define __ENQUEUE_ACQUIRE_GL_ERR            CL_HPP_ERR_STR_(clEnqueueAcquireGLObjects)
#define __ENQUEUE_RELEASE_GL_ERR            CL_HPP_ERR_STR_(clEnqueueReleaseGLObjects)

#define __CREATE_PIPE_ERR             CL_HPP_ERR_STR_(clCreatePipe)
#define __GET_PIPE_INFO_ERR           CL_HPP_ERR_STR_(clGetPipeInfo)


#define __RETAIN_ERR                        CL_HPP_ERR_STR_(Retain Object)
#define __RELEASE_ERR                       CL_HPP_ERR_STR_(Release Object)
#define __FLUSH_ERR                         CL_HPP_ERR_STR_(clFlush)
#define __FINISH_ERR                        CL_HPP_ERR_STR_(clFinish)
#define __VECTOR_CAPACITY_ERR               CL_HPP_ERR_STR_(Vector capacity error)

/**
 * CL 1.2 version that uses device fission.
 */
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __CREATE_SUB_DEVICES_ERR            CL_HPP_ERR_STR_(clCreateSubDevices)
#else
#define __CREATE_SUB_DEVICES_ERR            CL_HPP_ERR_STR_(clCreateSubDevicesEXT)
#endif // CL_HPP_TARGET_OPENCL_VERSION >= 120

/**
 * Deprecated APIs for 1.2
 */
#if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)
#define __ENQUEUE_MARKER_ERR                CL_HPP_ERR_STR_(clEnqueueMarker)
#define __ENQUEUE_WAIT_FOR_EVENTS_ERR       CL_HPP_ERR_STR_(clEnqueueWaitForEvents)
#define __ENQUEUE_BARRIER_ERR               CL_HPP_ERR_STR_(clEnqueueBarrier)
#define __UNLOAD_COMPILER_ERR               CL_HPP_ERR_STR_(clUnloadCompiler)
#define __CREATE_GL_TEXTURE_2D_ERR          CL_HPP_ERR_STR_(clCreateFromGLTexture2D)
#define __CREATE_GL_TEXTURE_3D_ERR          CL_HPP_ERR_STR_(clCreateFromGLTexture3D)
#define __CREATE_IMAGE2D_ERR                CL_HPP_ERR_STR_(clCreateImage2D)
#define __CREATE_IMAGE3D_ERR                CL_HPP_ERR_STR_(clCreateImage3D)
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

/**
 * Deprecated APIs for 2.0
 */
#if defined(CL_USE_DEPRECATED_OPENCL_1_2_APIS)
#define __CREATE_COMMAND_QUEUE_ERR          CL_HPP_ERR_STR_(clCreateCommandQueue)
#define __ENQUEUE_TASK_ERR                  CL_HPP_ERR_STR_(clEnqueueTask)
#define __CREATE_SAMPLER_ERR                CL_HPP_ERR_STR_(clCreateSampler)
#endif // #if defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)

/**
 * CL 1.2 marker and barrier commands
 */
#if CL_HPP_TARGET_OPENCL_VERSION >= 120
#define __ENQUEUE_MARKER_WAIT_LIST_ERR                CL_HPP_ERR_STR_(clEnqueueMarkerWithWaitList)
#define __ENQUEUE_BARRIER_WAIT_LIST_ERR               CL_HPP_ERR_STR_(clEnqueueBarrierWithWaitList)
#endif // CL_HPP_TARGET_OPENCL_VERSION >= 120

#endif // CL_HPP_USER_OVERRIDE_ERROR_STRINGS
//! \endcond


namespace detail {

// Generic getInfoHelper. The final parameter is used to guide overload
// resolution: the actual parameter passed is an int, which makes this
// a worse conversion sequence than a specialization that declares the
// parameter as an int.
template<typename Functor, typename T>
inline cl_int getInfoHelper(Functor f, cl_uint name, T* param, long)
{
    return f(name, sizeof(T), param, NULL);
}

// Specialized for getInfo<CL_PROGRAM_BINARIES>
// Assumes that the output vector was correctly resized on the way in
template <typename Func>
inline cl_int getInfoHelper(Func f, cl_uint name, vector<vector<unsigned char>>* param, int)
{
    if (name != CL_PROGRAM_BINARIES) {
        return CL_INVALID_VALUE;
    }
    if (param) {
        // Create array of pointers, calculate total size and pass pointer array in
        size_type numBinaries = param->size();
        vector<unsigned char*> binariesPointers(numBinaries);

        for (size_type i = 0; i < numBinaries; ++i)
        {
            binariesPointers[i] = (*param)[i].data();
        }

        cl_int err = f(name, numBinaries * sizeof(unsigned char*), binariesPointers.data(), NULL);

        if (err != CL_SUCCESS) {
            return err;
        }
    }


    return CL_SUCCESS;
}

// Specialized getInfoHelper for vector params
template <typename Func, typename T>
inline cl_int getInfoHelper(Func f, cl_uint name, vector<T>* param, long)
{
    size_type required;
    cl_int err = f(name, 0, NULL, &required);
    if (err != CL_SUCCESS) {
        return err;
    }
    const size_type elements = required / sizeof(T);

    // Temporary to avoid changing param on an error
    vector<T> localData(elements);
    err = f(name, required, localData.data(), NULL);
    if (err != CL_SUCCESS) {
        return err;
    }
    if (param) {
        *param = std::move(localData);
    }

    return CL_SUCCESS;
}

/* Specialization for reference-counted types. This depends on the
 * existence of Wrapper<T>::cl_type, and none of the other types having the
 * cl_type member. Note that simplify specifying the parameter as Wrapper<T>
 * does not work, because when using a derived type (e.g. Context) the generic
 * template will provide a better match.
 */
template <typename Func, typename T>
inline cl_int getInfoHelper(
    Func f, cl_uint name, vector<T>* param, int, typename T::cl_type = 0)
{
    size_type required;
    cl_int err = f(name, 0, NULL, &required);
    if (err != CL_SUCCESS) {
        return err;
    }

    const size_type elements = required / sizeof(typename T::cl_type);

    vector<typename T::cl_type> value(elements);
    err = f(name, required, value.data(), NULL);
    if (err != CL_SUCCESS) {
        return err;
    }

    if (param) {
        // Assign to convert CL type to T for each element
        param->resize(elements);

        // Assign to param, constructing with retain behaviour
        // to correctly capture each underlying CL object
        for (size_type i = 0; i < elements; i++) {
            (*param)[i] = T(value[i], true);
        }
    }
    return CL_SUCCESS;
}

// Specialized GetInfoHelper for string params
template <typename Func>
inline cl_int getInfoHelper(Func f, cl_uint name, string* param, long)
{
    size_type required;
    cl_int err = f(name, 0, NULL, &required);
    if (err != CL_SUCCESS) {
        return err;
    }

    // std::string has a constant data member
    // a char vector does not
    if (required > 0) {
        vector<char> value(required);
        err = f(name, required, value.data(), NULL);
        if (err != CL_SUCCESS) {
            return err;
        }
        if (param) {
            param->assign(begin(value), prev(end(value)));
        }
    }
    else if (param) {
        param->assign("");
    }
    return CL_SUCCESS;
}

// Specialized GetInfoHelper for clsize_t params
template <typename Func, size_type N>
inline cl_int getInfoHelper(Func f, cl_uint name, array<size_type, N>* param, long)
{
    size_type required;
    cl_int err = f(name, 0, NULL, &required);
    if (err != CL_SUCCESS) {
        return err;
    }

    size_type elements = required / sizeof(size_type);
    vector<size_type> value(elements, 0);

    err = f(name, required, value.data(), NULL);
    if (err != CL_SUCCESS) {
        return err;
    }
    
    // Bound the copy with N to prevent overruns
    // if passed N > than the amount copied
    if (elements > N) {
        elements = N;
    }
    for (size_type i = 0; i < elements; ++i) {
        (*param)[i] = value[i];
    }

    return CL_SUCCESS;
}

template<typename T> struct ReferenceHandler;

/* Specialization for reference-counted types. This depends on the
 * existence of Wrapper<T>::cl_type, and none of the other types having the
 * cl_type member. Note that simplify specifying the parameter as Wrapper<T>
 * does not work, because when using a derived type (e.g. Context) the generic
 * template will provide a better match.
 */
template<typename Func, typename T>
inline cl_int getInfoHelper(Func f, cl_uint name, T* param, int, typename T::cl_type = 0)
{
    typename T::cl_type value;
    cl_int err = f(name, sizeof(value), &value, NULL);
    if (err != CL_SUCCESS) {
        return err;
    }
    *param = value;
    if (value != NULL)
    {
        err = param->retain();
        if (err != CL_SUCCESS) {
            return err;
        }
    }
    return CL_SUCCESS;
}

#define CL_HPP_PARAM_NAME_INFO_1_0_(F) \
    F(cl_platform_info, CL_PLATFORM_PROFILE, string) \
    F(cl_platform_info, CL_PLATFORM_VERSION, string) \
    F(cl_platform_info, CL_PLATFORM_NAME, string) \
    F(cl_platform_info, CL_PLATFORM_VENDOR, string) \
    F(cl_platform_info, CL_PLATFORM_EXTENSIONS, string) \
    \
    F(cl_device_info, CL_DEVICE_TYPE, cl_device_type) \
    F(cl_device_info, CL_DEVICE_VENDOR_ID, cl_uint) \
    F(cl_device_info, CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint) \
    F(cl_device_info, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, cl_uint) \
    F(cl_device_info, CL_DEVICE_MAX_WORK_GROUP_SIZE, size_type) \
    F(cl_device_info, CL_DEVICE_MAX_WORK_ITEM_SIZES, cl::vector<size_type>) \
    F(cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, cl_uint) \
    F(cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, cl_uint) \
    F(cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, cl_uint) \
    F(cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, cl_uint) \
    F(cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, cl_uint) \
    F(cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, cl_uint) \
    F(cl_device_info, CL_DEVICE_MAX_CLOCK_FREQUENCY, cl_uint) \
    F(cl_device_info, CL_DEVICE_ADDRESS_BITS, cl_uint) \
    F(cl_device_info, CL_DEVICE_MAX_READ_IMAGE_ARGS, cl_uint) \
    F(cl_device_info, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, cl_uint) \
    F(cl_device_info, CL_DEVICE_MAX_MEM_ALLOC_SIZE, cl_ulong) \
    F(cl_device_info, CL_DEVICE_IMAGE2D_MAX_WIDTH, size_type) \
    F(cl_device_info, CL_DEVICE_IMAGE2D_MAX_HEIGHT, size_type) \
    F(cl_device_info, CL_DEVICE_IMAGE3D_MAX_WIDTH, size_type) \
    F(cl_device_info, CL_DEVICE_IMAGE3D_MAX_HEIGHT, size_type) \
    F(cl_device_info, CL_DEVICE_IMAGE3D_MAX_DEPTH, size_type) \
    F(cl_device_info, CL_DEVICE_IMAGE_SUPPORT, cl_bool) \
    F(cl_device_info, CL_DEVICE_MAX_PARAMETER_SIZE, size_type) \
    F(cl_device_info, CL_DEVICE_MAX_SAMPLERS, cl_uint) \
    F(cl_device_info, CL_DEVICE_MEM_BASE_ADDR_ALIGN, cl_uint) \
    F(cl_device_info, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, cl_uint) \
    F(cl_device_info, CL_DEVICE_SINGLE_FP_CONFIG, cl_device_fp_config) \
    F(cl_device_info, CL_DEVICE_DOUBLE_FP_CONFIG, cl_device_fp_config) \
    F(cl_device_info, CL_DEVICE_HALF_FP_CONFIG, cl_device_fp_config) \
    F(cl_device_info, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, cl_device_mem_cache_type) \
    F(cl_device_info, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, cl_uint)\
    F(cl_device_info, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, cl_ulong) \
    F(cl_device_info, CL_DEVICE_GLOBAL_MEM_SIZE, cl_ulong) \
    F(cl_device_info, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, cl_ulong) \
    F(cl_device_info, CL_DEVICE_MAX_CONSTANT_ARGS, cl_uint) \
    F(cl_device_info, CL_DEVICE_LOCAL_MEM_TYPE, cl_device_local_mem_type) \
    F(cl_device_info, CL_DEVICE_LOCAL_MEM_SIZE, cl_ulong) \
    F(cl_device_info, CL_DEVICE_ERROR_CORRECTION_SUPPORT, cl_bool) \
    F(cl_device_info, CL_DEVICE_PROFILING_TIMER_RESOLUTION, size_type) \
    F(cl_device_info, CL_DEVICE_ENDIAN_LITTLE, cl_bool) \
    F(cl_device_info, CL_DEVICE_AVAILABLE, cl_bool) \
    F(cl_device_info, CL_DEVICE_COMPILER_AVAILABLE, cl_bool) \
    F(cl_device_info, CL_DEVICE_EXECUTION_CAPABILITIES, cl_device_exec_capabilities) \
    F(cl_device_info, CL_DEVICE_PLATFORM, cl_platform_id) \
    F(cl_device_info, CL_DEVICE_NAME, string) \
    F(cl_device_info, CL_DEVICE_VENDOR, string) \
    F(cl_device_info, CL_DRIVER_VERSION, string) \
    F(cl_device_info, CL_DEVICE_PROFILE, string) \
    F(cl_device_info, CL_DEVICE_VERSION, string) \
    F(cl_device_info, CL_DEVICE_EXTENSIONS, string) \
    \
    F(cl_context_info, CL_CONTEXT_REFERENCE_COUNT, cl_uint) \
    F(cl_context_info, CL_CONTEXT_DEVICES, cl::vector<Device>) \
    F(cl_context_info, CL_CONTEXT_PROPERTIES, cl::vector<cl_context_properties>) \
    \
    F(cl_event_info, CL_EVENT_COMMAND_QUEUE, cl::CommandQueue) \
    F(cl_event_info, CL_EVENT_COMMAND_TYPE, cl_command_type) \
    F(cl_event_info, CL_EVENT_REFERENCE_COUNT, cl_uint) \
    F(cl_event_info, CL_EVENT_COMMAND_EXECUTION_STATUS, cl_int) \
    \
    F(cl_profiling_info, CL_PROFILING_COMMAND_QUEUED, cl_ulong) \
    F(cl_profiling_info, CL_PROFILING_COMMAND_SUBMIT, cl_ulong) \
    F(cl_profiling_info, CL_PROFILING_COMMAND_START, cl_ulong) \
    F(cl_profiling_info, CL_PROFILING_COMMAND_END, cl_ulong) \
    \
    F(cl_mem_info, CL_MEM_TYPE, cl_mem_object_type) \
    F(cl_mem_info, CL_MEM_FLAGS, cl_mem_flags) \
    F(cl_mem_info, CL_MEM_SIZE, size_type) \
    F(cl_mem_info, CL_MEM_HOST_PTR, void*) \
    F(cl_mem_info, CL_MEM_MAP_COUNT, cl_uint) \
    F(cl_mem_info, CL_MEM_REFERENCE_COUNT, cl_uint) \
    F(cl_mem_info, CL_MEM_CONTEXT, cl::Context) \
    \
    F(cl_image_info, CL_IMAGE_FORMAT, cl_image_format) \
    F(cl_image_info, CL_IMAGE_ELEMENT_SIZE, size_type) \
    F(cl_image_info, CL_IMAGE_ROW_PITCH, size_type) \
    F(cl_image_info, CL_IMAGE_SLICE_PITCH, size_type) \
    F(cl_image_info, CL_IMAGE_WIDTH, size_type) \
    F(cl_image_info, CL_IMAGE_HEIGHT, size_type) \
    F(cl_image_info, CL_IMAGE_DEPTH, size_type) \
    \
    F(cl_sampler_info, CL_SAMPLER_REFERENCE_COUNT, cl_uint) \
    F(cl_sampler_info, CL_SAMPLER_CONTEXT, cl::Context) \
    F(cl_sampler_info, CL_SAMPLER_NORMALIZED_COORDS, cl_bool) \
    F(cl_sampler_info, CL_SAMPLER_ADDRESSING_MODE, cl_addressing_mode) \
    F(cl_sampler_info, CL_SAMPLER_FILTER_MODE, cl_filter_mode) \
    \
    F(cl_program_info, CL_PROGRAM_REFERENCE_COUNT, cl_uint) \
    F(cl_program_info, CL_PROGRAM_CONTEXT, cl::Context) \
    F(cl_program_info, CL_PROGRAM_NUM_DEVICES, cl_uint) \
    F(cl_program_info, CL_PROGRAM_DEVICES, cl::vector<Device>) \
    F(cl_program_info, CL_PROGRAM_SOURCE, string) \
    F(cl_program_info, CL_PROGRAM_BINARY_SIZES, cl::vector<size_type>) \
    F(cl_program_info, CL_PROGRAM_BINARIES, cl::vector<cl::vector<unsigned char>>) \
    \
    F(cl_program_build_info, CL_PROGRAM_BUILD_STATUS, cl_build_status) \
    F(cl_program_build_info, CL_PROGRAM_BUILD_OPTIONS, string) \
    F(cl_program_build_info, CL_PROGRAM_BUILD_LOG, string) \
    \
    F(cl_kernel_info, CL_KERNEL_FUNCTION_NAME, string) \
    F(cl_kernel_info, CL_KERNEL_NUM_ARGS, cl_uint) \
    F(cl_kernel_info, CL_KERNEL_REFERENCE_COUNT, cl_uint) \
    F(cl_kernel_info, CL_KERNEL_CONTEXT, cl::Context) \
    F(cl_kernel_info, CL_KERNEL_PROGRAM, cl::Program) \
    \
    F(cl_kernel_work_group_info, CL_KERNEL_WORK_GROUP_SIZE, size_type) \
    F(cl_kernel_work_group_info, CL_KERNEL_COMPILE_WORK_GROUP_SIZE, cl::detail::size_t_array) \
    F(cl_kernel_work_group_info, CL_KERNEL_LOCAL_MEM_SIZE, cl_ulong) \
    \
    F(cl_command_queue_info, CL_QUEUE_CONTEXT, cl::Context) \
    F(cl_command_queue_info, CL_QUEUE_DEVICE, cl::Device) \
    F(cl_command_queue_info, CL_QUEUE_REFERENCE_COUNT, cl_uint) \
    F(cl_command_queue_info, CL_QUEUE_PROPERTIES, cl_command_queue_properties)


#define CL_HPP_PARAM_NAME_INFO_1_1_(F) \
    F(cl_context_info, CL_CONTEXT_NUM_DEVICES, cl_uint)\
    F(cl_device_info, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, cl_uint) \
    F(cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, cl_uint) \
    F(cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, cl_uint) \
    F(cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, cl_uint) \
    F(cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, cl_uint) \
    F(cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, cl_uint) \
    F(cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, cl_uint) \
    F(cl_device_info, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, cl_uint) \
    F(cl_device_info, CL_DEVICE_OPENCL_C_VERSION, string) \
    \
    F(cl_mem_info, CL_MEM_ASSOCIATED_MEMOBJECT, cl::Memory) \
    F(cl_mem_info, CL_MEM_OFFSET, size_type) \
    \
    F(cl_kernel_work_group_info, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, size_type) \
    F(cl_kernel_work_group_info, CL_KERNEL_PRIVATE_MEM_SIZE, cl_ulong) \
    \
    F(cl_event_info, CL_EVENT_CONTEXT, cl::Context)

#define CL_HPP_PARAM_NAME_INFO_1_2_(F) \
    F(cl_program_info, CL_PROGRAM_NUM_KERNELS, size_type) \
    F(cl_program_info, CL_PROGRAM_KERNEL_NAMES, string) \
    \
    F(cl_program_build_info, CL_PROGRAM_BINARY_TYPE, cl_program_binary_type) \
    \
    F(cl_kernel_info, CL_KERNEL_ATTRIBUTES, string) \
    \
    F(cl_kernel_arg_info, CL_KERNEL_ARG_ADDRESS_QUALIFIER, cl_kernel_arg_address_qualifier) \
    F(cl_kernel_arg_info, CL_KERNEL_ARG_ACCESS_QUALIFIER, cl_kernel_arg_access_qualifier) \
    F(cl_kernel_arg_info, CL_KERNEL_ARG_TYPE_NAME, string) \
    F(cl_kernel_arg_info, CL_KERNEL_ARG_NAME, string) \
    F(cl_kernel_arg_info, CL_KERNEL_ARG_TYPE_QUALIFIER, cl_kernel_arg_type_qualifier) \
    \
    F(cl_device_info, CL_DEVICE_PARENT_DEVICE, cl::Device) \
    F(cl_device_info, CL_DEVICE_PARTITION_PROPERTIES, cl::vector<cl_device_partition_property>) \
    F(cl_device_info, CL_DEVICE_PARTITION_TYPE, cl::vector<cl_device_partition_property>)  \
    F(cl_device_info, CL_DEVICE_REFERENCE_COUNT, cl_uint) \
    F(cl_device_info, CL_DEVICE_PREFERRED_INTEROP_USER_SYNC, size_type) \
    F(cl_device_info, CL_DEVICE_PARTITION_AFFINITY_DOMAIN, cl_device_affinity_domain) \
    F(cl_device_info, CL_DEVICE_BUILT_IN_KERNELS, string) \
    \
    F(cl_image_info, CL_IMAGE_ARRAY_SIZE, size_type) \
    F(cl_image_info, CL_IMAGE_NUM_MIP_LEVELS, cl_uint) \
    F(cl_image_info, CL_IMAGE_NUM_SAMPLES, cl_uint)

#define CL_HPP_PARAM_NAME_INFO_2_0_(F) \
    F(cl_device_info, CL_DEVICE_QUEUE_ON_HOST_PROPERTIES, cl_command_queue_properties) \
    F(cl_device_info, CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES, cl_command_queue_properties) \
    F(cl_device_info, CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE, cl_uint) \
    F(cl_device_info, CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE, cl_uint) \
    F(cl_device_info, CL_DEVICE_MAX_ON_DEVICE_QUEUES, cl_uint) \
    F(cl_device_info, CL_DEVICE_MAX_ON_DEVICE_EVENTS, cl_uint) \
    F(cl_device_info, CL_DEVICE_MAX_PIPE_ARGS, cl_uint) \
    F(cl_device_info, CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS, cl_uint) \
    F(cl_device_info, CL_DEVICE_PIPE_MAX_PACKET_SIZE, cl_uint) \
    F(cl_device_info, CL_DEVICE_SVM_CAPABILITIES, 