#!/usr/bin/sh

for FILE in $(ls gauntlet);
do
	echo $FILE
	$1 -s gauntlet/$FILE
done
