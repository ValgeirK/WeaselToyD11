#pragma once

#define MAX_RESORCES 4
#define SIZE_OF_FLOAT 4
#define SIZE_OF_INT 4

#define DEFAULT_PROJECT_NAME "NewProject"

#define PROJECT_PATH "ShaderToyLibrary/"
#define PROJECT_PATH_W L"ShaderToyLibrary/"

#define PROJECT_PATH_DOUBLE_SLASH "ShaderToyLibrary\\"

#define MEMORY_ALIGNINT( val, alignment )       (size_t)((~(alignment-1))&(((size_t)val)+(alignment-1)))