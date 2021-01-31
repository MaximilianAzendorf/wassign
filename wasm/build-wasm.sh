#!/bin/bash

echo "+--------------------------------------+"
echo "|    Experimental WASM build script    |"
echo "|            NO  GUARANTEES            |"
echo "+--------------------------------------+"
echo "                                        "

set -e

pushd "$(git rev-parse --show-toplevel)"
    if [ ! -d "build-wasm" ]; then
        echo "Creating build directory..."
        mkdir build-wasm
    fi

    pushd build-wasm
        if [ ! -d "or-tools" ]; then
            echo "Cloning wasm-or-tools..."
            git clone https://github.com/MaximilianAzendorf/wasm-or-tools or-tools
        fi

        if [ ! -d "boost" ]; then
            echo "Downloading boost..."
            wget https://dl.bintray.com/boostorg/release/1.74.0/source/boost_1_74_0.tar.bz2 -q --show-progress
            echo "Unpacking boost..."
            tar -xf boost_1_74_0.tar.bz2
            echo "Cleaning up after boost download..."
            mv boost_1_74_0 boost
            rm boost_1_74_0.tar.bz2
        fi

        pushd or-tools
            if [ ! -d "emscripten" ]; then
                echo "Calling emscripten setup..."
                ./em-setup.sh
            fi
            echo "Building or-tools..."
            ./build.sh

            pushd emscripten
                echo "Setting up emscripten environment..."
                source emsdk_env.sh
            popd
        popd

        if [ ! -d "bin" ]; then
            echo "Creating bin directory..."
            mkdir bin
        fi

        shopt -s extglob
        shopt -s globstar
        echo "Building wassign..."
        (
            set -x; \
            emcc \
                -O3 \
                -flto \
                -pthread \
                --bind \
                --no-entry \
                --std=c++17 \
                --closure=1 \
                -s WASM=1 \
                -s LLD_REPORT_UNDEFINED \
                -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
                -I or-tools/wasmbuild/install/include \
                -I boost \
                -I ../deps/popl/include \
                -I ../deps/magic_enum/include \
                -I ../deps/chaiscript/include \
		-I ../deps/chaiscript_extras/include \
		-I ../deps/rapidcsv/src \
                -Lor-tools/wasmbuild/install/lib \
                -lm \
                -lglog \
                -labsl_bad_any_cast_impl \
                -labsl_log_severity \
                -labsl_bad_optional_access \
                -labsl_malloc_internal \
                -labsl_bad_variant_access \
                -labsl_periodic_sampler \
                -labsl_base \
                -labsl_random_distributions \
                -labsl_city \
                -labsl_random_internal_distribution_test_util \
                -labsl_civil_time \
                -labsl_random_internal_pool_urbg \
                -labsl_cord \
                -labsl_random_internal_randen \
                -labsl_debugging_internal \
                -labsl_random_internal_randen_hwaes \
                -labsl_demangle_internal \
                -labsl_random_internal_randen_hwaes_impl \
                -labsl_random_internal_randen_slow \
                -labsl_examine_stack \
                -labsl_random_internal_seed_material \
                -labsl_exponential_biased \
                -labsl_random_seed_gen_exception \
                -labsl_failure_signal_handler \
                -labsl_random_seed_sequences \
                -labsl_flags \
                -labsl_raw_hash_set \
                -labsl_flags_config \
                -labsl_raw_logging_internal \
                -labsl_flags_internal \
                -labsl_scoped_set_env \
                -labsl_flags_marshalling \
                -labsl_spinlock_wait \
                -labsl_flags_parse \
                -labsl_stacktrace \
                -labsl_flags_program_name \
                -labsl_status \
                -labsl_str_format_internal \
                -labsl_flags_usage \
                -labsl_strings \
                -labsl_flags_usage_internal \
                -labsl_strings_internal \
                -labsl_graphcycles_internal \
                -labsl_symbolize \
                -labsl_hash \
                -labsl_synchronization \
                -labsl_hashtablez_sampler \
                -labsl_throw_delegate \
                -labsl_int128 \
                -labsl_time \
                -labsl_leak_check \
                -labsl_time_zone \
                -labsl_leak_check_disable \
                -lCbc \
                -lCbcSolver \
                -lCgl \
                -lClp \
                -lClpSolver \
                -lCoinUtils \
                -lgflags_nothreads \
                -lortools \
                -lOsi \
                -lOsiCbc \
                -lOsiClp \
                -lprotobuf \
                -lscip \
                -lz \
                ../src/**/!(main).cpp \
                ../wasm/*.cpp \
                -o bin/wassign.html \
        )
        echo "Done. Output was written to ./build-wasm/bin"
    popd
popd
