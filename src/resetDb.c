#include "dbFiles.h"

int main() {
    int resetDbResult = initDb();
    raiseIfError(resetDbResult);
    return SUCCESS;
}