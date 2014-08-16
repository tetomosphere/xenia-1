/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <xenia/xenia.h>

#include <gflags/gflags.h>
#include <poly/main.h>

using namespace xe;

DEFINE_string(target, "", "Specifies the target .xex or .iso to execute.");

int xenia_run(std::vector<std::wstring>& args) {
  int result_code = 1;

  Profiler::Initialize();
  Profiler::ThreadEnter("main");

  Emulator* emulator = NULL;

  // Grab path from the flag or unnamed argument.
  if (!FLAGS_target.size() && args.size() < 2) {
    google::ShowUsageWithFlags("xenia-run");
    XEFATAL("Pass a file to launch.");
    return 1;
  }
  std::wstring path;
  if (FLAGS_target.size()) {
    // Passed as a named argument.
    // TODO(benvanik): find something better than gflags that supports unicode.
    path = poly::to_wstring(FLAGS_target);
  } else {
    // Passed as an unnamed argument.
    path = args[1];
  }
  // Normalize the path and make absolute.
  std::wstring abs_path = poly::to_absolute_path(path);

  // Create platform abstraction layer.
  xe_pal_options_t pal_options;
  xe_zero_struct(&pal_options, sizeof(pal_options));
  XEEXPECTZERO(xe_pal_init(pal_options));

  // Create the emulator.
  emulator = new Emulator(L"");
  XEEXPECTNOTNULL(emulator);
  X_STATUS result = emulator->Setup();
  if (XFAILED(result)) {
    XELOGE("Failed to setup emulator: %.8X", result);
    XEFAIL();
  }

  // Launch based on file type.
  // This is a silly guess based on file extension.
  auto file_system_type = emulator->file_system()->InferType(abs_path);
  switch (file_system_type) {
    case kernel::fs::FileSystemType::STFS_TITLE:
      result = emulator->LaunchSTFSTitle(abs_path);
      break;
    case kernel::fs::FileSystemType::XEX_FILE:
      result = emulator->LaunchXexFile(abs_path);
      break;
    case kernel::fs::FileSystemType::DISC_IMAGE:
      result = emulator->LaunchDiscImage(abs_path);
      break;
  }
  if (XFAILED(result)) {
    XELOGE("Failed to launch target: %.8X", result);
    XEFAIL();
  }

  result_code = 0;
XECLEANUP:
  delete emulator;
  if (result_code) {
    XEFATAL("Failed to launch emulator: %d", result_code);
  }
  Profiler::Dump();
  Profiler::Shutdown();
  return result_code;
}

DEFINE_ENTRY_POINT(L"xenia-run", L"xenia-run some.xex", xenia_run);
