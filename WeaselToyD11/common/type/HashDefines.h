#pragma once

typedef long HRESULT;
typedef unsigned int UINT;

#define MAX_RESORCESCHANNELS	(4)
#define QUAD_VERTICE_NUMB		(4)
#define QUAD_INDICE_NUMB		(6)

#define DEFAULT_PROJECT_NAME "NewProject"

#define PROJECT_PATH "ShaderToyLibrary/"
#define PROJECT_PATH_W L"ShaderToyLibrary/"

#define PROJECT_PATH_DOUBLE_SLASH "ShaderToyLibrary\\"

#define WEASEL_CAPTURE "/renderdoc/WeaseltoyD11"

#define PS_SHADER_COMPILE_VER "ps_5_0"
#define VS_SHADER_COMPILE_VER "vs_5_0"

#define MEMORY_ALIGNINT( val, alignment )       (size_t)((~(alignment-1))&(((size_t)val)+(alignment-1)))

#define MASK_RELOAD_SHADERS		(1 << 0)
#define MASK_RELOAD_TEXTURES	(1 << 1)

#define FLOAT4_SIZE				(sizeof(float) * 4)

#define MAX_PATH_LENGTH         (260)


#define D3D_VERIFY(func)					\
	HRESULT hr = func;						\
	assert(hr == S_OK);						


#define SAFE_RELEASE(ptr)					\
	if (ptr)								\
	{										\
		UINT RefCount = ptr->Release();		\
		assert(RefCount == 0);				\
		ptr = nullptr;						\
	}
