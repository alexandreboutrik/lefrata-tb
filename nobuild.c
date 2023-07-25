#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"

#define CFLAGS "-W", "-Wall", "-Wextra", "-Wformat=2", "-std=c99"
#define LINKS "-ltermbox"

#define BIN_PATH "./bin"

void		 mkdir_bin(void);
void		 check_dep(const char* dep);

void		 run_game(void);
void		 compile_src(const char* cfile);

void mkdir_bin(void) {

	if (PATH_EXISTS(BIN_PATH) != 1)
		MKDIRS(BIN_PATH);

}

void check_dep(const char* dep) {

	if (PATH_EXISTS(dep) != 1)
		PANIC("could not find %s", dep);

}

void run_game(void) {

	const char* exe_path	=	PATH(BIN_PATH, "frata");

	if (PATH_EXISTS(exe_path) != 1)
		PANIC("frata was not compiled yet.");

	CMD(exe_path);

}

void compile_src(const char* cfile) {

	const char* src_path	=	PATH("./src", cfile);
	const char* exe_path	=	PATH(BIN_PATH, cfile);

	CMD("cc", LINKS, CFLAGS, "-o", NOEXT(exe_path), src_path);

}

int main(int argc, const char* argv[]) {

	GO_REBUILD_URSELF(argc, argv);

	mkdir_bin();

	check_dep("/usr/include/termbox.h");
	check_dep("/usr/local/lib/libtermbox.a");
	check_dep("/usr/local/lib/libtermbox.so");

	if (argc > 1)
		if (argv[1][0] == 'r')
			run_game();

	compile_src("frata.c");

	return 0;

}
