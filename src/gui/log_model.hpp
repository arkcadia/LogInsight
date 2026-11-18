#pragma once

#include "loginsight/app/analysis_service.hpp"
#include "loginsight/rules/error_burst.hpp"
#include <QAbstractTableModel>
#include <QColor>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <array>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace loginsight::gui {
struct Dataset {
  Dataset() = default;
  Dataset(const Dataset&) = delete;
  Dataset& operator=(const Dataset&) = delete;
  std::vector<app::AnalysisResult> files;
  std::vector<const core::LogEntry*> entries;
  QStringList sources;
  std::array<std::size_t, 7> counts{};
  std::size_t importedFiles{};
  std::size_t malformed{};
  QString diagnostics;
  std::vector<rules::ErrorBurst> bursts;
};
// Construct completely in the worker, then share immutable results with UI/export.
std::shared_ptr<const Dataset> load_files(const QStringList& paths, std::stop_token stop = {},
                                          const std::function<void(int)>& progress = {},
                                          parsers::JsonParserConfig config = {},
                                          const rules::BurstRule& rule = {});
QColor level_color(core::LogLevel level);

class LogModel final : public QAbstractTableModel {
public:
  explicit LogModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}
  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  void setDataset(std::shared_ptr<const Dataset> dataset);
  const core::LogEntry* entry(int row) const;

private:
  std::shared_ptr<const Dataset> dataset_;
};

class LogFilter final : public QSortFilterProxyModel {
public:
  explicit LogFilter(QObject* parent = nullptr);
  void setSearch(QString search);
  void setSeverity(int level);
  void setSource(QString source);
  void setBurst(std::span<const core::LogEntry* const> burst, const QString& description = {});
  void setCorrelation(std::string key, std::string value);
  [[nodiscard]] core::LogAttributes activeFilters() const;
  void setTimeRange(std::optional<core::LogTimestamp> from,
                    std::optional<core::LogTimestamp> through);

protected:
  bool filterAcceptsRow(int row, const QModelIndex& parent) const override;

private:
  QString search_;
  QString source_;
  int level_{-1};
  std::set<const core::LogEntry*> burst_;
  QString burstDescription_;
  std::string correlationKey_;
  std::string correlationId_;
  std::optional<core::LogTimestamp> from_;
  std::optional<core::LogTimestamp> through_;
};
} // namespace loginsight::gui
