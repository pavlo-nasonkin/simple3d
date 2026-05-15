cmake_minimum_required(VERSION 3.15)
project(simple3d)

set(CMAKE_CXX_STANDARD 20)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmakeModules")

# Place the executable next to the source tree's "models", "shaders" etc. resources.
# Also ensures the DLLs vcpkg auto-copies land beside the .exe.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(SOURCE_FILES
        src/camera/Camera.cpp
        src/camera/Camera.h
        src/camera/FirstPersonCamera.h
        src/camera/FreeLookCamera.cpp
        src/camera/FreeLookCamera.h
        src/events/EventDispatcher.cpp
        src/events/EventDispatcher.h
        src/events/IKeyboardListener.h
        src/events/IMouseListener.h
        src/events/IObjectSelectorListener.h
        src/events/IUpdateListener.h
        src/input/KeyboardInput.cpp
        src/input/KeyboardInput.h
        src/input/MouseInput.cpp
        src/input/MouseInput.h
        src/materials/ColorMaterial.cpp
        src/materials/ColorMaterial.h
        src/materials/MaterialBase.cpp
        src/materials/MaterialBase.h
        src/materials/ObjectIdMaterial.cpp
        src/materials/ObjectIdMaterial.h
        src/materials/ShaderFactory.cpp
        src/materials/ShaderFactory.h
        src/materials/SkinnedMaterial3D.cpp
        src/materials/SkinnedMaterial3D.h
        src/materials/Filter3d.h
        src/materials/Filter3d.cpp
        src/materials/filters/ColorFilter.h
        src/materials/filters/ColorFilter.cpp
        src/materials/filters/TextureMapFilter.cpp
        src/materials/filters/TextureMapFilter.h
        src/object_selector/ObjectSelector.cpp
        src/object_selector/ObjectSelector.h
        src/render/RenderModeHelper.cpp
        src/render/RenderModeHelper.h
        src/resources/Texture2D.cpp
        src/resources/Texture2D.h
        src/resources/TextureManager.cpp
        src/resources/TextureManager.h
        src/utils/FileUtils.cpp
        src/utils/FileUtils.h
        src/utils/Math.h
        src/utils/Math3d.cpp
        src/utils/Math3d.h
        src/utils/StringUtils.h
        src/utils/StringUtils.cpp
        src/BoxModel.cpp
        src/BoxModel.h
        src/Device3D.cpp
        src/Device3D.h
        src/Engine.cpp
        src/Engine.h
        src/ExternalModel.cpp
        src/ExternalModel.h
        src/GLUtils.cpp
        src/GLUtils.h
        src/Material3D.cpp
        src/Material3D.h
        src/Mesh.cpp
        src/Mesh.h
        src/Model.cpp
        src/Model.h
        src/Pivot3D.cpp
        src/Pivot3D.h
        src/Scene3D.cpp
        src/Scene3D.h
        src/Shader.cpp
        src/Shader.h
        src/UpdateBroadcaster.cpp
        src/UpdateBroadcaster.h
        src/GLEWImporter.h
        GLFWKeyboardInput.cpp
        GLFWKeyboardInput.h
        GLFWMouseInput.cpp
        GLFWMouseInput.h
        include/GL/glew.c
        main.cpp)

set_source_files_properties(
        ${PROJECT_SOURCE_DIR}/include/GL/glew.c
        PROPERTIES LANGUAGE C
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(OpenGL REQUIRED)

# glfw3 + assimp come from vcpkg via the vcpkg toolchain file.
# Configure with: -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

# SOIL2 lives in libs/SOIL/ and is resolved by cmakeModules/FindSOIL.cmake.
find_package(SOIL REQUIRED)

# GLM is header-only and ships in include/glm; no library to link.

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        assimp::assimp
        ${SOIL2_LIBRARY}
        ${OPENGL_gl_LIBRARY})

if(APPLE)
    message("Linking CoreFoundation")
    target_link_libraries(${PROJECT_NAME} PRIVATE
            "-framework CoreFoundation"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo")
endif(APPLE)

# Copy the SOIL2 DLL next to the .exe (vcpkg handles its own DLLs automatically).
if(WIN32)
    set(_soil2_dll_release "${CMAKE_CURRENT_SOURCE_DIR}/libs/SOIL/soil2.dll")
    set(_soil2_dll_debug   "${CMAKE_CURRENT_SOURCE_DIR}/libs/SOIL/soil2-debug.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<IF:$<CONFIG:Debug>,${_soil2_dll_debug},${_soil2_dll_release}>"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
endif()