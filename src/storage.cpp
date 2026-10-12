#include "shivrai/storage.hpp"
#include <stdexcept>
namespace shivrai {
Storage::Storage(const std::string& p) {
  if (sqlite3_open(p.c_str(), &db_) != SQLITE_OK) throw std::runtime_error("sqlite open");
  sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
}
Storage::~Storage() { if (db_) sqlite3_close(db_); }
void Storage::init_schema() {
  const char* s = "CREATE TABLE IF NOT EXISTS kv (tbl TEXT,key TEXT,val TEXT,PRIMARY KEY(tbl,key));"
                  "CREATE TABLE IF NOT EXISTS blocks (height INTEGER PRIMARY KEY, data TEXT);";
  char* e=nullptr;
  if (sqlite3_exec(db_, s, nullptr, nullptr, &e) != SQLITE_OK) {
    std::string m = e?e:"?"; sqlite3_free(e); throw std::runtime_error("schema: "+m);
  }
}
void Storage::put(const std::string& t, const std::string& k, const std::string& v) {
  std::lock_guard<std::mutex> l(mu_);
  sqlite3_stmt* st=nullptr;
  sqlite3_prepare_v2(db_,"INSERT OR REPLACE INTO kv(tbl,key,val) VALUES(?,?,?);",-1,&st,nullptr);
  sqlite3_bind_text(st,1,t.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_text(st,2,k.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_text(st,3,v.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_step(st); sqlite3_finalize(st);
}
void Storage::begin_batch()    { std::lock_guard<std::mutex> l(mu_); sqlite3_exec(db_,"BEGIN;",nullptr,nullptr,nullptr); }
void Storage::commit_batch()   { std::lock_guard<std::mutex> l(mu_); sqlite3_exec(db_,"COMMIT;",nullptr,nullptr,nullptr); }
void Storage::rollback_batch() { std::lock_guard<std::mutex> l(mu_); sqlite3_exec(db_,"ROLLBACK;",nullptr,nullptr,nullptr); }

std::optional<std::string> Storage::get(const std::string& t, const std::string& k) {
  std::lock_guard<std::mutex> l(mu_);
  sqlite3_stmt* st=nullptr;
  sqlite3_prepare_v2(db_,"SELECT val FROM kv WHERE tbl=? AND key=?;",-1,&st,nullptr);
  sqlite3_bind_text(st,1,t.c_str(),-1,SQLITE_TRANSIENT);
  sqlite3_bind_text(st,2,k.c_str(),-1,SQLITE_TRANSIENT);
  std::optional<std::string> o;
  if (sqlite3_step(st)==SQLITE_ROW) o = std::string((const char*)sqlite3_column_text(st,0));
  sqlite3_finalize(st);
  return o;
}
}
