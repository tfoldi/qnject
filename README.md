# qnject [![Build Status](https://travis-ci.org/tfoldi/qnject.svg?branch=master)](https://travis-ci.org/tfoldi/qnject)

Hello my friend. Yes, this is an another hacky project to make Qt developers life easier. It injects a full featured 
web server into binary process to modify its behaviours (hook, alter QObjects, etc). 

## What can it do?

Modify `QObject` properties on the fly, grab screenshots from widgets, inject assembly code into text segment. Fancy stuff.

![img](https://github.com/tfoldi/qnject/blob/master/qnject-tableau.gif?raw=true)

## Is this ready?

No.

## Is this documented?

At some point it will be.

## How does it work?

Using `DYLIB_LIBRARY_PATH` or `LD_PRELOAD` or ptrace or windows api calls it adds a shared library to a running applicaion. The shared
library starts a mongoose server that calls registred plugins when their registed URL handlers are called. It's very cool it's
just not ready.
