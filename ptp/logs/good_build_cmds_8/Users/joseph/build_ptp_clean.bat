@echo off
cd /d "C:\Users\joseph\esp-idf\v5.5.2\esp-idf"
call export.bat
cd /d "C:\Users\joseph\esp-idf\v5.5.2\esp-idf\developer\examples1\ptp"
idf.py fullclean
idf.py build