from building import *
import rtconfig
import os

src = []
inc = []
group = []
cwd = GetCurrentDir() # get current dir path

def check_h_hpp_exists(path):
    file_dirs = os.listdir(path)
    for file_dir in file_dirs:
        if os.path.splitext(file_dir)[1] in ['.h', '.hpp']:
            return True
    return False

mlevk_cwd = cwd

mlevk_subdir = [
    #'source/',
    'dependencies/cmsis-dsp',
    'dependencies/cmsis-nn',
    #'dependencies/tensorflow',
    #'dependencies/cmsis-dsp/PrivateInclude',
    #'dependencies/cmsis-dsp/Include',
    #'dependencies/cmsis-dsp/Source/TransformFunctions',
    #'dependencies/cmsis-dsp/Source/BasicMathFunctions',
    #'dependencies/cmsis-dsp/Source/CommonTables',
    #'dependencies/cmsis-dsp/Source/ComplexMathFunctions',
    #'dependencies/cmsis-dsp/Source/FastMathFunctions',
    #'dependencies/cmsis-dsp/Source/StatisticsFunctions',
    #'dependencies/cmsis-dsp/Source/StatisticsFunctions',
    #'dependencies/cmsis-nn/Source',
    #'dependencies/cmsis-nn/Include',
    #'dependencies/tensorflow',
]

# ml-embedded-evaluation-kit-source
filter = set()
for subdir in mlevk_subdir:
    mlevk_src_cwd = os.path.join(mlevk_cwd, subdir)
    for root, dirs, files in os.walk(mlevk_src_cwd, topdown=True):
        PathxScons = os.path.join(root, 'xScons')
        if os.path.exists(PathxScons):
            print('Filter out -> ' + root)
            filter.add(root)

for subdir in mlevk_subdir:
    mlevk_src_cwd = os.path.join(mlevk_cwd, subdir)
    for root, dirs, files in os.walk(mlevk_src_cwd, topdown=True):
        for dir in dirs:

            buildit = True
            for s in filter:
                #print('root ->' + root + ', ' + s)
                if os.path.join(root, dir).find(s) == 0:
                    #print('found')
                    buildit = False
                    break

            if buildit:
                current_path = os.path.join(root, dir)
                #print(current_path)
                src = src + Glob(os.path.join(current_path,'*.c')) # add all .c files
                src = src + Glob(os.path.join(current_path,'*.cc')) # add all .cc files
                src = src + Glob(os.path.join(current_path,'*.cpp')) # add all .cpp files
                if check_h_hpp_exists(current_path): # add .h and .hpp path
                    inc = inc + [current_path]
                if check_h_hpp_exists(root): # add .h and .hpp path
                    inc = inc + [root]

inc = inc + [
    cwd + '/dependencies/cmsis-nn/',
    cwd + '/dependencies/tensorflow/',
    cwd + '/dependencies/tensorflow/tensorflow/lite/micro/tools/make/downloads/flatbuffers/include/',
    cwd + '/dependencies/tensorflow/tensorflow/lite/micro/tools/make/downloads/gemmlowp/',
    cwd + '/dependencies/tensorflow/tensorflow/lite/micro/tools/make/downloads/ruy/',
    cwd + '/dependencies/tensorflow/tensorflow/lite/micro/tools/make/downloads/kissfft/',
]

tflm_SRCS = Split("""
dependencies/tensorflow/tensorflow/compiler/mlir/lite/schema/schema_utils.cc
dependencies/tensorflow/tensorflow/lite/core/c/common.cc
dependencies/tensorflow/tensorflow/lite/core/api/tensor_utils.cc
dependencies/tensorflow/tensorflow/lite/core/api/error_reporter.cc
dependencies/tensorflow/tensorflow/lite/core/api/flatbuffer_conversions.cc
dependencies/tensorflow/tensorflow/lite/kernels/internal/tensor_ctypes.cc
dependencies/tensorflow/tensorflow/lite/kernels/internal/tensor_utils.cc
dependencies/tensorflow/tensorflow/lite/kernels/internal/runtime_shape.cc
dependencies/tensorflow/tensorflow/lite/kernels/internal/common.cc
dependencies/tensorflow/tensorflow/lite/kernels/internal/reference/comparisons.cc
dependencies/tensorflow/tensorflow/lite/kernels/internal/reference/portable_tensor_utils.cc
dependencies/tensorflow/tensorflow/lite/kernels/internal/quantization_util.cc
dependencies/tensorflow/tensorflow/lite/kernels/internal/portable_tensor_utils.cc
dependencies/tensorflow/tensorflow/lite/kernels/kernel_util.cc
dependencies/tensorflow/tensorflow/lite/micro/tflite_bridge/flatbuffer_conversions_bridge.cc
dependencies/tensorflow/tensorflow/lite/micro/tflite_bridge/micro_error_reporter.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_allocator.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_interpreter_graph.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_context.cc
dependencies/tensorflow/tensorflow/lite/micro/arena_allocator/non_persistent_arena_buffer_allocator.cc
dependencies/tensorflow/tensorflow/lite/micro/arena_allocator/recording_single_arena_buffer_allocator.cc
dependencies/tensorflow/tensorflow/lite/micro/arena_allocator/single_arena_buffer_allocator.cc
dependencies/tensorflow/tensorflow/lite/micro/arena_allocator/persistent_arena_buffer_allocator.cc
dependencies/tensorflow/tensorflow/lite/micro/fake_micro_context.cc
dependencies/tensorflow/tensorflow/lite/micro/span_test.cc
dependencies/tensorflow/tensorflow/lite/micro/system_setup.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_profiler.cc
dependencies/tensorflow/tensorflow/lite/micro/test_helper_custom_ops.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_log.cc
dependencies/tensorflow/tensorflow/lite/micro/memory_planner/linear_memory_planner.cc
dependencies/tensorflow/tensorflow/lite/micro/memory_planner/greedy_memory_planner.cc
dependencies/tensorflow/tensorflow/lite/micro/memory_planner/non_persistent_buffer_planner_shim.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_utils.cc
dependencies/tensorflow/tensorflow/lite/micro/mock_micro_graph.cc
dependencies/tensorflow/tensorflow/lite/micro/test_helpers.cc
dependencies/tensorflow/tensorflow/lite/micro/recording_micro_allocator.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_interpreter_context.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_interpreter.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_resource_variable.cc
dependencies/tensorflow/tensorflow/lite/micro/flatbuffer_utils.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_allocation_info.cc
dependencies/tensorflow/tensorflow/lite/micro/micro_op_resolver.cc
dependencies/tensorflow/tensorflow/lite/micro/static_vector_test.cc
dependencies/tensorflow/tensorflow/lite/micro/cortex_m_generic/micro_time.cc
dependencies/tensorflow/tensorflow/lite/micro/cortex_m_generic/debug_log.cc
dependencies/tensorflow/tensorflow/lite/micro/memory_helpers.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cmsis_nn/pooling.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cmsis_nn/mul.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cmsis_nn/fully_connected.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cmsis_nn/softmax.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cmsis_nn/depthwise_conv.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cmsis_nn/conv.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cmsis_nn/add.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cmsis_nn/svdf.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cmsis_nn/transpose_conv.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cmsis_nn/unidirectional_sequence_lstm.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/div.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/split.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/expand_dims.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/gather.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/pad.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/logical_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/resize_nearest_neighbor.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/quantize_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/space_to_depth.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cumsum.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/circular_buffer.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/conv_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/var_handle.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/elementwise.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/hard_swish_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/cast.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/arg_min_max.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/activations_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/unpack.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/log_softmax.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/logical.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/select.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/lstm_eval_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/sub_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/leaky_relu.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/l2_pool_2d.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/svdf_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/sub.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/kernel_runner.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/concatenation.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/floor_mod.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/comparisons.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/assign_variable.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/activations.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/tanh.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/fill.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/broadcast_to.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/elu.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/floor.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/strided_slice.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/logistic.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/add_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/logistic_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/transpose.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/exp.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/reshape.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/reduce.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/pack.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/if.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/add_n.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/neg.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/ethos_u/ethosu.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/lstm_eval.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/reshape_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/gather_nd.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/reduce_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/strided_slice_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/squeeze.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/leaky_relu_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/dequantize_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/maximum_minimum.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/circular_buffer_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/call_once.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/floor_div.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/squared_difference.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/hard_swish.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/mirror_pad.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/shape.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/micro_tensor_utils.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/batch_to_space_nd.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/while.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/batch_matmul.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/round.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/fully_connected_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/space_to_batch_nd.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/read_variable.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/quantize.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/resize_bilinear.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/kernel_util.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/split_v.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/broadcast_args.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/l2norm.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/mul_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/slice.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/softmax_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/prelu_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/detection_postprocess.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/pooling_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/embedding_lookup.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/dequantize.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/ceil.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/depth_to_space.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/depthwise_conv_common.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/prelu.cc
dependencies/tensorflow/tensorflow/lite/micro/kernels/zeros_like.cc
dependencies/tensorflow/signal/src/msb_64.cc
dependencies/tensorflow/signal/src/rfft_int32.cc
dependencies/tensorflow/signal/src/filter_bank_spectral_subtraction.cc
dependencies/tensorflow/signal/src/max_abs.cc
dependencies/tensorflow/signal/src/irfft_int32.cc
dependencies/tensorflow/signal/src/pcan_argc_fixed.cc
dependencies/tensorflow/signal/src/filter_bank_log.cc
dependencies/tensorflow/signal/src/circular_buffer.cc
dependencies/tensorflow/signal/src/square_root_64.cc
dependencies/tensorflow/signal/src/overlap_add.cc
dependencies/tensorflow/signal/src/filter_bank_square_root.cc
dependencies/tensorflow/signal/src/window.cc
dependencies/tensorflow/signal/src/irfft_int16.cc
dependencies/tensorflow/signal/src/energy.cc
dependencies/tensorflow/signal/src/rfft_float.cc
dependencies/tensorflow/signal/src/square_root_32.cc
dependencies/tensorflow/signal/src/log.cc
dependencies/tensorflow/signal/src/rfft_int16.cc
dependencies/tensorflow/signal/src/irfft_float.cc
dependencies/tensorflow/signal/src/kiss_fft_wrappers/kiss_fft_int16.cc
dependencies/tensorflow/signal/src/kiss_fft_wrappers/kiss_fft_float.cc
dependencies/tensorflow/signal/src/kiss_fft_wrappers/kiss_fft_int32.cc
dependencies/tensorflow/signal/src/msb_32.cc
dependencies/tensorflow/signal/src/filter_bank.cc
dependencies/tensorflow/signal/src/fft_auto_scale.cc
dependencies/tensorflow/signal/micro/kernels/fft_auto_scale_common.cc
dependencies/tensorflow/signal/micro/kernels/filter_bank_square_root.cc
dependencies/tensorflow/signal/micro/kernels/overlap_add.cc
dependencies/tensorflow/signal/micro/kernels/filter_bank_square_root_common.cc
dependencies/tensorflow/signal/micro/kernels/irfft.cc
dependencies/tensorflow/signal/micro/kernels/rfft.cc
dependencies/tensorflow/signal/micro/kernels/filter_bank.cc
dependencies/tensorflow/signal/micro/kernels/window.cc
dependencies/tensorflow/signal/micro/kernels/energy.cc
dependencies/tensorflow/signal/micro/kernels/filter_bank_log.cc
dependencies/tensorflow/signal/micro/kernels/fft_auto_scale_kernel.cc
dependencies/tensorflow/signal/micro/kernels/pcan.cc
dependencies/tensorflow/signal/micro/kernels/framer.cc
dependencies/tensorflow/signal/micro/kernels/delay.cc
dependencies/tensorflow/signal/micro/kernels/stacker.cc
dependencies/tensorflow/signal/micro/kernels/filter_bank_spectral_subtraction.cc
""")

src += tflm_SRCS

LOCAL_CFLAGS = ' -std=c99 '
LOCAL_CXXFLAGS = ' -std=c++17 -DCMSIS_NN=1 -DARM_MODEL_USE_PMU_COUNTERS=1 -DCMSIS_DEVICE_ARM_CORTEX_M_XX_HEADER_FILE=\\\"NuMicro.h\\\" -DNDEBUG=1  -DETHOSU=1 -DFLATBUFFERS_LOCALE_INDEPENDENT=0 '

group = group + DefineGroup('MLEVK', src, depend = ['NU_PKG_USING_MLEVK'], CPPPATH = inc, LOCAL_CFLAGS = LOCAL_CFLAGS, LOCAL_CXXFLAGS = LOCAL_CXXFLAGS)

list = os.listdir(cwd)
for d in list:
    path = os.path.join(cwd, d)
    if os.path.isfile(os.path.join(path, 'SConscript')):
        group = group + SConscript(os.path.join(d, 'SConscript'))

Return('group')
