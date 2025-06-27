
#!/bin/bash

var=\"$1\"

analyzeSiglent --f $1 && eval "SigRoot '$SIG_PATH/macros/savespc.C($var)'"


