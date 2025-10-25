#ifndef VI2_TESTS_TEST_H_
#	define VI2_TESTS_TEST_H_
#	pragma once

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>
#include <memory>

class ViTimingRegistryFixture : public ::testing::Test
{   using unique_registryl_t = std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmRegistryClose)>;
	unique_registryl_t registry_{ nullptr, vi_tmRegistryClose };
protected:
	void SetUp() override
	{   registry_.reset(vi_tmRegistryCreate());
		ASSERT_NE(registry_, nullptr) << "vi_tmRegistryCreate should return a valid descriptor";
	}
	auto registry() const noexcept { return registry_.get(); }
};

#endif // VI2_TESTS_TEST_H_
