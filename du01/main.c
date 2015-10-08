#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>

/**
 * Set LD_LIBRARY_PATH to current directory.
 * @return 0 when succeeds, otherwise -1 and print errno into stderr
 */
int setPreferredLibPath()
{
    char *currentPath = getcwd(NULL, 0);
    if (currentPath == NULL) {
        perror("Invoking getcwd() failed");
        return -1;
    }

    if (setenv("LD_LIBRARY_PATH", currentPath, 1) != 0) {
        perror("Invoking setenv() failed");
        free(currentPath); // TODO: Ask teacher, how to solve resource closing in C
        return -1;
    }

    free(currentPath);
    return 0;
}

/**
 * Invoke function funName in libName
 */
int invokeFunction(const char *const libName, const char *const funName)
{
    // Load library
    void *handle = dlopen(libName, RTLD_LAZY);
    if (handle == NULL) {
        fprintf(stderr, "Invoking dlopen() with filename '%s' failed: %s\n", libName, dlerror());
        return -1;
    }

    // Find function address
    dlerror();
    void (*function)(void);
    *(void **) (&function) = dlsym(handle, funName);
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Invoking dlsym() with symbol '%s' failed: %s\n", funName, error);
        dlclose(handle);
        return -1;
    }

    // Invoke function
    (*function)();

    dlclose(handle);
    return 0;
}

int main(int argc, char *const *const argv)
{
    // Name of the library which will be loaded
    char *libName = NULL;
    // Name of the function which will be called
    char *funName = NULL;

    // Process arguments
    extern char *optarg;
    int c;
    while ((c = getopt(argc, argv, "hl:f:")) != -1) {
        switch (c) {
            case 'h':
                printf("This application loads dynamic library and invoke function placed in this library\n");
                printf("Function have to has return type void and cannot takes any parameters.\n");                
                printf(" -l     Set library name\n");
                printf(" -f     Set function name\n");
                return 0;
            case 'l':
                libName = optarg;
                break;
            case 'f':
                funName = optarg;
                break;
            case '?':
                fputs("Invalid argument. Use -h for help.\n", stderr);
                return -1;
            default:
                break;
        }
    }

    if (libName == NULL) {
        fputs("Library name has not been set. Use -h for help.\n", stderr);
        return -1;
    }
    if (funName == NULL) {
        fputs("function name has not been set. Use -h for help.\n", stderr);
        return -1;
    }


    if (setPreferredLibPath() != 0) {
        return -1;
    }

    if (invokeFunction(libName, funName) != 0) {
        return -1;
    }

    return 0;
}
