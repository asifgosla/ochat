#!/bin/bash
set -e # exit immediately on error

# run all unit tests with combinded code coverage enabled
echo Running all tests...
trap 'echo "An error occured running tests"; exit 1' ERR # trap errors
bazel test --collect_code_coverage --combined_report=lcov //...

# generate a report in html
echo Generating composite coverage html report under ./coverage
temp_file=$(mktemp -p ./)
echo $temp_file
trap 'echo "An error occured generating the report"; rm -f "$temp_file"; exit 1' ERR # trap errors
cp $(bazel info output_path)/_coverage/_coverage_report.dat "$temp_file"
mkdir -p coverage
genhtml "$temp_file" -o coverage
rm -f "$temp_file"

echo "Coverage report available at: $(pwd)/coverage/index.html"