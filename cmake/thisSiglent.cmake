# Write thisSiglent.sh to INSTALL directory


# install thisREST script, sh VERSION
install( CODE
"
file( WRITE \${CMAKE_INSTALL_PREFIX}/thisSiglent.sh
\"\#!/bin/bash

\# check active shell by checking for existence of _VERSION variable
if [[ -n \\\"\\\${BASH_VERSION}\\\" ]] ; then
    thisdir=\\\$(cd \\\$(dirname \\\${BASH_ARGV[0]}) ; pwd)
else
    echo \\\"Invalid shell! Only bash is supported!\\\"
    return 1
fi

if [ \\\$SIG_PATH ] ; then
echo switching to SIG installed in \\\${thisdir}
_PATH=`echo \\\$PATH | sed -e \\\"s\#\\\${SIG_PATH}/bin:\#\#g\\\"`
_LD_LIBRARY_PATH=`echo \\\$LD_LIBRARY_PATH | sed -e \\\"s\#\\\${SIG_PATH}/lib:\#\#g\\\"`
else
_PATH=\\\$PATH
_LD_LIBRARY_PATH=\\\$LD_LIBRARY_PATH
fi

export SIG_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}
export SIG_PATH=\\\${thisdir}
export SIG_INCLUDE_PATH=\\\${thisdir}/inc
export PATH=\\\$SIG_PATH/bin:\\\$SIG_PATH/python:\\\$_PATH
export LD_LIBRARY_PATH=\\\$SIG_PATH/lib:\\\$_LD_LIBRARY_PATH
export LIBRARY_PATH=\\\$LIBRARY_PATH:\\\$SIG_PATH/lib

alias SIGRoot=\\\"SigRoot -l\\\"

alias processdata=\\\"bash $SIG_PATH/scripts/processdata.sh $1 \\\"

\"
)
        "
)

