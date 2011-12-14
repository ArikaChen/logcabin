/* Copyright (c) 2011 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <algorithm>
#include <string>
#include <gtest/gtest.h>

#include "Common.h"
#include "DLogStorage.h"
#include "libDLogStorage/MemoryStorageModule.h"

namespace DLog {
namespace Storage {

using std::string;
using std::vector;
using std::deque;

namespace {

class LogAppendCallback : public Log::AppendCallback {
  public:
    void appended(LogEntry entry) {
        lastEntry = entry;
    }
    static LogEntry lastEntry;
};
LogEntry LogAppendCallback::lastEntry {
    0xdeadbeef,
    0xdeadbeef,
    0xdeadbeefdeadbeef,
    Chunk::makeChunk("deadbeef", 9),
    { 0xdeadbeef }
};

class SMDeleteCallback : public StorageModule::DeleteCallback {
  public:
    void deleted(LogId logId) {
        lastLogId = logId;
    }
    static LogId lastLogId;
};
LogId SMDeleteCallback::lastLogId;

template<typename Container>
vector<string>
eStr(const Container& container)
{
    vector<string> ret;
    for (auto it = container.begin();
         it != container.end();
         ++it) {
        ret.push_back(it->toString());
    }
    return ret;
}

vector<LogId>
logIds(const vector<Ref<Log>>& logs)
{
    vector<LogId> ids;
    for (auto it = logs.begin();
         it != logs.end();
         ++it) {
        Ref<Log> log = *it;
        ids.push_back(log->getLogId());
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

} // anonymous namespace

TEST(MemoryLog, constructor) {
    Ref<MemoryLog> log = make<MemoryLog>(92);
    EXPECT_EQ(92U, log->getLogId());
}

TEST(MemoryLog, getLastId) {
    Ref<MemoryLog> log = make<MemoryLog>(92);
    EXPECT_EQ(NO_ENTRY_ID, log->getLastId());
    LogEntry e1(1, 2, 3, Chunk::makeChunk("hello", 6));
    log->append(e1, unique<LogAppendCallback>());
    EXPECT_EQ(0U, log->getLastId());
    log->append(e1, unique<LogAppendCallback>());
    EXPECT_EQ(1U, log->getLastId());
}

TEST(MemoryLog, readFrom) {
    Ref<MemoryLog> log = make<MemoryLog>(92);
    EXPECT_EQ(vector<string>{}, eStr(log->readFrom(0)));
    EXPECT_EQ(vector<string>{}, eStr(log->readFrom(12)));
    LogEntry e1(1, 2, 3, Chunk::makeChunk("hello", 6));
    log->append(e1, unique<LogAppendCallback>());
    LogEntry e2(4, 5, 6, Chunk::makeChunk("world!", 7));
    log->append(e2, unique<LogAppendCallback>());
    EXPECT_EQ((vector<string> {
                "(92, 0) 'hello'",
                "(92, 1) 'world!'",
              }),
              eStr(log->readFrom(0)));
    EXPECT_EQ((vector<string> {
                "(92, 1) 'world!'",
              }),
              eStr(log->readFrom(1)));
    EXPECT_EQ((vector<string> {}),
              eStr(log->readFrom(2)));
}

TEST(MemoryLog, append) {
    Ref<MemoryLog> log = make<MemoryLog>(92);
    LogEntry e1(1, 2, 3, Chunk::makeChunk("hello", 6), {4, 5});
    log->append(e1, unique<LogAppendCallback>());
    EXPECT_EQ(92U, e1.logId);
    EXPECT_EQ(0U, e1.entryId);
    EXPECT_EQ("(92, 0) 'hello' [inv 4, 5]",
              LogAppendCallback::lastEntry.toString());
    LogEntry e2(1, 2, 3, Chunk::makeChunk("goodbye", 8), {4, 5});
    log->append(e2, unique<LogAppendCallback>());
    EXPECT_EQ(1U, e2.entryId);
}

TEST(MemoryStorageModule, getLogs) {
    MemoryStorageModule sm;
    EXPECT_EQ((vector<LogId>{}), logIds(sm.getLogs()));
    sm.createLog(38);
    sm.createLog(755);
    sm.createLog(129);
    EXPECT_EQ((vector<LogId>{38, 129, 755}), logIds(sm.getLogs()));
}

TEST(MemoryStorageModule, createLog) {
    MemoryStorageModule sm;
    Ref<Log> log = sm.createLog(12);
    EXPECT_EQ(12U, log->getLogId());
    EXPECT_EQ((vector<LogId>{12}), logIds(sm.getLogs()));
}

TEST(MemoryStorageModule, deleteLog) {
    SMDeleteCallback::lastLogId = 0;
    MemoryStorageModule sm;
    Ref<Log> log = sm.createLog(12);
    sm.deleteLog(10, unique<SMDeleteCallback>());
    EXPECT_EQ(10U, SMDeleteCallback::lastLogId);
    sm.deleteLog(12, unique<SMDeleteCallback>());
    EXPECT_EQ(12U, SMDeleteCallback::lastLogId);
    EXPECT_EQ((vector<LogId>{}), logIds(sm.getLogs()));
}

} // namespace DLog::Storage
} // namespace DLog
