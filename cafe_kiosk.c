#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define MAX_MENU_ITEMS 16
#define MAX_CART_ITEMS 48
#define MAX_NAME_LEN 64
#define INPUT_BUF_LEN 128
#define LOW_STOCK_THRESHOLD 5
#define VAT_RATE 0.05
#define ADMIN_PIN "1234"
#define MAX_PIN_ATTEMPTS 3

/* ANSI color codes */
#define CLR_RESET "\033[0m"
#define CLR_BOLD "\033[1m"
#define CLR_CYAN "\033[36m"
#define CLR_GREEN "\033[32m"
#define CLR_YELLOW "\033[33m"
#define CLR_RED "\033[31m"
#define CLR_MAGENTA "\033[35m"

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    double price;
    int stock;
} MenuItem;

typedef struct {
    int menuId;
    char name[MAX_NAME_LEN];
    double unitPrice;
    int quantity;
} CartItem;

typedef struct {
    double subtotal;
    double vat;
    double grandTotal;
    double cashTendered;
    double change;
} BillingSummary;

typedef struct {
    int transactions;
    int itemsSold;
    double subtotalRevenue;
    double vatCollected;
    double grossRevenue;
} SalesReport;

static void trim_newline(char *text) {
    size_t len = strlen(text);
    if (len > 0 && text[len - 1] == '\n') {
        text[len - 1] = '\0';
    }
}

static int read_line(char *buffer, size_t size) {
    size_t len;
    int ch;

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    trim_newline(buffer);

    /* Flush remaining characters if input is longer than buffer. */
    len = strlen(buffer);
    if (size > 1 && len == size - 1 && buffer[len - 1] != '\n') {
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }

    return 1;
}

static int is_blank_string(const char *text) {
    size_t i;
    for (i = 0; text[i] != '\0'; i++) {
        if (text[i] != ' ' && text[i] != '\t' && text[i] != '\r') {
            return 0;
        }
    }
    return 1;
}

static int parse_int_strict(const char *text, int *outValue) {
    char *endPtr;
    long value;

    if (text == NULL || outValue == NULL || is_blank_string(text)) {
        return 0;
    }

    value = strtol(text, &endPtr, 10);
    if (endPtr == text) {
        return 0;
    }

    while (*endPtr == ' ' || *endPtr == '\t' || *endPtr == '\r') {
        endPtr++;
    }

    if (*endPtr != '\0' || value < INT_MIN || value > INT_MAX) {
        return 0;
    }

    *outValue = (int)value;
    return 1;
}

static int parse_double_strict(const char *text, double *outValue) {
    char *endPtr;
    double value;

    if (text == NULL || outValue == NULL || is_blank_string(text)) {
        return 0;
    }

    value = strtod(text, &endPtr);
    if (endPtr == text) {
        return 0;
    }

    while (*endPtr == ' ' || *endPtr == '\t' || *endPtr == '\r') {
        endPtr++;
    }

    if (*endPtr != '\0') {
        return 0;
    }

    *outValue = value;
    return 1;
}

static int find_menu_index_by_id(const MenuItem menu[], int menuCount, int id) {
    (void)menu;
    if (id < 1 || id > menuCount) {
        return -1;
    }
    return id - 1;
}

static int find_cart_index_by_menu_id(const CartItem cart[], int cartCount, int menuId) {
    int i;
    for (i = 0; i < cartCount; i++) {
        if (cart[i].menuId == menuId) {
            return i;
        }
    }
    return -1;
}

static const char *stock_status_text(int stock) {
    if (stock <= 0) {
        return "OUT";
    }
    if (stock <= LOW_STOCK_THRESHOLD) {
        return "LOW";
    }
    return "OK";
}

static const char *stock_status_color(int stock) {
    if (stock <= 0) {
        return CLR_RED;
    }
    if (stock <= LOW_STOCK_THRESHOLD) {
        return CLR_YELLOW;
    }
    return CLR_GREEN;
}

static void print_menu_table(const MenuItem menu[], int menuCount) {
    int i;
    printf("\n%s%s=========================== MENU ===========================%s\n",
           CLR_BOLD, CLR_CYAN, CLR_RESET);
    printf("%-6s %-24s %-10s %-10s %-8s\n", "ID", "Item", "Price", "Stock", "Status");
    printf("--------------------------------------------------------------\n");

    for (i = 0; i < menuCount; i++) {
        printf("%-6d %-24s $%-9.2f %-10d %s%-8s%s\n",
               menu[i].id,
               menu[i].name,
               menu[i].price,
               menu[i].stock,
               stock_status_color(menu[i].stock),
               stock_status_text(menu[i].stock),
               CLR_RESET);
    }
    printf("--------------------------------------------------------------\n");
}

static void print_cart_snapshot(const CartItem cart[], int cartCount) {
    int i;
    double subtotal = 0.0;

    if (cartCount == 0) {
        printf("%sCart is empty.%s\n", CLR_MAGENTA, CLR_RESET);
        return;
    }

    printf("%sCurrent Cart:%s\n", CLR_BOLD, CLR_RESET);
    for (i = 0; i < cartCount; i++) {
        double lineTotal = cart[i].unitPrice * cart[i].quantity;
        subtotal += lineTotal;
        printf("  - [ID:%d] %-20s x%-3d = $%.2f\n",
               cart[i].menuId,
               cart[i].name,
               cart[i].quantity,
               lineTotal);
    }
    printf("%sCart Subtotal: $%.2f%s\n", CLR_GREEN, subtotal, CLR_RESET);
}

static int add_to_cart(MenuItem menu[], int menuIndex, CartItem cart[], int *cartCount, int quantity) {
    int existingIndex;

    if (quantity <= 0 || quantity > menu[menuIndex].stock) {
        return 0;
    }

    existingIndex = find_cart_index_by_menu_id(cart, *cartCount, menu[menuIndex].id);
    if (existingIndex < 0 && *cartCount >= MAX_CART_ITEMS) {
        return 0;
    }

    menu[menuIndex].stock -= quantity;

    if (existingIndex >= 0) {
        cart[existingIndex].quantity += quantity;
        return 1;
    }

    cart[*cartCount].menuId = menu[menuIndex].id;
    strncpy(cart[*cartCount].name, menu[menuIndex].name, MAX_NAME_LEN - 1);
    cart[*cartCount].name[MAX_NAME_LEN - 1] = '\0';
    cart[*cartCount].unitPrice = menu[menuIndex].price;
    cart[*cartCount].quantity = quantity;
    (*cartCount)++;
    return 1;
}

static int remove_from_cart(MenuItem menu[], int menuCount, CartItem cart[], int *cartCount, int menuId, int quantityToRemove) {
    int cartIndex = find_cart_index_by_menu_id(cart, *cartCount, menuId);
    int menuIndex;
    int i;

    if (cartIndex < 0 || quantityToRemove <= 0 || quantityToRemove > cart[cartIndex].quantity) {
        return 0;
    }

    menuIndex = find_menu_index_by_id(menu, menuCount, menuId);
    if (menuIndex < 0) {
        return 0;
    }

    cart[cartIndex].quantity -= quantityToRemove;
    menu[menuIndex].stock += quantityToRemove;

    if (cart[cartIndex].quantity > 0) {
        return 1;
    }

    for (i = cartIndex; i < *cartCount - 1; i++) {
        cart[i] = cart[i + 1];
    }
    (*cartCount)--;
    return 1;
}

static void clear_cart(MenuItem menu[], int menuCount, CartItem cart[], int *cartCount) {
    int i;
    for (i = 0; i < *cartCount; i++) {
        int menuIndex = find_menu_index_by_id(menu, menuCount, cart[i].menuId);
        if (menuIndex >= 0) {
            menu[menuIndex].stock += cart[i].quantity;
        }
    }
    *cartCount = 0;
}

static double calculate_subtotal(const CartItem cart[], int cartCount) {
    int i;
    double subtotal = 0.0;
    for (i = 0; i < cartCount; i++) {
        subtotal += cart[i].unitPrice * cart[i].quantity;
    }
    return subtotal;
}

static void calculate_bill(const CartItem cart[], int cartCount, BillingSummary *bill) {
    bill->subtotal = calculate_subtotal(cart, cartCount);
    bill->vat = bill->subtotal * VAT_RATE;
    bill->grandTotal = bill->subtotal + bill->vat;
    bill->cashTendered = 0.0;
    bill->change = 0.0;
}

static int generate_transaction_id(void) {
    return (rand() % 90000) + 10000;
}

static void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *localNow = localtime(&now);

    if (localNow == NULL) {
        strncpy(buffer, "Unavailable", size - 1);
        buffer[size - 1] = '\0';
        return;
    }

    if (strftime(buffer, size, "%Y-%m-%d %H:%M:%S", localNow) == 0) {
        strncpy(buffer, "Unavailable", size - 1);
        buffer[size - 1] = '\0';
    }
}

static int process_payment(BillingSummary *bill) {
    char input[INPUT_BUF_LEN];
    double cash;

    while (1) {
        printf("Enter cash amount (minimum $%.2f): ", bill->grandTotal);
        if (!read_line(input, sizeof(input))) {
            return 0;
        }

        if (!parse_double_strict(input, &cash) || cash < 0.0) {
            printf("%sInvalid cash amount.%s\n", CLR_RED, CLR_RESET);
            continue;
        }

        if (cash < bill->grandTotal) {
            printf("%sInsufficient cash. Need $%.2f more.%s\n",
                   CLR_RED, bill->grandTotal - cash, CLR_RESET);
            continue;
        }

        bill->cashTendered = cash;
        bill->change = cash - bill->grandTotal;
        return 1;
    }
}

static void print_receipt(const CartItem cart[], int cartCount, const BillingSummary *bill) {
    int i;
    int transactionId = generate_transaction_id();
    char timestamp[32];

    get_timestamp(timestamp, sizeof(timestamp));

    printf("\n%s%s===================== PAYMENT RECEIPT =====================%s\n",
           CLR_BOLD, CLR_GREEN, CLR_RESET);
    printf("Transaction ID: %s#%05d%s\n", CLR_YELLOW, transactionId, CLR_RESET);
    printf("Date/Time:      %s\n", timestamp);
    printf("-------------------------------------------------------------\n");
    printf("%-24s %-8s %-10s %-10s\n", "Item", "Qty", "Unit($)", "Total($)");
    printf("-------------------------------------------------------------\n");

    for (i = 0; i < cartCount; i++) {
        double lineTotal = cart[i].unitPrice * cart[i].quantity;
        printf("%-24s %-8d %-10.2f %-10.2f\n",
               cart[i].name, cart[i].quantity, cart[i].unitPrice, lineTotal);
    }

    printf("-------------------------------------------------------------\n");
    printf("%-44s %10.2f\n", "Subtotal:", bill->subtotal);
    printf("%-44s %10.2f\n", "VAT (5%):", bill->vat);
    printf("%s%-44s %10.2f%s\n", CLR_BOLD, "Grand Total:", bill->grandTotal, CLR_RESET);
    printf("%-44s %10.2f\n", "Cash Received:", bill->cashTendered);
    printf("%-44s %10.2f\n", "Change:", bill->change);
    printf("=============================================================\n");
}

static void update_sales_report(SalesReport *report, const CartItem cart[], int cartCount, const BillingSummary *bill) {
    int i;
    int items = 0;

    for (i = 0; i < cartCount; i++) {
        items += cart[i].quantity;
    }

    report->transactions += 1;
    report->itemsSold += items;
    report->subtotalRevenue += bill->subtotal;
    report->vatCollected += bill->vat;
    report->grossRevenue += bill->grandTotal;
}

static void print_sales_report(const SalesReport *report) {
    printf("\n%s%s======================= SALES REPORT =======================%s\n",
           CLR_BOLD, CLR_MAGENTA, CLR_RESET);
    printf("Total Transactions : %d\n", report->transactions);
    printf("Total Items Sold   : %d\n", report->itemsSold);
    printf("Subtotal Revenue   : $%.2f\n", report->subtotalRevenue);
    printf("VAT Collected      : $%.2f\n", report->vatCollected);
    printf("%sGross Revenue      : $%.2f%s\n", CLR_BOLD, report->grossRevenue, CLR_RESET);
    printf("==============================================================\n");
}

static int ask_int_in_range(const char *prompt, int minValue, int maxValue, int *outValue) {
    char input[INPUT_BUF_LEN];
    int value;

    while (1) {
        printf("%s", prompt);
        if (!read_line(input, sizeof(input))) {
            return 0;
        }

        if (!parse_int_strict(input, &value)) {
            printf("%sPlease enter a valid number.%s\n", CLR_RED, CLR_RESET);
            continue;
        }

        if (value < minValue || value > maxValue) {
            printf("%sEnter a value between %d and %d.%s\n",
                   CLR_RED, minValue, maxValue, CLR_RESET);
            continue;
        }

        *outValue = value;
        return 1;
    }
}

static int verify_admin_pin(void) {
    char input[INPUT_BUF_LEN];
    int attemptsLeft = MAX_PIN_ATTEMPTS;

    while (attemptsLeft > 0) {
        printf("Enter Admin PIN: ");
        if (!read_line(input, sizeof(input))) {
            return 0;
        }

        if (strcmp(input, ADMIN_PIN) == 0) {
            printf("%sAccess granted.%s\n", CLR_GREEN, CLR_RESET);
            return 1;
        }

        attemptsLeft--;
        if (attemptsLeft > 0) {
            printf("%sIncorrect PIN. Attempts left: %d%s\n",
                   CLR_RED, attemptsLeft, CLR_RESET);
        }
    }

    printf("%sAdmin access denied.%s\n", CLR_RED, CLR_RESET);
    return 0;
}

static void customer_mode(MenuItem menu[], int menuCount, SalesReport *report) {
    CartItem cart[MAX_CART_ITEMS];
    int cartCount = 0;
    int running = 1;

    memset(cart, 0, sizeof(cart));

    while (running) {
        int choice;

        printf("\n%s%sCustomer Mode%s\n", CLR_BOLD, CLR_CYAN, CLR_RESET);
        printf("1. Add item to cart\n");
        printf("2. Remove item from cart\n");
        printf("3. View cart\n");
        printf("4. Clear cart\n");
        printf("5. Checkout\n");
        printf("0. Back to role menu\n");

        if (!ask_int_in_range("Choose an option: ", 0, 5, &choice)) {
            return;
        }

        if (choice == 0) {
            clear_cart(menu, menuCount, cart, &cartCount);
            printf("%sReturning to role menu...%s\n", CLR_YELLOW, CLR_RESET);
            return;
        }

        if (choice == 1) {
            int itemId;
            int menuIndex;
            int qty;

            print_menu_table(menu, menuCount);

            if (!ask_int_in_range("Enter menu item ID: ", 1, menuCount, &itemId)) {
                return;
            }

            menuIndex = find_menu_index_by_id(menu, menuCount, itemId);
            if (menuIndex < 0) {
                printf("%sInvalid item ID.%s\n", CLR_RED, CLR_RESET);
                continue;
            }

            if (menu[menuIndex].stock <= 0) {
                printf("%sItem is out of stock.%s\n", CLR_RED, CLR_RESET);
                continue;
            }

            if (!ask_int_in_range("Enter quantity: ", 1, menu[menuIndex].stock, &qty)) {
                return;
            }

            if (!add_to_cart(menu, menuIndex, cart, &cartCount, qty)) {
                printf("%sCould not add item to cart.%s\n", CLR_RED, CLR_RESET);
                continue;
            }

            printf("%sAdded %d x %s.%s\n", CLR_GREEN, qty, menu[menuIndex].name, CLR_RESET);
            continue;
        }

        if (choice == 2) {
            int itemId;
            int cartIndex;
            int qty;
            char itemName[MAX_NAME_LEN];

            if (cartCount == 0) {
                printf("%sCart is empty.%s\n", CLR_YELLOW, CLR_RESET);
                continue;
            }

            print_cart_snapshot(cart, cartCount);

            if (!ask_int_in_range("Enter item ID to remove: ", 1, menuCount, &itemId)) {
                return;
            }

            cartIndex = find_cart_index_by_menu_id(cart, cartCount, itemId);
            if (cartIndex < 0) {
                printf("%sItem not found in cart.%s\n", CLR_RED, CLR_RESET);
                continue;
            }

            strncpy(itemName, cart[cartIndex].name, MAX_NAME_LEN - 1);
            itemName[MAX_NAME_LEN - 1] = '\0';

            if (!ask_int_in_range("Enter quantity to remove: ", 1, cart[cartIndex].quantity, &qty)) {
                return;
            }

            if (!remove_from_cart(menu, menuCount, cart, &cartCount, itemId, qty)) {
                printf("%sRemove failed.%s\n", CLR_RED, CLR_RESET);
                continue;
            }

            printf("%sRemoved %d x %s.%s\n", CLR_GREEN, qty, itemName, CLR_RESET);
            continue;
        }

        if (choice == 3) {
            print_cart_snapshot(cart, cartCount);
            continue;
        }

        if (choice == 4) {
            clear_cart(menu, menuCount, cart, &cartCount);
            printf("%sCart cleared.%s\n", CLR_GREEN, CLR_RESET);
            continue;
        }

        if (choice == 5) {
            BillingSummary bill;

            if (cartCount == 0) {
                printf("%sCart is empty. Add items first.%s\n", CLR_YELLOW, CLR_RESET);
                continue;
            }

            calculate_bill(cart, cartCount, &bill);
            printf("%sSubtotal: $%.2f | VAT: $%.2f | Grand Total: $%.2f%s\n",
                   CLR_BOLD, bill.subtotal, bill.vat, bill.grandTotal, CLR_RESET);

            if (!process_payment(&bill)) {
                printf("%sPayment cancelled.%s\n", CLR_RED, CLR_RESET);
                continue;
            }

            print_receipt(cart, cartCount, &bill);
            update_sales_report(report, cart, cartCount, &bill);
            clear_cart(menu, menuCount, cart, &cartCount);
            printf("%sOrder complete.%s\n", CLR_GREEN, CLR_RESET);
            continue;
        }
    }
}

static void admin_mode(MenuItem menu[], int menuCount, const SalesReport *report) {
    int running = 1;

    while (running) {
        int choice;

        printf("\n%s%sAdmin Mode%s\n", CLR_BOLD, CLR_MAGENTA, CLR_RESET);
        printf("1. View menu stock\n");
        printf("2. Restock item\n");
        printf("3. View sales report\n");
        printf("0. Back to role menu\n");

        if (!ask_int_in_range("Choose an option: ", 0, 3, &choice)) {
            return;
        }

        if (choice == 0) {
            printf("%sReturning to role menu...%s\n", CLR_YELLOW, CLR_RESET);
            return;
        }

        if (choice == 1) {
            print_menu_table(menu, menuCount);
            continue;
        }

        if (choice == 2) {
            int itemId;
            int menuIndex;
            int qty;

            print_menu_table(menu, menuCount);

            if (!ask_int_in_range("Enter item ID to restock: ", 1, menuCount, &itemId)) {
                return;
            }

            menuIndex = find_menu_index_by_id(menu, menuCount, itemId);
            if (menuIndex < 0) {
                printf("%sInvalid item ID.%s\n", CLR_RED, CLR_RESET);
                continue;
            }

            if (!ask_int_in_range("Enter quantity to add: ", 1, 10000, &qty)) {
                return;
            }

            menu[menuIndex].stock += qty;
            printf("%sRestocked %s by %d. New stock: %d%s\n",
                   CLR_GREEN, menu[menuIndex].name, qty, menu[menuIndex].stock, CLR_RESET);
            continue;
        }

        if (choice == 3) {
            print_sales_report(report);
            continue;
        }
    }
}

int main(void) {
    MenuItem menu[MAX_MENU_ITEMS] = {
        {1, "Espresso", 2.50, 20},
        {2, "Cappuccino", 3.20, 15},
        {3, "Latte", 3.80, 14},
        {4, "Blueberry Muffin", 2.10, 12},
        {5, "Chicken Sandwich", 4.75, 10},
        {6, "French Fries", 2.95, 18},
        {7, "Veggie Wrap", 4.20, 9},
        {8, "Chocolate Brownie", 2.60, 11},
        {9, "Iced Americano", 3.00, 17},
        {10, "Mocha", 4.10, 13},
        {11, "Club Sandwich", 5.25, 10},
        {12, "Cheese Burger", 5.80, 10},
        {13, "Chicken Nuggets", 4.50, 14},
        {14, "Caesar Salad", 4.90, 9},
        {15, "Mineral Water", 1.20, 30},
        {16, "Orange Juice", 2.80, 16}
    };
    SalesReport report;

    memset(&report, 0, sizeof(report));
    srand((unsigned int)time(NULL));

    printf("%s%sWelcome to Cafe Management & Vending Kiosk%s\n",
           CLR_BOLD, CLR_CYAN, CLR_RESET);

    while (1) {
        int roleChoice;

        printf("\n%sSelect Role%s\n", CLR_BOLD, CLR_RESET);
        printf("1. Customer Mode\n");
        printf("2. Admin Mode\n");
        printf("0. Exit\n");

        if (!ask_int_in_range("Choose role: ", 0, 2, &roleChoice)) {
            break;
        }

        if (roleChoice == 0) {
            printf("%sGoodbye!%s\n", CLR_GREEN, CLR_RESET);
            break;
        }

        if (roleChoice == 1) {
            customer_mode(menu, MAX_MENU_ITEMS, &report);
            continue;
        }

        if (roleChoice == 2) {
            if (!verify_admin_pin()) {
                continue;
            }
            admin_mode(menu, MAX_MENU_ITEMS, &report);
            continue;
        }
    }

    return 0;
}
