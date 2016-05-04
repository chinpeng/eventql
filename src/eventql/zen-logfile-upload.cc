/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <unistd.h>
#include "eventql/util/stdtypes.h"
#include "eventql/util/application.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/cli/CLI.h"
#include "eventql/util/csv/CSVInputStream.h"
#include "eventql/util/csv/BinaryCSVOutputStream.h"
#include "eventql/util/io/file.h"
#include "eventql/util/io/BufferedOutputStream.h"
#include "eventql/util/inspect.h"
#include "eventql/util/human.h"
#include "eventql/util/http/httpclient.h"
#include "eventql/util/util/SimpleRateLimit.h"
#include "eventql/util/protobuf/MessageSchema.h"

using namespace stx;

void run(const cli::FlagParser& flags) {
  auto logfile_name = flags.getString("logfile_name");
  auto input_file = flags.getString("input_file");
  auto api_token = flags.getString("api_token");
  auto tail = flags.isSet("tail");
  String api_url = "http://api.zbase.io/api/v1";

  stx::logInfo("dx-logfile-upload", "Tailing logfile '$0'", input_file);

  String source_attrs;
  for (const auto& a : flags.getStrings("attr")) {
    auto x = a.find("=");
    if (x == String::npos) {
      RAISEF(kParseError, "invalid attribute '$0'; format is '<key>=<val>", a);
    }

    source_attrs += StringUtil::format(
        "&$0=$1",
        URI::urlEncode(a.substr(0, x)),
        URI::urlEncode(a.substr(x + 1)));
  }

  uint64_t position = 0;
  uint64_t inode = FileUtil::inodeID(input_file);
  size_t num_rows_uploaded = 0;

  auto input_base_path = FileUtil::basePath(input_file);
  auto metafile_path = StringUtil::format(
      "$0/.$1.dxmeta",
      input_base_path,
      input_file.substr(input_base_path.size() + 1));

  if (FileUtil::exists(metafile_path)) {
    auto metafile = FileInputStream::openFile(metafile_path);
    inode = metafile->readVarUInt();
    position = metafile->readVarUInt();
  }

  /* http client */
  http::HTTPClient http_client(nullptr);
  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  /* status line */
  util::SimpleRateLimitedFn status_line(
      kMicrosPerSecond,
      [&num_rows_uploaded, &position, &inode] () {
    stx::logDebug(
        "dx-logfile-upload",
        "Uploading... rows_uploaded=$0 position=$1 inode=$2",
        num_rows_uploaded,
        position,
        inode);
  });

  static const size_t kMaxBatchSize = 1024 * 1024 * 50;
  for (;;) {
    auto finode = FileUtil::inodeID(input_file);
    auto fsize = FileUtil::size(input_file);
    if (finode != inode || fsize < position) {
      position = 0;
      inode = finode;
    }

    auto file = File::openFile(input_file, File::O_READ);
    file.seekTo(position);

    auto is = FileInputStream::fromFile(std::move(file));
    bool eof = false;
    while (!eof) {
      String buf;
      while (buf.size() < kMaxBatchSize) {
        if (!is->readLine(&buf)) {
          eof = true;
          break;
        }

        ++num_rows_uploaded;
      }

      position += buf.size();

      if (buf.size() > 0) {
        auto upload_uri = StringUtil::format(
            "$0/logfiles/upload?logfile=$1$2",
            api_url,
            URI::urlEncode(logfile_name),
            source_attrs);

        status_line.runMaybe();

        auto upload_res = http_client.executeRequest(
            http::HTTPRequest::mkPost(
                upload_uri,
                buf,
                auth_headers));

        if (upload_res.statusCode() != 201) {
          RAISE(kRuntimeError, upload_res.body().toString());
        }
      }

      if (tail) {
        BufferedOutputStream metafile(
            FileOutputStream::fromFile(
                File::openFile(
                    metafile_path,
                    File::O_CREATEOROPEN | File::O_WRITE | File::O_TRUNCATE)));

        metafile.appendVarUInt(inode);
        metafile.appendVarUInt(position);
        metafile.flush();
      }

      status_line.runMaybe();
    }

    status_line.runMaybe();

    if (tail) {
      usleep(10 * kMicrosPerSecond);
      status_line.runMaybe();
    } else {
      break;
    }
  }

  status_line.runForce();
}

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "loglevel",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "input_file",
      stx::cli::FlagParser::T_STRING,
      true,
      "i",
      NULL,
      "input CSV file",
      "<filename>");

  flags.defineFlag(
      "logfile_name",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "table name",
      "<name>");

  flags.defineFlag(
      "tail",
      stx::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "tail the logfile?",
      "<switch>");

  flags.defineFlag(
      "attr",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "source attribute",
      "<key>=<val>");

  flags.defineFlag(
      "api_token",
      stx::cli::FlagParser::T_STRING,
      true,
      "x",
      NULL,
      "DeepAnalytics API Token",
      "<token>");
  try {
    flags.parseArgv(argc, argv);

    Logger::get()->setMinimumLogLevel(
        strToLogLevel(flags.getString("loglevel")));

    run(flags);
  } catch (const StandardException& e) {
    stx::logError("dx-logfile-upload", "[FATAL ERROR] $0", e.what());
  }

  return 0;
}
