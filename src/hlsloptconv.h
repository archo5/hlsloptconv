

#pragma once
#ifndef HOC_HEADER_NO_STDIO
#  include <stdio.h>
#endif
#include <inttypes.h>


#ifdef __cplusplus
#define HOC_APIFUNC extern "C"
#else
#define HOC_APIFUNC
#endif


#ifdef HOC_INTERNAL
#  define HOC_(x) x
#else
#  define HOC_(x) HOC_##x
#endif

typedef uint8_t HOC_BoolU8;
#define HOC_TRUE 1
#define HOC_FALSE 0


enum HOC_ShaderStage
{
	HOC_(ShaderStage_Vertex),
	HOC_(ShaderStage_Pixel),
};

enum HOC_OutputShaderFormat
{
	HOC_(OSF_HLSL_SM3),
	HOC_(OSF_HLSL_SM4),
	HOC_(OSF_GLSL_140),
	HOC_(OSF_GLSL_ES_100),
};

struct HOC_ShaderMacro
{
	const char* name;
	const char* value;
};

enum HOC_ShaderVarType
{
	HOC_(SVT_StructBegin)       = 1, /* nested structs are possible */
	HOC_(SVT_StructEnd)         = 2,
	HOC_(SVT_Uniform)           = 3, /* only numeric SDT_* types, if inside block - register quad=/4, comp=%4 */
	HOC_(SVT_UniformBlockBegin) = 4, /* there cannot be any nesting */
	HOC_(SVT_UniformBlockEnd)   = 5,
	HOC_(SVT_VSInput)           = 6, /* only numeric SDT_* types */
	HOC_(SVT_Sampler)           = 7, /* only SDT_Sampler* types */
	HOC_(SVT_PSOutputDepth)     = 8, /* only scalar types */
	HOC_(SVT_PSOutputColor)     = 9, /* only vector types */
};

enum HOC_ShaderDataType
{
	HOC_(SDT_None)    = 0,
	HOC_(SDT_Bool)    = 1,
	HOC_(SDT_Int32)   = 2,
	HOC_(SDT_UInt32)  = 3,
	HOC_(SDT_Float16) = 4,
	HOC_(SDT_Float32) = 5,

	HOC_(SDT_Sampler1D)       = 20,
	HOC_(SDT_Sampler2D)       = 21,
	HOC_(SDT_Sampler3D)       = 22,
	HOC_(SDT_SamplerCube)     = 23,
	HOC_(SDT_Sampler1DComp)   = 24,
	HOC_(SDT_Sampler2DComp)   = 25,
	HOC_(SDT_SamplerCubeComp) = 26,
};

struct HOC_ShaderVariable /* 20 bytes */
{
	uint32_t name;      /* offset from beginning of string buffer */
	uint32_t semantic;  /* offset from beginning of string buffer; only for VS input */
	/*       ^ for uniforms @ HLSL3/D3D9 - the assigned shader slot or 0xffffffff if none is assigned */
	int32_t  regSemIdx; /* register number/semantic index, -1 when not determined by the shader */
	uint32_t arraySize; /* 0 if no array */
	uint8_t  svType;    /* ShaderVarType */
	uint8_t  dataType;  /* ShaderDataType */
	uint8_t  sizeX;     /* 0 if scalar, 1-4 for vector/matrix */
	uint8_t  sizeY;     /* 0 if scalar/vector, 1-4 for matrix */
};


/* loads include file
- file is the exact string in #include
  requester is the exact string used to load file that contains #include
- called with file=NULL to free memory pointed to by *outbuf
- return nonzero for success, zero for failure */
typedef int(*HOC_LoadIncludeFilePFN)(const char* file, const char* requester, char** outbuf, void* userData);

/* writes output data
- any non-fatal errors should be caught internally and ignored until the end of the build step */
typedef void(*HOC_WriteStr)(const char* str, size_t size, void* userData);


#ifdef __cplusplus
template<class Str> inline void HOC_WriteStr_String(const char* str, size_t size, void* userData)
{
	Str* S = (Str*) userData;
	S->append(str, size);
}
#endif

#ifndef HOC_HEADER_NO_STDIO
static void HOC_WriteStr_FILE(const char* str, size_t size, void* userData)
{
	fwrite(str, size, 1, (FILE*) userData);
}
#endif

struct HOC_TextOutput
{
	HOC_WriteStr func;
	void*        userData;
};

struct HOC_InterfaceOutput
{
#ifdef __cplusplus
	HOC_InterfaceOutput()
	{
		outVarBuf = NULL;
		outVarBufSize = 0;
		outVarStrBuf = NULL;
		outVarStrBufSize = 0;
		overflowAlloc = HOC_TRUE;
		didOverflowVar = HOC_FALSE;
		didOverflowStr = HOC_FALSE;
	}
#endif

	HOC_ShaderVariable* outVarBuf;
	size_t outVarBufSize;     /* changed to output data size after compilation */
	char* outVarStrBuf;       /* string buffer for names/semantics in HOC_ShaderVariable array */
	size_t outVarStrBufSize;  /* changed to output data size after compilation */
	HOC_BoolU8 overflowAlloc;
	HOC_BoolU8 didOverflowVar;
	HOC_BoolU8 didOverflowStr;
};

#define HOC_OF_GLSL_RENAME_PSOUT 0x0001 /* rename PS outputs to PSCOLOR# for easier binding */
#define HOC_OF_GLSL_RENAME_CBUFS 0x0002 /* rename constant buffers to CBUF# for easier binding */
#define HOC_OF_LOCK_UNIFORM_POS  0x0004 /* pick and export the registers of unassigned uniforms */

struct HOC_Config
{
#ifdef __cplusplus
	HOC_Config()
	{
		entryPoint = "main";
		stage = HOC_(ShaderStage_Vertex);
		outputFmt = HOC_(OSF_HLSL_SM3);
		outputFlags =
			HOC_OF_GLSL_RENAME_PSOUT |
			HOC_OF_GLSL_RENAME_CBUFS;
		loadIncludeFileFunc = NULL;
		loadIncludeFileUserData = NULL;
		defines = NULL;
		errorOutputStream = NULL;
		codeOutputStream = NULL;
		ASTDumpStream = NULL;
		interfaceOutput = NULL;
	}
#endif

	const char*            entryPoint;
	uint8_t                stage;       /* HOC_ShaderStage */
	uint8_t                outputFmt;   /* HOC_OutputShaderFormat */
	uint32_t               outputFlags; /* HOC_OF_* */

	/* preprocessor */
	HOC_LoadIncludeFilePFN loadIncludeFileFunc;
	void*                  loadIncludeFileUserData;
	HOC_ShaderMacro*       defines;

	HOC_TextOutput*        errorOutputStream; /* stderr output if null */
	HOC_TextOutput*        codeOutputStream;  /* stdout output if null */
	HOC_TextOutput*        ASTDumpStream;     /* no output if null */

	HOC_InterfaceOutput*   interfaceOutput;
};


HOC_APIFUNC HOC_BoolU8 HOC_CompileShader(const char* name, const char* code, HOC_Config* config);
HOC_APIFUNC void HOC_FreeInterfaceOutputBuffers(HOC_InterfaceOutput* ifo);

