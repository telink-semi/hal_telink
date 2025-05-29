echo  "---------------------------  SDK version info ---------------------------"
str=$(grep -E "[\$]{3}[a-zA-Z0-9 _.]+[\$]{3}" --text -o $1.bin | sed 's/\$//g')
if [ -z "$str" ]; then echo "no SDK version found at the end of firmware, please check sdk_version.c and sdk_version.h"
else echo "$str";fi;echo  "---------------------------  SDK version end  ---------------------------"
echo "Linux OS"
echo "check_fw in Linux..."
chmod 755 ../../../check_fw
../../../check_fw $1.bin
