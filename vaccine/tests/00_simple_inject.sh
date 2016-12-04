#!/bin/sh -x

unamestr=`uname`
if [[ "$unamestr" == 'Linux' ]]; then
  (LD_PRELOAD=../libqnject.so sleep 0.1 & (sleep 0.05 && nc -v 127.0.0.1 8000)) &&
    (LD_PRELOAD=../libqnject.so VACCINE_HTTP_PORT=8001 sleep 0.1 & (sleep 0.05 && nc -v 127.0.0.1 8001))
elif [[ "$unamestr" == 'Darwin' ]]; then
  (DYLD_INSERT_LIBRARIES=../libqnject.dylib sleep 0.1 & (sleep 0.05 && nc -v 127.0.0.1 8000)) && 
  (DYLD_INSERT_LIBRARIES=../libqnject.dylib VACCINE_HTTP_PORT=8001 sleep 0.1 & (sleep 0.05 && nc -v 127.0.0.1 8001))
fi

exit $?
