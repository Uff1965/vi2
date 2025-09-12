#ifndef VI2_TESTS_TEST_H_
#	define VI2_TESTS_TEST_H_
#	pragma once

#include <vi_timing/vi_timing.hpp>

#include <gtest/gtest.h>
#include <memory>

class ViTimingJournalFixture : public ::testing::Test
{   using unique_journal_t = std::unique_ptr<std::remove_pointer_t<VI_TM_HJOUR>, decltype(&vi_tmJournalClose)>;
	unique_journal_t journal_{ nullptr, vi_tmJournalClose };
protected:
	void SetUp() override
	{   journal_.reset(vi_tmJournalCreate());
		ASSERT_NE(journal_, nullptr) << "vi_tmJournalCreate should return a valid descriptor";
	}
	auto journal() const noexcept { return journal_.get(); }
};

#endif // VI2_TESTS_TEST_H_
