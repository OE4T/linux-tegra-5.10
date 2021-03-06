Introduction
------------

There are cases where it is useful to shard (break apart) the kernel source
into separate directories, yet still create a single kernel image from the
combination of all of those directories. These separate directories could then
be distributed separately, or on different timescales. One example is during
development of support for a new piece of hardware, where there is often a
desire to defer release of software support until the hardware is released,
while still releasing modifications to the base kernel. This document
describes the "kernel overlays" feature, which supports this requirement.

High-level Description
----------------------

The kernel's build system can be explicitly told about so-called overlay source
trees. A list of overlays will be provided each time the build system is
invoked using the KERNEL_OVERLAYS environment variable, or a list of possible
overlays may be embedded into the top-level Makefile. The kernel build process
will automatically process source contained in these overlays along with the
base kernel source. This will be handled entirely by the core kernel build
process, so that no Kconfig file or Makefile will need to reference any
overlay.

The kernel source tree will be considered to be the union of the main kernel
source tree and all overlays. Any file access will search or combine files from
all overlays as appropriate. A consequence of this concept is that no overlay
should contain the same filename as any other overlay, except for Kconfig
files and Makefiles, which are explicitly handled as described below.

Kconfig Details
---------------

When the kernel configuration utility parses any Kconfig file, it will
automatically include equivalent files in each overlay. The algorithm will be
approximately:

	def parse_file(filename):
	    parse_file_imp(${srctree}/Kconfig)
	    for each overlay in overlays:
	        parse_file_imp(${srctree.${overlay}}/Kconfig)

Thus, any Kconfig options defined in overlay Kconfig files will be placed into
the Kconfig menu structure in the same location as (immediately following) any
options defined in the base kernel's Kconfig file. The choice_append and
menu_append Kconfig statements also allow appending to existing menus in some
cases.

Makefile Details
----------------

When the kernel build system reads any Makefile, it will automatically include
equivalent files in each overlay. The algorithm will be approximately:

	def include_makefile(filename):
	    include ${srctree}/filename
	    for each overlay in overlays:
	        include ${srctree.${overlay}}/Makefile

Thus, any assignments to obj-y will be considered part of the same Makefile as
the base kernel's Makefile, and hence end up in the same built-in.o.

Referencing Other Overlays
--------------------------

Overlays may depend on each-other. Dependency order should be well-defined and
non-cyclic; the base kernel and all overlays should form a Directed Acyclic
Graph. For example, an overlay that adds support for all released $vendor
CPUs could rely on the base kernel. An overlay that adds support for an as yet
unreleased CPU from the same vendor could rely on both the base vendor, and
the aforementioned $vendor overlay. One use-case is one overlay implementing
public functions, and prototyping those functions in a header file, and
another overlay needing to use those functions, and hence needing to include
header files from the first overlay.

Overlays should not hard-code the path (even relative) to other overlays. The
build system provides variables to allow overlays to be referenced by name.
Each overlay is assumed to be identified by a unique name. It is also assumed
that each overlay is stored in a directory with that name as the last
component. Given that, just as ${srctree} contains the path of the base kernel,
then ${srctree.${overlay_name}} contains the path of the given named overlay.

Examples
--------

Add a new source file to an existing directory
----------------------------------------------

In the overlay, write a Kconfig and Makefile that contain any new Kconfig
config statement that is required, and standard Makefile content. No
boilerplate logic is required:

overlay/drivers/i2c/busses/Kconfig:

	config I2C_FOO
	    tristate "FOO I2C"

overlay/drivers/i2c/busses/Makefile:

	obj-$(CONFIG_I2C_FOO) += foo.o

Add a new directory into the source tree
----------------------------------------

Determine the parent directory that should reference the new directory's
Kconfig and Makefile files, as if you were to add the directory to the base
kernel. This is typically the physical parent directory of the new directory,
although in some cases, directories are referenced from other places instead.
This parent directory must already be linked into the Kconfig and Makefiles in
the base kernel, or in some earlier overlay.

Assume we wish to add drivers/foo/, and reference this from the existing Kconfig
and Makefile in drivers/.

In the overlay that contains the new directory, create or edit the chosen
parent directory's (drivers/) Kconfig file and Makefile to reference the new
directory as if it existed in the base kernel, without any boilerplate logic.
For example:

overlay/drivers/Kconfig:

	source "drivers/foo/Kconfig"

overlay/drivers/Makefile:

	obj-y += foo/

In the overlay that contains the new directory, create the content for the new
directory in exactly the same way that you would if the new directory were part
of the base kernel:

overlay/drivers/foo/Kconfig:

	config FOO
	    bool "Description"

overlay/drivers/foo/Makefile:

	# Just an example of $srctree.xxx usage; not required in all cases
	ccflags-y += -I${srctree.other_overlay}/some/utility/lib/include

	obj-$(CONFIG_FOO) += foo.o

overlay/drivers/foo/foo.c

	Standard source code
