// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/python_utils.h"

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/process/launch.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(PythonUtils, SetPythonPathInEnvironment) {
  base::EnvironmentMap env;
  SetPythonPathInEnvironment({base::FilePath(FILE_PATH_LITERAL("test/path1")),
                              base::FilePath(FILE_PATH_LITERAL("test/path2"))},
                             &env);
#if defined(OS_WIN)
  EXPECT_EQ(L"test/path1;test/path2", env[L"PYTHONPATH"]);
#else
  EXPECT_EQ("test/path1:test/path2", env["PYTHONPATH"]);
#endif
  EXPECT_NE(env.end(), env.find(FILE_PATH_LITERAL("VPYTHON_CLEAR_PYTHONPATH")));
  EXPECT_EQ(base::NativeEnvironmentString(),
            env[FILE_PATH_LITERAL("VPYTHON_CLEAR_PYTHONPATH")]);
}

TEST(PythonUtils, PythonRunTime) {
  base::CommandLine cmd_line(base::CommandLine::NO_PROGRAM);
  EXPECT_TRUE(GetPythonCommand(&cmd_line));

  // Run a python command to print a string and make sure the output is what
  // we want.
  cmd_line.AppendArg("-c");
  std::string input("PythonUtilsTest");
  std::string python_cmd = base::StringPrintf("print '%s';", input.c_str());
  cmd_line.AppendArg(python_cmd);
  std::string output;
  EXPECT_TRUE(base::GetAppOutput(cmd_line, &output));
  base::TrimWhitespaceASCII(output, base::TRIM_TRAILING, &output);
  EXPECT_EQ(input, output);
}