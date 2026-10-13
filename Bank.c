/*
 * Bank / ATM Simulator (C)
 * Concepts: structs, pointers, file persistence, PIN authentication,
 *           input validation, transaction history
 * Compile: gcc bank.c -o bank.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_ACCOUNTS 200
#define ACCOUNTS_FILE "accounts.txt"
#define TXN_FILE "transactions.txt"

typedef struct {
    int accNo;
    char name[60];
    char pin[5];          /* 4 digits + '\0' */
    double balance;
} Account;

Account accounts[MAX_ACCOUNTS];
int count = 0;

/* ---------- Input helpers ---------- */

void readLine(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    if (fgets(buf, size, stdin) == NULL) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\n")] = '\0';
    for (int i = 0; buf[i]; i++)
        if (buf[i] == '|') buf[i] = '/';
}

double readNumber(const char *prompt) {
    char line[50];
    double x;
    while (1) {
        printf("%s", prompt);
        if (fgets(line, sizeof(line), stdin) == NULL) return 0;
        if (sscanf(line, "%lf", &x) == 1) return x;
        printf("Enter a valid number.\n");
    }
}

int validPin(const char *p) {
    if (strlen(p) != 4) return 0;
    for (int i = 0; i < 4; i++)
        if (!isdigit((unsigned char)p[i])) return 0;
    return 1;
}

/* ---------- File handling ---------- */

void loadAccounts(void) {
    FILE *fp = fopen(ACCOUNTS_FILE, "r");
    char line[300];
    if (!fp) return;
    while (fgets(line, sizeof(line), fp) && count < MAX_ACCOUNTS) {
        Account a;
        char *tok;
        line[strcspn(line, "\n")] = '\0';
        tok = strtok(line, "|");  if (!tok) continue;  a.accNo = atoi(tok);
        tok = strtok(NULL, "|");  strncpy(a.name, tok ? tok : "", sizeof(a.name) - 1);
        a.name[sizeof(a.name) - 1] = '\0';
        tok = strtok(NULL, "|");  strncpy(a.pin, tok ? tok : "0000", 4);
        a.pin[4] = '\0';
        tok = strtok(NULL, "|");  a.balance = tok ? atof(tok) : 0;
        accounts[count++] = a;
    }
    fclose(fp);
}

void saveAccounts(void) {
    FILE *fp = fopen(ACCOUNTS_FILE, "w");
    if (!fp) { printf("Error: could not save accounts!\n"); return; }
    for (int i = 0; i < count; i++)
        fprintf(fp, "%d|%s|%s|%.2f\n", accounts[i].accNo, accounts[i].name,
                accounts[i].pin, accounts[i].balance);
    fclose(fp);
}

void logTxn(int accNo, const char *type, double amt, double balAfter) {
    FILE *fp = fopen(TXN_FILE, "a");
    char when[32];
    time_t t = time(NULL);
    if (!fp) return;
    strftime(when, sizeof(when), "%d-%m-%Y %H:%M", localtime(&t));
    fprintf(fp, "%d|%s|%s|%.2f|%.2f\n", accNo, when, type, amt, balAfter);
    fclose(fp);
}

/* ---------- Bank operations ---------- */

Account *findAccount(int accNo) {
    for (int i = 0; i < count; i++)
        if (accounts[i].accNo == accNo) return &accounts[i];
    return NULL;
}

void createAccount(void) {
    if (count >= MAX_ACCOUNTS) { printf("Bank is full.\n"); return; }
    Account a;
    readLine("Full name: ", a.name, sizeof(a.name));
    do {
        readLine("Set 4-digit PIN: ", a.pin, sizeof(a.pin));
    } while (!validPin(a.pin));
    a.balance = readNumber("Opening deposit: ");
    if (a.balance < 0) a.balance = 0;
    a.accNo = 1001 + count;
    accounts[count++] = a;
    logTxn(a.accNo, "OPEN", a.balance, a.balance);
    printf("Account created! Your account number is %d\n", a.accNo);
}

Account *login(void) {
    int no = (int)readNumber("Account number: ");
    Account *a = findAccount(no);
    char pin[20];
    if (!a) { printf("Account not found.\n"); return NULL; }
    for (int tries = 3; tries > 0; tries--) {
        readLine("PIN: ", pin, sizeof(pin));
        if (strcmp(pin, a->pin) == 0) return a;
        printf("Wrong PIN. %d attempt(s) left.\n", tries - 1);
    }
    printf("Too many wrong attempts.\n");
    return NULL;
}

void deposit(Account *a) {
    double amt = readNumber("Amount to deposit: ");
    if (amt <= 0) { printf("Invalid amount.\n"); return; }
    a->balance += amt;
    logTxn(a->accNo, "DEPOSIT", amt, a->balance);
    printf("Deposited. New balance: %.2f\n", a->balance);
}

void withdraw(Account *a) {
    double amt = readNumber("Amount to withdraw: ");
    if (amt <= 0) { printf("Invalid amount.\n"); return; }
    if (amt > a->balance) { printf("Insufficient balance.\n"); return; }
    a->balance -= amt;
    logTxn(a->accNo, "WITHDRAW", amt, a->balance);
    printf("Please collect your cash. Balance: %.2f\n", a->balance);
}

void transfer(Account *from) {
    int toNo = (int)readNumber("Receiver account number: ");
    Account *to = findAccount(toNo);
    double amt;
    if (!to || to == from) { printf("Invalid receiver.\n"); return; }
    amt = readNumber("Amount to transfer: ");
    if (amt <= 0 || amt > from->balance) { printf("Transfer failed.\n"); return; }
    from->balance -= amt;
    to->balance += amt;
    logTxn(from->accNo, "TRANSFER-OUT", amt, from->balance);
    logTxn(to->accNo, "TRANSFER-IN", amt, to->balance);
    printf("Transferred %.2f to %s.\n", amt, to->name);
}

void changePin(Account *a) {
    char oldPin[20], newPin[20];
    readLine("Current PIN: ", oldPin, sizeof(oldPin));
    if (strcmp(oldPin, a->pin) != 0) { printf("Wrong PIN.\n"); return; }
    do {
        readLine("New 4-digit PIN: ", newPin, sizeof(newPin));
    } while (!validPin(newPin));
    strcpy(a->pin, newPin);
    printf("PIN changed.\n");
}

void miniStatement(const Account *a) {
    FILE *fp = fopen(TXN_FILE, "r");
    char lines[1000][120];
    int n = 0;
    char line[200];
    if (!fp) { printf("No transactions yet.\n"); return; }
    while (fgets(line, sizeof(line), fp) && n < 1000) {
        line[strcspn(line, "\n")] = '\0';
        if (atoi(line) == a->accNo) { strncpy(lines[n], line, 119); lines[n][119] = '\0'; n++; }
    }
    fclose(fp);
    printf("\n--- Last 5 transactions ---\n");
    if (n == 0) { printf("No transactions yet.\n"); return; }
    for (int i = (n > 5 ? n - 5 : 0); i < n; i++) {
        char *no = strtok(lines[i], "|");
        char *date = strtok(NULL, "|");
        char *type = strtok(NULL, "|");
        char *amt = strtok(NULL, "|");
        char *bal = strtok(NULL, "|");
        (void)no;
        printf("%s  %-13s %-10s Bal: %s\n", date, type, amt, bal);
    }
}

void atmMenu(Account *acc) {
    int choice;
    do {
        printf("\n--- Welcome, %s ---\n"
               "1. Check balance\n2. Deposit\n3. Withdraw\n4. Transfer\n"
               "5. Mini statement\n6. Change PIN\n0. Logout\n", acc->name);
        choice = (int)readNumber("Choice: ");
        switch (choice) {
            case 1: printf("Balance: %.2f\n", acc->balance); break;
            case 2: deposit(acc); break;
            case 3: withdraw(acc); break;
            case 4: transfer(acc); break;
            case 5: miniStatement(acc); break;
            case 6: changePin(acc); break;
            case 0: printf("Logged out.\n"); break;
            default: printf("Invalid choice.\n");
        }
        saveAccounts();       /* save after every action so nothing is lost */
    } while (choice != 0);
}

int main(void) {
    int choice;
    loadAccounts();
    do {
        printf("\n===== BANK / ATM SIMULATOR =====\n"
               "1. Create account\n2. Login\n0. Exit\n");
        choice = (int)readNumber("Choice: ");
        if (choice == 1) { createAccount(); saveAccounts(); }
        else if (choice == 2) {
            Account *acc = login();
            if (acc) atmMenu(acc);
        }
    } while (choice != 0);
    saveAccounts();
    printf("Thank you for banking with us.\n");
    return 0;
}