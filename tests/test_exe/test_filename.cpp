#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <string_view>
#include <string>

#if defined(_WIN32)
#	include <windows.h> // GetModuleHandleExW(), GetModuleFileNameW(), FreeLibrary()
#elif defined(__linux__)
#	include <dlfcn.h> // dladdr()
#	include <link.h> // link_map 
#else
#	error "Error: Unknown platform!"
#endif

namespace fs = std::filesystem;

namespace
{
	const std::string suffix = VI_TM_LIB_SUFFIX;

	bool ends_with(std::string_view l, std::string_view r)
	{	return(l.size() >= r.size() && 0 == l.compare(l.size() - r.size(), r.size(), r));
	}

	[[nodiscard]] fs::path get_module_path(const void* addr)
	{	if(!addr)
		{	assert(false && "Address cannot be null");
			return {};
		}

#if defined(_WIN32)
		HMODULE hModule = NULL;
		if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCWSTR>(addr), &hModule) || !hModule)
		{	assert(false);
			return {};
		}

		std::wstring path(MAX_PATH, L'\0');
		DWORD len = 0;
		while((len = GetModuleFileNameW(hModule, path.data(), static_cast<DWORD>(path.size()))) >= path.size())
		{	path.resize(len * 2);
		}

		FreeLibrary(hModule);
		return { path.begin(), next(path.begin(), len) };
#elif defined(__linux__)
		Dl_info info{};
#	if defined(__GLIBC__)
		link_map *lm = nullptr;
		if (!dladdr1(addr, &info, reinterpret_cast<void**>(&lm), RTLD_DL_LINKMAP))
#	else
		if (!dladdr(addr, &info))
#	endif
		{	assert(false && "dladdr failed");
			return {};
		}

		fs::path result;
#	if defined(__GLIBC__)
		if (lm && lm->l_name && lm->l_name[0] != '\0')
		{	return{ lm->l_name };
		}
#	endif
		if (info.dli_fname && info.dli_fname[0] != '\0')
		{	return{ info.dli_fname };
		}
		assert(false && "No valid module path found");
		return {};
#else
#	error "Unsupported platform!"
#endif
	}
} // namespace

TEST(filename, exe)
{	auto module_name = get_module_path(reinterpret_cast<const void *>(&get_module_path));
#ifdef _WIN32
	module_name = module_name.stem();
#endif
	EXPECT_TRUE(ends_with(module_name.string(), suffix)) <<
		"Where module_name: \'" << module_name <<
		"\' and suffix: \'" << suffix << "\'.";
}

#if VI_TM_SHARED
TEST(filename, lib)
{	auto* const flags = vi_tmStaticInfo(vi_tmInfoFlags); // We get a pointer to the data stored in the library.
	const auto module_name = get_module_path(flags).stem();
	EXPECT_TRUE(ends_with(module_name.string(), suffix)) <<
		"Where module_name: \'" << module_name <<
		"\' and suffix: \'" << suffix << "\'.";
}
#endif
