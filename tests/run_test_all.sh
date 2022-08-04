# SPDX-License-Identifier: MIT or GPL-2.0-only
#!/bin/bash

DIR=$(cd "$(dirname "$0")";pwd)

GRPS="generic null loop"
for G in $GRPS; do
		$DIR/run_test.sh $DIR/$G $1
done
