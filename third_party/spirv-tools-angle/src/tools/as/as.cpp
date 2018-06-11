// Copyright (c) 2015-2016 The Khronos Group Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdio>
#include <cstring>
#include <vector>

#include "source/spirv_target_env.h"
#include "spirv-tools/libspirv.h"
#include "tools/io.h"

void print_usage(char* argv0) {
  printf(
      R"(%s - Create a SPIR-V binary module from SPIR-V assembly text

Usage: %s [options] [<filename>]

The SPIR-V assembly text is read from <filename>.  If no file is specified,
or if the filename is "-", then the binary is read from standard input.
The SPIR-V binary module is written to file "out.spv", unless the -o option
is used.

Options:

  -h, --help      Print this help.

  -o <filename>   Set the output filename. Use '-' to mean stdout.
  --version       Display assembler version information.
  --target-env {vulkan1.0|spv1.0|spv1.1}
                  Use Vulkan1.0/SPIR-V1.0/SPIR-V1.1 validation rules.
)",
      argv0, argv0);
}

int main(int argc, char** argv) {
  const char* inFile = nullptr;
  const char* outFile = nullptr;
  spv_target_env target_env = SPV_ENV_UNIVERSAL_1_1;
  for (int argi = 1; argi < argc; ++argi) {
    if ('-' == argv[argi][0]) {
      switch (argv[argi][1]) {
        case 'h': {
          print_usage(argv[0]);
          return 0;
        }
        case 'o': {
          if (!outFile && argi + 1 < argc) {
            outFile = argv[++argi];
          } else {
            print_usage(argv[0]);
            return 1;
          }
        } break;
        case 0: {
          // Setting a filename of "-" to indicate stdin.
          if (!inFile) {
            inFile = argv[argi];
          } else {
            fprintf(stderr, "error: More than one input file specified\n");
            return 1;
          }
        } break;
        case '-': {
          // Long options
          if (0 == strcmp(argv[argi], "--version")) {
            printf("%s\n", spvSoftwareVersionDetailsString());
            printf("Target: %s\n",
                   spvTargetEnvDescription(SPV_ENV_UNIVERSAL_1_1));
            return 0;
          }
          if (0 == strcmp(argv[argi], "--help")) {
            print_usage(argv[0]);
            return 0;
          }
          if (0 == strcmp(argv[argi], "--target-env")) {
            if (argi + 1 < argc) {
              const auto env_str = argv[++argi];
              if (!spvParseTargetEnv(env_str, &target_env)) {
                fprintf(stderr, "error: Unrecognized target env: %s\n",
                        env_str);
                return 1;
              }
            } else {
              fprintf(stderr, "error: Missing argument to --target-env\n");
              return 1;
            }
          }
        } break;
        default:
          print_usage(argv[0]);
          return 1;
      }
    } else {
      if (!inFile) {
        inFile = argv[argi];
      } else {
        fprintf(stderr, "error: More than one input file specified\n");
        return 1;
      }
    }
  }

  if (!outFile) {
    outFile = "out.spv";
  }

  std::vector<char> contents;
  if (!ReadFile<char>(inFile, "r", &contents)) return 1;

  spv_binary binary;
  spv_diagnostic diagnostic = nullptr;
  spv_context context = spvContextCreate(target_env);
  spv_result_t error = spvTextToBinary(context, contents.data(),
                                       contents.size(), &binary, &diagnostic);
  spvContextDestroy(context);
  if (error) {
    spvDiagnosticPrint(diagnostic);
    spvDiagnosticDestroy(diagnostic);
    return error;
  }

  if (!WriteFile<uint32_t>(outFile, "wb", binary->code, binary->wordCount)) {
    spvBinaryDestroy(binary);
    return 1;
  }

  spvBinaryDestroy(binary);

  return 0;
}