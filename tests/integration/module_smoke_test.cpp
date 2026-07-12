#include "loginsight/alerts/alerts_module.hpp"
#include "loginsight/parsers/parser_module.hpp"
#include "loginsight/rules/rules_module.hpp"
#include "loginsight/storage/storage_module.hpp"

#include <gtest/gtest.h>

TEST(ModuleSmokeTest, ModulesExposeNames) {
  EXPECT_EQ(loginsight::alerts::module_name(), "alerts");
  EXPECT_EQ(loginsight::parsers::module_name(), "parsers");
  EXPECT_EQ(loginsight::rules::module_name(), "rules");
  EXPECT_EQ(loginsight::storage::module_name(), "storage");
}
