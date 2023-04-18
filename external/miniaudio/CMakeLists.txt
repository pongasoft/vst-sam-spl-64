# Copyright (c) 2023 pongasoft
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# @author Yan Pujante

cmake_minimum_required(VERSION 3.17)

project(miniaudio)

set(MINIAUDIO_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

add_library(miniaudio miniaudio.c)
target_include_directories(miniaudio PUBLIC "${MINIAUDIO_ROOT_DIR}")
target_compile_definitions(miniaudio PUBLIC MA_NO_DEVICE_IO MA_ENABLE_ONLY_SPECIFIC_BACKENDS MA_NO_RESOURCE_MANAGER MA_NO_NODE_GRAPH MA_NO_ENGINE MA_NO_THREADING MA_NO_GENERATION)
