#!/bin/bash 
system=$(uname);echo   
system_windows=$(echo $system | grep -i "mingw\|cygwin"); system_linux=$(echo $system | grep -i "linux"); echo System: $system
echo "**************** post process begin ****************"
if [ ! -z $system_linux ]; then echo   
	${1}/linux/Secure_Boot_Post_Tool_Linux --config ${2} --input ${3} --output ${4}; echo    
elif [ ! -z $system_windows ]; then echo 
	${1}/../../tl_check_fw.bat ${5}
	${1}/windows/Secure_Boot_Post_Tool_Win.exe --config ${2} --input ${3} --output ${4}; echo      
else echo   
	echo [Error] $system not support
fi; echo   
echo "**************** post process end ******************"