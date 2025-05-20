#!/bin/bash
set -e


build_list=$(grep -oP '"\$target" = "\K\w*.exe(?=")' build.sh)


build() {
  ./build.sh $1 -clean
}

rm -f FDOOM*.EXE FDOOM*.MAP|| true
for target in $build_list;
do
  build $target
done

./stub.sh
