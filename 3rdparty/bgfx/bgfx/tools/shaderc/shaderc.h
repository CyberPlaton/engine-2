/*
 * Copyright 2011-2024 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#ifndef SHADERC_H_HEADER_GUARD
#define SHADERC_H_HEADER_GUARD

namespace bgfx
{
	extern bool g_verbose;
}

#ifndef SHADERC_CONFIG_HLSL
#	define SHADERC_CONFIG_HLSL BX_PLATFORM_WINDOWS
#endif // SHADERC_CONFIG_HLSL

#include <bx/bx.h>
#include <bx/platform.h>
#include <bx/debug.h>
#include <bx/commandline.h>
#include <bx/endian.h>
#include <bx/uint32_t.h>
#include <bx/string.h>
#include <bx/hash.h>
#include <bx/file.h>
#include "../../src/vertexlayout.h"

#include <string.h>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>

namespace bgfx
{
	extern bool g_verbose;

	bx::StringView nextWord(bx::StringView& _parse);

	static constexpr uint8_t cUniformFragmentBit = 0x10;
	static constexpr uint8_t cUniformSamplerBit = 0x20;
	static constexpr uint8_t cUniformReadOnlyBit = 0x40;
	static constexpr uint8_t cUniformCompareBit = 0x80;
	static constexpr uint8_t cUniformMask = 0
		| cUniformFragmentBit
		| cUniformSamplerBit
		| cUniformReadOnlyBit
		| cUniformCompareBit
		;

	struct Uniform
	{
		Uniform()
			: type(UniformType::Count)
			, num(0)
			, regIndex(0)
			, regCount(0)
			, texComponent(0)
			, texDimension(0)
			, texFormat(0)
		{
		}

		std::string name;
		UniformType::Enum type;
		uint8_t num;
		uint16_t regIndex;
		uint16_t regCount;
		uint8_t texComponent;
		uint8_t texDimension;
		uint16_t texFormat;
	};

	struct Options
	{
		Options();

		void dump();

		char shaderType;
		std::string platform;
		std::string profile;

		std::string	inputFilePath;
		std::string	outputFilePath;

		std::vector<std::string> includeDirs;
		std::vector<std::string> defines;
		std::vector<std::string> dependencies;

		bool disasm;
		bool raw;
		bool preprocessOnly;
		bool depends;

		bool debugInformation;

		bool avoidFlowControl;
		bool noPreshader;
		bool partialPrecision;
		bool preferFlowControl;
		bool backwardsCompatibility;
		bool warningsAreErrors;
		bool keepIntermediate;

		bool optimize;
		uint32_t optimizationLevel;
	};

	typedef std::vector<Uniform> UniformArray;

	void printCode(const char* _code, int32_t _line = 0, int32_t _start = 0, int32_t _end = INT32_MAX, int32_t _column = -1);
	void strReplace(char* _str, const char* _find, const char* _replace);
	int32_t writef(bx::WriterI* _writer, const char* _format, ...);
	void writeFile(const char* _filePath, const void* _data, int32_t _size);

	bool compileGLSLShader(const Options& _options, uint32_t _version, const std::string& _code, bx::WriterI* _writer, bx::WriterI* _messages);
	bool compileHLSLShader(const Options& _options, uint32_t _version, const std::string& _code, bx::WriterI* _writer, bx::WriterI* _messages);
	bool compileMetalShader(const Options& _options, uint32_t _version, const std::string& _code, bx::WriterI* _writer, bx::WriterI* _messages);
	bool compilePSSLShader(const Options& _options, uint32_t _version, const std::string& _code, bx::WriterI* _writer, bx::WriterI* _messages);
	bool compileSPIRVShader(const Options& _options, uint32_t _version, const std::string& _code, bx::WriterI* _writer, bx::WriterI* _messages);
	bool compileShader(const char* _varying, const char* _comment, char* _shader, uint32_t _shaderLen, bool _shaderOwning, const Options& _options,
		bx::WriterI* _shaderWriter, bx::WriterI* _messageWriter);
	const char* getPsslPreamble();

} // namespace bgfx

#endif // SHADERC_H_HEADER_GUARD
