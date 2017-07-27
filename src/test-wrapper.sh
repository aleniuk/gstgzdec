#!/bin/sh

PREFIX=$HOME/GNOME/src-new/build
TEST_DATA_DIR=$HOME/GNOME/nikon

CC=gcc
CFLAGS="-O0 -g -DTEST"

#env vars
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
ZFLAGS="$(pkg-config --cflags --libs zlib) -lbz2"

#build
echo compiling zip-dec-wrapper
$CC -static zip-dec-wrapper.c $CFLAGS $ZFLAGS -o wrapper-test-bin

#test
mkdir $TEST_TMP_DIR
for file in $TEST_DATA_DIR/*
do
    if [ -f "$file" ]; then
	echo bzip
	echo compressing "$file" ...
	bzip2 -k --compress "$file"
	if [ "$?" -eq 0 ]; then
	    rm test_tmp_file
	    echo decompressing "$file"...
	    ./wrapper-test-bin "$file.bz2" test_tmp_file 1
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
	    ./wrapper-test-bin "$file.gz" test_tmp_file 2
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
