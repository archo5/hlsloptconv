
#include "../compiler.hpp"

using namespace HOC;

struct ArgParser
{
	bool FlagArg(int& i, const char* shortArg, const char* longArg)
	{
		const char* curArg = argv[i];
		if (curArg[0] != '-')
		{
			return false;
		}
		if (strcmp(curArg + 1, shortArg) == 0)
		{
			return true;
		}
		if (curArg[1] == '-' && strcmp(curArg + 2, longArg) == 0)
		{
			return true;
		}
		return false;
	}
	const char* ValueArg(int& i, const char* shortArg, const char* longArg)
	{
		const char* curArg = argv[i];
		if (curArg[0] != '-')
		{
			return nullptr;
		}
		if (i + 1 < argc && strcmp(curArg + 1, shortArg) == 0)
		{
			return argv[++i];
		}
		size_t len = strlen(longArg);
		if (curArg[1] == '-' &&
			strncmp(curArg + 2, longArg, len) == 0 &&
			curArg[2 + len] == '=' &&
			curArg[3 + len] != '\0')
		{
			return curArg + 3 + len;
		}
		return nullptr;
	}

	int argc;
	char** argv;
};

static const char* OUTPUT_FORMATS[] =
{
	"hlsl_sm3",
	"hlsl_sm4",
	"glsl_140",
	"glsl_es_100",
};
#define NUM_OUTPUT_FORMATS (sizeof(OUTPUT_FORMATS)/sizeof(OUTPUT_FORMATS[0]))
static OutputShaderFormat StringToOutputFmt(const char* str)
{
	for (size_t i = 0; i < NUM_OUTPUT_FORMATS; ++i)
	{
		if (strcmp(OUTPUT_FORMATS[i], str) == 0)
			return (OutputShaderFormat) i;
	}
	fprintf(stderr, "error: output format string not recognized - '%s'\n", str);
	fprintf(stderr, "available options:\n");
	for (size_t i = 0; i < NUM_OUTPUT_FORMATS; ++i)
	{
		fprintf(stderr, "  - %s\n", OUTPUT_FORMATS[i]);
	}
	exit(1);
}

static ShaderStage StringToShaderStage(const char* str)
{
	size_t n = strlen(str);
	if (strncmp(str, "vertex", n) == 0)
		return ShaderStage_Vertex;
	if (strncmp(str, "pixel", n) == 0)
		return ShaderStage_Pixel;
	fprintf(stderr, "error: shader stage string not recognized - '%s'\n", str);
	fprintf(stderr, "available options:\n");
	fprintf(stderr, "  - vertex\n");
	fprintf(stderr, "  - pixel\n");
	exit(1);
}

static uint32_t FindOutputFlagByName(const char* str)
{
	if (!strcmp(str, "specify-registers"))
		return HOC_OF_SPECIFY_REGISTERS;
	if (!strcmp(str, "hlsl3-buffer-slots"))
		return HOC_OF_HLSL3_BUFFER_SLOTS;
	if (!strcmp(str, "glsl-rename-psoutput"))
		return HOC_OF_GLSL_RENAME_PSOUTPUT;
	if (!strcmp(str, "glsl-rename-samplers"))
		return HOC_OF_GLSL_RENAME_SAMPLERS;
	if (!strcmp(str, "glsl-rename-cbuffers"))
		return HOC_OF_GLSL_RENAME_CBUFFERS;
	return 0;
}

static void PrintHelp()
{
	fprintf(stderr, "-- HLSL Optimizing Converter v0.7 --\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "usage: hlsloptconv [options] <source>\n");
	fprintf(stderr, "  options:\n");
	fprintf(stderr, "    -h, --help        - show this help screen\n");
	fprintf(stderr, "    -o, --output      - name of output file (default='<input>.<fmt>.<fmt[4]>')\n");
	fprintf(stderr, "    -P, --stdout      - write code to output\n");
	fprintf(stderr, "    -D<name>[=<val>]  - add a preprocessor define\n");
	fprintf(stderr, "    -e, --entrypoint  - name of shader entry point function (default='main')\n");
	fprintf(stderr, "    -f, --format      - output format (required, see options below)\n");
	fprintf(stderr, "    -s, --stage       - shader stage (required, see options below)\n");
	fprintf(stderr, "    -x, --transform   - apply code transformation (see options below)\n");
	fprintf(stderr, "    -d, --dump        - dump AST before/after modifications\n");
	fprintf(stderr, "    -f<name>          - enable a build flag\n");
	fprintf(stderr, "    -fno-<name>       - disable a build flag\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  supported shader stages: vertex, pixel\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  available output formats:\n");
	for (size_t i = 0; i < NUM_OUTPUT_FORMATS; ++i)
	{
		fprintf(stderr, "    - %s\n", OUTPUT_FORMATS[i]);
	}
	fprintf(stderr, "\n");
	fprintf(stderr, "  available code transformations:\n");
	fprintf(stderr, "    - cstr            - convert to a C string\n");
	fprintf(stderr, "    - jsstr           - convert to a JavaScript string\n");
	fprintf(stderr, "  available build flags:\n");
	fprintf(stderr, "    - specify-registers (default: off)\n");
	fprintf(stderr, "     pick and export the registers of unassigned I/O variables\n");
	fprintf(stderr, "    - hlsl3-buffer-slots (default: off)\n");
	fprintf(stderr, "     interpret buffer registers as slot offsets and apply them\n");
	fprintf(stderr, "    - glsl-rename-psoutput (default: on)\n");
	fprintf(stderr, "     rename pixel shader outputs to PSCOLOR# for easier binding\n");
	fprintf(stderr, "    - glsl-rename-samplers (default: on)\n");
	fprintf(stderr, "     rename texture samplers to SAMPLER# for easier binding\n");
	fprintf(stderr, "    - glsl-rename-cbuffers (default: on)\n");
	fprintf(stderr, "     rename constant buffers to CBUF# for easier binding\n");
}

static void Stringify(String& out, const String& in, bool jsconcat)
{
	out = "\"";
	// jsconcat = add '+' after each line except the last
	const char* nlstr = jsconcat ? "\\n\"+\n\"" : "\\n\"\n\"";
	for (auto ch : in)
	{
		if (ch == '\n')
			out += nlstr;
		else
			out += ch;
	}
	out += "\"";
}

int main(int argc, char** argv)
{
	ArgParser ap = { argc, argv };
	HOC_Config cfg;
	Array<ShaderMacro> macros;
	String genCode;
	const char* inputFileName = nullptr;
	const char* outputFileName = nullptr;
	const char* xform = "none";
	const char* usedFmtString = nullptr;
	bool hasStage = false;
	bool codeToStdout = false;

	HOC_TextOutput toStdout = { &HOC_WriteStr_FILE, stdout };
	HOC_TextOutput toCode = { &HOC_WriteStr_String<String>, &genCode };
	cfg.codeOutputStream = &toCode;

	for (int i = 0; i < argc; ++i)
	{
		if (ap.FlagArg(i, "h", "help"))
		{
			PrintHelp();
			return 0;
		}
		else if (const char* outf = ap.ValueArg(i, "o", "output"))
		{
			outputFileName = outf;
		}
		else if (ap.FlagArg(i, "P", "stdout"))
		{
			codeToStdout = true;
		}
		else if (strncmp(argv[i], "-D", 2) == 0 && argv[i][2])
		{
			macros.push_back({ argv[i] + 2, nullptr });
		}
		else if (const char* ep = ap.ValueArg(i, "e", "entrypoint"))
		{
			cfg.entryPoint = ep;
		}
		else if (const char* fmt = ap.ValueArg(i, "f", "format"))
		{
			usedFmtString = fmt;
			cfg.outputFmt = StringToOutputFmt(fmt);
		}
		else if (const char* stage = ap.ValueArg(i, "s", "stage"))
		{
			cfg.stage = StringToShaderStage(stage);
			hasStage = true;
		}
		else if (const char* xf = ap.ValueArg(i, "x", "transform"))
		{
			if (strcmp(xf, "cstr") == 0 || strcmp(xf, "jsstr") == 0)
			{
				xform = xf;
			}
			else
			{
				fprintf(stderr, "error: transform type not recognized - '%s'\n", xf);
				fprintf(stderr, "available options:\n");
				fprintf(stderr, "  - cstr\n");
				fprintf(stderr, "  - jsstr\n");
				return 1;
			}
		}
		else if (ap.FlagArg(i, "d", "dump"))
		{
			cfg.ASTDumpStream = &toStdout;
		}
		else if (strncmp(argv[i], STRLIT_SIZE("-f")) == 0)
		{
			bool off = strncmp(argv[i], STRLIT_SIZE("-fno-")) == 0;
			const char* rest = argv[i] + 2;
			if (off)
				rest += 3;
			uint32_t flag = FindOutputFlagByName(rest);
			if (!flag)
			{
				fprintf(stderr, "error: build flag not recognized - '%s'\n", rest);
				return 1;
			}
			if (off)
				cfg.outputFlags &= ~flag;
			else
				cfg.outputFlags |= flag;
		}
		else
		{
			inputFileName = argv[i];
		}
	}

	if (!inputFileName)
	{
		fprintf(stderr, "error: input file name not specified\n\n");
		PrintHelp();
		return 1;
	}
	if (!usedFmtString)
	{
		fprintf(stderr, "error: output format (-f, --format) not specified\n\n");
		PrintHelp();
		return 1;
	}
	if (!hasStage)
	{
		fprintf(stderr, "error: shader stage (-s, --stage) not specified\n\n");
		PrintHelp();
		return 1;
	}

	if (!macros.empty())
	{
		macros.push_back({ nullptr, nullptr });
		cfg.defines = macros.data();
	}

	String inCode = GetFileContents(inputFileName, true);
	if (!HOC_CompileShader(inputFileName, inCode.c_str(), &cfg))
	{
		fprintf(stderr, "compilation failed, no output generated\n");
		return 1;
	}

	String outCode;
	if (strcmp(xform, "cstr") == 0)
	{
		Stringify(outCode, genCode, false);
	}
	else if (strcmp(xform, "jsstr") == 0)
	{
		Stringify(outCode, genCode, true);
	}
	else
	{
		outCode = genCode;
	}

	if (codeToStdout)
	{
		fprintf(stdout, "%s", outCode.c_str());
	}
	if (!outputFileName && !codeToStdout)
	{
		char path[8300];
		strncpy(path, inputFileName, 8192);
		strcat(path, usedFmtString);
		strncat(path, usedFmtString, 4);
		SetFileContents(path, outCode, true);
	}
	if (outputFileName)
	{
		SetFileContents(outputFileName, outCode, true);
	}
	return 0;
}
