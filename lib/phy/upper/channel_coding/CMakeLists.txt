#
# Copyright 2021-2025 Software Radio Systems Limited
#
# This file is part of srsRAN
#
# srsRAN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# srsRAN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#

set(CRC_CALCULATOR_SOURCES
        crc_calculator_generic_impl.cpp
        crc_calculator_lut_impl.cpp)
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
    list(APPEND CRC_CALCULATOR_SOURCES crc_calculator_clmul_impl.cpp)
    set_source_files_properties(crc_calculator_clmul_impl.cpp PROPERTIES COMPILE_OPTIONS "-mpclmul;-msse4.1")
endif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    list(APPEND CRC_CALCULATOR_SOURCES crc_calculator_neon_impl.cpp)
    set_source_files_properties(crc_calculator_neon_impl.cpp PROPERTIES COMPILE_OPTIONS "-march=${MARCH}+crypto")
endif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")


add_library(srsran_crc_calculator STATIC ${CRC_CALCULATOR_SOURCES})

add_subdirectory(polar)
add_subdirectory(ldpc)
add_subdirectory(short)

add_library(srsran_channel_coding STATIC channel_coding_factories.cpp)

target_link_libraries(srsran_channel_coding
        srsran_crc_calculator
        srsran_polar
        srsran_ldpc
        srsran_short_block)

add_to_exported_libs(srsran_channel_coding
        srsran_crc_calculator
        srsran_polar
        srsran_ldpc
        srsran_short_block)
