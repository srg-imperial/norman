#!/usr/bin/bash
set -euo pipefail

# adapted from https://stackoverflow.com/a/246128/65678
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
	DIR="$(cd -P "$(dirname "$SOURCE")" && pwd)"
	SOURCE="$(readlink "$SOURCE")"
	[[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIR="$(cd -P "$(dirname "$SOURCE")" && pwd)"

if [[ $# != 3 ]] ; then
  echo "USAGE: $0 PATH_TO_NORMAN PATH_TO_CLANG_FORMAT TESTCASE" >&2
  exit 1
fi

rm -rf -- "$3"
mkdir -p "$(dirname "$3")"
cp -r -- "$DIR/$3/" "$3/"

# norman currently does not support the `--` syntax due to option parser limitations...
"$1" "$3/input.c"
mv -- "$3/input.c" "$3/actual.c"

"$2" -i -- "$3/"{actual,expected}.c
diff --minimal -- "$3/expected.c" "$3/actual.c"