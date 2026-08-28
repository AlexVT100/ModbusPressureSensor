template<typename P, typename T>
static void printf(P *t, T type, const char *fmt, ...) {
    static char buffer[160];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    t->print(type, buffer);
}
