#include "dbFiles.h"

int main() {
    system("mkdir -p data");
    int resetDbResult = initDb();
    raiseIfNotSuccess(resetDbResult, "Failed to reset database");
    return SUCCESS;
}
