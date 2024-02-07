/* Object file "section" support for the BFD library.
   Copyright (C) 1990-2017 Free Software Foundation, Inc.
   Written by Cygnus Support.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/*
SECTION
	Sections

	The raw data contained within a BFD is maintained through the
	section abstraction.  A single BFD may have any number of
	sections.  It keeps hold of them by pointing to the first;
	each one points to the next in the list.

	Sections are supported in BFD in <<section.c>>.

@menu
@* Section Input::
@* Section Output::
@* typedef asection::
@* section prototypes::
@end menu

INODE
Section Input, Section Output, Sections, Sections
SUBSECTION
	Section input

	When a BFD is opened for reading, the section structures are
	created and attached to the BFD.

	Each section has a name which describes the section in the
	outside world---for example, <<a.out>> would contain at least
	three sections, called <<.text>>, <<.data>> and <<.bss>>.

	Names need not be unique; for example a COFF file may have several
	sections named <<.data>>.

	Sometimes a BFD will contain more than the ``natural'' number of
	sections. A back end may attach other sections containing
	constructor data, or an application may add a section (using
	<<bfd_make_section>>) to the sections attached to an already open
	BFD. For example, the linker creates an extra section
	<<COMMON>> for each input file's BFD to hold information about
	common storage.

	The raw data is not necessarily read in when
	the section descriptor is created. Some targets may leave the
	data in place until a <<bfd_get_section_contents>> call is
	made. Other back ends may read in all the data at once.  For
	example, an S-record file has to be read once to determine the
	size of the data. An IEEE-695 file doesn't contain raw data in
	sections, but data and relocation expressions intermixed, so
	the data area has to be parsed to get out the data and
	relocations.

INODE
Section Output, typedef asection, Section Input, Sections

SUBSECTION
	Section output

	To write a new object style BFD, the various sections to be
	written have to be created. They are attached to the BFD in
	the same way as input sections; data is written to the
	sections using <<bfd_set_section_contents>>.

	Any program that creates or combines sections (e.g., the assembler
	and linker) must use the <<asection>> fields <<output_section>> and
	<<output_offset>> to indicate the file sections to which each
	section must be written.  (If the section is being created from
	scratch, <<output_section>> should probably point to the section
	itself and <<output_offset>> should probably be zero.)

	The data to be written comes from input sections attached
	(via <<output_section>> pointers) to
	the output sections.  The output section structure can be
	considered a filter for the input section: the output section
	determines the vma of the output data and the name, but the
	input section determines the offset into the output section of
	the data to be written.

	E.g., to create a section "O", starting at 0x100, 0x123 long,
	containing two subsections, "A" at offset 0x0 (i.e., at vma
	0x100) and "B" at offset 0x20 (i.e., at vma 0x120) the <<asection>>
	structures would look like:

|   section name          "A"
|     output_offset   0x00
|     size            0x20
|     output_section ----------->  section name    "O"
|                             |    vma             0x100
|   section name          "B" |    size            0x123
|     output_offset   0x20    |
|     size            0x103   |
|     output_section  --------|

SUBSECTION
	Link orders

	The data within a section is stored in a @dfn{link_order}.
	These are much like the fixups in <<gas>>.  The link_order
	abstraction allows a section to grow and shrink within itself.

	A link_order knows how big it is, and which is the next
	link_order and where the raw data for it is; it also points to
	a list of relocations which apply to it.

	The link_order is used by the linker to perform relaxing on
	final code.  The compiler creates code which is as big as
	necessary to make it work without relaxing, and the user can
	select whether to relax.  Sometimes relaxing takes a lot of
	time.  The linker runs around the relocations to see if any
	are attached to data which can be shrunk, if so it does it on
	a link_order by link_order basis.

*/

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "bfdlink.h"
#include <stdio.h>
#include <string.h>

/*
DOCDD
INODE
typedef asection, section prototypes, Section Output, Sections
SUBSECTION
	typedef asection

	Here is the section structure:

CODE_FRAGMENT
.
.typedef struct bfd_section
.{
.  {* The name of the section; the name isn't a copy, the pointer is
.     the same as that passed to bfd_make_section.  *}
.  const char *name;
.
.  {* A unique sequence number.  *}
.  unsigned int id;
.
.  {* Which section in the bfd; 0..n-1 as sections are created in a bfd.  *}
.  unsigned int index;
.
.  {* The next section in the list belonging to the BFD, or NULL.  *}
.  struct bfd_section *next;
.
.  {* The previous section in the list belonging to the BFD, or NULL.  *}
.  struct bfd_section *prev;
.
.  {* The field flags contains attributes of the section. Some
.     flags are read in from the object file, and some are
.     synthesized from other information.  *}
.  flagword flags;
.
.#define SEC_NO_FLAGS   0x000
.
.  {* Tells the OS to allocate space for this section when loading.
.     This is clear for a section containing debug information only.  *}
.#define SEC_ALLOC      0x001
.
.  {* Tells the OS to load the section from the file when loading.
.     This is clear for a .bss section.  *}
.#define SEC_LOAD       0x002
.
.  {* The section contains data still to be relocated, so there is
.     some relocation information too.  *}
.#define SEC_RELOC      0x004
.
.  {* A signal to the OS that the section contains read only data.  *}
.#define SEC_READONLY   0x008
.
.  {* The section contains code only.  *}
.#define SEC_CODE       0x010
.
.  {* The section contains data only.  *}
.#define SEC_DATA       0x020
.
.  {* The section will reside in ROM.  *}
.#define SEC_ROM        0x040
.
.  {* The section contains constructor information. This section
.     type is used by the linker to create lists of constructors and
.     destructors used by <<g++>>. When a back end sees a symbol
.     which should be used in a constructor list, it creates a new
.     section for the type of name (e.g., <<__CTOR_LIST__>>), attaches
.     the symbol to it, and builds a relocation. To build the lists
.     of constructors, all the linker has to do is catenate all the
.     sections called <<__CTOR_LIST__>> and relocate the data
.     contained within - exactly the operations it would peform on
.     standard data.  *}
.#define SEC_CONSTRUCTOR 0x080
.
.  {* The section has contents - a data section could be
.     <<SEC_ALLOC>> | <<SEC_HAS_CONTENTS>>; a debug section could be
.     <<SEC_HAS_CONTENTS>>  *}
.#define SEC_HAS_CONTENTS 0x100
.
.  {* An instruction to the linker to not output the section
.     even if it has information which would normally be written.  *}
.#define SEC_NEVER_LOAD 0x200
.
.  {* The section contains thread local data.  *}
.#define SEC_THREAD_LOCAL 0x400
.
.  {* The section has GOT references.  This flag is only for the
.     linker, and is currently only used by the elf32-hppa back end.
.     It will be set if global offset table references were detected
.     in this section, which indicate to the linker that the section
.     contains PIC code, and must be handled specially when doing a
.     static link.  *}
.#define SEC_HAS_GOT_REF 0x800
.
.  {* The section contains common symbols (symbols may be defined
.     multiple times, the value of a symbol is the amount of
.     space it requires, and the largest symbol value is the one
.     used).  Most targets have exactly one of these (which we
.     translate to bfd_com_section_ptr), but ECOFF has two.  *}
.#define SEC_IS_COMMON 0x1000
.
.  {* The section contains only debugging information.  For
.     example, this is set for ELF .debug and .stab sections.
.     strip tests this flag to see if a section can be
.     discarded.  *}
.#define SEC_DEBUGGING 0x2000
.
.  {* The contents of this section are held in memory pointed to
.     by the contents field.  This is checked by bfd_get_section_contents,
.     and the data is retrieved from memory if appropriate.  *}
.#define SEC_IN_MEMORY 0x4000
.
.  {* The contents of this section are to be excluded by the
.     linker for executable and shared objects unless those
.     objects are to be further relocated.  *}
.#define SEC_EXCLUDE 0x8000
.
.  {* The contents of this section are to be sorted based on the sum of
.     the symbol and addend values specified by the associated relocation
.     entries.  Entries without associated relocation entries will be
.     appended to the end of the section in an unspecified order.  *}
.#define SEC_SORT_ENTRIES 0x10000
.
.  {* When linking, duplicate sections of the same name should be
.     discarded, rather than being combined into a single section as
.     is usually done.  This is similar to how common symbols are
.     handled.  See SEC_LINK_DUPLICATES below.  *}
.#define SEC_LINK_ONCE 0x20000
.
.  {* If SEC_LINK_ONCE is set, this bitfield describes how the linker
.     should handle duplicate sections.  *}
.#define SEC_LINK_DUPLICATES 0xc0000
.
.  {* This value for SEC_LINK_DUPLICATES means that duplicate
.     sections with the same name should simply be discarded.  *}
.#define SEC_LINK_DUPLICATES_DISCARD 0x0
.
.  {* This value for SEC_LINK_DUPLICATES means that the linker
.     should warn if there are any duplicate sections, although
.     it should still only link one copy.  *}
.#define SEC_LINK_DUPLICATES_ONE_ONLY 0x40000
.
.  {* This value for SEC_LINK_DUPLICATES means that the linker
.     should warn if any duplicate sections are a different size.  *}
.#define SEC_LINK_DUPLICATES_SAME_SIZE 0x80000
.
.  {* This value for SEC_LINK_DUPLICATES means that the linker
.     should warn if any duplicate sections contain different
.     contents.  *}
.#define SEC_LINK_DUPLICATES_SAME_CONTENTS \
.  (SEC_LINK_DUPLICATES_ONE_ONLY | SEC_LINK_DUPLICATES_SAME_SIZE)
.
.  {* This section was created by the linker as part of dynamic
.     relocation or other arcane processing.  It is skipped when
.     going through the first-pass output, trusting that someone
.     else up the line will take care of it later.  *}
.#define SEC_LINKER_CREATED 0x100000
.
.  {* This section should not be subject to garbage collection.
.     Also set to inform the linker that this section should not be
.     listed in the link map as discarded.  *}
.#define SEC_KEEP 0x200000
.
.  {* This section contains "short" data, and should be placed
.     "near" the GP.  *}
.#define SEC_SMALL_DATA 0x400000
.
.  {* Attempt to merge identical entities in the section.
.     Entity size is given in the entsize field.  *}
.#define SEC_MERGE 0x800000
.
.  {* If given with SEC_MERGE, entities to merge are zero terminated
.     strings where entsize specifies character size instead of fixed
.     size entries.  *}
.#define SEC_STRINGS 0x1000000
.
.  {* This section contains data about section groups.  *}
.#define SEC_GROUP 0x2000000
.
.  {* The section is a COFF shared library section.  This flag is
.     only for the linker.  If this type of section appears in
.     the input file, the linker must copy it to the output file
.     without changing the vma or size.  FIXME: Although this
.     was originally intended to be general, it really is COFF
.     specific (and the flag was renamed to indicate this).  It
.     might be cleaner to have some more general mechanism to
.     allow the back end to control what the linker does with
.     sections.  *}
.#define SEC_COFF_SHARED_LIBRARY 0x4000000
.
.  {* This input section should be copied to output in reverse order
.     as an array of pointers.  This is for ELF linker internal use
.     only.  *}
.#define SEC_ELF_REVERSE_COPY 0x4000000
.
.  {* This section contains data which may be shared with other
.     executables or shared objects. This is for COFF only.  *}
.#define SEC_COFF_SHARED 0x8000000
.
.  {* This section should be compressed.  This is for ELF linker
.     internal use only.  *}
.#define SEC_ELF_COMPRESS 0x8000000
.
.  {* When a section with this flag is being linked, then if the size of
.     the input section is less than a page, it should not cross a page
.     boundary.  If the size of the input section is one page or more,
.     it should be aligned on a page boundary.  This is for TI
.     TMS320C54X only.  *}
.#define SEC_TIC54X_BLOCK 0x10000000
.
.  {* This section should be renamed.  This is for ELF linker
.     internal use only.  *}
.#define SEC_ELF_RENAME 0x10000000
.
.  {* Conditionally link this section; do not link if there are no
.     references found to any symbol in the section.  This is for TI
.     TMS320C54X only.  *}
.#define SEC_TIC54X_CLINK 0x20000000
.
.  {* This section contains vliw code.  This is for Toshiba MeP only.  *}
.#define SEC_MEP_VLIW 0x20000000
.
.  {* Indicate that section has the no read flag set. This happens
.     when memory read flag isn't set. *}
.#define SEC_COFF_NOREAD 0x40000000
.
.  {* Indicate that section has the purecode flag set.  *}
.#define SEC_ELF_PURECODE 0x80000000
.
.  {*  End of section flags.  *}
.
.  {* Some internal packed boolean fields.  *}
.
.  {* See the vma field.  *}
.  unsigned int user_set_vma : 1;
.
.  {* A mark flag used by some of the linker backends.  *}
.  unsigned int linker_mark : 1;
.
.  {* Another mark flag used by some of the linker backends.  Set for
.     output sections that have an input section.  *}
.  unsigned int linker_has_input : 1;
.
.  {* Mark flag used by some linker backends for garbage collection.  *}
.  unsigned int gc_mark : 1;
.
.  {* Section compression status.  *}
.  unsigned int compress_status : 2;
.#define COMPRESS_SECTION_NONE    0
.#define COMPRESS_SECTION_DONE    1
.#define DECOMPRESS_SECTION_SIZED 2
.
.  {* The following flags are used by the ELF linker. *}
.
.  {* Mark sections which have been allocated to segments.  *}
.  unsigned int segment_mark : 1;
.
.  {* Type of sec_info information.  *}
.  unsigned int sec_info_type:3;
.#define SEC_INFO_TYPE_NONE      0
.#define SEC_INFO_TYPE_STABS     1
.#define SEC_INFO_TYPE_MERGE     2
.#define SEC_INFO_TYPE_EH_FRAME  3
.#define SEC_INFO_TYPE_JUST_SYMS 4
.#define SEC_INFO_TYPE_TARGET    5
.#define SEC_INFO_TYPE_EH_FRAME_ENTRY 6
.
.  {* Nonzero if this section uses RELA relocations, rather than REL.  *}
.  unsigned int use_rela_p:1;
.
.  {* Bits used by various backends.  The generic code doesn't touch
.     these fields.  *}
.
.  unsigned int sec_flg0:1;
.  unsigned int sec_flg1:1;
.  unsigned int sec_flg2:1;
.  unsigned int sec_flg3:1;
.  unsigned int sec_flg4:1;
.  unsigned int sec_flg5:1;
.
.  {* End of internal packed boolean fields.  *}
.
.  {*  The virtual memory address of the section - where it will be
.      at run time.  The symbols are relocated against this.  The
.      user_set_vma flag is maintained by bfd; if it's not set, the
.      backend can assign addresses (for example, in <<a.out>>, where
.      the default address for <<.data>> is dependent on the specific
.      target and various flags).  *}
.  bfd_vma vma;
.
.  {*  The load address of the section - where it would be in a
.      rom image; really only used for writing section header
.      information.  *}
.  bfd_vma lma;
.
.  {* The size of the section in *octets*, as it will be output.
.     Contains a value even if the section has no contents (e.g., the
.     size of <<.bss>>).  *}
.  bfd_size_type size;
.
.  {* For input sections, the original size on disk of the section, in
.     octets.  This field should be set for any section whose size is
.     changed by linker relaxation.  It is required for sections where
.     the linker relaxation scheme doesn't cache altered section and
.     reloc contents (stabs, eh_frame, SEC_MERGE, some coff relaxing
.     targets), and thus the original size needs to be kept to read the
.     section multiple times.  For output sections, rawsize holds the
.     section size calculated on a previous linker relaxation pass.  *}
.  bfd_size_type rawsize;
.
.  {* The compressed size of the section in octets.  *}
.  bfd_size_type compressed_size;
.
.  {* Relaxation table. *}
.  struct relax_table *relax;
.
.  {* Count of used relaxation table entries. *}
.  int relax_count;
.
.
.  {* If this section is going to be output, then this value is the
.     offset in *bytes* into the output section of the first byte in the
.     input section (byte ==> smallest addressable unit on the
.     target).  In most cases, if this was going to start at the
.     100th octet (8-bit quantity) in the output section, this value
.     would be 100.  However, if the target byte size is 16 bits
.     (bfd_octets_per_byte is "2"), this value would be 50.  *}
.  bfd_vma output_offset;
.
.  {* The output section through which to map on output.  *}
.  struct bfd_section *output_section;
.
.  {* The alignment requirement of the section, as an exponent of 2 -
.     e.g., 3 aligns to 2^3 (or 8).  *}
.  unsigned int alignment_power;
.
.  {* If an input section, a pointer to a vector of relocation
.     records for the data in this section.  *}
.  struct reloc_cache_entry *relocation;
.
.  {* If an output section, a pointer to a vector of pointers to
.     relocation records for the data in this section.  *}
.  struct reloc_cache_entry **orelocation;
.
.  {* The number of relocation records in one of the above.  *}
.  unsigned reloc_count;
.
.  {* Information below is back end specific - and not always used
.     or updated.  *}
.
.  {* File position of section data.  *}
.  file_ptr filepos;
.
.  {* File position of relocation info.  *}
.  file_ptr rel_filepos;
.
.  {* File position of line data.  *}
.  file_ptr line_filepos;
.
.  {* Pointer to data for applications.  *}
.  void *userdata;
.
.  {* If the SEC_IN_MEMORY flag is set, this points to the actual
.     contents.  *}
.  unsigned char *contents;
.
.  {* Attached line number information.  *}
.  alent *lineno;
.
.  {* Number of line number records.  *}
.  unsigned int lineno_count;
.
.  {* Entity size for merging purposes.  *}
.  unsigned int entsize;
.
.  {* Points to the kept section if this section is a link-once section,
.     and is discarded.  *}
.  struct bfd_section *kept_section;
.
.  {* When a section is being output, this value changes as more
.     linenumbers are written out.  *}
.  file_ptr moving_line_filepos;
.
.  {* What the section number is in the target world.  *}
.  int target_index;
.
.  void *used_by_bfd;
.
.  {* If this is a constructor section then here is a list of the
.     relocations created to relocate items within it.  *}
.  struct relent_chain *constructor_chain;
.
.  {* The BFD which owns the section.  *}
.  bfd *owner;
.
.  {* A symbol which points at this section only.  *}
.  struct bfd_symbol *symbol;
.  struct bfd_symbol **symbol_ptr_ptr;
.
.  {* Early in the link process, map_head and map_tail are used to build
.     a list of input sections attached to an output section.  Later,
.     output sections use these fields for a list of bfd_link_order
.     structs.  *}
.  union {
.    struct bfd_link_order *link_order;
.    struct bfd_section *s;
.  } map_head, map_tail;
.} asection;
.
.{* Relax table contains information about instructions which can
.   be removed by relaxation -- replacing a long address with a
.   short address.  *}
.struct relax_table {
.  {* Address where bytes may be deleted. *}
.  bfd_vma addr;
.
.  {* Number of bytes to be deleted.  *}
.  int size;
.};
.
.{* Note: the following are provided as inline functions rather than macros
.   because not all callers use the return value.  A macro implementation
.   would use a comma expression, eg: "((ptr)->foo = val, TRUE)" and some
.   compilers will complain about comma expressions that have no effect.  *}
.static inline bfd_boolean
.bfd_set_section_userdata (bfd * abfd ATTRIBUTE_UNUSED, asection * ptr, void * val)
.{
.  ptr->userdata = val;
.  return TRUE;
.}
.
.static inline bfd_boolean
.bfd_set_section_vma (bfd * abfd ATTRIBUTE_UNUSED, asection * ptr, bfd_vma val)
.{
.  ptr->vma = ptr->lma = val;
.  ptr->user_set_vma = TRUE;
.  return TRUE;
.}
.
.static inline bfd_boolean
.bfd_set_section_alignment (bfd * abfd ATTRIBUTE_UNUSED, asection * ptr, unsigned int val)
.{
.  ptr->alignment_power = val;
.  return TRUE;
.}
.
.{* These sections are global, and are managed by BFD.  The application
.   and target back end are not permitted to change the values in
.   these sections.  *}
.extern asection _bfd_std_section[4];
.
.#define BFD_ABS_SECTION_NAME "*ABS*"
.#define BFD_UND_SECTION_NAME "*UND*"
.#define BFD_COM_SECTION_NAME "*COM*"
.#define BFD_IND_SECTION_NAME "*IND*"
.
.{* Pointer to the common section.  *}
.#define bfd_com_section_ptr (&_bfd_std_section[0])
.{* Pointer to the undefined section.  *}
.#define bfd_und_section_ptr (&_bfd_std_section[1])
.{* Pointer to the absolute section.  *}
.#define bfd_abs_section_ptr (&_bfd_std_section[2])
.{* Pointer to the indirect section.  *}
.#define bfd_ind_section_ptr (&_bfd_std_section[3])
.
.#define bfd_is_und_section(sec) ((sec) == bfd_und_section_ptr)
.#define bfd_is_abs_section(sec) ((sec) == bfd_abs_section_ptr)
.#define bfd_is_ind_section(sec) ((sec) == bfd_ind_section_ptr)
.
.#define bfd_is_const_section(SEC)		\
. (   ((SEC) == bfd_abs_section_ptr)		\
.  || ((SEC) == bfd_und_section_ptr)		\
.  || ((SEC) == bfd_com_section_ptr)		\
.  || ((SEC) == bfd_ind_section_ptr))
.
.{* Macros to handle insertion and deletion of a bfd's sections.  These
.   only handle the list pointers, ie. do not adjust section_count,
.   target_index etc.  *}
.#define bfd_section_list_remove(ABFD, S) \
.  do							\
.    {							\
.      asection *_s = S;				\
.      asection *_next = _s->next;			\
.      asection *_prev = _s->prev;			\
.      if (_prev)					\
.        _prev->next = _next;				\
.      else						\
.        (ABFD)->sections = _next;			\
.      if (_next)					\
.        _next->prev = _prev;				\
.      else						\
.        (ABFD)->section_last = _prev;			\
.    }							\
.  while (0)
.#define bfd_section_list_append(ABFD, S) \
.  do							\
.    {							\
.      asection *_s = S;				\
.      bfd *_abfd = ABFD;				\
.      _s->next = NULL;					\
.      if (_abfd->section_last)				\
.        {						\
.          _s->prev = _abfd->section_last;		\
.          _abfd->section_last->next = _s;		\
.        }						\
.      else						\
.        {						\
.          _s->prev = NULL;				\
.          _abfd->sections = _s;			\
.        }						\
.      _abfd->section_last = _s;			\
.    }							\
.  while (0)
.#define bfd_section_list_prepend(ABFD, S) \
.  do							\
.    {							\
.      asection *_s = S;				\
.      bfd *_abfd = ABFD;				\
.      _s->prev = NULL;					\
.      if (_abfd->sections)				\
.        {						\
.          _s->next = _abfd->sections;			\
.          _abfd->sections->prev = _s;			\
.        }						\
.      else						\
.        {						\
.          _s->next = NULL;				\
.          _abfd->section_last = _s;			\
.        }						\
.      _abfd->sections = _s;				\
.    }							\
.  while (0)
.#define bfd_section_list_insert_after(ABFD, A, S) \
.  do							\
.    {							\
.      asection *_a = A;				\
.      asection *_s = S;				\
.      asection *_next = _a->next;			\
.      _s->next = _next;				\
.      _s->prev = _a;					\
.      _a->next = _s;					\
.      if (_next)					\
.        _next->prev = _s;				\
.      else						\
.        (ABFD)->section_last = _s;			\
.    }							\
.  while (0)
.#define bfd_section_list_insert_before(ABFD, B, S) \
.  do							\
.    {							\
.      asection *_b = B;				\
.      asection *_s = S;				\
.      asection *_prev = _b->prev;			\
.      _s->prev = _prev;				\
.      _s->next = _b;					\
.      _b->prev = _s;					\
.      if (_prev)					\
.        _prev->next = _s;				\
.      else						\
.        (ABFD)->sections = _s;				\
.    }							\
.  while (0)
.#define bfd_section_removed_from_list(ABFD, S) \
.  ((S)->next == NULL ? (ABFD)->section_last != (S) : (S)->next->prev != (S))
.
.#define BFD_FAKE_SECTION(SEC, SYM, NAME, IDX, FLAGS)			\
.  {* name, id,  index, next, prev, flags, user_set_vma,            *}	\
.  {  NAME, IDX, 0,     NULL, NULL, FLAGS, 0,				\
.									\
.  {* linker_mark, linker_has_input, gc_mark, decompress_status,    *}	\
.     0,           0,                1,       0,			\
.									\
.  {* segment_mark, sec_info_type, use_rela_p,                      *}	\
.     0,            0,             0,					\
.									\
.  {* sec_flg0, sec_flg1, sec_flg2, sec_flg3, sec_flg4, sec_flg5,   *}	\
.     0,        0,        0,        0,        0,        0,		\
.									\
.  {* vma, lma, size, rawsize, compressed_size, relax, relax_count, *}	\
.     0,   0,   0,    0,       0,               0,     0,		\
.									\
.  {* output_offset, output_section, alignment_power,               *}	\
.     0,             &SEC,           0,					\
.									\
.  {* relocation, orelocation, reloc_count, filepos, rel_filepos,   *}	\
.     NULL,       NULL,        0,           0,       0,			\
.									\
.  {* line_filepos, userdata, contents, lineno, lineno_count,       *}	\
.     0,            NULL,     NULL,     NULL,   0,			\
.									\
.  {* entsize, kept_section, moving_line_filepos,		     *}	\
.     0,       NULL,	      0,					\
.									\
.  {* target_index, used_by_bfd, constructor_chain, owner,          *}	\
.     0,            NULL,        NULL,              NULL,		\
.									\
.  {* symbol,                    symbol_ptr_ptr,                    *}	\
.     (struct bfd_symbol *) SYM, &SEC.symbol,				\
.									\
.  {* map_head, map_tail                                            *}	\
.     { NULL }, { NULL }						\
.    }
.
*/

/* We use a macro to initialize the static asymbol structures because
   traditional C does not permit us to initialize a union member while
   gcc warns if we don't initialize it.  */
 /* the_bfd, name, value, attr, section [, udata] */
#ifdef __STDC__
#define GLOBAL_SYM_INIT(NAME, SECTION) \
  { 0, NAME, 0, BSF_SECTION_SYM, SECTION, { 0 }}
#else
#define GLOBAL_SYM_INIT(NAME, SECTION) \
  { 0, NAME, 0, BSF_SECTION_SYM, SECTION }
#endif

/************************************************************************/
/* Gap specific, support for bfd encryption/decryption, used by gas and ld */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

/*
 *      Source is from https://github.com/kokke/tiny-AES-c/tree/master
 *
 *      Small portable AES128/192/256 in C. Pruned to support only CTR mode. AES128/192/256 are supported
 *      through static compilation flags.
 *
 */

#define AES128 1
//#define AES192 1
//#define AES256 1

#define	ENC_STR_SIZE	512
#define	AES_KEY_LEN	16
#define	AES_IV_LEN	16

#define AES_BLOCKLEN 16 // Block length in bytes - AES is 128b block only

// The number of columns comprising a state in AES. This is a constant in AES. Value=4
#define AES_Nb 4

#if defined(AES256) && (AES256 == 1)
    #define AES_KEYLEN 32
    #define AES_keyExpSize 240
    #define AES_Nk 8
    #define AES_Nr 14
#elif defined(AES192) && (AES192 == 1)
    #define AES_KEYLEN 24
    #define AES_keyExpSize 208
    #define AES_Nk 6
    #define AES_Nr 12
#else
    #define AES_KEYLEN 16   // Key length in bytes
    #define AES_keyExpSize 176
    #define AES_Nk 4        // The number of 32 bit words in a key.
    #define AES_Nr 10       // The number of rounds in AES Cipher.
#endif

typedef struct AES_ctx {
        uint8_t RoundKey[AES_keyExpSize];
        uint8_t Iv[AES_BLOCKLEN];
} AES_ctx_t;

typedef struct A_CryptedComponentT CryptedComponentT;

typedef struct A_CryptedComponentT {
	char *Name;
	char *Vendor;
	char *Server;
	char *UserAuth;
	unsigned char *Key;	// AES KeyLen/8
	unsigned char *Iv;	// Initialization variable, retrieved from Vendor's server. Always 128b, 16B
	unsigned char *Nonce;	// Nonce coming from the PulpChipInfo section of this coomponent's obj file
	CryptedComponentT *Next;
} CryptedComponentT;


typedef struct {
	int Mode;				// 0: ASM, 1: Linker, 2: Dump
	int Verbose;
	CryptedComponentT *Components;		// All components
	CryptedComponentT *OutComponent;	// Active output Component
	AES_ctx_t AES_Ctx;			// AES context
} EncryptInfoT;

typedef enum {
	ERR_NOERR_EOF = -1,
	ERR_NOERR = 0,
	ERR_UNEXPECTED_EOF = 1,
	ERR_EXPECT_COMP = 2,
	ERR_EXPECT_SET = 3,
	ERR_EXPECT_STRING = 4,
	ERR_EXPECT_VENDOR = 5,
	ERR_EXPECT_SERVER = 6,
	ERR_EXPECT_KEY = 7,
	ERR_EXPECT_NAME = 8,
	ERR_BADKEYLEN = 9,
	ERR_KEYNONHEX = 10,
	ERR_EXPECT_USER = 11,
	ERR_EXPECT_SECTION = 12,
	ERR_EXPECT_COMP_OR_IV = 13,
	ERR_WRONG_COMP = 14,
} CompErrorT;

typedef enum {
	T_STRING=0,
	T_NAME=1,
	T_EOFT=2,
	T_UNKNOWN=3,
	T_UNTERM=4,
	T_SET=5,
	T_SEMI=6,
	T_COMPONENT=7,
	T_SERVER=8,
	T_VENDOR=9,
	T_KEY=10,
	T_USER=11,
	T_IV=12,
	T_VERBOSE=13,
} TokenT;

static EncryptInfoT EncryptInfo = {0, 0, 0, 0, {{0}, {0}}};

typedef uint8_t AES_state_t[4][4];

// The lookup-tables are marked const so they can be placed in read-only storage instead of RAM
// The numbers below can be computed dynamically trading ROM for RAM -
// This can be useful in (embedded) bootloader applications, where ROM is often limited.
static const uint8_t AES_sbox[256] = {
  //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };



// The round constant word array, AES_Rcon[i], contains the values given by
// x to the power (i-1) being powers of x (x is denoted as {02}) in the field GF(2^8)
static const uint8_t AES_Rcon[11] = { 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };

#define AES_getSBoxValue(num) (AES_sbox[(num)])

// This function produces AES_Nb(AES_Nr+1) round keys. The round keys are used in each round to decrypt the states.
static void AES_KeyExpansion(uint8_t* RoundKey, const uint8_t* Key)

{
	uint8_t tempa[4]; // Used for the column/row operations

	// The first round key is the key itself.
	for (int i = 0; i < AES_Nk; ++i) {
		RoundKey[(i * 4) + 0] = Key[(i * 4) + 0];
		RoundKey[(i * 4) + 1] = Key[(i * 4) + 1];
		RoundKey[(i * 4) + 2] = Key[(i * 4) + 2];
		RoundKey[(i * 4) + 3] = Key[(i * 4) + 3];
	}
	// All other round keys are found from the previous round keys.
	for (int i = AES_Nk; i < AES_Nb * (AES_Nr + 1); ++i) {
		int k = (i - 1) * 4;
		tempa[0] = RoundKey[k + 0];
		tempa[1] = RoundKey[k + 1];
		tempa[2] = RoundKey[k + 2];
		tempa[3] = RoundKey[k + 3];
		if (i % AES_Nk == 0) {
			// This function shifts the 4 bytes in a word to the left once.
			// [a0,a1,a2,a3] becomes [a1,a2,a3,a0]

			// Function RotWord()
			{
				const uint8_t u8tmp = tempa[0];
				tempa[0] = tempa[1];
				tempa[1] = tempa[2];
				tempa[2] = tempa[3];
				tempa[3] = u8tmp;
			}
			// SubWord() is a function that takes a four-byte input word and
			// applies the S-box to each of the four bytes to produce an output word.

			// Function Subword()
			{
				tempa[0] = AES_getSBoxValue(tempa[0]);
				tempa[1] = AES_getSBoxValue(tempa[1]);
				tempa[2] = AES_getSBoxValue(tempa[2]);
				tempa[3] = AES_getSBoxValue(tempa[3]);
			}
			tempa[0] = tempa[0] ^ AES_Rcon[i/AES_Nk];
		}
#if defined(AES256) && (AES256 == 1)
		if (i % AES_Nk == 4) {
			// Function Subword()
			{
				tempa[0] = AES_getSBoxValue(tempa[0]);
				tempa[1] = AES_getSBoxValue(tempa[1]);
				tempa[2] = AES_getSBoxValue(tempa[2]);
				tempa[3] = AES_getSBoxValue(tempa[3]);
			}
		}
#endif
		int j = i * 4;
		k = (i - AES_Nk) * 4;
		RoundKey[j + 0] = RoundKey[k + 0] ^ tempa[0];
		RoundKey[j + 1] = RoundKey[k + 1] ^ tempa[1];
		RoundKey[j + 2] = RoundKey[k + 2] ^ tempa[2];
		RoundKey[j + 3] = RoundKey[k + 3] ^ tempa[3];
	}
}


static void AES_init_ctx(AES_ctx_t* ctx, const uint8_t* key)

{
	AES_KeyExpansion(ctx->RoundKey, key);
}

static void AES_init_ctx_iv(AES_ctx_t* ctx, const uint8_t* key, const uint8_t* iv)

{
	AES_KeyExpansion(ctx->RoundKey, key);
	memcpy (ctx->Iv, iv, AES_BLOCKLEN);
}

static void AES_ctx_set_iv(AES_ctx_t* ctx, const uint8_t* iv, const uint8_t* nounce)

{
	for (int i=0; i<AES_BLOCKLEN; i++) ctx->Iv[i] = iv[i] ^ nounce[i];
	// memcpy (ctx->Iv, iv, AES_BLOCKLEN);
}

// This function adds the round key to state.
// The round key is added to the state by an XOR function.
static void AES_AddRoundKey(uint8_t round, AES_state_t* state, const uint8_t* RoundKey)

{
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			(*state)[i][j] ^= RoundKey[(round * AES_Nb * 4) + (i * AES_Nb) + j];
}

// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
static void AES_SubBytes(AES_state_t* state)

{
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j) (*state)[j][i] = AES_getSBoxValue((*state)[j][i]);
}

// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
static void AES_ShiftRows(AES_state_t* state)

{
	uint8_t temp;

	// Rotate first row 1 columns to left
	temp	   = (*state)[0][1];
	(*state)[0][1] = (*state)[1][1];
	(*state)[1][1] = (*state)[2][1];
	(*state)[2][1] = (*state)[3][1];
	(*state)[3][1] = temp;

	// Rotate second row 2 columns to left
	temp	   = (*state)[0][2];
	(*state)[0][2] = (*state)[2][2];
	(*state)[2][2] = temp;

	temp	   = (*state)[1][2];
	(*state)[1][2] = (*state)[3][2];
	(*state)[3][2] = temp;

	// Rotate third row 3 columns to left
	temp	   = (*state)[0][3];
	(*state)[0][3] = (*state)[3][3];
	(*state)[3][3] = (*state)[2][3];
	(*state)[2][3] = (*state)[1][3];
	(*state)[1][3] = temp;
}

static uint8_t AES_xtime(uint8_t x)

{
	return ((x<<1) ^ (((x>>7) & 1) * 0x1b));
}

// MixColumns function mixes the columns of the state matrix
static void AES_MixColumns(AES_state_t* state)

{
	uint8_t Tmp, Tm, t;
	for (int i = 0; i < 4; ++i) {
		t   = (*state)[i][0];
		Tmp = (*state)[i][0] ^ (*state)[i][1] ^ (*state)[i][2] ^ (*state)[i][3] ;
		Tm  = (*state)[i][0] ^ (*state)[i][1] ; Tm = AES_xtime(Tm);  (*state)[i][0] ^= Tm ^ Tmp ;
		Tm  = (*state)[i][1] ^ (*state)[i][2] ; Tm = AES_xtime(Tm);  (*state)[i][1] ^= Tm ^ Tmp ;
		Tm  = (*state)[i][2] ^ (*state)[i][3] ; Tm = AES_xtime(Tm);  (*state)[i][2] ^= Tm ^ Tmp ;
		Tm  = (*state)[i][3] ^ t ;	      Tm = AES_xtime(Tm);  (*state)[i][3] ^= Tm ^ Tmp ;
	}
}

// Cipher is the main function that encrypts the PlainText.
static void AES_Cipher(AES_state_t* state, const uint8_t* RoundKey)

{
	uint8_t round = 0;

	// Add the First round key to the state before starting the rounds.
	AES_AddRoundKey(0, state, RoundKey);

	// There will be AES_Nr rounds.
	// The first AES_Nr-1 rounds are identical.
	// These AES_Nr rounds are executed in the loop below.
	// Last one without MixColumns()
	for (round = 1; ; ++round) {
		AES_SubBytes(state);
		AES_ShiftRows(state);
		if (round == AES_Nr) break;
		AES_MixColumns(state);
		AES_AddRoundKey(round, state, RoundKey);
	}
	// Add round key to last round
	AES_AddRoundKey(AES_Nr, state, RoundKey);
}


/* Symmetrical operation: same function for encrypting as for decrypting. Note any IV/nonce should never be reused with the same key */
static void AES_CTR_xcrypt_buffer(AES_ctx_t* ctx, uint8_t* buf, size_t length)

{
	uint8_t buffer[AES_BLOCKLEN];

	size_t i;
	int bi;
	for (i = 0, bi = AES_BLOCKLEN; i < length; ++i, ++bi) {
		if (bi == AES_BLOCKLEN) { /* we need to regen xor compliment in buffer */
			memcpy(buffer, ctx->Iv, AES_BLOCKLEN);
			AES_Cipher((AES_state_t*)buffer, ctx->RoundKey);
			/* Increment Iv and handle overflow */
			for (bi = (AES_BLOCKLEN - 1); bi >= 0; --bi) {
				/* inc will overflow */
				if (ctx->Iv[bi] == 255) {
					ctx->Iv[bi] = 0; continue;
				}
				ctx->Iv[bi] += 1;
				break;
			}
			bi = 0;
		}
		buf[i] = (buf[i] ^ buffer[bi]);
	}
}

static void AES_CTR_xcrypt_buffer_From(AES_ctx_t* ctx, uint8_t* buf, unsigned int from, size_t length)

{
	uint8_t buffer[AES_BLOCKLEN];

	size_t i;
	int bi;
	// First increment iv till we reach from-1
	for (i = 0, bi = AES_BLOCKLEN; i < from; ++i, ++bi) {
		if (bi == AES_BLOCKLEN) { /* we need to regen xor compliment in buffer */
			if ((i+AES_BLOCKLEN) >= from) {
				memcpy(buffer, ctx->Iv, AES_BLOCKLEN);
				AES_Cipher((AES_state_t*)buffer, ctx->RoundKey);
			}
			for (bi = (AES_BLOCKLEN - 1); bi >= 0; --bi) {
				/* inc will overflow */
				if (ctx->Iv[bi] == 255) {
					ctx->Iv[bi] = 0; continue;
				}
				ctx->Iv[bi] += 1;
				break;
			}
			bi = 0;
		}
	}
	for (i = 0; i < length; ++i, ++bi) {
		if (bi == AES_BLOCKLEN) { /* we need to regen xor compliment in buffer */
			memcpy(buffer, ctx->Iv, AES_BLOCKLEN);
			AES_Cipher((AES_state_t*)buffer, ctx->RoundKey);
			/* Increment Iv and handle overflow */
			for (bi = (AES_BLOCKLEN - 1); bi >= 0; --bi) {
				/* inc will overflow */
				if (ctx->Iv[bi] == 255) {
					ctx->Iv[bi] = 0; continue;
				}
				ctx->Iv[bi] += 1;
				break;
			}
			bi = 0;
		}
		buf[i] = (buf[i] ^ buffer[bi]);
	}
}

#ifdef TEST_AES
void Dump(char *Mess, uint8_t *Buf, int Len)

{
	char S0[5], S1[5];
	char Str0[5*15+1], Str1[5*15+1];
	int N = 15;
	printf("====================== %s ===========================\n", Mess);
	Str0[0] = 0; Str1[0] = 0;
	for (int i=0; i<Len; i++) {
		char C = Buf[i];
		if (((i+1)%N)==0) {
			printf("%s\n", Str0);
			printf("%s\n", Str1);
			Str0[0] = 0; Str1[0] = 0;
		}
		if (isprint(C)) sprintf(S0, "  %c", C); else sprintf(S0, "???");
		sprintf(S1, "%3u", Buf[i]);
		strcat(Str0, S0);
		strcat(Str1, S1);
	}
	printf("%s\n", Str0);
	printf("%s\n", Str1);
}

char *Copy(char *From)

{
	int L = strlen(From)+1;
	char *S = (char *) malloc(sizeof(char)*L);
	strcpy(S, From);
	return S;
}

int Compare(char *Mess, uint8_t *I0, uint8_t *I1, int From, int Len)

{
	int Err=0;
	for (int i=From; i<(From+Len); i++) {
		if (I0[i] != I1[i]) {
			Err++;
			printf("\t%20s: At %4d I0=%3u, I1=%3u\n", Mess, i, I0[i], I1[i]);
		}
	}
	printf("%20s[%4d..%4d]: Check %s\n", Mess, From, From+Len-1, Err?"FAIL":"OK");
}

#define ASTR	"Ceci est un test d'AES 256 en partant d'un point arbitaire et pour une certaine longeur"
void TestAES()

{
	AES_ctx_t Ctx;
	uint8_t Key[32] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
                        0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4 };
	uint8_t Iv[16]  = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };
	uint8_t Nonce[16]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	char *Input = "1 " ASTR "2 " ASTR "3 " ASTR  "4 " ASTR  "6 " ASTR  "6 " ASTR  "7 " ASTR;
	char *In = (char *) malloc((strlen(Input)+1)*sizeof(char));
	strcpy(In, Input);

	AES_init_ctx(&Ctx, Key);

	// Dump("Before xcrypt", In, strlen(Input));
	char *RefIn = Copy(Input);
	AES_ctx_set_iv(&Ctx, Iv, Nonce);
	AES_CTR_xcrypt_buffer_From(&Ctx, (uint8_t*) In, 0, strlen(Input));

	char *RefEnc = Copy(In);
	AES_ctx_set_iv(&Ctx, Iv, Nonce);
	AES_CTR_xcrypt_buffer_From(&Ctx, (uint8_t*) In, 0, strlen(Input));
	
	Compare("Enc Dec Full", RefIn, In, 0, strlen(Input));


	int From = 15, Len = 35;

	for (int i=0; i<28; i++) {
		strcpy(In, Input);
		AES_ctx_set_iv(&Ctx, Iv, Nonce);
		AES_CTR_xcrypt_buffer_From(&Ctx, (uint8_t*) In + From, From, Len);
		printf("From: %4d, Len: %4d\n", From, Len);
		Compare("Enc Dec Part1", RefIn,  In, 0, From);
		Compare("Enc Dec Part2", RefEnc, In, From, Len);
		Compare("Enc Dec Part3", RefIn,  In, From+Len, strlen(Input)-From-Len);
		From++;
	}
	free(In);
	free(RefIn);
	free(RefEnc);
}
#endif

static char *EncryptReportError(CompErrorT Err, char *Str, int LineNo)

{
	if (LineNo>=1) sprintf(Str, "At line %d: ", LineNo); else Str[0] = 0;
	switch (Err) {
		case ERR_NOERR: strcat(Str,  "NoError"); break;
		case ERR_UNEXPECTED_EOF: strcat(Str,  "Unexpected EOF"); break;
		case ERR_EXPECT_COMP: strcat(Str,  "Expecting Component keyword here"); break;
		case ERR_EXPECT_SET: strcat(Str,  "Expecting : or = here"); break;
		case ERR_EXPECT_STRING: strcat(Str,  "Expecting string here"); break;
		case ERR_EXPECT_VENDOR: strcat(Str,  "Expecting Vendor keyword here"); break;
		case ERR_EXPECT_SERVER: strcat(Str,  "Expecting Server keyword here"); break;
		case ERR_EXPECT_KEY: strcat(Str,  "Expecting AES key here in hex format"); break;
		case ERR_EXPECT_NAME: strcat(Str,  "Expecting name here (sequence of letter and digit)"); break;
		case ERR_BADKEYLEN: strcat(Str,  "Wrong AES key length"); break;
		case ERR_KEYNONHEX: strcat(Str,  "Wrong AES key, not an hexadecimal number"); break;
		case ERR_EXPECT_USER: strcat(Str,  "Expecting User keyword here"); break;
		case ERR_EXPECT_SECTION: strcat(Str,  "Expecting one of {component, vendor, server, user, key, iv} here"); break;
		case ERR_EXPECT_COMP_OR_IV: strcat(Str,  "Expecting Component keyword or Iv keyword here"); break;
		case ERR_WRONG_COMP: strcat(Str, "Already defined component"); break;
		default: strcat(Str,  "Unknown error"); break;
	}
	return Str;
}

static TokenT NameToToken(char *Str, char *Buf)

{
	char c;
	int l=0;

	while (c=Str[l]) Buf[l++] = toupper(c);
	Buf[l] = 0;
	if      (strcmp(Buf, "COMPONENT") == 0) return T_COMPONENT;
	else if (strcmp(Buf, "VENDOR") == 0) return T_VENDOR;
	else if (strcmp(Buf, "SERVER") == 0) return T_SERVER;
	else if (strcmp(Buf, "USER") == 0) return T_USER;
	else if (strcmp(Buf, "KEY") == 0) return T_KEY;
	else if (strcmp(Buf, "IV") == 0) return T_IV;
	else if (strcmp(Buf, "VERBOSE") == 0) return T_VERBOSE;
	else return T_NAME;
}

static TokenT GetNextToken(FILE *Fi, char *Str, int *LineNo, int *TokLine)

{
	int c, p=0;

	/* Skip blanks */
	while (1) {
		c = fgetc(Fi);
		if (c==EOF) break;
		if (c=='/') {	// Comment
			if ((c = fgetc(Fi)) && (c=='*')) {;
				while (1) {
	      				while (((c = fgetc(Fi)) != EOF) && (c != '*')) {
					}
					if (c==EOF) return T_UNTERM;	// Unterminated comment
					c = fgetc(Fi);
					if (c=='/') {
						break;
					}
					ungetc(c, Fi);
				}
			} else if (c=='/') {
				while (1) {
	      				while (((c = fgetc(Fi)) != EOF) && (c != '\n')) ;
					if (c==EOF) return T_UNTERM;	// Unterminated comment
					break;
				}
			} else {
				ungetc(c, Fi); break;
			}
		} else if (!((c==' ') || (c=='\t') || (c=='\n') || (c=='\r'))) break;
		else if (c=='\n') {
			*LineNo = *LineNo + 1;
		}

	}
	*TokLine = *LineNo;
	if (c=='"') {
	      	while (((c = fgetc(Fi)) != EOF) && (c != '"')) {
			if (c == '\\') c = fgetc(Fi);
			if (c!=EOF) Str[p++] = c;
			else break;
		}
		if (c==EOF) return T_UNTERM;
		Str[p] = 0;
		return T_STRING;
	} else if (isalnum(c)) {
		while (isalnum(c)) {
			Str[p++] = c;
			c = fgetc(Fi);
		}
		ungetc(c, Fi);
		Str[p] = 0;
		return T_NAME;
	} else if (c=='=' || c==':') {
		return T_SET;
	} else if (c==';') {
		return T_SEMI;
	} else if (c==EOF)
		return T_EOFT;
	else {
		Str[p++] = c;
		Str[p] = 0;
		return T_UNKNOWN;
	}
}

static char *TokenImage(TokenT t)

{
	switch (t) {
		case T_STRING: return "String";
		case T_NAME: return "Name";
		case T_EOFT: return "EOF";
		case T_UNKNOWN: return "Unknown";
		case T_UNTERM: return "Unterm";
		case T_SEMI: return "Semi";
		case T_SET: return "Set";
		case T_COMPONENT: return "Component";
		case T_VENDOR: return "Vendor";
		case T_SERVER: return "Server";
		case T_KEY: return "Key";
		case T_USER: return "User";
		case T_IV: return "Iv";
		case T_VERBOSE: return "Verbose";
	}
}
       
static char *TokenContent(TokenT t, char *Str)

{
	if (t==T_STRING || t==T_NAME || t==T_UNKNOWN) {
		if (Str[0] == 0) return "<Empty>";
		else return Str;
	}else return 0;
}

static int TokenHasContent(TokenT t)

{
	return (t==T_STRING || t==T_NAME || t==T_UNKNOWN);
}



static int ToHex(char *S)

{
	char S0 = tolower(S[0]), S1 = tolower(S[1]);
	int R0, R1;
	if (S0>='0' && S0<='9') R0 = S0-'0';
	else if (S0>='a' && S0<='f') R0 = S0 - 'a' + 10;
	else return -1;

	if (S1>='0' && S1<='9') R1 = S1-'0';
	else if (S1>='a' && S1<='f') R1 = S1 - 'a' + 10;
	else return -1;

	return ((R0<<4)|(R1));
}
static int CheckKey(char *Str, int Len, unsigned char *Key)

{
	int L = strlen(Str);
	if (L != Len*2) return ERR_BADKEYLEN;
	for (int i=0; i<Len; i++) {
		int R = ToHex(Str+2*i);
		if (R==-1) return ERR_KEYNONHEX;
		Key[i] = R;
	}
	return ERR_NOERR;
}

CryptedComponentT *PushComponent(char *Name, CryptedComponentT **Head)

{
	CryptedComponentT *Pt, *PtPrev = 0;

	for (Pt = *Head; Pt; Pt = Pt->Next) {
		if (strcmp(Name, Pt->Name) == 0) break;
		PtPrev = Pt;
	}
	if (Pt) {
		printf("Component %s is already declared\n", Name);
		return 0;
	}
	Pt = (CryptedComponentT *) malloc(sizeof(CryptedComponentT));
	Pt->Name = strdup(Name);
	Pt->Vendor = 0;
	Pt->Server = 0;
	Pt->Key = 0;
	Pt->Iv = 0;
	Pt->Nonce = 0;
	Pt->Next = 0;
	if (PtPrev) PtPrev->Next = Pt;
	else *Head = Pt;
	return Pt;
}

CryptedComponentT *ComponentLookUp(char *Name, CryptedComponentT *HeadComp)

{
	unsigned int Ln = strlen(Name);
	for (CryptedComponentT *Pt = HeadComp; Pt; Pt = Pt->Next) {
		unsigned int Lc = strlen(Pt->Name);
		if (Ln>Lc) { // Matches the end of Name and checks that immediate predecessor is a directory separator
			if (strcmp(Name + (Ln-Lc), Pt->Name) == 0 && (Name[Ln-Lc-1] == '/' || Name[Ln-Lc-1] == '\\')) return Pt;
		} else if (Ln==Lc && strcmp(Name, Pt->Name) == 0) return Pt;
	}
	return 0;
}

int ComponentNonceUpdate(char *Name, unsigned char *Nonce)

{
	CryptedComponentT *Comp = ComponentLookUp(Name, EncryptInfo.Components);
	if (Comp==0) {
		return 0;
	}

	Comp->Nonce = (unsigned char *) malloc(sizeof(unsigned char)*AES_BLOCKLEN);
	for (int i=0; i<AES_BLOCKLEN; i++) Comp->Nonce[i] = Nonce[i];
	if (EncryptInfo.Verbose) {
		printf("Updating Obj: %s, Comp: %s with Nonce ", Name, Comp->Name);
		for (int i=0; i<AES_BLOCKLEN; i++) printf("%2x", Nonce[i]);
		printf("\n");
	}

	return 1;
}

int SetOutComponentIV(char *Name)

{
	int Trace = 0;
	uint8_t IvOut[AES_KEY_LEN];

	if (EncryptInfo.Components == 0) return 0;	// There are no EncryptInfos
	CryptedComponentT *OutComp = ComponentLookUp(Name, EncryptInfo.Components);

	int EncryptedIn = 0;
	for (CryptedComponentT *Pt = EncryptInfo.Components; Pt; Pt = Pt->Next) {
		if (Pt == OutComp) continue;
		if (Pt->Iv == 0) return 4;	// A Component without IV
		if (Trace) {
			printf("IV Out %20s, %20s, IV=", OutComp?OutComp->Name:"No OUT", Pt->Name);
			if (Pt->Iv)
				for (int i=0; i<AES_KEY_LEN; i++) printf("%.2x", Pt->Iv[i]);
			else
				for (int i=0; i<AES_KEY_LEN; i++) printf("??");
			printf("\n");
		}
//		if (Pt->Nonce) {
			if (EncryptedIn == 0) {
				for (int i=0; i<AES_KEY_LEN; i++) IvOut[i] = Pt->Iv[i];
			} else {
				for (int i=0; i<AES_KEY_LEN; i++) IvOut[i] ^= Pt->Iv[i];
			}
			EncryptedIn = 1;
//		}
	}
	if (OutComp) {
		// Linker out is in EncryptInfo and we have encrypted inputs, set Iv(Out) to exor of all used inputs
		if (OutComp->Iv == 0) OutComp->Iv = (uint8_t *) malloc(sizeof(uint8_t)*AES_KEY_LEN);
		for (int i=0; i<AES_KEY_LEN; i++) OutComp->Iv[i] = IvOut[i];
		return 1;
	} else if (EncryptedIn) {
		return 2;	// Linker out is not in EncryptInfo and we have encrypted input, this is an error, link is aborted
	} else {
		return 3;	// Linker out is not in EncryptInfo and we have no encrypted input, this is ok
	}
}

int ComponentMustBeEncrypted(char *Name)

{
	CryptedComponentT *Comp = ComponentLookUp(Name, EncryptInfo.Components);
	// if (Comp) printf("%s must be Encrypted\n", Name);
	return (Comp != 0);
}


static void DumpComponents(CryptedComponentT *Comp)

{
	int C = 1;
	for (CryptedComponentT *Pt=Comp; Pt; Pt = Pt->Next, C++) {
		// printf("%10s: %s\n", "Name", Pt->Name);
		printf("[%2d]%6s: %s\n", C, "Name", Pt->Name);
		printf("%10s: %s\n", "Vendor", Pt->Vendor);
		printf("%10s: %s\n", "Server", Pt->Server);
		printf("%10s: %s\n", "User", Pt->UserAuth);
		printf("%10s: ", "Key");
		for (int i=0; i<AES_KEY_LEN; i++) printf("%.2x", Pt->Key[i]);
		printf("\n");
		if (Pt->Iv) {
			printf("%10s: ", "Iv");
			for (int i=0; i<AES_KEY_LEN; i++) printf("%.2x", Pt->Iv[i]);
			printf("\n");
		} else printf("%10s: %s\n", "Iv", "None");
		if (Pt->Nonce) {
			for (int i=0; i<AES_BLOCKLEN; i++) printf("%.2x", Pt->Nonce[i]);
			printf("%10s: ", "Nonce");
		} else printf("%10s: %s\n", "Nonce", "None");
		printf("\n");
	}
}

static void DumpKeys(CryptedComponentT *Comp)

{
	printf("Key: ");
	for (int i=0; i<AES_KEY_LEN; i++) printf("%.2x", Comp->Key[i]);
	printf(" Iv: ");
	if (Comp->Iv)
		for (int i=0; i<AES_KEY_LEN; i++) printf("%.2x", Comp->Iv[i]);
	else
		for (int i=0; i<AES_KEY_LEN; i++) printf("??");
	printf(" Nonce: ");
	if (Comp->Nonce)
		for (int i=0; i<AES_BLOCKLEN; i++) printf("%.2x", Comp->Nonce[i]);
	else
		for (int i=0; i<AES_BLOCKLEN; i++) printf("??");
	printf("\n");
}

static int AcquireComponentIV(CryptedComponentT *Comp)

{
	if (Comp->Iv == 0) {
		/* Use infos in pointed components to retrieve the IV from the component owner */
		if (EncryptInfo.Verbose) {
			printf("Acquiring IV for Component %s\n", Comp->Name);
		}
	}
	return 0;
}

int OneSection(FILE *Fi, int *LineNo, int *TokLine, TokenT *Section, char *Content)

{
	char Buf[ENC_STR_SIZE];
	TokenT Tok;

	Tok = GetNextToken(Fi, Content, LineNo, TokLine);
	if (Tok == T_EOFT) return ERR_NOERR_EOF;
	if (Tok==T_NAME) Tok = NameToToken(Content, Buf);
	if (Tok==T_VERBOSE) {
		EncryptInfo.Verbose = 1;
		Tok = GetNextToken(Fi, Content, LineNo, TokLine);
		if (Tok == T_EOFT) return ERR_NOERR_EOF;
		if (Tok==T_NAME) Tok = NameToToken(Content, Buf);
	}
	if (!(Tok == T_COMPONENT || Tok == T_VENDOR || Tok == T_SERVER || Tok == T_USER || Tok == T_KEY || Tok == T_IV)) return ERR_EXPECT_SECTION;
	*Section = Tok;

	Tok = GetNextToken(Fi, Content, LineNo, TokLine);
	if (Tok == T_EOFT) return ERR_UNEXPECTED_EOF;
	if (Tok!=T_SET) return ERR_EXPECT_SET;

	Tok = GetNextToken(Fi, Content, LineNo, TokLine);
	if (Tok == T_EOFT) return ERR_UNEXPECTED_EOF;
	if (Tok!=T_STRING) return ERR_EXPECT_STRING;

	return ERR_NOERR;
}

static int ProcessComponents(FILE *Fi, int *LineNo, int *TokLine, CryptedComponentT **Comp, CryptedComponentT **Head)

{
	char Str[ENC_STR_SIZE], Buf[ENC_STR_SIZE];
	TokenT Section;
	int Trace = 0;
	int Err;
       
	// Component = "...."
	Err = OneSection(Fi, LineNo, TokLine, &Section, Str);
	if (Err) return Err;
	if (Section != T_COMPONENT) return ERR_EXPECT_COMP;
	if (Trace) printf("Component : %s\n", TokenContent(Section, Str));
	*Comp = PushComponent(Str, Head);
	if (*Comp == 0) return ERR_WRONG_COMP;

	while (1) {
		// Vendor = "...."
		Err = OneSection(Fi, LineNo, TokLine, &Section, Str);
		if (Err) return Err;
		if (Section != T_VENDOR) return ERR_EXPECT_VENDOR;
		if (Trace) printf("Vendor    : %s\n", TokenContent(Section, Str));
		(*Comp)->Vendor = strdup(Str);
	
		// Server = "..."
		Err = OneSection(Fi, LineNo, TokLine, &Section, Str);
		if (Err) return Err;
		if (Section != T_SERVER) return ERR_EXPECT_SERVER;
		if (Trace) printf("Server    : %s\n", TokenContent(Section, Str));
		(*Comp)->Server = strdup(Str);
	
		// User = "..."
		Err = OneSection(Fi, LineNo, TokLine, &Section, Str);
		if (Err) return Err;
		if (Section != T_USER) return ERR_EXPECT_USER;
		if (Trace) printf("User    : %s\n", TokenContent(Section, Str));
		(*Comp)->UserAuth = strdup(Str);
	
		// Key = "HexNum"
		Err = OneSection(Fi, LineNo, TokLine, &Section, Str);
		if (Err) return Err;
		if (Section != T_KEY) return ERR_EXPECT_KEY;
		unsigned char *Key = (unsigned char *) malloc(sizeof(unsigned char)*AES_KEY_LEN);
		int Status = CheckKey(Str, AES_KEY_LEN, Key);
		if (Status) {
			free(Key); return Status;
		}
		if (Trace) {
			for (int i=0; i<AES_KEY_LEN; i++) printf("%.2x", Key[i]);
			printf(">\n");
		}
		(*Comp)->Key = Key;
		(*Comp)->Iv = 0;
		(*Comp)->Nonce = 0;
	
		// Component or Iv = "HexNum"  Optional
		Err = OneSection(Fi, LineNo, TokLine, &Section, Str);
		if (Err) return Err;
		if (Section == T_COMPONENT) {
			if (Trace) printf("Component : %s\n", TokenContent(Section, Str));
			*Comp = PushComponent(Str, Head);
		} else if (Section == T_IV) {
			unsigned char *Iv = (unsigned char *) malloc(sizeof(unsigned char)*AES_KEY_LEN);
			int Status = CheckKey(Str, AES_KEY_LEN, Iv);
			if (Status) {
				free(Iv); return Status;
			}
			if (Trace) {
				for (int i=0; i<AES_KEY_LEN; i++) printf("%.2x", Iv[i]);
				printf(">\n");
			}
			(*Comp)->Iv = Iv;

			Err = OneSection(Fi, LineNo, TokLine, &Section, Str);
			if (Err) return Err;
			if (Section != T_COMPONENT) return ERR_EXPECT_COMP;
			if (Trace) printf("Component : %s\n", TokenContent(Section, Str));
			*Comp = PushComponent(Str, Head);
			if (*Comp == 0) return ERR_WRONG_COMP;
		} else return ERR_EXPECT_COMP_OR_IV;
	}
	return ERR_NOERR;
}

static char *EncryptModeImage(int Mode)

{
	switch (Mode) {
		case 0: return "ASM";
		case 1: return "LINKER";
		case 2: return "DUMP";
		default: return "Unknown";
	}
}

int ProcessEncryptionInfos(char *InfoName, int Mode)

{
	int Trace = 1;
	FILE *Fi;

	if (EncryptInfo.Verbose)
		printf("ENTERING ProcessEncryptionInfos, %s\n", EncryptModeImage(Mode));
	if (EncryptInfo.Components) {
		if (EncryptInfo.Verbose) printf("Encryption infos %s already loaded\n", InfoName);
		return 1;
	}
	Fi = fopen(InfoName, "r");
	if (Fi == NULL) {
		printf("-mencrypt-info=%s, failed to open %s\n", InfoName, InfoName);
		return 0;
	}

	int C, LineNo = 1, TokLine = 1;
	CryptedComponentT *HeadComp = 0, *Comp;
	C = ProcessComponents(Fi, &LineNo, &TokLine, &Comp, &HeadComp);
	if (C==ERR_NOERR || C==ERR_NOERR_EOF) {
		 if (EncryptInfo.Verbose) printf("Encrypt Infos %s Parsing OK\n", InfoName);
	} else {
		char ErrStr[256];
		EncryptReportError(C, ErrStr, TokLine);
		printf("Aborting. %s\n", ErrStr);
		return 0;
	}
	if (EncryptInfo.Verbose) DumpComponents(HeadComp);
	EncryptInfo.Components = HeadComp;
	fclose(Fi);
	return 1;
}

static void EncryptObjSection(CryptedComponentT *Comp, unsigned char *InBuffer, unsigned int Pos, bfd_size_type Len)

{
	AES_ctx_t Ctx;

	AES_init_ctx(&Ctx, Comp->Key);
	AES_ctx_set_iv(&Ctx, Comp->Iv, Comp->Nonce);
	AES_CTR_xcrypt_buffer_From(&Ctx, (uint8_t*) InBuffer, Pos, Len);
}

void SetEncryptMode(int Mode)

{
	// Mode=0 ASM, Mode=1 LINKER, Mode=2 DUMP
	EncryptInfo.Mode = Mode;
}

int EncryptVerbose()

{
	return (EncryptInfo.Verbose);
}

void SetEncryptActiveComponent(char *Name)

{
	CryptedComponentT *Comp = ComponentLookUp(Name, EncryptInfo.Components);
	EncryptInfo.OutComponent = Comp;
}

/************************************************************************/




/* These symbols are global, not specific to any BFD.  Therefore, anything
   that tries to change them is broken, and should be repaired.  */

static const asymbol global_syms[] =
{
  GLOBAL_SYM_INIT (BFD_COM_SECTION_NAME, bfd_com_section_ptr),
  GLOBAL_SYM_INIT (BFD_UND_SECTION_NAME, bfd_und_section_ptr),
  GLOBAL_SYM_INIT (BFD_ABS_SECTION_NAME, bfd_abs_section_ptr),
  GLOBAL_SYM_INIT (BFD_IND_SECTION_NAME, bfd_ind_section_ptr)
};

#define STD_SECTION(NAME, IDX, FLAGS) \
  BFD_FAKE_SECTION(_bfd_std_section[IDX], &global_syms[IDX], NAME, IDX, FLAGS)

asection _bfd_std_section[] = {
  STD_SECTION (BFD_COM_SECTION_NAME, 0, SEC_IS_COMMON),
  STD_SECTION (BFD_UND_SECTION_NAME, 1, 0),
  STD_SECTION (BFD_ABS_SECTION_NAME, 2, 0),
  STD_SECTION (BFD_IND_SECTION_NAME, 3, 0)
};
#undef STD_SECTION

/* Initialize an entry in the section hash table.  */

struct bfd_hash_entry *
bfd_section_hash_newfunc (struct bfd_hash_entry *entry,
			  struct bfd_hash_table *table,
			  const char *string)
{
  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (entry == NULL)
    {
      entry = (struct bfd_hash_entry *)
	bfd_hash_allocate (table, sizeof (struct section_hash_entry));
      if (entry == NULL)
	return entry;
    }

  /* Call the allocation method of the superclass.  */
  entry = bfd_hash_newfunc (entry, table, string);
  if (entry != NULL)
    memset (&((struct section_hash_entry *) entry)->section, 0,
	    sizeof (asection));

  return entry;
}

#define section_hash_lookup(table, string, create, copy) \
  ((struct section_hash_entry *) \
   bfd_hash_lookup ((table), (string), (create), (copy)))

/* Create a symbol whose only job is to point to this section.  This
   is useful for things like relocs which are relative to the base
   of a section.  */

bfd_boolean
_bfd_generic_new_section_hook (bfd *abfd, asection *newsect)
{
  newsect->symbol = bfd_make_empty_symbol (abfd);
  if (newsect->symbol == NULL)
    return FALSE;

  newsect->symbol->name = newsect->name;
  newsect->symbol->value = 0;
  newsect->symbol->section = newsect;
  newsect->symbol->flags = BSF_SECTION_SYM;

  newsect->symbol_ptr_ptr = &newsect->symbol;
  return TRUE;
}

static unsigned int section_id = 0x10;  /* id 0 to 3 used by STD_SECTION.  */

/* Initializes a new section.  NEWSECT->NAME is already set.  */

static asection *
bfd_section_init (bfd *abfd, asection *newsect)
{
  newsect->id = section_id;
  newsect->index = abfd->section_count;
  newsect->owner = abfd;

  if (! BFD_SEND (abfd, _new_section_hook, (abfd, newsect)))
    return NULL;

  section_id++;
  abfd->section_count++;
  bfd_section_list_append (abfd, newsect);
  return newsect;
}

/*
DOCDD
INODE
section prototypes,  , typedef asection, Sections
SUBSECTION
	Section prototypes

These are the functions exported by the section handling part of BFD.
*/

/*
FUNCTION
	bfd_section_list_clear

SYNOPSIS
	void bfd_section_list_clear (bfd *);

DESCRIPTION
	Clears the section list, and also resets the section count and
	hash table entries.
*/

void
bfd_section_list_clear (bfd *abfd)
{
  abfd->sections = NULL;
  abfd->section_last = NULL;
  abfd->section_count = 0;
  memset (abfd->section_htab.table, 0,
	  abfd->section_htab.size * sizeof (struct bfd_hash_entry *));
  abfd->section_htab.count = 0;
}

/*
FUNCTION
	bfd_get_section_by_name

SYNOPSIS
	asection *bfd_get_section_by_name (bfd *abfd, const char *name);

DESCRIPTION
	Return the most recently created section attached to @var{abfd}
	named @var{name}.  Return NULL if no such section exists.
*/

asection *
bfd_get_section_by_name (bfd *abfd, const char *name)
{
  struct section_hash_entry *sh;

  sh = section_hash_lookup (&abfd->section_htab, name, FALSE, FALSE);
  if (sh != NULL)
    return &sh->section;

  return NULL;
}

/*
FUNCTION
       bfd_get_next_section_by_name

SYNOPSIS
       asection *bfd_get_next_section_by_name (bfd *ibfd, asection *sec);

DESCRIPTION
       Given @var{sec} is a section returned by @code{bfd_get_section_by_name},
       return the next most recently created section attached to the same
       BFD with the same name, or if no such section exists in the same BFD and
       IBFD is non-NULL, the next section with the same name in any input
       BFD following IBFD.  Return NULL on finding no section.
*/

asection *
bfd_get_next_section_by_name (bfd *ibfd, asection *sec)
{
  struct section_hash_entry *sh;
  const char *name;
  unsigned long hash;

  sh = ((struct section_hash_entry *)
	((char *) sec - offsetof (struct section_hash_entry, section)));

  hash = sh->root.hash;
  name = sec->name;
  for (sh = (struct section_hash_entry *) sh->root.next;
       sh != NULL;
       sh = (struct section_hash_entry *) sh->root.next)
    if (sh->root.hash == hash
       && strcmp (sh->root.string, name) == 0)
      return &sh->section;

  if (ibfd != NULL)
    {
      while ((ibfd = ibfd->link.next) != NULL)
	{
	  asection *s = bfd_get_section_by_name (ibfd, name);
	  if (s != NULL)
	    return s;
	}
    }

  return NULL;
}

/*
FUNCTION
	bfd_get_linker_section

SYNOPSIS
	asection *bfd_get_linker_section (bfd *abfd, const char *name);

DESCRIPTION
	Return the linker created section attached to @var{abfd}
	named @var{name}.  Return NULL if no such section exists.
*/

asection *
bfd_get_linker_section (bfd *abfd, const char *name)
{
  asection *sec = bfd_get_section_by_name (abfd, name);

  while (sec != NULL && (sec->flags & SEC_LINKER_CREATED) == 0)
    sec = bfd_get_next_section_by_name (NULL, sec);
  return sec;
}

/*
FUNCTION
	bfd_get_section_by_name_if

SYNOPSIS
	asection *bfd_get_section_by_name_if
	  (bfd *abfd,
	   const char *name,
	   bfd_boolean (*func) (bfd *abfd, asection *sect, void *obj),
	   void *obj);

DESCRIPTION
	Call the provided function @var{func} for each section
	attached to the BFD @var{abfd} whose name matches @var{name},
	passing @var{obj} as an argument. The function will be called
	as if by

|	func (abfd, the_section, obj);

	It returns the first section for which @var{func} returns true,
	otherwise <<NULL>>.

*/

asection *
bfd_get_section_by_name_if (bfd *abfd, const char *name,
			    bfd_boolean (*operation) (bfd *,
						      asection *,
						      void *),
			    void *user_storage)
{
  struct section_hash_entry *sh;
  unsigned long hash;

  sh = section_hash_lookup (&abfd->section_htab, name, FALSE, FALSE);
  if (sh == NULL)
    return NULL;

  hash = sh->root.hash;
  for (; sh != NULL; sh = (struct section_hash_entry *) sh->root.next)
    if (sh->root.hash == hash
	&& strcmp (sh->root.string, name) == 0
	&& (*operation) (abfd, &sh->section, user_storage))
      return &sh->section;

  return NULL;
}

/*
FUNCTION
	bfd_get_unique_section_name

SYNOPSIS
	char *bfd_get_unique_section_name
	  (bfd *abfd, const char *templat, int *count);

DESCRIPTION
	Invent a section name that is unique in @var{abfd} by tacking
	a dot and a digit suffix onto the original @var{templat}.  If
	@var{count} is non-NULL, then it specifies the first number
	tried as a suffix to generate a unique name.  The value
	pointed to by @var{count} will be incremented in this case.
*/

char *
bfd_get_unique_section_name (bfd *abfd, const char *templat, int *count)
{
  int num;
  unsigned int len;
  char *sname;

  len = strlen (templat);
  sname = (char *) bfd_malloc (len + 8);
  if (sname == NULL)
    return NULL;
  memcpy (sname, templat, len);
  num = 1;
  if (count != NULL)
    num = *count;

  do
    {
      /* If we have a million sections, something is badly wrong.  */
      if (num > 999999)
	abort ();
      sprintf (sname + len, ".%d", num++);
    }
  while (section_hash_lookup (&abfd->section_htab, sname, FALSE, FALSE));

  if (count != NULL)
    *count = num;
  return sname;
}

/*
FUNCTION
	bfd_make_section_old_way

SYNOPSIS
	asection *bfd_make_section_old_way (bfd *abfd, const char *name);

DESCRIPTION
	Create a new empty section called @var{name}
	and attach it to the end of the chain of sections for the
	BFD @var{abfd}. An attempt to create a section with a name which
	is already in use returns its pointer without changing the
	section chain.

	It has the funny name since this is the way it used to be
	before it was rewritten....

	Possible errors are:
	o <<bfd_error_invalid_operation>> -
	If output has already started for this BFD.
	o <<bfd_error_no_memory>> -
	If memory allocation fails.

*/

asection *
bfd_make_section_old_way (bfd *abfd, const char *name)
{
  asection *newsect;

  if (abfd->output_has_begun)
    {
      bfd_set_error (bfd_error_invalid_operation);
      return NULL;
    }

  if (strcmp (name, BFD_ABS_SECTION_NAME) == 0)
    newsect = bfd_abs_section_ptr;
  else if (strcmp (name, BFD_COM_SECTION_NAME) == 0)
    newsect = bfd_com_section_ptr;
  else if (strcmp (name, BFD_UND_SECTION_NAME) == 0)
    newsect = bfd_und_section_ptr;
  else if (strcmp (name, BFD_IND_SECTION_NAME) == 0)
    newsect = bfd_ind_section_ptr;
  else
    {
      struct section_hash_entry *sh;

      sh = section_hash_lookup (&abfd->section_htab, name, TRUE, FALSE);
      if (sh == NULL)
	return NULL;

      newsect = &sh->section;
      if (newsect->name != NULL)
	{
	  /* Section already exists.  */
	  return newsect;
	}

      newsect->name = name;
      return bfd_section_init (abfd, newsect);
    }

  /* Call new_section_hook when "creating" the standard abs, com, und
     and ind sections to tack on format specific section data.
     Also, create a proper section symbol.  */
  if (! BFD_SEND (abfd, _new_section_hook, (abfd, newsect)))
    return NULL;
  return newsect;
}

/*
FUNCTION
	bfd_make_section_anyway_with_flags

SYNOPSIS
	asection *bfd_make_section_anyway_with_flags
	  (bfd *abfd, const char *name, flagword flags);

DESCRIPTION
   Create a new empty section called @var{name} and attach it to the end of
   the chain of sections for @var{abfd}.  Create a new section even if there
   is already a section with that name.  Also set the attributes of the
   new section to the value @var{flags}.

   Return <<NULL>> and set <<bfd_error>> on error; possible errors are:
   o <<bfd_error_invalid_operation>> - If output has already started for @var{abfd}.
   o <<bfd_error_no_memory>> - If memory allocation fails.
*/

sec_ptr
bfd_make_section_anyway_with_flags (bfd *abfd, const char *name,
				    flagword flags)
{
  struct section_hash_entry *sh;
  asection *newsect;

  if (abfd->output_has_begun)
    {
      bfd_set_error (bfd_error_invalid_operation);
      return NULL;
    }

  sh = section_hash_lookup (&abfd->section_htab, name, TRUE, FALSE);
  if (sh == NULL)
    return NULL;

  newsect = &sh->section;
  if (newsect->name != NULL)
    {
      /* We are making a section of the same name.  Put it in the
	 section hash table.  Even though we can't find it directly by a
	 hash lookup, we'll be able to find the section by traversing
	 sh->root.next quicker than looking at all the bfd sections.  */
      struct section_hash_entry *new_sh;
      new_sh = (struct section_hash_entry *)
	bfd_section_hash_newfunc (NULL, &abfd->section_htab, name);
      if (new_sh == NULL)
	return NULL;

      new_sh->root = sh->root;
      sh->root.next = &new_sh->root;
      newsect = &new_sh->section;
    }

  newsect->flags = flags;
  newsect->name = name;
  return bfd_section_init (abfd, newsect);
}

/*
FUNCTION
	bfd_make_section_anyway

SYNOPSIS
	asection *bfd_make_section_anyway (bfd *abfd, const char *name);

DESCRIPTION
   Create a new empty section called @var{name} and attach it to the end of
   the chain of sections for @var{abfd}.  Create a new section even if there
   is already a section with that name.

   Return <<NULL>> and set <<bfd_error>> on error; possible errors are:
   o <<bfd_error_invalid_operation>> - If output has already started for @var{abfd}.
   o <<bfd_error_no_memory>> - If memory allocation fails.
*/

sec_ptr
bfd_make_section_anyway (bfd *abfd, const char *name)
{
  return bfd_make_section_anyway_with_flags (abfd, name, 0);
}

/*
FUNCTION
	bfd_make_section_with_flags

SYNOPSIS
	asection *bfd_make_section_with_flags
	  (bfd *, const char *name, flagword flags);

DESCRIPTION
   Like <<bfd_make_section_anyway>>, but return <<NULL>> (without calling
   bfd_set_error ()) without changing the section chain if there is already a
   section named @var{name}.  Also set the attributes of the new section to
   the value @var{flags}.  If there is an error, return <<NULL>> and set
   <<bfd_error>>.
*/

asection *
bfd_make_section_with_flags (bfd *abfd, const char *name,
			     flagword flags)
{
  struct section_hash_entry *sh;
  asection *newsect;

  if (abfd->output_has_begun)
    {
      bfd_set_error (bfd_error_invalid_operation);
      return NULL;
    }

  if (strcmp (name, BFD_ABS_SECTION_NAME) == 0
      || strcmp (name, BFD_COM_SECTION_NAME) == 0
      || strcmp (name, BFD_UND_SECTION_NAME) == 0
      || strcmp (name, BFD_IND_SECTION_NAME) == 0)
    return NULL;

  sh = section_hash_lookup (&abfd->section_htab, name, TRUE, FALSE);
  if (sh == NULL)
    return NULL;

  newsect = &sh->section;
  if (newsect->name != NULL)
    {
      /* Section already exists.  */
      return NULL;
    }

  newsect->name = name;
  newsect->flags = flags;
  return bfd_section_init (abfd, newsect);
}

/*
FUNCTION
	bfd_make_section

SYNOPSIS
	asection *bfd_make_section (bfd *, const char *name);

DESCRIPTION
   Like <<bfd_make_section_anyway>>, but return <<NULL>> (without calling
   bfd_set_error ()) without changing the section chain if there is already a
   section named @var{name}.  If there is an error, return <<NULL>> and set
   <<bfd_error>>.
*/

asection *
bfd_make_section (bfd *abfd, const char *name)
{
  return bfd_make_section_with_flags (abfd, name, 0);
}

/*
FUNCTION
	bfd_get_next_section_id

SYNOPSIS
	int bfd_get_next_section_id (void);

DESCRIPTION
	Returns the id that the next section created will have.
*/

int
bfd_get_next_section_id (void)
{
  return section_id;
}

/*
FUNCTION
	bfd_set_section_flags

SYNOPSIS
	bfd_boolean bfd_set_section_flags
	  (bfd *abfd, asection *sec, flagword flags);

DESCRIPTION
	Set the attributes of the section @var{sec} in the BFD
	@var{abfd} to the value @var{flags}. Return <<TRUE>> on success,
	<<FALSE>> on error. Possible error returns are:

	o <<bfd_error_invalid_operation>> -
	The section cannot have one or more of the attributes
	requested. For example, a .bss section in <<a.out>> may not
	have the <<SEC_HAS_CONTENTS>> field set.

*/

bfd_boolean
bfd_set_section_flags (bfd *abfd ATTRIBUTE_UNUSED,
		       sec_ptr section,
		       flagword flags)
{
  section->flags = flags;
  return TRUE;
}

/*
FUNCTION
	bfd_rename_section

SYNOPSIS
	void bfd_rename_section
	  (bfd *abfd, asection *sec, const char *newname);

DESCRIPTION
	Rename section @var{sec} in @var{abfd} to @var{newname}.
*/

void
bfd_rename_section (bfd *abfd, sec_ptr sec, const char *newname)
{
  struct section_hash_entry *sh;

  sh = (struct section_hash_entry *)
    ((char *) sec - offsetof (struct section_hash_entry, section));
  sh->section.name = newname;
  bfd_hash_rename (&abfd->section_htab, newname, &sh->root);
}

/*
FUNCTION
	bfd_map_over_sections

SYNOPSIS
	void bfd_map_over_sections
	  (bfd *abfd,
	   void (*func) (bfd *abfd, asection *sect, void *obj),
	   void *obj);

DESCRIPTION
	Call the provided function @var{func} for each section
	attached to the BFD @var{abfd}, passing @var{obj} as an
	argument. The function will be called as if by

|	func (abfd, the_section, obj);

	This is the preferred method for iterating over sections; an
	alternative would be to use a loop:

|	   asection *p;
|	   for (p = abfd->sections; p != NULL; p = p->next)
|	      func (abfd, p, ...)

*/

void
bfd_map_over_sections (bfd *abfd,
		       void (*operation) (bfd *, asection *, void *),
		       void *user_storage)
{
  asection *sect;
  unsigned int i = 0;

  for (sect = abfd->sections; sect != NULL; i++, sect = sect->next)
    (*operation) (abfd, sect, user_storage);

  if (i != abfd->section_count)	/* Debugging */
    abort ();
}

/*
FUNCTION
	bfd_sections_find_if

SYNOPSIS
	asection *bfd_sections_find_if
	  (bfd *abfd,
	   bfd_boolean (*operation) (bfd *abfd, asection *sect, void *obj),
	   void *obj);

DESCRIPTION
	Call the provided function @var{operation} for each section
	attached to the BFD @var{abfd}, passing @var{obj} as an
	argument. The function will be called as if by

|	operation (abfd, the_section, obj);

	It returns the first section for which @var{operation} returns true.

*/

asection *
bfd_sections_find_if (bfd *abfd,
		      bfd_boolean (*operation) (bfd *, asection *, void *),
		      void *user_storage)
{
  asection *sect;

  for (sect = abfd->sections; sect != NULL; sect = sect->next)
    if ((*operation) (abfd, sect, user_storage))
      break;

  return sect;
}

/*
FUNCTION
	bfd_set_section_size

SYNOPSIS
	bfd_boolean bfd_set_section_size
	  (bfd *abfd, asection *sec, bfd_size_type val);

DESCRIPTION
	Set @var{sec} to the size @var{val}. If the operation is
	ok, then <<TRUE>> is returned, else <<FALSE>>.

	Possible error returns:
	o <<bfd_error_invalid_operation>> -
	Writing has started to the BFD, so setting the size is invalid.

*/

bfd_boolean
bfd_set_section_size (bfd *abfd, sec_ptr ptr, bfd_size_type val)
{
  /* Once you've started writing to any section you cannot create or change
     the size of any others.  */

  if (abfd->output_has_begun)
    {
      bfd_set_error (bfd_error_invalid_operation);
      return FALSE;
    }

  ptr->size = val;
  return TRUE;
}

static int IsTextSection(bfd *abfd, sec_ptr section, int *IsEncrypted)

{

	if ((section->flags & SEC_CODE)) {
		if (IsEncrypted) *IsEncrypted = ((abfd->flags & BFD_ENCRYPTED) != 0);
		// printf("In %s, found text section %s, bfd flags: %x, Encrypt: %x\n", bfd_get_filename (abfd), section->name, abfd->flags, BFD_ENCRYPTED);
		return 1;
	}
	return 0;
}

static void DumpSecFlags(sec_ptr section)

{
	char Str[256];
	int Cpt = 0;
	int N = 5;

	Str[0] = 0;
	sprintf(Str, "\t\t");

	if ((section->flags & SEC_ALLOC) != 0) { strcat(Str, " Alloc"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_LOAD) != 0) { strcat(Str, " Load"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_RELOC) != 0) { strcat(Str, " Reloc"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_READONLY) != 0) { strcat(Str, " ReadOnly"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_CODE) != 0) { strcat(Str, " Code"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_DATA) != 0) { strcat(Str, " Data"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_ROM) != 0) { strcat(Str, " Rom"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_CONSTRUCTOR) != 0) { strcat(Str, " Construct"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_HAS_CONTENTS) != 0) { strcat(Str, " HasCont"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_NEVER_LOAD) != 0) { strcat(Str, " NeverLoad"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_THREAD_LOCAL) != 0) { strcat(Str, " ThreadLoc"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_HAS_GOT_REF) != 0) { strcat(Str, " HasGotRef"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_IS_COMMON) != 0) { strcat(Str, " IsCommon"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_DEBUGGING) != 0) { strcat(Str, " Debug"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_IN_MEMORY) != 0) { strcat(Str, " InMem"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_EXCLUDE) != 0) { strcat(Str, " Exclude"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_SORT_ENTRIES) != 0) { strcat(Str, " Sort"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_LINK_ONCE) != 0) { strcat(Str, " LinkOnce"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_LINK_DUPLICATES_ONE_ONLY) != 0) { strcat(Str, " LinkDupOne"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_LINK_DUPLICATES_SAME_SIZE) != 0) { strcat(Str, " LinkDupSS"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_LINKER_CREATED) != 0) { strcat(Str, " LkCreated"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_KEEP) != 0) { strcat(Str, " Keep"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_SMALL_DATA) != 0) { strcat(Str, " SmallData"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_MERGE) != 0) { strcat(Str, " Merge"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_STRINGS) != 0) { strcat(Str, " Strings"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_GROUP) != 0) { strcat(Str, " Group"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_ELF_REVERSE_COPY) != 0) { strcat(Str, " RevCopy"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_ELF_COMPRESS) != 0) { strcat(Str, " Compress"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_ELF_RENAME) != 0) { strcat(Str, " Rename"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_MEP_VLIW) != 0) { strcat(Str, " MAGIC"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_COFF_NOREAD) != 0) { strcat(Str, " CoffNoRd"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if ((section->flags & SEC_ELF_PURECODE) != 0) { strcat(Str, " PureCode"); Cpt++; if (Cpt>N) {printf("%s\n", Str); Cpt=0; Str[0] = 0;} }
	if (Cpt) printf("%s\n", Str);
}

/*
FUNCTION
	bfd_set_section_contents

SYNOPSIS
	bfd_boolean bfd_set_section_contents
	  (bfd *abfd, asection *section, const void *data,
	   file_ptr offset, bfd_size_type count);

DESCRIPTION
	Sets the contents of the section @var{section} in BFD
	@var{abfd} to the data starting in memory at @var{data}. The
	data is written to the output section starting at offset
	@var{offset} for @var{count} octets.

	Normally <<TRUE>> is returned, else <<FALSE>>. Possible error
	returns are:
	o <<bfd_error_no_contents>> -
	The output section does not have the <<SEC_HAS_CONTENTS>>
	attribute, so nothing can be written to it.
	o and some more too

	This routine is front end to the back end function
	<<_bfd_set_section_contents>>.

*/

bfd_boolean
bfd_set_section_contents (bfd *abfd,
			  sec_ptr section,
			  const void *location,
			  file_ptr offset,
			  bfd_size_type count)
{
  bfd_size_type sz;

  int Trace = 0;
  CryptedComponentT *Comp = 0;
  int IsEncrypted;
  if (IsTextSection(abfd, section, &IsEncrypted)) {
	if (IsEncrypted) Comp = ComponentLookUp(bfd_get_filename (abfd), EncryptInfo.Components);
	/*
	printf("\tSetting content to bfd %s (file format %s), section %s. Offset: %d, Count: %d%s%s\n",
		bfd_get_filename (abfd), abfd->xvec->name, section->name, (int) offset, (int) count,
		IsEncrypted?" ENCRYPTED":"", IsEncrypted?(Comp?" FOUND IT":" NOT FOUND"):"");
	*/
	if (Comp) AcquireComponentIV(Comp);
  }
  if (!(bfd_get_section_flags (abfd, section) & SEC_HAS_CONTENTS))
    {
      bfd_set_error (bfd_error_no_contents);
      return FALSE;
    }

  sz = section->size;
  if ((bfd_size_type) offset > sz
      || count > sz
      || offset + count > sz
      || count != (size_t) count)
    {
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  if (!bfd_write_p (abfd))
    {
      bfd_set_error (bfd_error_invalid_operation);
      return FALSE;
    }

  /* Record a copy of the data in memory if desired.  */
  if (section->contents
      && location != section->contents + offset)
    memcpy (section->contents + offset, location, (size_t) count);

  if (Comp) {
	if (EncryptInfo.Verbose) {
		printf("\tSetting Encrypted section %s from bfd %s (Comp: %s). Offset: %d, Count: %d%s%s%s\n",
			section->name, bfd_get_filename (abfd), Comp->Name, (int) offset, (int) count,
			Comp->Key?" (Key)":" (No Key)", Comp->Iv?" (Iv)":" (No Iv)", Comp->Nonce?" (Nonce)":" (No Nonce)");
		if (Trace) DumpKeys(Comp);
	}
  	void *CopyIn = (void *) malloc (count);
    	memcpy (CopyIn, location, (size_t) count);
	/* Encrypt CopyIn */
	EncryptObjSection(Comp, (unsigned char *) CopyIn, offset, count);

  	if (BFD_SEND (abfd, _bfd_set_section_contents,
			(abfd, section, CopyIn, offset, count)))
    	{
      		abfd->output_has_begun = TRUE;
		free(CopyIn);
      		return TRUE;
    	}
	free(CopyIn);
  } else {
  	if (BFD_SEND (abfd, _bfd_set_section_contents,
			(abfd, section, location, offset, count)))
    	{
      		abfd->output_has_begun = TRUE;
      		return TRUE;
    	}
  }

  return FALSE;
}

/*
FUNCTION
	bfd_get_section_contents

SYNOPSIS
	bfd_boolean bfd_get_section_contents
	  (bfd *abfd, asection *section, void *location, file_ptr offset,
	   bfd_size_type count);

DESCRIPTION
	Read data from @var{section} in BFD @var{abfd}
	into memory starting at @var{location}. The data is read at an
	offset of @var{offset} from the start of the input section,
	and is read for @var{count} bytes.

	If the contents of a constructor with the <<SEC_CONSTRUCTOR>>
	flag set are requested or if the section does not have the
	<<SEC_HAS_CONTENTS>> flag set, then the @var{location} is filled
	with zeroes. If no errors occur, <<TRUE>> is returned, else
	<<FALSE>>.

*/
bfd_boolean
bfd_get_section_contents (bfd *abfd,
			  sec_ptr section,
			  void *location,
			  file_ptr offset,
			  bfd_size_type count)
{
  bfd_size_type sz;
  int Trace = 0;
  CryptedComponentT *Comp = 0;

  int IsEncrypted;
  if (IsTextSection(abfd, section, &IsEncrypted)) {
	if (IsEncrypted) Comp = ComponentLookUp(bfd_get_filename (abfd), EncryptInfo.Components);
	if (Comp) AcquireComponentIV(Comp);

	/* printf("\tGetting from bfd %s (file format %s), section %s. Offset: %d, Count: %d%s%s\n",
		bfd_get_filename (abfd), abfd->xvec->name, section->name, (int) offset, (int) count,
		IsEncrypted?" ENCRYPTED":"", IsEncrypted?(Comp?" FOUND IT":" NOT FOUND"):"");
	*/
  }

  if (section->flags & SEC_CONSTRUCTOR)
    {
      memset (location, 0, (size_t) count);
      return TRUE;
    }

  if (abfd->direction != write_direction && section->rawsize != 0)
    sz = section->rawsize;
  else
    sz = section->size;
  if ((bfd_size_type) offset > sz
      || count > sz
      || offset + count > sz
      || count != (size_t) count)
    {
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  if (count == 0)
    /* Don't bother.  */
    return TRUE;

  if ((section->flags & SEC_HAS_CONTENTS) == 0)
    {
      memset (location, 0, (size_t) count);
      return TRUE;
    }

  if ((section->flags & SEC_IN_MEMORY) != 0)
    {
      if (section->contents == NULL)
	{
	  /* This can happen because of errors earlier on in the linking process.
	     We do not want to seg-fault here, so clear the flag and return an
	     error code.  */
	  section->flags &= ~ SEC_IN_MEMORY;
	  bfd_set_error (bfd_error_invalid_operation);
	  return FALSE;
	}

      memmove (location, section->contents + offset, (size_t) count);
      return TRUE;
    }

  if (Comp) {
	if (EncryptInfo.Verbose) {
		printf("\tGetting Encrypted section %s from bfd %s (Comp: %s). Offset: %d, Count: %d%s%s%s\n",
			section->name, bfd_get_filename (abfd), Comp->Name, (int) offset, (int) count,
			Comp->Key?" (Key)":" (No Key)", Comp->Iv?" (Iv)":" (No Iv)", Comp->Nonce?" (Nonce)":" (No Nonce)");
		if (Trace) DumpKeys(Comp);
	}
  	void *CopyOut = (void *) malloc (count);
	bfd_boolean Status = BFD_SEND (abfd, _bfd_get_section_contents,
		   		       (abfd, section, CopyOut, offset, count));

	/* Decrypt CopyOut */
	EncryptObjSection(Comp, (unsigned char *) CopyOut, offset, count);
    	memcpy (location, CopyOut, (size_t) count);
	free(CopyOut);
	return Status;
  } else {
  	return BFD_SEND (abfd, _bfd_get_section_contents,
		   	(abfd, section, location, offset, count));
  }
}

/*
FUNCTION
	bfd_malloc_and_get_section

SYNOPSIS
	bfd_boolean bfd_malloc_and_get_section
	  (bfd *abfd, asection *section, bfd_byte **buf);

DESCRIPTION
	Read all data from @var{section} in BFD @var{abfd}
	into a buffer, *@var{buf}, malloc'd by this function.
*/

bfd_boolean
bfd_malloc_and_get_section (bfd *abfd, sec_ptr sec, bfd_byte **buf)
{
  *buf = NULL;
  return bfd_get_full_section_contents (abfd, sec, buf);
}
/*
FUNCTION
	bfd_copy_private_section_data

SYNOPSIS
	bfd_boolean bfd_copy_private_section_data
	  (bfd *ibfd, asection *isec, bfd *obfd, asection *osec);

DESCRIPTION
	Copy private section information from @var{isec} in the BFD
	@var{ibfd} to the section @var{osec} in the BFD @var{obfd}.
	Return <<TRUE>> on success, <<FALSE>> on error.  Possible error
	returns are:

	o <<bfd_error_no_memory>> -
	Not enough memory exists to create private data for @var{osec}.

.#define bfd_copy_private_section_data(ibfd, isection, obfd, osection) \
.     BFD_SEND (obfd, _bfd_copy_private_section_data, \
.		(ibfd, isection, obfd, osection))
*/

/*
FUNCTION
	bfd_generic_is_group_section

SYNOPSIS
	bfd_boolean bfd_generic_is_group_section (bfd *, const asection *sec);

DESCRIPTION
	Returns TRUE if @var{sec} is a member of a group.
*/

bfd_boolean
bfd_generic_is_group_section (bfd *abfd ATTRIBUTE_UNUSED,
			      const asection *sec ATTRIBUTE_UNUSED)
{
  return FALSE;
}

/*
FUNCTION
	bfd_generic_discard_group

SYNOPSIS
	bfd_boolean bfd_generic_discard_group (bfd *abfd, asection *group);

DESCRIPTION
	Remove all members of @var{group} from the output.
*/

bfd_boolean
bfd_generic_discard_group (bfd *abfd ATTRIBUTE_UNUSED,
			   asection *group ATTRIBUTE_UNUSED)
{
  return TRUE;
}
