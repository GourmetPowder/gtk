# NMake Makefile portion for enabling features for Windows builds

!include detectenv-msvc.mak

# Configurable paths to the various interpreters we need
!ifndef PERL
PERL = perl
!endif

!ifndef PYTHON
PYTHON=python
!endif

# Configurable paths to the various scripts and tools that we are using
!ifndef GLIB_MKENUMS
GLIB_MKENUMS = $(PREFIX)\bin\glib-mkenums
!endif

!ifndef GLIB_GENMARSHAL
GLIB_GENMARSHAL = $(PREFIX)\bin\glib-genmarshal
!endif

!ifndef GLIB_COMPILE_RESOURCES
GLIB_COMPILE_RESOURCES = $(PREFIX)\bin\glib-compile-resources.exe
!endif

!ifndef GDBUS_CODEGEN
GDBUS_CODEGEN = $(PREFIX)\bin\gdbus-codegen
!endif

# Please do not change anything beneath this line unless maintaining the NMake Makefiles
GTK_MAJOR_VERSION = 3
GTK_MINOR_VERSION = 24
GTK_MICRO_VERSION = 10