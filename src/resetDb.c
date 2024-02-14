#include "dbFiles.h"

int main() {
    system("mkdir -p data");
    int resetDbResult = initDb();
    raiseIfError(resetDbResult);
    return SUCCESS;
}