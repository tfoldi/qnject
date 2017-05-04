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



## Ok, I'm ready, tell me more

You will need to find a QT version whos ABI is close enough to the version of
Tableau Desktop you are using. This may be tricky, but lets trust the QT guys
for now.

I will document any version matchings for Tableau Desktop / QT version here so
you dont have to go through it.

- Tableau 10.1 : QT 5.6(seems to work...)


### Clean way: Using Nix

The easy way out is to use the [Nix]() package manager and use the provided `default.nix` file to grab everything. As this version does not pin down the Qt version, you may (but unlikely) have to work the version magic of nix and hydra.

### Less clean way: Using homebrew

For now, I'm assuming that you are trying this on a mac. Windows is currently untested.

#### Step 1. Grab QT

I used homebrew to get the proper version, there is a very nice and detailed
guide for ["How do I install a specific version of a formula in
homebrew?"](http://stackoverflow.com/a/4158763)


#### Step 2. Build the software

After installing QT, homebrew told me:

```
We agreed to the Qt opensource license for you.
If this is unacceptable you should uninstall.

This formula is keg-only, which means it was not symlinked into /usr/local.

Qt 5 has CMake issues when linked

Generally there are no consequences of this for you. If you build your
own software and it requires this formula, you'll need to add to your
build variables:

    LDFLAGS:  -L/usr/local/opt/qt5/lib
    CPPFLAGS: -I/usr/local/opt/qt5/include
    PKG_CONFIG_PATH: /usr/local/opt/qt5/lib/pkgconfig
```

So we need to add the proper QT location

```bash
# Create a build directory so we are nice and clean
mkdir _build && cd _build
# Generate the build
cmake . .. -DCMAKE_PREFIX_PATH=/usr/local/opt/qt5

# By default, command line Cmake creates unix makefiles,
# so we'll use just that
make
```

## Building on Windows

- You should install a fairly recent Qt (5.6+) via the Qt toolkit
  installer. This generally goes to `c:\qt`
  After the install, CMake requires that you set `CMAKE_PREFIX_PATH` to
  point to your specific Qt version of the toolkit (like
  `C:/Qt/5.7/msvc2015_64`)

- Building the statically served files requires `xxd` to be present.
  If CMake cannot find it, set the `RESOURCE_COMPILER` cache variable to
  point to your installed `xxd`

- 


