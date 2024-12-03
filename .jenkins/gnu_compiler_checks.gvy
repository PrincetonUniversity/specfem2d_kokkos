pipeline{
    agent {
        node {
            label 'della_rk9481'
        }
    }
    stages{
        stage(' GNU Host Compiler Check '){
            matrix {
                axes {
                    axis{
                        name 'GNUCompiler'
                        values 'GCC8;gcc/8', 'GCC13;gcc-toolset/13'
                    }
                    axis{
                        name 'SIMD'
                        values 'SIMD_NONE;-DENABLE_SIMD=OFF', 'SIMD_NATIVE;-DENABLE_SIMD=ON -DKokkos_ARCH_NATIVE=ON -DKokkos_ENABLE_AGGRESSIVE_VECTORIZATION=ON'
                    }
                    axis{
                        name 'HostSpace'
                        values 'SERIAL;-DKokkos_ENABLE_SERIAL=ON -DKokkos_ENABLE_ATOMICS_BYPASS=ON;-n 1', 'OPENMP;-DKokkos_ENABLE_OPENMP=ON;-n 10'
                    }
                }
                stages {
                    stage ('Build and Clean '){
                        environment {
                            // CMAKE build flags
                            GNU_COMPILER_NAME = """${sh(
                                                    returnStdout: true,
                                                    script: 'cut -d";" -f1 <<<"${GNUCompiler}"'
                                                ).trim()}"""
                            GNU_COMPILER_MODULE = """${sh(
                                                    returnStdout: true,
                                                    script: 'cut -d";" -f2 <<<"${GNUCompiler}"'
                                                ).trim()}"""
                            CMAKE_HOST_NAME = """${sh(
                                                    returnStdout: true,
                                                    script: 'cut -d";" -f1 <<<"${HostSpace}"'
                                                ).trim()}"""
                            CMAKE_HOST_FLAGS = """${sh(
                                                    returnStdout: true,
                                                    script: 'cut -d";" -f2 <<<"${HostSpace}"'
                                                ).trim()}"""
                            HOST_RUN_FLAGS = """${sh(
                                                    returnStdout: true,
                                                    script: 'cut -d";" -f3 <<<"${HostSpace}"'
                                                ).trim()}"""
                            SIMD_NAME = """${sh(
                                                    returnStdout: true,
                                                    script: 'cut -d";" -f1 <<<"${SIMD}"'
                                                ).trim()}"""
                            SIMD_FLAGS = """${sh(
                                                    returnStdout: true,
                                                    script: 'cut -d";" -f2 <<<"${SIMD}"'
                                                ).trim()}"""
                        }
                        stages {
                            stage (' Copy source to workdir '){
                                steps {
                                    echo ' Copying source to workdir '
                                    sh """
                                        cp -r . workdir_cpu_${GNU_COMPILER_NAME}_${CMAKE_HOST_NAME}_${SIMD_NAME}_${env.BUILD_TAG}
                                        cd workdir_cpu_${GNU_COMPILER_NAME}_${CMAKE_HOST_NAME}_${SIMD_NAME}_${env.BUILD_TAG}
                                    """
                                    echo ' Copy completed '
                                }
                            }
                            stage (' Initialize git modules '){
                                steps {
                                    echo ' Initializing git modules '
                                    sh """
                                        git submodule init
                                        git submodule update
                                    """
                                    echo ' Initialization completed '
                                }
                            }
                            stage (' Build '){
                                steps {
                                    echo "Building ${CMAKE_HOST_FLAGS} ${SIMD_FLAGS} with ${GNU_COMPILER_NAME}"
                                    sh """
                                        module load boost/1.73.0
                                        module load ${GNU_COMPILER_MODULE}
                                        cmake3 -S . -B build_cpu_${GNU_COMPILER_NAME}_${CMAKE_HOST_NAME}_${SIMD_NAME}_${env.BUILD_TAG} -DCMAKE_BUILD_TYPE=Release ${CMAKE_HOST_FLAGS} ${SIMD_FLAGS} -DBUILD_TESTS=ON
                                        cmake3 --build build_cpu_${GNU_COMPILER_NAME}_${CMAKE_HOST_NAME}_${SIMD_NAME}_${env.BUILD_TAG}
                                    """
                                    echo ' Build completed '
                                }
                            }
                            stage (' Test '){
                                steps {
                                    echo ' Testing '
                                    sh """
                                        module load boost/1.73.0
                                        module load ${GNU_COMPILER_MODULE}
                                        cd build_cpu_${GNU_COMPILER_NAME}_${CMAKE_HOST_NAME}_${SIMD_NAME}_${env.BUILD_TAG}/tests/unit-tests
                                        srun -N 1 -t 00:20:00 ${HOST_RUN_FLAGS} --constraint=skylake bash -c 'export OMP_PROC_BIND=spread; export OMP_THREADS=places; ctest --verbose;'
                                    """
                                    echo ' Testing completed '
                                }
                            }
                        }
                        post {
                            always {
                                echo ' Cleaning '
                                sh "rm -rf build_cpu_${GNU_COMPILER_NAME}_${CMAKE_HOST_NAME}_${SIMD_NAME}_${env.BUILD_TAG}"
                            }
                        }
                    }
                }
            }
        }
    }
}
