# CSE-115-Group-4
# Daily Expense Tracker and Cash Balance Manager

## About the Project

This is a simple C programming project for keeping track of daily income and expenses. The user can enter transactions using the keyboard. The program can calculate total expenses and show how much cash is left.

This project runs in the console or terminal. It uses a text-based menu, so it does not have a mobile app interface.

## Main Features

- Add a new expense with its amount, category, payment method, date, time, and a short note.
- Record money received, such as salary, gifts, refunds, or cash withdrawn from a bank or ATM.
- View the saved income and expense records.
- Show a daily summary of cash received, cash expenses, digital expenses, total expenses, and remaining cash.
- Compare the expected cash balance with the actual cash counted at the end of the day.
- Carry the closing cash balance forward as the next day's opening balance.
- Save transaction information in a file so it can be used again when the program is reopened.
- Export transaction information to a CSV file.

## Expense Categories

Food, Transport, Tea/Coffee, Shopping, Bills, Family, Business, Medical, Entertainment, and Other.

## Payment Methods

Cash, bKash, Nagad, Bank Card, and Bank Transfer.

## Cash Balance Calculation

The program calculates the remaining cash using this formula:

**Opening Cash + Cash Received + Bank/ATM Cash Withdrawals - Cash Expenses = Current Cash Balance**

Digital payments are recorded separately because they do not change the cash in the user's wallet.

## C Concepts Used

This project uses basic C programming concepts, including:

- struct to store transaction details
- Functions to organize different tasks
- Loops and switch statements to run the menu
- File handling to save and read transaction data

## Project Summary

The Daily Expense Tracker and Cash Balance Manager is a console-based C program for recording daily income and expenses. It helps the user track cash and digital payments separately, calculate the remaining wallet balance, and keep transaction records in a file.
