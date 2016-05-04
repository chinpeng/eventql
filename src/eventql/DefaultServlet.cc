/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <eventql/util/http/cookies.h>
#include <eventql/util/assets.h>
#include <eventql/DefaultServlet.h>
#include <eventql/HTTPAuth.h>

namespace zbase {

void DefaultServlet::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  URI uri(request->uri());

  if (uri.path() == "/" &&
      request->hasHeader("X-ZenBase-Production")) {
    String auth_cookie;
    http::Cookies::getCookie(
        request->cookies(),
        HTTPAuth::kSessionCookieKey,
        &auth_cookie);

    if (auth_cookie.empty()) {
      response->setStatus(http::kStatusFound);
      response->addHeader("Location", "http://app.zbase.io/");
      return;
    }
  }

  if (uri.path() == "/") {
    response->setStatus(http::kStatusFound);
    response->addHeader("Location", "/a/");
    return;
  }

  if (uri.path() == "/ping") {
    response->setStatus(http::kStatusOK);
    response->addBody("pong");
    return;
  }

  response->setStatus(http::kStatusNotFound);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(Assets::getAsset("eventql/webui/404.html"));
}

}
