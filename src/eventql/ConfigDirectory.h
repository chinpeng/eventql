/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/SHA1.h>
#include <eventql/util/mdb/MDB.h>
#include <eventql/util/net/inetaddr.h>
#include <eventql/util/http/httpclient.h>
#include <eventql/CustomerConfig.h>
#include <eventql/core/ClusterConfig.pb.h>
#include <eventql/TableDefinition.h>

using namespace stx;

namespace zbase {

enum ConfigTopic : uint64_t {
  CUSTOMERS = 1,
  TABLES = 2,
  USERDB = 3,
  CLUSTERCONFIG = 4
};

class ConfigDirectoryClient {
public:

  ConfigDirectoryClient(InetAddr master_addr);

  ClusterConfig fetchClusterConfig();
  ClusterConfig updateClusterConfig(const ClusterConfig& config);

protected:
  InetAddr master_addr_;
};

class ConfigDirectory {
public:

  ConfigDirectory(
      const String& path,
      InetAddr master_addr,
      uint64_t topics);

  ClusterConfig clusterConfig() const;
  void updateClusterConfig(ClusterConfig config);
  void onClusterConfigChange(Function<void (const ClusterConfig& cfg)> fn);

  RefPtr<CustomerConfigRef> configFor(const String& customer_key) const;
  void updateCustomerConfig(CustomerConfig config);
  void listCustomers(
      Function<void (const CustomerConfig& cfg)> fn) const;
  void onCustomerConfigChange(Function<void (const CustomerConfig& cfg)> fn);

  void updateTableDefinition(const TableDefinition& table, bool force = false);
  void listTableDefinitions(
      Function<void (const TableDefinition& tbl)> fn) const;
  void onTableDefinitionChange(Function<void (const TableDefinition& tbl)> fn);

  Option<UserConfig> findUser(const String& userid);

  void sync();

  void startWatcher();
  void stopWatcher();

protected:

  void loadCustomerConfigs();
  HashMap<String, uint64_t> fetchMasterHeads() const;

  void syncObject(const String& obj);

  void syncClusterConfig();
  void commitClusterConfig(const ClusterConfig& config);

  void syncCustomerConfig(const String& customer);
  void commitCustomerConfig(const CustomerConfig& config);

  void syncTableDefinitions(const String& customer);
  void commitTableDefinition(const TableDefinition& tbl);

  void syncUserDB();
  void commitUserConfig(const UserConfig& usr);

  InetAddr master_addr_;
  ConfigDirectoryClient cclient_;
  uint64_t topics_;
  RefPtr<mdb::MDB> db_;
  mutable std::mutex mutex_;
  ClusterConfig cluster_config_;
  HashMap<String, RefPtr<CustomerConfigRef>> customers_;

  Vector<Function<void (const ClusterConfig& cfg)>> on_cluster_change_;
  Vector<Function<void (const CustomerConfig& cfg)>> on_customer_change_;
  Vector<Function<void (const TableDefinition& cfg)>> on_table_change_;
  Vector<Function<void (const UserConfig& cfg)>> on_user_change_;

  std::atomic<bool> watcher_running_;
  std::thread watcher_thread_;
};

} // namespace zbase
