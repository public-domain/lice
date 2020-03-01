### Epilogue
LICE is a work in progress C99 compiler designed as a solution for
teaching myself and others about the internals of the C programming
language, how to write a compiler for it, and code generation.

Part of the philosophy behind LICE is to provide a public domain
implementation of a working conformant C99 compiler. As well as borrowing
extensions and ideas from existing compilers to ensure a wider range of
support.

### Status
See the STANDARDS file for the status on which standards LICE supports.

### Prologue
If you don't find yourself needing any of the stuff which is marked as being
unsupported above then you may find that LICE will happily compile your
source into x86-64 assembly. The code generation is close from optimal.
LICE treats the entire system as a giant stack machine, since it's easier
to generate code that way. The problem is it's hardly efficent. All local
variables are assigned on the stack for operations. All operations operate
from the stack and write back the result to the stack location that is
the destination operand for that operation.

### Porting
LICE should be farily straightforward to retarget for a specific architecture
or ABI. Start by making a copy of `arch_dummy.h`, naming it `arch_yourarch.h`
create an entry in `lice.h` for that header guarded by conditional include.
Supply that as a default option in the Makefile, remove `gen_amd64.c` from
the Makefile, add your own to it. Then write the code generator. Documentation
may be found in `gen.h`.


### Future Endeavors
-   Full C90 support

-   Full C99 support

-   Full C11 support

-   Preprocessor

-   Intermediate stage with optimizations (libfirm?)

-   Code generation (directly to elf/coff, et. all)

-   Support for x86, ARM, PPC

### Sources
The following sources where used in the construction of LICE

-   Aho, Alfred V., and Alfred V. Aho. Compilers: Principles, Techniques, & Tools. Boston: Pearson/Addison Wesley, 2007. Print.
    http://www.amazon.ca/Compilers-Principles-Techniques-Alfred-Aho/dp/0201100886

-   Degener, Jutta. "ANSI C Grammar, Lex Specification." ANSI C Grammar (Lex). N.p., 1995. Web.
    http://www.lysator.liu.se/c/ANSI-C-grammar-l.html

-   Matz, Michael, Jan Hubicka, Andreas Jaeger, and Mark Mitchell. "System V Application Binary Interface AMD64 Architecture Processor Supplement." N.p., 07 Oct. 2013. Print.
    http://www.x86-64.org/documentation/abi.pdf

-   Kahan, W., Prof. "IEEE Standard 754 for Binary Floating-Point Arithmetic." N.p., 1 Oct. 1997. Print.
    http://www.eecs.berkeley.edu/~wkahan/ieee754status/IEEE754.PDF

-   Crenshaw, Jack. "Let's Build a Compiler." I.E.C.C., 1995. Web.
    http://compilers.iecc.com/crenshaw/

-   "C99 Final Draft." ISO/IEC, 06 May 2006. Print.
    http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1124.pdf

-   "C11 Final Draft." ISO/IEC, 12 April 2011. Print.
    http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf

-   "Instruction Set Reference, A-Z." Intel 64 and IA-32 Architectures Software Developer's Manual. Vol. 2. [Calif.?]: Intel, 2013. N. pag. Print.
    http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.pdf

-   Bendersky, Eli. "Complete C99 parser in pure Python." pycparser. N.p., N.d. Web.
    https://github.com/eliben/pycparser

### Inspiration
The following projects were seen as inspiration in the construciton of
LICE.

-   SubC
    http://www.t3x.org/subc/

-   TCC
    http://bellard.org/tcc/

-   lcc
    https://sites.google.com/site/lccretargetablecompiler/

-   Kaleidoscope
    http://llvm.org/docs/tutorial/index.html
