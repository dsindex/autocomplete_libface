// -*- mode:c++; c-basic-offset:4 -*-
#if !defined LIBFACE_LIBFACE_HPP
#define LIBFACE_LIBFACE_HPP

// C-headers
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>
#include <fcntl.h>


// Custom-includes
#include <segtree.hpp>
#include <sparsetable.hpp>
#include <benderrmq.hpp>
#include <phrase_map.hpp>
#include <suggest.hpp>
#include <types.hpp>
#include <utils.hpp>

// C++-headers
#include <string>
#include <fstream>
#include <algorithm>

#if !defined NMAX
#define NMAX 32
#endif


#if !defined INPUT_LINE_SIZE
// Max. line size is 8191 bytes.
#define INPUT_LINE_SIZE 8192
#endif

// How many bytes to reserve for the output string
#define OUTPUT_SIZE_RESERVE 4096

// Undefine the macro below to use C-style I/O routines.
// #define USE_CXX_IO
//
#define BOUNDED_RETURN(CH,LB,UB,OFFSET) if (ch >= LB && CH <= UB) { return CH - LB + OFFSET; }
#undef  BOUNDED_RETURN

enum {
    // We are in a non-WS state
    ILP_BEFORE_NON_WS  = 0,

    // We are parsing the weight (integer)
    ILP_WEIGHT         = 1,

    // We are in the state after the weight but before the TAB
    // character separating the weight & the phrase
    ILP_BEFORE_PTAB    = 2,

    // We are in the state after the TAB character and potentially
    // before the phrase starts (or at the phrase)
    ILP_AFTER_PTAB     = 3,

    // The state parsing the phrase
    ILP_PHRASE         = 4,

    // The state after the TAB character following the phrase
    // (currently unused)
    ILP_AFTER_STAB     = 5,

    // The state in which we are parsing the snippet
    ILP_SNIPPET        = 6
};

enum { IMPORT_FILE_NOT_FOUND = 1,
       IMPORT_MUNMAP_FAILED  = 2,
       IMPORT_MMAP_FAILED    = 3
};

struct libface_obj {
	PhraseMap pm;                   // Phrase Map (usually a sorted array of strings)
	RMQ st;                         // An instance of the RMQ Data Structure
	char *if_mmap_addr;  		    // Pointer to the mmapped area of the file
	off_t if_length;            	// The length of the input file
	volatile bool building;			// TRUE if the structure is being built
};
typedef struct libface_obj libface_obj_t;

struct InputLineParser {
    int state;            // Current parsing state
    const char *mem_base; // Base address of the mmapped file
    const char *buff;     // A pointer to the current line to be parsed
    size_t buff_offset;   // Offset of 'buff' [above] relative to the beginning of the file. Used to index into mem_base
    int *pn;              // A pointer to any integral field being parsed
    std::string *pphrase; // A pointer to a string field being parsed

    // The input file is mmap()ped in the process' address space.

    StringProxy *psnippet_proxy; // The psnippet_proxy is a pointer to a Proxy String object that points to memory in the mmapped region

    InputLineParser(const char *_mem_base, size_t _bo, 
                    const char *_buff, int *_pn, 
                    std::string *_pphrase, StringProxy *_psp)
        : state(ILP_BEFORE_NON_WS), mem_base(_mem_base), buff(_buff), 
          buff_offset(_bo), pn(_pn), pphrase(_pphrase), psnippet_proxy(_psp)
    { }

    void start_parsing(libface_obj_t* handler) {
        int i = 0;                  // The current record byte-offset.
        int n = 0;                  // Temporary buffer for numeric (integer) fields.
        const char *p_start = NULL; // Beginning of the phrase.
        const char *s_start = NULL; // Beginning of the snippet.
        int p_len = 0;              // Phrase Length.
        int s_len = 0;              // Snippet length.

        while (this->buff[i]) {
            char ch = this->buff[i];
            DCERR("["<<this->state<<":"<<ch<<"]");

            switch (this->state) {
            case ILP_BEFORE_NON_WS:
                if (!isspace(ch)) {
                    this->state = ILP_WEIGHT;
                }
                else {
                    ++i;
                }
                break;

            case ILP_WEIGHT:
                if (isdigit(ch)) {
                    n *= 10;
                    n += (ch - '0');
                    ++i;
                }
                else {
                    this->state = ILP_BEFORE_PTAB;
                    on_weight(n);
                }
                break;

            case ILP_BEFORE_PTAB:
                if (ch == '\t') {
                    this->state = ILP_AFTER_PTAB;
                }
                ++i;
                break;

            case ILP_AFTER_PTAB:
                if (isspace(ch)) {
                    ++i;
                }
                else {
                    p_start = this->buff + i;
                    this->state = ILP_PHRASE;
                }
                break;

            case ILP_PHRASE:
                // DCERR("State: ILP_PHRASE: "<<buff[i]<<endl);
                if (ch != '\t') {
                    ++p_len;
                }
                else {
                    // Note: Skip to ILP_SNIPPET since the snippet may
                    // start with a white-space that we wish to
                    // preserve.
                    // 
                    this->state = ILP_SNIPPET;
                    s_start = this->buff + i + 1;
                }
                ++i;
                break;

            case ILP_SNIPPET:
                ++i;
                ++s_len;
                break;

            };
        }
        DCERR("\n");
        on_phrase(p_start, p_len);
        on_snippet(handler, s_start, s_len);
    }

    void on_weight(int n) {
        *(this->pn) = n;
    }

    void on_phrase(const char *data, int len) {
        if (len && this->pphrase) {
            // DCERR("on_phrase("<<data<<", "<<len<<")\n");
            this->pphrase->assign(data, len);
        }
    }

    void on_snippet(libface_obj_t* handler, const char *data, int len) {
		char* if_mmap_addr = handler->if_mmap_addr;
		off_t if_length = handler->if_length;
        if (len && this->psnippet_proxy) {
            const char *base = this->mem_base + this->buff_offset + 
                (data - this->buff);
            if (base < if_mmap_addr || base + len > if_mmap_addr + if_length) {
                fprintf(stderr, "base: %p, if_mmap_addr: %p, if_mmap_addr+if_length: %p\n", base, if_mmap_addr, if_mmap_addr + if_length);
                assert(base >= if_mmap_addr);
                assert(base <= if_mmap_addr + if_length);
                assert(base + len <= if_mmap_addr + if_length);
            }
            DCERR("on_snippet::base: "<<(void*)base<<", len: "<<len<<"\n");
            this->psnippet_proxy->assign(base, len);
        }
    }

};


libface_obj_t* create_libface_obj();
void destroy_libface_obj(libface_obj_t* handler);

int handle_import(libface_obj_t* handler, std::string file, uint_t limit);
void handle_export(libface_obj_t* handler, std::string file);
void handle_suggest(libface_obj_t* handler, std::string q, std::string sn, std::string type);

#endif // LIBFACE_LIBFACE_HPP
