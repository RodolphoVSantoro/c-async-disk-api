#ifndef DBFILES_H
#define DBFILES_H

// Header file for the database files
// Saves and reads user data to and from binary files

#include <fcntl.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

#include "helpers.h"

// Comment this line to keep the database on server start
#define RESET_DB 1

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
#define moveRightInTransactions(index) (index = (index + 1) % MAX_TRANSACTIONS)

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
// returns ERROR if it fails to lock the file
// returns FILE_NOT_FOUND if the user is not found
// returns LIMIT_EXCEEDED_ERROR if the user has no limit
// returns INVALID_TIPO_ERROR if the tipo is not valid
int updateUserWithTransaction(int id, Transaction* transaction, User* user);

// Returns INVALID_TIPO_ERROR if the tipo is not valid
// Returns LIMIT_EXCEEDED_ERROR if the user has no limit
// Returns SUCCESS if the transaction was successful
int addTransaction(User* user, Transaction* transaction);
// Tries to add or subtract the transaction value from the user's total
// Returns ERROR if the user doesn't have enough limit
int addSaldo(User* user, Transaction* transaction);

// Fills the orderedTransactions array with the user's transactions ordered by the oldest

int initDb() {
    User user;
    user.total = 0;
    user.nTransactions = 0;
    user.oldestTransaction = 0;

    for (int id = 0; id < numberInitialUsers; id++) {
        user.id = id + 1;
        user.limit = userInitialLimits[id];
        int writeResult = writeUser(&user);
        raiseIfNotSuccess(writeResult, "Failed to write user to file");
    }

    return SUCCESS;
}

int writeUser(User* user) {
    char fname[FILE_NAME_SIZE];
    sprintf(fname, userFileTemplate, user->id);
    FILE* userFile = fopen(fname, WRITE_BINARY);
    errIfNull(userFile);
    int userFileDescriptor = fileno(userFile);
    
    int lockResult = flock(userFileDescriptor, LOCK_EX);
    raiseIfNotSuccess(lockResult, "Failed to lock user file");
    
    int userCount = 1;
    int writeResult = fwrite(user, sizeof(User), userCount, userFile);
    if(writeResult != userCount) {
        return UNSUCCESSFUL_WRITE_ERROR;
    }
    
    int flushResult = fflush(userFile);
    raiseIfNotSuccess(flushResult, "Failed to flush user file");
    
    int release = flock(userFileDescriptor, LOCK_UN);
    raiseIfNotSuccess(release, "Failed to unlock user file");
    
    int closeResult = fclose(userFile);
    raiseIfNotSuccess(closeResult, "Failed to close user file");
    return SUCCESS;
}

int readUser(User* user, int id) {
    char fname[FILE_NAME_SIZE];
    sprintf(fname, userFileTemplate, id);
    FILE* userFile = fopen(fname, READ_BINARY);
    errIfNull(userFile);
    int userFileDescriptor = fileno(userFile);
    
    int lockResult = flock(userFileDescriptor, LOCK_SH);
    raiseIfNotSuccess(lockResult, "Failed to lock user file");

    int userCount = 1;
    int readResult = fread(user, sizeof(User), userCount, userFile);
    int release = flock(userFileDescriptor, LOCK_UN);
    int closeResult = fclose(userFile);
    
    if(readResult != userCount) {
        printf("Failed to read user %d from file\n", id);
        return UNSUCCESSFUL_READ_ERROR;
    }
    raiseIfNotSuccess(release, "Failed to unlock user file");
    raiseIfNotSuccess(closeResult, "Failed to close user file");

    return SUCCESS;
}

int updateUserWithTransaction(int id, Transaction* transaction, User* user) {
    char fname[FILE_NAME_SIZE];
    sprintf(fname, userFileTemplate, id);

    if ((access(fname, F_OK) != 0)) {
        return ERROR;
    }
    FILE* userFile = fopen(fname, READ_WRITE_BINARY);
    raiseIfFileNotFound(userFile);
    int userFileDescriptor = fileno(userFile);
    log("Locking for transaction %s\n", transaction->descricao);
    int lockResult = flock(userFileDescriptor, LOCK_EX);
    log("Locked for transaction %s\n", transaction->descricao);
    raiseIfNotSuccess(lockResult, "Failed to lock user file");

    int userCount = 1;
    int readResult = fread(user, sizeof(User), userCount, userFile);
    log("Read user file %s\n", fname);
    
    int transactionResult = addTransaction(user, transaction);
    log("Added transaction\n");
    
    // Go back to the beginning of the file, because fread moved the cursor
    int seekResult = fseek(userFile, 0, SEEK_SET);
    log("Seek set\n");
    int writeResult = fwrite(user, sizeof(User), userCount, userFile);
    log("Wrote user file %s\n", fname);
    int flushResult = fflush(userFile);
    int releaseResult = flock(userFileDescriptor, LOCK_UN);
    int closeResult = fclose(userFile);
    log("Closed user file %s\n", fname);

    if(readResult != userCount) {
        return UNSUCCESSFUL_READ_ERROR;
    }
    raiseIfNotSuccess(transactionResult, "Failed to add transaction");
    raiseIfNotSuccess(seekResult, "Failed to seek user file");
    if(writeResult != userCount) {
        return UNSUCCESSFUL_READ_ERROR;
    }
    raiseIfNotSuccess(flushResult, "Failed to flush user file");
    raiseIfNotSuccess(releaseResult, "Failed to unlock user file");
    raiseIfNotSuccess(closeResult, "Failed to close user file");
    
    return SUCCESS;
}

int addTransaction(User* user, Transaction* transaction) {
    int resultSaldo = addSaldo(user, transaction);
    raiseIfNotSuccess(resultSaldo, "Failed to add saldo");
    user->transactions[user->oldestTransaction] = *transaction;
    moveRightInTransactions(user->oldestTransaction);
    if(user->nTransactions < MAX_TRANSACTIONS){
        user->nTransactions++;
    }
    return SUCCESS;
}

int addSaldo(User* user, Transaction* transaction) {
    if (transaction->tipo == 'd') {
        int newTotal = user->total - transaction->valor;
        if (-1 * newTotal > user->limit) {
            return LIMIT_EXCEEDED_ERROR;
        }
        user->total = newTotal;
        return SUCCESS;
    } else if (transaction->tipo == 'c') {
        user->total += transaction->valor;
        return SUCCESS;
    }
    return INVALID_TIPO_ERROR;
}

#endif
