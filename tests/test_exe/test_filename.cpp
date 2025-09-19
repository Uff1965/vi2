// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <string_view>
#include <string>

namespace fs = std::filesystem;

#if defined(_WIN32)
#	include <windows.h>
#elif defined(__APPLE__)
#	include <mach-o/dyld.h>
#elif defined(__linux__)
#	include <dlfcn.h>
#	include <limits.h>
#	include <unistd.h>
#endif

namespace
{
	const std::string suffix = VI_TM_SUFFIX;
	void dummy() {}

	bool ends_with(std::string_view l, std::string_view r)
	{	return(l.size() >= r.size() && 0 == l.compare(l.size() - r.size(), r.size(), r));
	}
}

namespace platform
{
	fs::path get_module_path(const void* addr)
	{
		std::string result;

#if defined(_WIN32)
		if (HMODULE hModule = nullptr; GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCTSTR>(addr), &hModule))
		{	assert(hModule);
			result.resize(MAX_PATH);
			DWORD len = 0;
			while((len = GetModuleFileNameA(hModule, result.data(), static_cast<DWORD>(result.size()))))
			{	if (len < result.size())
				{	break;
				}

				result.resize(len * 2);
			}
			result.resize(len);
			FreeLibrary(hModule);
		}
#elif defined(__linux__)
		Dl_info info{};
		if (dladdr(addr, &info)) {
			result = info.dli_fname;
#else
#	error "Error: Unknown platform!"
#endif
		return { result };
	}

	std::string filename(fs::path path)
	{	return path.filename().string();
	}
}

TEST(filename, exe)
{	auto name = platform::get_module_path(&dummy).stem().string();
	EXPECT_TRUE(ends_with(name, suffix));
}

#if VI_TM_SHARED
TEST(filename, lib)
{	auto name = platform::get_module_path(&vi_tmStaticInfo).stem().string();
	EXPECT_TRUE(ends_with(name, suffix));
}
#endif
