#include <libface.hpp>

static off_t file_size(const char *path) {
    struct stat sbuf;
    int r = stat(path, &sbuf);

    assert(r == 0);
    if (r < 0) {
        return 0;
    }

    return sbuf.st_size;
}

// TODO: Fix for > 2GiB memory usage by using uint64_t
static int get_memory_usage(pid_t pid) {
    char cbuff[4096];
    sprintf(cbuff, "pmap -x %d | tail -n +3 | awk 'BEGIN { S=0;T=0 } { if (match($3, /\\-/)) {S=1} if (S==0) {T+=$3} } END { print T }'", pid);
    FILE *pf = popen(cbuff, "r");
    if (!pf) {
        return -1;
    }
    int r = fread(cbuff, 1, 100, pf);
    if (r < 0) {
        fclose(pf);
        return -1;
    }
    cbuff[r-1] = '\0';
    r = atoi(cbuff);
    fclose(pf);
    return r;
}

static char to_lowercase(char c) {
    return std::tolower(c);
}

static inline void str_lowercase(std::string &str) {
    std::transform(str.begin(), str.end(), 
                   str.begin(), to_lowercase);

}

static inline std::string uint_to_string(uint_t n, uint_t pad = 0) {
    std::string ret;
    if (!n) {
        ret = "0";
    }
    else {
        while (n) {
            ret.insert(0, 1, ('0' + (n % 10)));
            n /= 10;
        }
    }
    while (pad && ret.size() < pad) {
        ret = "0" + ret;
    }

    return ret;
}

static void escape_special_chars(std::string& str) {
    std::string ret;
    ret.reserve(str.size() + 10);
    for (size_t j = 0; j < str.size(); ++j) {
        switch (str[j]) {
        case '"':
            ret += "\\\"";
            break;

        case '\\':
            ret += "\\\\";
            break;

        case '\n':
            ret += "\\n";
            break;

        case '\t':
            ret += "\\t";
            break;

        default:
            ret += str[j];
        }
    }
    ret.swap(str);
}

static std::string rich_suggestions_json_array(vp_t& suggestions) {
    std::string ret = "[";
    ret.reserve(OUTPUT_SIZE_RESERVE);
    for (vp_t::iterator i = suggestions.begin(); i != suggestions.end(); ++i) {
        escape_special_chars(i->phrase);
        std::string snippet = i->snippet;
        escape_special_chars(snippet);

        std::string trailer = i + 1 == suggestions.end() ? "\n" : ",\n";
        ret += " { \"phrase\": \"" + i->phrase + "\", \"score\": " + uint_to_string(i->weight) + 
            (snippet.empty() ? "" : ", \"snippet\": \"" + snippet + "\"") + " }" + trailer;
    }
    ret += "]";
    return ret;
}

static std::string suggestions_json_array(vp_t& suggestions) {
    std::string ret = "[";
    ret.reserve(OUTPUT_SIZE_RESERVE);
    for (vp_t::iterator i = suggestions.begin(); i != suggestions.end(); ++i) {
        escape_special_chars(i->phrase);

        std::string trailer = i + 1 == suggestions.end() ? "\n" : ",\n";
        ret += "\"" + i->phrase + "\"" + trailer;
    }
    ret += "]";
    return ret;
}

static std::string results_json(std::string q, vp_t& suggestions, std::string const& type) {
    if (type == "list") {
        escape_special_chars(q);
        return "[ \"" + q + "\", " + suggestions_json_array(suggestions) + " ]";
    }
    else {
        return rich_suggestions_json_array(suggestions);
    }
}

static bool is_EOF(FILE *pf) { return feof(pf); }
static bool is_EOF(std::ifstream fin) { return !!fin; }

static void get_line(FILE *pf, char *buff, int buff_len, int &read_len) {
    char *got = fgets(buff, INPUT_LINE_SIZE, pf);
    if (!got) {
        read_len = -1;
        return;
    }
    read_len = strlen(buff);
    if (read_len > 0 && buff[read_len - 1] == '\n') {
        buff[read_len - 1] = '\0';
    }
}

static void get_line(std::ifstream fin, char *buff, int buff_len, int &read_len) {
    fin.getline(buff, buff_len);
    read_len = fin.gcount();
    buff[INPUT_LINE_SIZE - 1] = '\0';
}


static int do_import(libface_obj_t* handler, std::string file, uint_t limit, int &rnadded, int &rnlines) {
    bool is_input_sorted = true;
#if defined USE_CXX_IO
    std::ifstream fin(file.c_str());
#else
    FILE *fin = fopen(file.c_str(), "r");
#endif

    int fd = open(file.c_str(), O_RDONLY);

    DCERR("handle_import::file:" << file << "[fin: " << (!!fin) << ", fd: " << fd << "]" << endl);

    if (!fin || fd == -1) {
        perror("fopen");
        return -IMPORT_FILE_NOT_FOUND;
    }
    else {
        handler->building = true;
        int nlines = 0;
        int foffset = 0;

        if (handler->if_mmap_addr) {
            int r = munmap(handler->if_mmap_addr, handler->if_length);
            if (r < 0) {
                perror("munmap");
                handler->building = false;
                return -IMPORT_MUNMAP_FAILED;
            }
        }

        // Potential race condition + not checking for return value
        handler->if_length = file_size(file.c_str());

        // mmap() the input file in
        handler->if_mmap_addr = (char*)mmap(NULL, handler->if_length, PROT_READ, MAP_SHARED, fd, 0);
        if (handler->if_mmap_addr == MAP_FAILED) {
            fprintf(stderr, "length: %llu, fd: %d\n", (long long)handler->if_length, fd);
            perror("mmap");
            if (fin) { fclose(fin); }
            if (fd != -1) { close(fd); }
            handler->building = false;
            return -IMPORT_MMAP_FAILED;
        }

        handler->pm.repr.clear();
        char buff[INPUT_LINE_SIZE];
        std::string prev_phrase;

        while (!is_EOF(fin) && limit--) {
            buff[0] = '\0';

            int llen = -1;
            get_line(fin, buff, INPUT_LINE_SIZE, llen);
            if (llen == -1) {
                break;
            }

            ++nlines;

            int weight = 0;
            std::string phrase;
            StringProxy snippet;
            InputLineParser(handler->if_mmap_addr, foffset, buff, &weight, &phrase, &snippet).start_parsing(handler);

            foffset += llen;

            if (!phrase.empty()) {
                str_lowercase(phrase);
                DCERR("Adding: " << weight << ", " << phrase << ", " << std::string(snippet) << endl);
                handler->pm.insert(weight, phrase, snippet);
            }
            if (is_input_sorted && prev_phrase <= phrase) {
                prev_phrase.swap(phrase);
            } else if (is_input_sorted) {
                is_input_sorted = false;
            }
        }

        DCERR("Creating PhraseMap::Input is " << (!is_input_sorted ? "NOT " : "") << "sorted\n");

        fclose(fin);
        handler->pm.finalize(is_input_sorted);
        vui_t weights;
        for (size_t i = 0; i < handler->pm.repr.size(); ++i) {
            weights.push_back(handler->pm.repr[i].weight);
        }
        handler->st.initialize(weights);

        rnadded = weights.size();
        rnlines = nlines;

        handler->building = false;
    }

    return 0;
}

int handle_import(libface_obj_t* handler, std::string file, uint_t limit) {
    std::string body;
    int nadded, nlines;
    const time_t start_time = time(NULL);

    if (!limit) {
        limit = minus_one;
    }

    int ret = do_import(handler, file, limit, nadded, nlines);
    if (ret < 0) {
        switch (-ret) {
        case IMPORT_FILE_NOT_FOUND:
            body = "The file '" + file + "' was not found";
			std::cerr << body << std::endl;
            break;

        case IMPORT_MUNMAP_FAILED:
            body = "munmap(2) failed";
			std::cerr << body << std::endl;
            break;

        case IMPORT_MMAP_FAILED:
            body = "mmap(2) failed";
			std::cerr << body << std::endl;
            break;

        default:
            body = "Unknown Error";
			std::cerr << body << ret << std::endl;
        }
		return -1;
    }
    else {
        std::ostringstream os;
        os << "Successfully added " << nadded << "/" << nlines
           << "records from '" << file << "' in " << (time(NULL) - start_time)
           << "second(s)\n";
        body = os.str();
		std::cout << body << std::endl;
		return 1;
    }
}

void handle_export(libface_obj_t* handler, std::string file) {
    std::string body;

    ofstream fout(file.c_str());
    const time_t start_time = time(NULL);

    for (size_t i = 0; i < handler->pm.repr.size(); ++i) {
        fout<<handler->pm.repr[i].weight<<'\t'<<handler->pm.repr[i].phrase<<'\t'<<std::string(handler->pm.repr[i].snippet)<<'\n';
    }

    std::ostringstream os;
    os << "Successfully wrote " << handler->pm.repr.size()
       << " records to output file '" << file
       << "' in " << (time(NULL) - start_time) << "second(s)\n";
    body = os.str();
	std::cout << body << std::endl;
}

void handle_suggest(libface_obj_t* handler, std::string q, std::string sn, std::string type) {
	/*
	* q : prefix 
	* sn : number of results
	* type : json format, 'list' or 'dict' 
	*/
    std::string body;

    unsigned int n = sn.empty() ? NMAX : atoi(sn.c_str());
    if (n > NMAX) {
        n = NMAX;
    }
    if (n < 1) {
        n = 1;
    }

    str_lowercase(q);
    vp_t results = suggest(handler->pm, handler->st, q, n);

    /*
    for (size_t i = 0; i < results.size(); ++i) {
      	mg_printf(conn, "%s:%d\n", results[i].first.c_str(), results[i].second);
    }
    */
    body = results_json(q, results, type) + "\n";

	std::cout << body << std::endl;
}

libface_obj_t* create_libface_obj() {
	libface_obj_t* handler = (libface_obj_t*)malloc(sizeof(libface_obj_t));
	if(handler != NULL) {
		handler->if_mmap_addr = NULL;
		handler->if_length = 0;
		handler->building = false;
		return handler;
	}
	return NULL;
}

void destroy_libface_obj(libface_obj_t* handler) {
	if( handler != NULL) {
		free(handler);
	}
}
