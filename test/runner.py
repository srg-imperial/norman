#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import re
import shutil
from subprocess import run

SCRIPT_LOCATION = os.path.dirname(os.path.realpath(__file__))

def parse_test(test):
  with open(test, "r", encoding="utf-8") as f:
    dashes = False
    input = []
    for line in f:
      if line == "---\n" or line == "---\r\n":
        dashes = True
        break
      elif line.startswith("--"):
        pass
      else:
        input.append(line)
    if not dashes:
      print(f"Cannot parse test file `{test}`: missing expected output")
      os._exit(1)

    dashes = False
    expected = []
    for line in f:
      if line == "---\n" or line == "---\r\n":
        dashes = True
        break
      elif line.startswith("--"):
        pass
      else:
        expected.append(line)
    
    if dashes:
      config = []
      for line in f:
        if line.startswith("--"):
          pass
        else:
          config.append(line)
    else:
      config = None

  return (
    "".join(input).strip() + "\n",
    "".join(expected).strip() + "\n",
    None if config is None else "".join(config).strip(),
  )

def main():
  parser = argparse.ArgumentParser()

  parser.add_argument("--norman", type=Path, default="norman", help="location of norman binary")
  parser.add_argument("--clang-format", type=Path, default="clang-format", help="location of clang-format binary")
  parser.add_argument("test", type=Path, help="test case id")

  args = parser.parse_args()

  test = args.test
  test_source = SCRIPT_LOCATION / test
  if test.is_absolute():
    test = test.resolve().relative_to(SCRIPT_LOCATION)
  if not test_source.exists():
    print(f"Test `{SCRIPT_LOCATION / test}` does not exist in the filesystem")
    os._exit(1)

  shutil.rmtree(test, ignore_errors=True)
  os.makedirs(test, exist_ok=True)

  if test_source.is_dir():
    actual = test / "actual.c"
    shutil.copyfile(SCRIPT_LOCATION / test / "input.c", actual)

    expected = test / "expected.c"
    shutil.copyfile(SCRIPT_LOCATION / expected, expected)

    config = test / "config.json"
    if (SCRIPT_LOCATION / config).is_file():
      shutil.copyfile(SCRIPT_LOCATION / config, config)
    else:
      config = None

    # norman will find a `compile_commands.json` if cmake creates it at the top of the build folder, which will
    # cause unwanted behavior. Generating a phony `compile_flags.txt` is not great, but at least gives deterministic behavior.
    if not (test / "compile_commands.json").exists() and not (test / "compile_flags.txt").exists():
      with open(test / "compile_flags.txt", "w", encoding="utf-8") as f:
        f.write("\n")

  else:
    parts = parse_test(test_source)

    actual = test / "actual.c"
    with open(actual, "w", encoding="utf-8") as f:
      f.write(parts[0])

    expected = test / "expected.c"
    with open(expected, "w", encoding="utf-8") as f:
      f.write(parts[1])

    config = None
    if parts[2] is not None:
      config = test / "config.json"
      with open(config, "w", encoding="utf-8") as f:
        f.write(parts[2])

    # norman will find a `compile_commands.json` if cmake creates it at the top of the build folder, which will
    # cause unwanted behavior. Generating a phony `compile_flags.txt` is not great, but at least gives deterministic behavior.
    with open(test / "compile_flags.txt", "w", encoding="utf-8") as f:
      f.write("\n")

  # norman currently does not support the `--` syntax due to option parser limitations...
  if config is not None:
    run([args.norman, "--config", config, actual], check=True)
  else:
    run([args.norman, actual], check=True)

  run([args.clang_format, "-i", "--", expected, actual], check=True)
  result = run(["diff", "--minimal", "--", expected, actual])
  os._exit(result.returncode)

if __name__ == '__main__':
  main()