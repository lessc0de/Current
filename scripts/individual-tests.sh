#!/bin/bash
#
# Consecutively runs `make test` from within each directory that contains a `test.cc` file.

set -u -e

FAILURES=""

for i in $(find . -name test.cc | sort -g) ; do
  DIR=$(dirname $i)
  echo -e -n "\n\033[0m\033[1mDir\033[0m: \033[36m"
  echo $DIR
  cd $DIR
  if ! make -s test ; then
    FAILURES="$FAILURES\n- $DIR"
  fi
  cd - >/dev/null
done

if [ "$FAILURES" = "" ] ; then
 echo -e "\n\033[32m\033[1mALL TESTS PASS.\033[0m"
 exit 0
else
 echo -e "\n\033[31m\033[1mFAIL\033[0m"
 echo -e "Failed tests:$FAILURES"
 exit 1
fi
