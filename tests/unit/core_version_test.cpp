#include "loginsight/core/version.hpp"

#include <gtest/gtest.h>

TEST(CoreVersionTest, ExposesProjectMetadata) {
  EXPECT_EQ(loginsight::core::project_name(), "LogInsight");
  EXPECT_FALSE(loginsight::core::version().empty());
}
