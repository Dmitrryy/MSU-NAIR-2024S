cmake_minimum_required (VERSION 3.8)
project (task1)

# get rid of annoying MSVC warnings.
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set (CMAKE_CXX_STANDARD 17)
option(USE_VULKAN "Enable GPU implementation via Vulkan" OFF)
option(BUILD_WITH_KERNEL_SLICER "Build kernel slicer and use it automaicaly" OFF)

# common dependecies
#=---------------------------------------------------------
find_package(OpenMP)

include(cmake/CPM.cmake)

CPMAddPackage(NAME LiteMath 
GIT_REPOSITORY "https://github.com/msu-graphics-group/LiteMath"
GIT_TAG 05e7dd386922f46f7e498b38d3fd2a5dd7a1f2f8
DOWNLOAD_ONLY
)

CPMAddPackage(NAME CLI11 
GIT_REPOSITORY "https://github.com/CLIUtils/CLI11"
GIT_TAG v2.4.2
DOWNLOAD_ONLY
)
include_directories(${CLI11_SOURCE_DIR}/include)
#=---------------------------------------------------------

if(BUILD_WITH_KERNEL_SLICER)
  find_package(Vulkan)

  # gpu deps
  #=---------------------------------------------------------
  if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
  endif()
  CPMAddPackage(NAME kernel_slicer 
  GIT_REPOSITORY "https://github.com/Ray-Tracing-Systems/kernel_slicer"
  GIT_TAG 5e0e7ad60e25adac91430339e899b77b87c378f7
  )

  CPMAddPackage(NAME volk 
  GIT_REPOSITORY "https://github.com/zeux/volk"
  GIT_TAG 5982311be42f8277cad11ae4637c80ab69c79ce5
  OPTIONS "VOLK_STATIC_DEFINES ${VOLK_STATIC_DEFINES}"
  )

  CPMAddPackage(NAME vkutils 
  GIT_REPOSITORY "https://github.com/msu-graphics-group/vk-utils"
  GIT_TAG 87fa40b96393ef3168d5df4ff7c11469ff3f189e
  DOWNLOAD_ONLY
  )
  #=---------------------------------------------------------

  set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
  set(GENERATED_FILES
    ${GENERATED_DIR}/tracer_generated.cpp
    ${GENERATED_DIR}/tracer_generated.h 
    ${GENERATED_DIR}/tracer_generated_init.cpp
    ${GENERATED_DIR}/tracer_generated_ds.cpp
  )
  message("Files to generate: ${GENERATED_FILES}")
  add_custom_command(OUTPUT ${GENERATED_FILES}

  COMMAND ${CMAKE_COMMAND} -E remove_directory ${GENERATED_DIR}/
  COMMAND "cp" ARGS -r ${CMAKE_CURRENT_SOURCE_DIR}/tracer ${GENERATED_DIR}/
  COMMAND ${kernel_slicer_BINARY_DIR}/kslicer ARGS ${GENERATED_DIR}/tracer.cpp -mainClass RayMarcherExample -shaderCC glsl -pattern ipv -DKERNEL_SLICER -v -I${LiteMath_SOURCE_DIR} ignore -I${kernel_slicer_SOURCE_DIR}/TINYSTL ignore -stdlibfolder ${kernel_slicer_SOURCE_DIR}/TINYSTL
  
  WORKING_DIRECTORY ${kernel_slicer_SOURCE_DIR}
  DEPENDS kslicer ${CMAKE_CURRENT_SOURCE_DIR}/tracer/tracer.cpp ${CMAKE_CURRENT_SOURCE_DIR}/tracer/tracer.hpp
  )

  add_custom_target(compile_shaders
  COMMAND bash ./build.sh
  COMMAND cp -r ${GENERATED_DIR}/shaders_generated ${CMAKE_CURRENT_BINARY_DIR}
  
  WORKING_DIRECTORY ${GENERATED_DIR}/shaders_generated
  DEPENDS ${GENERATED_FILES}
  )


  add_executable(tracer_gpu main.cpp 
    tracer/tracer.cpp
    ${LiteMath_SOURCE_DIR}/Image2d.cpp 
  
    ${GENERATED_FILES}

    ${vkutils_SOURCE_DIR}/vk_utils.cpp
    ${vkutils_SOURCE_DIR}/vk_copy.cpp
    ${vkutils_SOURCE_DIR}/vk_context.cpp
    ${vkutils_SOURCE_DIR}/vk_alloc_simple.cpp
    ${vkutils_SOURCE_DIR}/vk_buffers.cpp
    ${vkutils_SOURCE_DIR}/vk_images.cpp
    ${vkutils_SOURCE_DIR}/vk_pipeline.cpp
    ${vkutils_SOURCE_DIR}/vk_descriptor_sets.cpp)

    add_compile_definitions(USE_VOLK)
    add_compile_definitions(USE_VULKAN)

    include_directories(${Vulkan_INCLUDE_DIR})

    include_directories(${LiteMath_SOURCE_DIR}
                        ${vkutils_SOURCE_DIR}
                        ${volk_SOURCE_DIR})
                      
    link_directories(${volk_BINARY_DIR}
    )
    add_dependencies(tracer_gpu compile_shaders)

    set_target_properties(tracer_gpu PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    target_link_libraries(tracer_gpu LINK_PUBLIC OpenMP::OpenMP_CXX ${Vulkan_LIBRARY} volk dl)
else()

if(USE_VULKAN)

  # gpu deps
  #=---------------------------------------------------------
  CPMAddPackage(NAME volk 
  GIT_REPOSITORY "https://github.com/zeux/volk"
  GIT_TAG 5982311be42f8277cad11ae4637c80ab69c79ce5
  OPTIONS "VOLK_STATIC_DEFINES ${VOLK_STATIC_DEFINES}"
  # DOWNLOAD_ONLY
  )

  CPMAddPackage(NAME vkutils 
  GIT_REPOSITORY "https://github.com/msu-graphics-group/vk-utils"
  GIT_TAG 87fa40b96393ef3168d5df4ff7c11469ff3f189e
  DOWNLOAD_ONLY
  )
  #=---------------------------------------------------------

  # add_subdirectory(external/volk)

  add_compile_definitions(USE_VOLK)
  add_compile_definitions(USE_VULKAN)
  
  include_directories(${Vulkan_INCLUDE_DIR})

  include_directories(${LiteMath_SOURCE_DIR}
                      ${vkutils_SOURCE_DIR}
                      ${volk_SOURCE_DIR})
                    
  link_directories(${volk_BINARY_DIR})
else()
  include_directories(${LiteMath_SOURCE_DIR})
endif()

include_directories(".")
if(WIN32)
  link_directories(${ADDITIONAL_LIBRARY_DIRS})
endif()

#uncomment this to detect broken memory problems via gcc sanitizers
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fsanitize=leak -fsanitize=undefined -fsanitize=bounds-strict")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")

if(USE_VULKAN)
  add_executable(tracer_gpu main.cpp 
                 tracer/tracer.cpp
                 ${LiteMath_SOURCE_DIR}/Image2d.cpp 
                 
                 tracer/tracer_generated.cpp
                 tracer/tracer_generated_ds.cpp
                 tracer/tracer_generated_init.cpp  

                 ${vkutils_SOURCE_DIR}/vk_utils.cpp
                 ${vkutils_SOURCE_DIR}/vk_copy.cpp
                 ${vkutils_SOURCE_DIR}/vk_context.cpp
                 ${vkutils_SOURCE_DIR}/vk_alloc_simple.cpp
                 ${vkutils_SOURCE_DIR}/vk_buffers.cpp
                 ${vkutils_SOURCE_DIR}/vk_images.cpp
                 ${vkutils_SOURCE_DIR}/vk_pipeline.cpp
                 ${vkutils_SOURCE_DIR}/vk_descriptor_sets.cpp)

  set_target_properties(tracer_gpu PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  target_link_libraries(tracer_gpu LINK_PUBLIC OpenMP::OpenMP_CXX ${Vulkan_LIBRARY} volk dl)

else()
  add_executable(tracer_cpu main.cpp 
                 tracer/tracer.cpp
                 ${LiteMath_SOURCE_DIR}/Image2d.cpp) 

  set_target_properties(tracer_cpu PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

  target_link_libraries(tracer_cpu LINK_PUBLIC OpenMP::OpenMP_CXX)
endif()

endif()
