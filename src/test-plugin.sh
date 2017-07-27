#!/bin/sh

PREFIX=$HOME/GNOME/src-new/build-0.1
TEST_DATA_DIR=$HOME/GNOME/nikon

LAUNCH="valgrind -q gst-launch-0.10"

export LD_LIBRARY_PATH=$PREFIX/lib

#test
for file in $TEST_DATA_DIR/*
do
    if [ -f "$file" ]; then
	echo bzip
	echo compressing "$file" ...
	bzip2 -k --compress "$file"
	if [ "$?" -eq 0 ]; then
	    rm test_tmp_file
	    echo decompressing "$file"...
	    $LAUNCH filesrc location="$file.bz2" ! gzdec ! filesink location=test_tmp_file || exit 2
	    if [ "$?" -eq 0 ]; then
		cmp "$file" test_tmp_file
		if [ "$?" -eq 0 ]; then
		    echo test on file "$file" completed
		else
		    echo wrong decoded stream
		fi
	    else
                echo error during decoding
	    fi
	    rm "$file.bz2"
	else
	    echo "couldn't compress"
	fi
    else
	echo "$file" is not a file
    fi

    # TODO: to func
    if [ -f "$file" ]; then
	echo gzip
	echo compressing "$file" ...
	gzip -k "$file"
	if [ "$?" -eq 0 ]; then
	    rm test_tmp_file
	    echo decompressing "$file"...
	    $LAUNCH filesrc location="$file.gz" ! gzdec ! filesink location=test_tmp_file || exit 2
	    if [ "$?" -eq 0 ]; then
		cmp "$file" test_tmp_file
		if [ "$?" -eq 0 ]; then
		    echo test on file "$file" completed
		else
		    echo wrong decoded stream
		fi
	    else
                echo error during decoding
	    fi
	    rm "$file.gz"
	else
	    echo "couldn't compress"
	fi
    else
	echo "$file" is not a file
    fi
done
