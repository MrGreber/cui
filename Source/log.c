#define _CRT_SECURE_NO_WARNINGS

#include <log.h>
#include <defines.h>

#include <shlwapi.h>
#include <stdarg.h>
#include <stdio.h>
#pragma comment(lib, "shlwapi.lib")



#ifndef debugBreak
#if _MSC_VER

#include <intrin.h>
#define debugBreak() __debugbreak()
#elif __MINGW32__ || __MINGW64__
#define debugBreak() DebugBreak()
#endif
#endif

static i32 __exit_code = 0;
static bool __exit_flag = false;
static bool __stderr_flag = false;
static FILE* __log_file = NULL;

#define log_default_name "__log__.txt"
#define error_format "%s:\n\t%s\nFile: %s:%d\n\n"
#define dialog_error_format "%s\nFile: %s:%d"
#define dialog_log_format "%s"
const char* log_labels[4] = {
	"Warning",
	"Error",
	"Fatal",
	"Log"
};
const u8 log_labels_lengths[4] = {
	7,
	5,
	5,
	3
};

void open_logging(const char* path, const bool stderr_flag) {
	__log_file = fopen(path == NULL ? log_default_name : path, "w");
	if (__log_file == NULL) {
		return;
	}
	__stderr_flag = stderr_flag;
}
void close_logging() {
	fclose(__log_file);
}

void __post_error(const error_t err, const char* file, const u32 line, const char* format, ...) {
	if (__log_file) {
		va_list args;
		va_start(args, format);

		char msg[1024] = { 0 };
		const u64 msg_length = (u64)vsprintf_s(msg, sizeof(msg), format, args);
		if (msg_length == (u64)-1) return;
		va_end(args);

		char* file_name = PathFindFileNameA(file) - 1;
		fprintf(__log_file, error_format, log_labels[err], msg, file_name, line);
		if (__stderr_flag) printf(error_format, log_labels[err], msg, file_name, line);


		if (err > 0 && err != __LOG__) error_exit();
	}
}
void set_exitFlag(const bool flag) {
	__exit_flag = flag;
}
void set_ExitCode(const i32 code) {
	__exit_code = code;
}
void error_exit() {
	if (__log_file && __exit_flag) {
		fclose(__log_file);
		debugBreak();
	}
}