/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/util/io/outputstream.h"
#include "eventql/util/protobuf/DynamicMessage.h"
#include "eventql/util/UnixTime.h"

using namespace stx;

namespace zbase {

struct EventScanRow {
  EventScanRow(RefPtr<msg::MessageSchema> schema);
  UnixTime time;
  msg::DynamicMessage obj;
};

class EventScanResult {
public:

  EventScanResult(
      RefPtr<msg::MessageSchema> schema,
      size_t max_rows = 1000);

  /**
   * Returns a pointer to a mutable (empty) logline if the line was inserted
   * and returns a null pointer if the line was not inserted (e.g. if the
   * result list is already full and this line is too old
   */
  EventScanRow* addRow(UnixTime time);

  /**
   * Returns the rows in the result list sorted from newest (first) to oldest
   * (last)
   */
  const List<EventScanRow>& rows() const;

  /**
   * Returns true if the result is full, false otherwise
   */
  bool isFull() const;

  /**
   * Returns the max number of rows this result can hold
   */
  size_t capacity() const;

  UnixTime scannedUntil() const;
  void setScannedUntil(UnixTime time);

  size_t rowScanned() const;
  void incrRowsScanned(size_t nrows);

  RefPtr<msg::MessageSchema> schema() const;

  void encode(OutputStream* os) const;
  void decode(InputStream* os);

protected:
  RefPtr<msg::MessageSchema> schema_;
  size_t max_rows_;
  List<EventScanRow> rows_;
  UnixTime scanned_until_;
  size_t rows_scanned_;
};

} // namespace zbase
