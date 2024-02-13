#include "dbFiles.h"

int main() {
    system("mkdir data");
    int resetDbResult = initDb();
    raiseIfError(resetDbResult);
    return SUCCESS;
}