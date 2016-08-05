#include <libface.hpp>

int  line_limit = -1;        // The number of lines to import from the input file
bool opt_show_help = false;  // Was --help requested?
const char *ac_file = NULL;  // Path to the input file

void show_usage(char *argv[]) {
    printf("Usage: %s [OPTION]...\n", basename(argv[0]));
    printf("Optional arguments:\n\n");
    printf("-h, --help           This screen\n");
    printf("-f, --file=PATH      Path of the file containing the phrases\n");
    printf("-l, --limit=LIMIT    Load only the first LIMIT lines from PATH (default: -1 [unlimited])\n");
    printf("\n");
}

void parse_options(int argc, char *argv[]) {
    int c;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"file", 1, 0, 'f'},
            {"limit", 1, 0, 'l'},
            {"help", 0, 0, 'h'},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "f:p:l:h",
                        long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0:
        case 'f':
            DCERR("File: "<<optarg<<endl);
            ac_file = optarg;
            break;

        case 'h':
            opt_show_help = true;
            break;

        case 'l':
            line_limit = atoi(optarg);
            DCERR("Limit # of lines to: " << line_limit << endl);
            break;

        case '?':
            cerr<<"ERROR::Invalid option: "<<optopt<<endl;
            break;
        }
    }


}

int main(int argc, char* argv[]) {
	int  ret;
	int  size;
	char line[INPUT_LINE_SIZE+1];
	int  cnt_line;

	libface_obj_t* handler = NULL;
	char* q;
	const char* sn = "100";
	const char* type = "dict";

    parse_options(argc, argv);
    if (opt_show_help || !ac_file) {
        show_usage(argv);
        return 0;
    }

	handler = create_libface_obj();
	if( handler == NULL ) {
		fprintf(stderr, "create libface object fail\n");
		return -1;
	}

	ret = handle_import(handler, ac_file, line_limit);
	if( !ret ) {
		fprintf(stderr,"import fail\n");
		destroy_libface_obj(handler);
		return -1;
	}

	cnt_line = 0;
	while(fgets(line, INPUT_LINE_SIZE, stdin) != NULL) {
		size = strlen(line);
		if(line[size-1] == '\n'){
			line[size-1] = '\0';
			--size;
		}
		if(size > 1 && line[size-1] == '\r'){
			line[size-1] = '\0';
			--size;
		}
		if(line[0] == '\0')
			continue;

		q = line;

		handle_suggest(handler, q, sn, type);

		cnt_line++;
	}

	destroy_libface_obj(handler);
    return 0;
}
