@echo off
SET TMP=/data/local/tmp

echo "Push ..."
adb push airfps %TMP%
adb shell chmod 777 %TMP%/airfps
adb shell %TMP%/airfps
