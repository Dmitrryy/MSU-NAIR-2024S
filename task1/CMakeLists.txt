cmake_minimum_required (VERSION 3.8)
project (task1)

# get rid of annoying MSVC warnings.
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set (CMAKE_CXX_STANDARD 17)
option(USE_VULKAN "Enable GPU implementation via Vulkan" OFF)

# include(cmake/list_dirs.cmake)
# include(cmake/upd_tar_list.cmake)

# common dependecies
#=---------------------------------------------------------
find_package(OpenMP)

include(cmake/CPM.cmake)

CPMAddPackage(NAME LiteMath 
GIT_REPOSITORY "https://github.com/msu-graphics-group/LiteMath"
GIT_TAG 05e7dd386922f46f7e498b38d3fd2a5dd7a1f2f8
DOWNLOAD_ONLY
)
#=---------------------------------------------------------

if(USE_VULKAN)
  find_package(Vulkan)

  if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
  endif()

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

