// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "remote_tensors_filling.hpp"

#include <memory>
#include <random>
#include <samples/slog.hpp>
#include <string>
#include <utility>
#include <vector>

#ifdef HAVE_DEVICE_MEM_SUPPORT
#    include <openvino/runtime/intel_gpu/ocl/ocl.hpp>
#    include <openvino/runtime/intel_gpu/ocl/ocl_wrapper.hpp>
#endif

namespace gpu {

template <typename T>
using uniformDistribution = typename std::conditional<
    std::is_floating_point<T>::value,
    std::uniform_real_distribution<T>,
    typename std::conditional<std::is_integral<T>::value, std::uniform_int_distribution<T>, void>::type>::type;

template <typename T, typename T2>
void fillBufferRandom(void* inputBuffer,
                      size_t elementsNum,
                      T rand_min = std::numeric_limits<uint8_t>::min(),
                      T rand_max = std::numeric_limits<uint8_t>::max()) {
    std::mt19937 gen(0);
    uniformDistribution<T2> distribution(rand_min, rand_max);
    auto inputBufferData = static_cast<T*>(inputBuffer);
    for (size_t i = 0; i < elementsNum; i++) {
        inputBufferData[i] = static_cast<T>(distribution(gen));
    }
}

void fillBuffer(void* inputBuffer, size_t elementsNum, const ov::element::Type& type) {
    if (type == ov::element::f32) {
        fillBufferRandom<float, float>(inputBuffer, elementsNum);
    } else if (type == ov::element::f16) {
        fillBufferRandom<short, short>(inputBuffer, elementsNum);
    } else if (type == ov::element::i32) {
        fillBufferRandom<int32_t, int32_t>(inputBuffer, elementsNum);
    } else if (type == ov::element::i64) {
        fillBufferRandom<int64_t, int64_t>(inputBuffer, elementsNum);
    } else if (type == ov::element::u8) {
        // uniform_int_distribution<uint8_t> is not allowed in the C++17
        // standard and vs2017/19
        fillBufferRandom<uint8_t, uint32_t>(inputBuffer, elementsNum);
    } else if (type == ov::element::i8) {
        // uniform_int_distribution<int8_t> is not allowed in the C++17 standard
        // and vs2017/19
        fillBufferRandom<int8_t, int32_t>(inputBuffer, elementsNum);
    } else if (type == ov::element::u16) {
        fillBufferRandom<uint16_t, uint16_t>(inputBuffer, elementsNum);
    } else if (type == ov::element::i16) {
        fillBufferRandom<int16_t, int16_t>(inputBuffer, elementsNum);
    } else if (type == ov::element::boolean) {
        fillBufferRandom<uint8_t, uint32_t>(inputBuffer, elementsNum, 0, 1);
    } else {
        IE_THROW() << "Requested type is not supported";
    }
}

std::map<std::string, ov::runtime::TensorVector> getRemoteInputTensors(
    const std::map<std::string, std::vector<std::string>>& inputFiles,
    const std::vector<benchmark_app::InputsInfo>& app_inputs_info,
    const ov::runtime::CompiledModel& compiledModel,
    std::vector<BufferType>& clBuffer) {
#ifdef HAVE_DEVICE_MEM_SUPPORT
    slog::info << "Device memory will be used for input and output blobs" << slog::endl;
    if (inputFiles.size()) {
        slog::warn << "Device memory supports only random data at this moment, input images will be ignored"
                   << slog::endl;
    }

    std::map<std::string, ov::runtime::TensorVector> remoteTensors;
    auto context = compiledModel.get_context();
    auto& oclContext = static_cast<ov::runtime::intel_gpu::ocl::ClContext&>(context);
    auto oclInstance = std::make_shared<gpu::OpenCL>(oclContext.get());

    for (auto& inputs_info : app_inputs_info) {
        for (auto& input : inputs_info) {
            // Fill random
            slog::info << "Prepare remote blob for input '" << input.first << "' with random values ("
                       << std::string((input.second.isImage() ? "image" : "some binary data")) << " is expected)"
                       << slog::endl;

            auto tensor = oclContext.create_tensor(input.second.type, input.second.dataShape, clBuffer.back().get());
            remoteTensors[input.first].push_back(tensor);

            // Creating and filling shared buffers
            cl_int err;
            auto elementsNum = std::accumulate(begin(input.second.dataShape),
                                               end(input.second.dataShape),
                                               1,
                                               std::multiplies<size_t>());
            auto inputSize = elementsNum * input.second.type.bitwidth() / 8;

            clBuffer.push_back(
                cl::Buffer(oclInstance->_context, CL_MEM_READ_WRITE, (cl::size_type)inputSize, NULL, &err));

            void* mappedPtr = oclInstance->_queue.enqueueMapBuffer(clBuffer.back(),
                                                                   CL_TRUE,
                                                                   CL_MEM_READ_WRITE,
                                                                   0,
                                                                   (cl::size_type)inputSize);
            if (inputFiles.empty()) {
                // Filling in random data
                fillBuffer(mappedPtr, elementsNum, input.second.type);
            } else {
                // TODO: add filling with real image data
            }
            oclInstance->_queue.enqueueUnmapMemObject(clBuffer.back(), mappedPtr);
        }
    }

    return remoteTensors;
#else
    IE_THROW() << "Device memory requested for GPU device, but OpenCL was not linked";
#endif
}

std::map<std::string, ov::runtime::Tensor> getRemoteOutputTensors(const ov::runtime::CompiledModel& compiledModel,
                                                                  std::map<std::string, ::gpu::BufferType>& clBuffer) {
#ifdef HAVE_DEVICE_MEM_SUPPORT
    std::map<std::string, ov::runtime::Tensor> outputTensors;
    for (auto& output : compiledModel.outputs()) {
        auto context = compiledModel.get_context();
        auto& oclContext = static_cast<ov::runtime::intel_gpu::ocl::ClContext&>(context);
        auto oclInstance = std::make_shared<OpenCL>(oclContext.get());

        cl_int err;
        auto elementsNum =
            std::accumulate(begin(output.get_shape()), end(output.get_shape()), 1, std::multiplies<size_t>());
        auto inputSize = elementsNum * output.get_element_type().bitwidth() / 8;

        cl::size_type bufferSize = 0;
        if (clBuffer.find(output.get_any_name()) == clBuffer.end()) {
            clBuffer[output.get_any_name()] =
                cl::Buffer(oclInstance->_context, CL_MEM_READ_WRITE, (cl::size_type)inputSize, NULL, &err);
        } else {
            auto& buff = clBuffer[output.get_any_name()];
            buff.getInfo(CL_MEM_SIZE, &bufferSize);
            if (inputSize != bufferSize) {
                buff = cl::Buffer(oclInstance->_context, CL_MEM_READ_WRITE, (cl::size_type)inputSize, NULL, &err);
            }
        }
        outputTensors[output.get_any_name()] = oclContext.create_tensor(output.get_element_type(),
                                                                        output.get_shape(),
                                                                        clBuffer[output.get_any_name()].get());
    }

    return outputTensors;
#else
    IE_THROW() << "Device memory requested for GPU device, but OpenCL was not linked";
#endif
}
}  // namespace gpu
