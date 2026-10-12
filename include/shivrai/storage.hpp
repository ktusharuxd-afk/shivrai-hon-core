#pragma once
#include <string>
#include <optional>
#include <sqlite3.h>
#include <mutex>
namespace shivrai {
class Storage {
public:
  explicit Storage(const std::string&);
  ~Storage();
  void init_schema();
  void put(const std::string&, const std::string&, const std::string&);
  std::optional<std::string> get(const std::string&, const std::string&);
  void begin_batch();
  void commit_batch();
  void rollback_batch();
private:
  sqlite3* db_=nullptr;
  std::mutex mu_;
};
}
