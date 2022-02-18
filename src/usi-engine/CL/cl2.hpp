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
 * c