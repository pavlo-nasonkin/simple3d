
find_path(GLM_INCLUDE_DIR glm/glm.hpp
        PATH_SUFFIXES include
        PATHS ${PROJECT_SOURCE_DIR}/include/)
message("Running FindGLM")


SET(GLM_FOUND "NO")
IF(GLM_INCLUDE_DIRS)
    SET(GLM_FOUND "YES")
ENDIF(GLM_INCLUDE_DIRS)