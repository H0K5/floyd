// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef FLOYD_SRC_FLOYD_IMPL_H_
#define FLOYD_SRC_FLOYD_IMPL_H_

#include <string>
#include <vector>
#include <utility>
#include <map>

#include "slash/include/slash_mutex.h"
#include "slash/include/slash_status.h"
#include "pink/include/bg_thread.h"

#include "floyd/include/floyd.h"
#include "floyd/include/floyd_options.h"
#include "floyd/src/raft_log.h"

namespace floyd {
using slash::Status;

class Log;
class ClientPool;
class RaftMeta;
class Peer;
class FloydPrimary;
class FloydApply;
class FloydWorker;
class FloydWorkerConn;
class FloydContext;
class Logger;
class CmdRequest;
class CmdResponse;
class CmdResponse_ServerStatus;

typedef std::map<std::string, Peer*> PeersSet;

class FloydImpl : public Floyd  {
 public:
  explicit FloydImpl(const Options& options);
  virtual ~FloydImpl();

  Status Init();

  virtual Status Write(const std::string& key, const std::string& value) override;
  virtual Status DirtyWrite(const std::string& key, const std::string& value) override;
  virtual Status Delete(const std::string& key) override;
  virtual Status Read(const std::string& key, std::string* value) override;
  virtual Status DirtyRead(const std::string& key, std::string* value) override;
  virtual Status AddServer(const std::string& new_server) override;

  // return true if leader has been elected
  virtual bool GetLeader(std::string* ip_port);
  virtual bool GetLeader(std::string* ip, int* port);
  virtual bool HasLeader();
  virtual bool GetAllNodes(std::vector<std::string>* nodes);
  virtual bool IsLeader();

  virtual bool GetServerStatus(std::string* msg);
  // log level can be modified
  virtual void set_log_level(const int log_level);

 private:
  // friend class Floyd;
  friend class FloydWorkerConn;
  friend class FloydWorkerHandle;
  friend class Peer;

  rocksdb::DB* db_;
  // state machine db point
  // raft log
  rocksdb::DB* log_and_meta_;  // used to store logs and meta data
  RaftLog* raft_log_;
  RaftMeta* raft_meta_;

  Options options_;
  // debug log used for ouput to file
  Logger* info_log_;

  FloydContext* context_;

  FloydWorker* worker_;
  FloydApply* apply_;
  FloydPrimary* primary_;
  PeersSet peers_;
  ClientPool* worker_client_pool_;

  std::map<int64_t, std::pair<std::string, int> > vote_for_;

  bool IsSelf(const std::string& ip_port);

  Status DoCommand(const CmdRequest& req, CmdResponse *res);
  Status ExecuteCommand(const CmdRequest& req, CmdResponse *res);
  Status ReplyExecuteDirtyCommand(const CmdRequest& req, CmdResponse *res);
  bool DoGetServerStatus(CmdResponse_ServerStatus* res);
  void GrantVote(uint64_t term, const std::string ip, int port);

  /*
   * these two are the response to the request vote and appendentries
   */
  void ReplyRequestVote(const CmdRequest& request, CmdResponse* response);
  void ReplyAppendEntries(const CmdRequest& request, CmdResponse* response);

  void ReplyAddServer(const CmdRequest& request, CmdResponse* response);

  bool AdvanceFollowerCommitIndex(uint64_t new_commit_index);

  // No coping allowed
  FloydImpl(const FloydImpl&);
  void operator=(const FloydImpl&);
};  // FloydImpl

}  // namespace floyd
#endif  // FLOYD_SRC_FLOYD_IMPL_H_
