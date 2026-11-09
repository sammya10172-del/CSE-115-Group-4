#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_TRANSACTIONS 500
#define MAX_DAYS 300
#define TX_FILE "data/transactions.txt"
#define DAY_FILE "data/days.txt"

/* One record for each income or expense. */
typedef struct
{
    int id;
    char type[10];
    double amount;
    char category[30];
    char method[20];
    char source[40];
    int physicalCash;
    char note[100];
    char date[15];
    char time[10];
} Transaction;

/* One record for each day. */
typedef struct
{
    char date[15];
    double openingCash;
    int closed;
    double actualCash;
    int needsReconcile;
} DayRecord;

Transaction transactions[MAX_TRANSACTIONS];
DayRecord days[MAX_DAYS];
int transactionCount = 0;
int dayCount = 0;

/* COMMON PART: input helpers used by the whole program. Keep these shared. */
void readText(const char *prompt, char answer[], int size)
{
    printf("%s", prompt);
    if (fgets(answer, size, stdin) == NULL)
    {
        answer[0] = '\0';
        return;
    }
    answer[strcspn(answer, "\n")] = '\0';
    /* Keep the text-file format safe. */
    for (int i = 0; answer[i] != '\0'; i++)
    {
        if (answer[i] == '|') answer[i] = '/';
    }
}

int readNumber(const char *prompt, int min, int max)
{
    char line[50];
    int number;
    while (1)
    {
        readText(prompt, line, sizeof(line));
        if (sscanf(line, "%d", &number) == 1 && number >= min && number <= max)
        {
            return number;
        }
        printf("Please enter a number from %d to %d.\n", min, max);
    }
}

double readAmount(const char *prompt, int allowZero)
{
    char line[50];
    double amount;
    while (1)
    {
        readText(prompt, line, sizeof(line));
        if (sscanf(line, "%lf", &amount) == 1 &&
                (allowZero ? amount >= 0 : amount > 0))
        {
            return amount;
        }
        printf(allowZero ? "Enter zero or a positive amount.\n" :
               "Enter an amount greater than zero.\n");
    }
}

void getCurrentDateTime(char date[], char timeText[])
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    if (local == NULL)
    {
        strcpy(date, "2000-01-01");
        strcpy(timeText, "00:00");
        return;
    }
    strftime(date, 15, "%Y-%m-%d", local);
    strftime(timeText, 10, "%H:%M", local);
}

/* GROUP MEMBER 1: save and load transaction records. */
void saveTransactions(void)
{
    FILE *file = fopen(TX_FILE, "w");
    int i;
    if (file == NULL)
    {
        printf("Could not save transactions. Keep the data folder beside the program.\n");
        return;
    }
    for (i = 0; i < transactionCount; i++)
    {
        Transaction t = transactions[i];
        fprintf(file, "%d|%s|%.2f|%s|%s|%s|%d|%s|%s|%s\n",
                t.id, t.type, t.amount, t.category, t.method, t.source,
                t.physicalCash, t.note, t.date, t.time);
    }
    fclose(file);
}

void loadTransactions(void)
{
    FILE *file = fopen(TX_FILE, "r");
    char line[400];
    if (file == NULL) return;
    while (transactionCount < MAX_TRANSACTIONS && fgets(line, sizeof(line), file))
    {
        Transaction t;
        if (sscanf(line, "%d|%9[^|]|%lf|%29[^|]|%19[^|]|%39[^|]|%d|%99[^|]|%14[^|]|%9[^\n]",
                   &t.id, t.type, &t.amount, t.category, t.method, t.source,
                   &t.physicalCash, t.note, t.date, t.time) == 10)
        {
            transactions[transactionCount++] = t;
        }
    }
    fclose(file);
}

/* GROUP MEMBER 2: save and load each day's opening and closing balance. */
void saveDays(void)
{
    FILE *file = fopen(DAY_FILE, "w");
    int i;
    if (file == NULL)
    {
        printf("Could not save day balances. Keep the data folder beside the program.\n");
        return;
    }
    for (i = 0; i < dayCount; i++)
    {
        fprintf(file, "%s|%.2f|%d|%.2f|%d\n", days[i].date,
                days[i].openingCash, days[i].closed, days[i].actualCash,
                days[i].needsReconcile);
    }
    fclose(file);
}

void loadDays(void)
{
    FILE *file = fopen(DAY_FILE, "r");
    char line[150];
    if (file == NULL) return;
    while (dayCount < MAX_DAYS && fgets(line, sizeof(line), file))
    {
        DayRecord d;
        if (sscanf(line, "%14[^|]|%lf|%d|%lf|%d", d.date,
                   &d.openingCash, &d.closed, &d.actualCash,
                   &d.needsReconcile) == 5)
        {
            days[dayCount++] = d;
        }
    }
    fclose(file);
}

/* GROUP MEMBER 2: wallet calculation and next-day balance setup. */
int findDay(const char date[])
{
    int i;
    for (i = 0; i < dayCount; i++)
    {
        if (strcmp(days[i].date, date) == 0) return i;
    }
    return -1;
}

double walletForDate(const char date[])
{
    int i, dayIndex = findDay(date);
    double wallet = dayIndex >= 0 ? days[dayIndex].openingCash : 0;
    for (i = 0; i < transactionCount; i++)
    {
        Transaction t = transactions[i];
        if (strcmp(t.date, date) != 0) continue;
        if (strcmp(t.type, "Income") == 0 && t.physicalCash) wallet += t.amount;
        if (strcmp(t.type, "Expense") == 0 && strcmp(t.method, "Cash") == 0)
            wallet -= t.amount;
    }
    return wallet;
}

void prepareToday(const char today[])
{
    int i, previous = -1;
    if (findDay(today) >= 0 || dayCount >= MAX_DAYS) return;

    /* Find the most recent earlier day. Dates are stored as YYYY-MM-DD. */
    for (i = 0; i < dayCount; i++)
    {
        if (strcmp(days[i].date, today) < 0 &&
                (previous < 0 || strcmp(days[i].date, days[previous].date) > 0))
        {
            previous = i;
        }
    }

    strcpy(days[dayCount].date, today);
    days[dayCount].closed = 0;
    days[dayCount].actualCash = 0;
    days[dayCount].needsReconcile = 0;

    if (previous >= 0)
    {
        if (days[previous].closed)
        {
            days[dayCount].openingCash = days[previous].actualCash;
        }
        else
        {
            days[dayCount].openingCash = walletForDate(days[previous].date);
            days[dayCount].needsReconcile = 1;
        }
    }
    else
    {
        days[dayCount].openingCash = readAmount("Starting wallet cash (BDT): ", 1);
    }
    dayCount++;
    saveDays();
}

/* GROUP MEMBER 1: add expenses and cash-in records. */
int nextId(void)
{
    int i, biggest = 0;
    for (i = 0; i < transactionCount; i++)
        if (transactions[i].id > biggest) biggest = transactions[i].id;
    return biggest + 1;
}

void addExpense(void)
{
    Transaction *t;
    int category, method;
    if (transactionCount >= MAX_TRANSACTIONS)
    {
        printf("Transaction list is full.\n");
        return;
    }
    t = &transactions[transactionCount];
    t->id = nextId();
    strcpy(t->type, "Expense");
    t->amount = readAmount("Expense amount (BDT): ", 0);

    printf("Categories: 1 Food, 2 Transport, 3 Tea/Coffee, 4 Shopping,\n");
    printf("5 Bills, 6 Family, 7 Business, 8 Medical, 9 Entertainment, 10 Other\n");
    category = readNumber("Choose category: ", 1, 10);
    {
        const char *names[] = {"Food", "Transport", "Tea/Coffee", "Shopping",
                               "Bills", "Family", "Business", "Medical", "Entertainment", "Other"
                              };
        strcpy(t->category, names[category - 1]);
    }

    printf("Payment: 1 Cash, 2 bKash, 3 Nagad, 4 Bank Card, 5 Bank Transfer\n");
    method = readNumber("Choose payment method: ", 1, 5);
    {
        const char *names[] = {"Cash", "bKash", "Nagad", "Bank Card", "Bank Transfer"};
        strcpy(t->method, names[method - 1]);
    }
    strcpy(t->source, "None");
    t->physicalCash = (method == 1);
    readText("Short note (one line): ", t->note, sizeof(t->note));
    getCurrentDateTime(t->date, t->time);
    transactionCount++;
    saveTransactions();
    printf("Expense added and saved.\n");
}

void addIncome(void)
{
    Transaction *t;
    int source, method;
    if (transactionCount >= MAX_TRANSACTIONS)
    {
        printf("Transaction list is full.\n");
        return;
    }
    t = &transactions[transactionCount];
    t->id = nextId();
    strcpy(t->type, "Income");
    t->amount = readAmount("Money received (BDT): ", 0);

    printf("Source: 1 Bank / ATM withdrawal, 2 Salary / income,\n");
    printf("3 Family / friends, 4 Refund, 5 Other\n");
    source = readNumber("Choose source: ", 1, 5);
    {
        const char *names[] = {"Bank / ATM withdrawal", "Salary / income",
                               "Family / friends", "Refund", "Other"
                              };
        strcpy(t->source, names[source - 1]);
    }

    printf("Received as: 1 Physical cash, 2 bKash, 3 Nagad, 4 Bank transfer\n");
    method = readNumber("Choose method: ", 1, 4);
    {
        const char *names[] = {"Cash", "bKash", "Nagad", "Bank transfer"};
        strcpy(t->method, names[method - 1]);
    }
    strcpy(t->category, "None");
    t->physicalCash = (method == 1);
    readText("Short note (one line): ", t->note, sizeof(t->note));
    getCurrentDateTime(t->date, t->time);
    transactionCount++;
    saveTransactions();
    printf("Money received and saved.\n");
}

/* GROUP MEMBER 3: show the saved transaction history. */
void viewTransactions(void)
{
    int i;
    if (transactionCount == 0)
    {
        printf("No transactions yet.\n");
        return;
    }
    printf("\nID | Date | Time | Type | Amount | Category/Source | Method | Note\n");
    for (i = 0; i < transactionCount; i++)
    {
        Transaction t = transactions[i];
        printf("%d | %s | %s | %s | %.2f | %s | %s | %s\n",
               t.id, t.date, t.time, t.type, t.amount,
               strcmp(t.type, "Expense") == 0 ? t.category : t.source,
               t.method, t.note);
    }
}

/* GROUP MEMBER 1: find, edit, or delete a transaction record. */
int findTransaction(int id)
{
    int i;
    for (i = 0; i < transactionCount; i++)
        if (transactions[i].id == id) return i;
    return -1;
}

void editTransaction(void)
{
    int id, index;
    double amount;
    char note[100];
    viewTransactions();
    if (transactionCount == 0) return;
    id = readNumber("Enter transaction ID to edit: ", 1, 999999);
    index = findTransaction(id);
    if (index < 0)
    {
        printf("That ID was not found.\n");
        return;
    }
    printf("Current amount: %.2f BDT\n", transactions[index].amount);
    amount = readAmount("New amount (enter 0 to keep current): ", 1);
    if (amount > 0) transactions[index].amount = amount;
    printf("Current note: %s\n", transactions[index].note);
    readText("New note (press Enter to keep current): ", note, sizeof(note));
    if (note[0] != '\0') strcpy(transactions[index].note, note);
    saveTransactions();
    printf("Transaction updated.\n");
}

void deleteTransaction(void)
{
    int id, index, i;
    char confirm[10];
    viewTransactions();
    if (transactionCount == 0) return;
    id = readNumber("Enter transaction ID to delete: ", 1, 999999);
    index = findTransaction(id);
    if (index < 0)
    {
        printf("That ID was not found.\n");
        return;
    }
    readText("Delete this record? (y/n): ", confirm, sizeof(confirm));
    if (confirm[0] != 'y' && confirm[0] != 'Y')
    {
        printf("Delete cancelled.\n");
        return;
    }
    for (i = index; i < transactionCount - 1; i++)
        transactions[i] = transactions[i + 1];
    transactionCount--;
    saveTransactions();
    printf("Transaction deleted.\n");
}

/* GROUP MEMBER 2: dashboard and end-of-day cash check. */
void showDashboard(const char today[])
{
    int i, d = findDay(today);
    double received = 0, cashSpent = 0, digitalSpent = 0, withdrawn = 0;
    for (i = 0; i < transactionCount; i++)
    {
        Transaction t = transactions[i];
        if (strcmp(t.date, today) != 0) continue;
        if (strcmp(t.type, "Income") == 0)
        {
            if (t.physicalCash && strcmp(t.source, "Bank / ATM withdrawal") != 0)
                received += t.amount;
            if (strcmp(t.source, "Bank / ATM withdrawal") == 0) withdrawn += t.amount;
        }
        else if (strcmp(t.method, "Cash") == 0) cashSpent += t.amount;
        else digitalSpent += t.amount;
    }
    printf("\n=== Today's Dashboard: %s ===\n", today);
    printf("Opening wallet cash: %.2f BDT\n", d >= 0 ? days[d].openingCash : 0);
    printf("Cash received (not bank withdrawals): %.2f BDT\n", received);
    printf("Cash expenses: %.2f BDT\n", cashSpent);
    printf("Digital expenses: %.2f BDT\n", digitalSpent);
    printf("All expenses: %.2f BDT\n", cashSpent + digitalSpent);
    printf("Bank / ATM withdrawals: %.2f BDT\n", withdrawn);
    printf("Current wallet cash: %.2f BDT\n", walletForDate(today));
    if (d >= 0 && days[d].needsReconcile)
        printf("Reminder: yesterday was not closed. Please reconcile it.\n");
}

void closeDay(const char today[])
{
    int d = findDay(today);
    double expected, actual;
    if (d < 0)
    {
        printf("Today's record is not ready.\n");
        return;
    }
    expected = walletForDate(today);
    printf("Expected wallet cash: %.2f BDT\n", expected);
    actual = readAmount("Count the actual cash in your wallet (BDT): ", 1);
    days[d].actualCash = actual;
    days[d].closed = 1;
    days[d].needsReconcile = 0;
    saveDays();
    printf("Cash difference: %.2f BDT\n", actual - expected);
    printf("Day closed. This counted cash will be tomorrow's opening cash.\n");
}

/* GROUP MEMBER 3: search, reports, and CSV export. */
int containsText(const char text[], const char key[])
{
    int i, j;
    if (key[0] == '\0') return 1;
    for (i = 0; text[i] != '\0'; i++)
    {
        for (j = 0; key[j] != '\0' && text[i + j] != '\0'; j++)
        {
            if (tolower((unsigned char)text[i + j]) !=
                    tolower((unsigned char)key[j])) break;
        }
        if (key[j] == '\0') return 1;
    }
    return 0;
}

void searchTransactions(void)
{
    char key[50];
    int i, found = 0;
    readText("Search by date, category, source, method, or note: ", key, sizeof(key));
    for (i = 0; i < transactionCount; i++)
    {
        Transaction t = transactions[i];
        if (containsText(t.date, key) || containsText(t.category, key) ||
                containsText(t.source, key) || containsText(t.method, key) ||
                containsText(t.note, key))
        {
            printf("%d | %s %s | %s | %.2f BDT | %s | %s | %s\n",
                   t.id, t.date, t.time, t.type, t.amount,
                   t.category, t.method, t.note);
            found = 1;
        }
    }
    if (!found) printf("No matching transactions found.\n");
}

void subtractDays(const char today[], int number, char result[])
{
    int year, month, day;
    struct tm dateValue;
    time_t stamp;
    sscanf(today, "%d-%d-%d", &year, &month, &day);
    memset(&dateValue, 0, sizeof(dateValue));
    dateValue.tm_year = year - 1900;
    dateValue.tm_mon = month - 1;
    dateValue.tm_mday = day - number;
    stamp = mktime(&dateValue);
    strftime(result, 15, "%Y-%m-%d", localtime(&stamp));
}

void showReport(const char today[])
{
    int choice, i, c;
    char start[15], monthPrefix[8];
    double cash = 0, digital = 0, total = 0;
    double categoryTotals[10] = {0};
    int entries = 0;
    const char *categories[] = {"Food", "Transport", "Tea/Coffee", "Shopping",
                                "Bills", "Family", "Business", "Medical", "Entertainment", "Other"
                               };

    printf("1. Daily  2. Last 7 days  3. This month\n");
    choice = readNumber("Choose report: ", 1, 3);
    if (choice == 1) strcpy(start, today);
    else if (choice == 2) subtractDays(today, 6, start);
    else
    {
        strncpy(monthPrefix, today, 7);
        monthPrefix[7] = '\0';
        strcpy(start, monthPrefix);
        strcat(start, "-01");
    }

    for (i = 0; i < transactionCount; i++)
    {
        Transaction t = transactions[i];
        if (strcmp(t.type, "Expense") != 0 ||
                strcmp(t.date, start) < 0 || strcmp(t.date, today) > 0) continue;
        entries++;
        total += t.amount;
        if (strcmp(t.method, "Cash") == 0) cash += t.amount;
        else digital += t.amount;
        for (c = 0; c < 10; c++)
        {
            if (strcmp(t.category, categories[c]) == 0)
            {
                categoryTotals[c] += t.amount;
                break;
            }
        }
    }

    printf("\n=== Expense Report (%s to %s) ===\n", start, today);
    printf("Cash expenses: %.2f BDT\n", cash);
    printf("Digital expenses: %.2f BDT\n", digital);
    printf("Total expenses: %.2f BDT\n", total);
    printf("Expense entries: %d\n", entries);
    if (entries > 0) printf("Average per expense entry: %.2f BDT\n", total / entries);
    printf("Category totals:\n");
    for (c = 0; c < 10; c++)
    {
        if (categoryTotals[c] > 0)
            printf("%s: %.2f BDT\n", categories[c], categoryTotals[c]);
    }
}

void exportCSV(void)
{
    FILE *file = fopen("data/transactions.csv", "w");
    int i;
    if (file == NULL)
    {
        printf("Could not create CSV file in the data folder.\n");
        return;
    }
    fprintf(file, "ID,Type,Amount,Category,Method,Source,PhysicalCash,Note,Date,Time\n");
    for (i = 0; i < transactionCount; i++)
    {
        Transaction t = transactions[i];
        fprintf(file, "%d,%s,%.2f,%s,%s,%s,%s,\"%s\",%s,%s\n",
                t.id, t.type, t.amount, t.category, t.method, t.source,
                t.physicalCash ? "Yes" : "No", t.note, t.date, t.time);
    }
    fclose(file);
    printf("CSV exported to data/transactions.csv\n");
}

/* ---------- Main menu: connects all group members' functions ---------- */
int main(void)
{
    int choice = -1;
    char today[15], nowTime[10];

    loadTransactions();
    loadDays();
    getCurrentDateTime(today, nowTime);
    prepareToday(today);

    printf("Daily Expense Tracker started. Loaded %d transactions.\n", transactionCount);

    for (; choice != 0; )
    {
        printf("\n=== Daily Expense Tracker ===\n");
        printf("1. Add expense\n");
        printf("2. Cash in / money received\n");
        printf("3. View all transactions\n");
        printf("4. Search transactions\n");
        printf("5. Show today's dashboard\n");
        printf("6. Edit a transaction\n");
        printf("7. Delete a transaction\n");
        printf("8. Close day\n");
        printf("9. Daily / weekly / monthly report\n");
        printf("10. Export CSV\n");
        printf("0. Exit\n");
        choice = readNumber("Choose an option: ", 0, 10);

        switch (choice)
        {
        case 1:
            addExpense();
            break;
        case 2:
            addIncome();
            break;
        case 3:
            viewTransactions();
            break;
        case 4:
            searchTransactions();
            break;
        case 5:
            showDashboard(today);
            break;
        case 6:
            editTransaction();
            break;
        case 7:
            deleteTransaction();
            break;
        case 8:
            closeDay(today);
            break;
        case 9:
            showReport(today);
            break;
        case 10:
            exportCSV();
            break;
        case 0:
            printf("Goodbye! Your records are saved.\n");
            break;
        }
    }
    return 0;
}



