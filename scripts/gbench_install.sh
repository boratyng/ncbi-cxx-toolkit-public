#! /bin/sh
# $Id$
#
# GBENCH installation script.
# Author: Anatoliy Kuznetsov, Denis Vakatov


script_name=`basename $0`
script_dir=`dirname $0`
script_dir=`(cd "${script_dir}" ; pwd)`

. ${script_dir}/common.sh


PLUGINS='doc_basic doc_table algo_basic algo_stdio algo_align view_text view_graphic view_align view_sequence view_table'
BINS='gbench-bin gbench_plugin_scan'
LIBS='gui_doc gui_view gui_algo'


Usage()
{
    cat <<EOF 1>&2
USAGE: $script_name [--copy] sourcedir targetdir
SYNOPSIS:
   Create Genome Workbench installation from the standard Toolkit build.
ARGUMENTS:
   --copy     -- use Unix 'cp' command instead of 'ln -s'.
   sourcedir  -- path to pre-built NCBI Toolkit.
   targetdir  -- target installation directory.

EOF
    test -z "$1"  ||  echo ERROR: $1 1>&2
    exit 1
}


FindDirPath()
{
   path="$1"
   while [ "$path" != '.' ]; do
     path=`dirname $path`
     if [ -d "$path/$2" ]; then
        echo $path
        break
     fi
   done
}


MakeDirs()
{
    COMMON_ExecRB mkdir -p $1
    COMMON_ExecRB mkdir -p $1/bin
    COMMON_ExecRB mkdir -p $1/lib
    COMMON_ExecRB mkdir -p $1/etc
    COMMON_ExecRB mkdir -p $1/plugins
}


DoCopy()
{
    COMMON_ExecRB $BINCOPY $1 $2

    case `uname` in
        Darwin*)
        dylib_file=`echo $1 | sed "s,\.so$,.dylib",`
        COMMON_ExecRB $BINCOPY $dylib_file $2
        ;;
    esac
}


CopyFiles()
{
    for x in $BINS; do
        echo copying: $x
        src_file=$src_dir/bin/$x 
        if [ -f $src_file ]; then
            mv -f $target_dir/bin/$x $target_dir/bin/$x.old  2>/dev/null
            rm -f $target_dir/bin/$x $target_dir/bin/$x.old
            COMMON_ExecRB $BINCOPY $src_file $target_dir/bin/
        else
            echo "++++++++ RP proc: $x_common_rb"
            $x_common_rb
            COMMON_Error "File not found: $src_file"
        fi
    done

    for x in $LIBS; do
        echo copying: lib_$x.so
        src_file=$src_dir/lib/lib$x.so 
        if [ -f $src_file ]; then
            rm -f $target_dir/lib/lib$x.so
            DoCopy $src_file $target_dir/lib/ 
        else
            $x_common_rb
            COMMON_Error "File not found: $src_file"
        fi
    done

    for x in $PLUGINS; do
        echo copying plugin: $x
        rm -f $target_dir/plugins/libgui_$x.so
        DoCopy $src_dir/lib/libgui_$x.so $target_dir/plugins/
    done

    for x in $src_dir/lib/libdbapi*.so; do
        if [ -f $x ]; then
            f=`basename $x`
            echo copying DB interface: $f
            rm -f $target_dir/lib/$f
            DoCopy $x $target_dir/lib/
        fi
    done
}



#
#  MAIN
#

if [ $# -eq 0 ]; then
    Usage "Wrong number of arguments"
fi 

copy_all="no"
if [ $1 = "--copy" ]; then
    copy_all="yes"
    BINCOPY="cp -pf"
    src_dir=$2
    target_dir=$3
else
    BINCOPY="ln -s"
    src_dir=$1
    target_dir=$2
fi


if [ -z "$src_dir" ]; then
    Usage "Source directory not provided"
fi

if [ -z "$target_dir" ]; then
    Usage "Target directory not provided"
fi

if [ ! -d $src_dir ]; then
    COMMON_Error "Source directory not found: $src_dir"
else
    echo Source: $src_dir    
    echo Target: $target_dir    
fi

if [ ! -d $src_dir/bin ]; then
    COMMON_Error "bin directory not found: $src_dir/bin"
fi

if [ ! -d $src_dir/lib ]; then
    COMMON_Error "lib directory not found: $src_dir/lib"
fi



source_dir=`FindDirPath $src_dir /src/gui/gbench`
source_dir="${source_dir}/src/gui/gbench"

# Set the rollback escape command (used by COMMON_ExecRB)
x_common_rb="rm -rf $target_dir"

MakeDirs $target_dir

CopyFiles


echo "Preparing scripts"

COMMON_ExecRB cp ${source_dir}/gbench_install/run-gbench.sh ${target_dir}/bin/run-gbench.sh
chmod 755 ${target_dir}/bin/run-gbench.sh

rm -f ${src_dir}/bin/gbench
ln -s ${target_dir}/bin/run-gbench.sh ${src_dir}/bin/gbench

COMMON_ExecRB cp -p ${source_dir}/gbench.ini ${target_dir}/etc/

COMMON_ExecRB cp -p ${source_dir}/gbench_install/move-gbench.sh ${target_dir}/bin/
chmod 755 ${target_dir}/bin/move-gbench.sh

COMMON_ExecRB ${target_dir}/bin/move-gbench.sh ${target_dir}

echo "Configuring plugin cache"
COMMON_AddRunpath ${target_dir}/lib
COMMON_ExecRB ${target_dir}/bin/gbench_plugin_scan -dir ${target_dir}/plugins
