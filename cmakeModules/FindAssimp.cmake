
find_path(ASSIMP_INCLUDE_DIR assimp/Importer.hpp
        PATH_SUFFIXES include
        PATHS ${PROJECT_SOURCE_DIR}/include/)
message("Running FindAssimp")

IF(APPLE)
    find_library(ASSIMP_LIBRARY
            NAMES assimp
            PATHS ${PROJECT_SOURCE_DIR}/libs/assimp/osx)
ELSE(APPLE) #Windows PS
    find_library(ASSIMP_LIBRARY
            NAMES libassimp.dll.a assimp
            PATHS ${PROJECT_SOURCE_DIR}/libs/assimp)
ENDIF(APPLE)


SET(ASSIMP_FOUND "NO")
IF(ASSIMP_LIBRARIES AND ASSIMP_INCLUDE_DIRS)
    SET(ASSIMP_FOUND "YES")
ENDIF(ASSIMP_LIBRARIES AND ASSIMP_INCLUDE_DIRS)
