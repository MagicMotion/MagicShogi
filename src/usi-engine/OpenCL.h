/*
    This file is part of Leela Zero.
    Copyright (C) 2017-2019 Gian-Carlo Pascutto and contributors

    Leela Zero is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Leela Zero is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Leela Zero.  If not, see <http://www.gnu.org/licenses/>.

    Additional permission under GNU GPL version 3 section 7

    If you modify this Program, or any covered work, by linking or
    combining it with NVIDIA Corporation's libraries from the
    NVIDIA CUDA Toolkit and/or the NVIDIA CUDA Deep Neural
    Network library and/or the NVIDIA TensorRT inference library
    (or a modified version of those libraries), containing parts covered
    by the terms of the respective license agreement, the licensors of
    this Program grant you additional permission to convey the resulting
    work.
*/

#ifndef OPENCL_H_INCLUDED
#define OPENCL_H_INCLUDED

#include "config.h"

#define CL_HPP_MINIMUM_OPENCL_VERSION   110
#define CL_HPP_TARGET_OPENCL_VERSION    120
#define CL_HPP_ENABLE_EXCEPTIONS
#include <CL/cl2.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <cassert>

#include "Tuner.h"

template <typename net_t> class OpenCL;
template <typename net_t> class OpenCL_Network;

class Layer {
    template <typename> friend class OpenCL_Network;
private:
    unsigned int channels{0};
    unsigned int outputs{0};
    unsigned int filter_size{0};
    bool is_input_convolution{false};
    bool is_residual_block{false};
    bool is_convolve1{false};
    std::vector<cl::Buffer> weights;
};

class OpenCLContext {
    template <typename> friend class OpenCL;
    template <typename> friend class OpenCL_Network;
private:
    bool m_is_initialized{false};
    cl::CommandQueue m_commandqueue;
    cl::Kernel m_convolve1_kernel;
    cl::Kernel m_merge_kernel;
    cl::Kernel m_in_transform_kernel;
    cl::Kernel m_sgemm_kernel;
    cl::Kernel m_out_transform_bn_kernel;
    cl::Kernel m_out_transform_bn_in_kernel;
    cl::Buffer m_inBuffer;
    cl::Buffer m_inBuffer2;
    cl::Buffer m_VBuffer;
    cl::Buffer m_MBuffer