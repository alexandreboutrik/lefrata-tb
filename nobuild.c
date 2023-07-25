#define NOBUILD_IMPLEMENTATION
#include "./nobuild.h"

#define CFLAGS "-W", "-Wall", "-Wextra", "-Wformat=2", "-std=c99"
#define LINKS "-ltermbox"

#define BIN_PATH "./bin"

void		 check_dep(const char* dep);
void		 compile_src(const char* cfile);

void check_dep(const char* dep) {

	if (PATH_EXISTS(dep) != 1)
		PANIC("could not find %s", dep);

}

void compile_src(const char* cfile) {

	const char* src_path	=	PATH("./src", cfile);
	const char* exe_path	=	PATH(BIN_PATH, cfile);

	CMD("cc", LINKS, CFLAGS, "-o", NOEXT(exe_path), src_path);

}

int main(int argc, const char* argv[]) {

	GO_REBUILD_URSELF(argc, argv);

	check_dep("/usr/include/termbox.h");
	check_dep("/usr/local/lib/libtermbox.a");
	check_dep("/usr/local/lib/libtermbox.so");

	compile_src("frata.c");

	return 0;

}
