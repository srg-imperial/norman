#!/usr/bin/env python3
import argparse
import os
from pathlib import Path
import shutil
from subprocess import run

SCRIPT_LOCATION = os.path.dirname(os.path.realpath(__file__))

def main():
  parser = argparse.ArgumentParser()

  parser.add_argument("--norman", type=Path, default="norman", help="location of norman binary")
  parser.add_argument("--clang-format", type=Path, default="clang-format", help="location of clang-format binary")
  parser.add_argument("test", type=Path, help="test case id")

  args = parser.parse_args()
  if args.test.is_absolute():
    print("The test id must be relative path (e.g., `NullStmt/basic`)")
    os._exit(1)

  shutil.rmtree(args.test, ignore_errors=True)
  os.makedirs(args.test.parent, exist_ok=True)
  shutil.copytree(SCRIPT_LOCATION / args.test, args.test)

  # norman currently does not support the `--` syntax due to option parser limitations...
  if (args.test / "config.json").exists():
    run([args.norman, "--config", args.test / "config.json", args.test / "input.c"], check=True)
  else:
    run([args.norman, args.test / "input.c"], check=True)
  os.rename(args.test / "input.c", args.test / "actual.c")

  run([args.clang_format, "-i", "--", args.test / "expected.c", args.test / "actual.c"], check=True)
  result = run(["diff", "--minimal", "--", args.test / "expected.c", args.test / "actual.c"])
  os._exit(result.returncode)

if __name__ == '__main__':
  main()