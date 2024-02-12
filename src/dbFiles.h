#ifndef DBFILES_H
#define DBFILES_H

// Header file for the database files
// Saves and reads user data to and from binary files

#include <time.h>

#include "helpers.h"

// Open file modes
#define READ_BINARY "rb"
#define WRITE_BINARY "wb"
#define READ_WRITE_BINARY "rb+"

// User File name template
const char* userFileTemplate = "data/user%d.bin";

// Initial database setup
const int userInitialLimits[] = {100000, 80000, 1000000, 10000000, 500000};
const int numberInitialUsers = sizeof(userInitialLimits) / sizeof(int);

// move right on a circular array
#define moveRight(index) (index = (index + 1) % MAX_TRANSACTIONS)

// User struct constants
#define MAX_TRANSACTIONS 10
#define DATE_SIZE 32
#define DESCRIPTION_SIZE 32
#define FILE_NAME_SIZE 32

typedef struct TRANSACTION {
    int valor;
    char tipo;
    char descricao[DESCRIPTION_SIZE];
    char realizada_em[DATE_SIZE];
} Transaction;

typedef struct USER {
    int id;
    int limit, total;
    int nTransactions;
    int oldestTransaction;
    Transaction transactions[MAX_TRANSACTIONS];
} User;

// Initializes the database with 5 users
// Returns ERROR it fails to write a user to the file
// Returns SUCCESS if the database was successfully initialized
int initDb();

// Returns ERROR if the file is not found
int readUser(User* user, int id);

// You should only use if this if it's a new user, or you want to reset the user
// Instead of doing subsequent readUser and writeUser, use the updateUser function to update the user
// Returns ERROR if it fails to write the user to the file
int writeUser(User* user);

// updates the user with the transaction
// writes the updated user to the user variable
// returns SUCCESS if transaction was successful
// returns ERROR if the user has no limit
// returns FILE_NOT_FOUND if the user is not found
int updateUserWithTransaction(int id, Transaction* transaction, User* user);

// Returns ERROR if the user has no limit
// Returns SUCCESS if the transaction was successful
int addTransaction(User* user, Transaction* transaction);
// Tries to add or subtract the transaction value from the user's total
// Returns ERROR if the user doesn't have enough limit
int addSaldo(User* user, Transaction* transaction);

// Fills the orderedTransactions array with the user's transactions ordered by the oldest
void getOrderedTransactions(User* user, Transaction* orderedTransactions);

int initDb() {
    User user;
    user.total = 0;
    user.nTransactions = 0;
    user.oldestTransaction = 0;

    for (int id = 0; id < numberInitialUsers; id++) {
        user.id = id + 1;
        user.limit = userInitialLimits[id];
        int writeResult = writeUser(&user);
        if (writeResult == ERROR) {
            return ERROR;
        }
    }

    return SUCCESS;
}

int writeUser(User* user) {
    char fname[FILE_NAME_SIZE];
    sprintf(fname, userFileTemplate, user->id);
    FILE* fpTotals = fopen(fname, WRITE_BINARY);
    errIfNull(fpTotals);
    fwrite(user, sizeof(User), 1, fpTotals);
    fclose(fpTotals);
    return SUCCESS;
}

int readUser(User* user, int id) {
    char fname[FILE_NAME_SIZE];
    sprintf(fname, userFileTemplate, id);
    FILE* fpTotals = fopen(fname, READ_BINARY);
    errIfNull(fpTotals);
    fread(user, sizeof(User), 1, fpTotals);
    fclose(fpTotals);
    return SUCCESS;
}

int updateUserWithTransaction(int id, Transaction* transaction, User* user) {
    char fname[FILE_NAME_SIZE];
    sprintf(fname, userFileTemplate, id);

    FILE* fpTotals = fopen(fname, READ_WRITE_BINARY);
    raiseIfFileNotFound(fpTotals);
    fread(user, sizeof(User), 1, fpTotals);

    int transactionResult = addTransaction(user, transaction);

    if (transactionResult == 0) {
        // Go back to the beginning of the file, because fread moved the cursor
        fseek(fpTotals, 0, SEEK_SET);
        fwrite(user, sizeof(User), 1, fpTotals);
    }
    fclose(fpTotals);
    return transactionResult;
}

int addTransaction(User* user, Transaction* transaction) {
    int resultSaldo = addSaldo(user, transaction);
    raiseIfError(resultSaldo);
    if (user->nTransactions == 10) {
        user->transactions[user->oldestTransaction] = *transaction;
        moveRight(user->oldestTransaction);
        return SUCCESS;
    }

    user->transactions[user->nTransactions] = *transaction;
    user->nTransactions++;
    return SUCCESS;
}

int addSaldo(User* user, Transaction* transaction) {
    if (transaction->tipo == 'd') {
        int newTotal = user->total - transaction->valor;
        if (-1 * newTotal > user->limit) {
            return ERROR;
        }
        user->total = newTotal;
        return SUCCESS;
    }

    user->total += transaction->valor;
    return SUCCESS;
}

void getOrderedTransactions(User* user, Transaction* orderedTransactions) {
    int i = user->oldestTransaction;
    for (int j = 0; j < user->nTransactions; j++) {
        orderedTransactions[j] = user->transactions[i];
        moveRight(i);
    }
}

#endif