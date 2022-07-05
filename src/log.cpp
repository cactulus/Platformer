#define LOG_INFO "INFO"
#define LOG_WARNING "WARNING"
#define LOG_ERROR "ERROR"
#define LOG_FATAL "FATAL"

void log(const char *level, const char *msg) {
	printf("[%s] %s\n", level, msg);
}

void die(const char *msg) {
	log(LOG_FATAL, msg);
	exit(EXIT_FAILURE);
}