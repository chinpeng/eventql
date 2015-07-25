/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "logjoin/SessionContext.h"

using namespace stx;

namespace cm {

class SessionPipeline : public RefCounted {
public:
  typedef Function<void (RefPtr<SessionContext> ctx)> PipelineStageFn;

  void run(RefPtr<SessionContext> ctx);

  void addStage(PipelineStageFn fn);

protected:
  Vector<PipelineStageFn> stages_;
};

} // namespace cm
