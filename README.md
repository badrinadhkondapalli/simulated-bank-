# Bank / ATM Simulator in C

A feature-rich console-based **Bank and ATM Simulator** written in C[cite: 2]. This project demonstrates fundamental systems programming concepts including file handling, pointer manipulation, structs, transaction logging, and secure PIN authentication[cite: 2].

---

## Features

* **Account Management**: Create new bank accounts with auto-incremented account numbers[cite: 2].
* **Secure Authentication**: 4-digit PIN verification system with a 3-attempt lockout mechanism[cite: 2].
* **ATM Operations**:
  * Check account balance[cite: 3]
  * Cash deposits with auto-saving[cite: 3]
  * Cash withdrawals with validation for sufficient funds[cite: 2, 3]
  * Fund transfers between accounts with dual-sided logging[cite: 2, 3]
  * Mini-statement viewer showing the recent transaction history[cite: 2, 3]
  * Secure PIN modification[cite: 2, 3]
* **Persistent Storage**: Automatically saves account updates and logs every transaction (`accounts.txt` and `transactions.txt`) so data persists between sessions[cite: 2].

---

## Project Structure

```text
├── Bank.c             # Main source code file
├── accounts.txt       # Flat-file database for account credentials & balances
└── transactions.txt   # Audit trail log for all financial transactions
