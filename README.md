**# Samrt Cafe Management System**
A modular, interactive command-line cafe kiosk system written in pure C using only standard libraries:
- `stdio.h`
- `stdlib.h`
- `string.h`
- `time.h`

## Features

### 1. Data Model
- `MenuItem` struct:
  - `id`
  - `name`
  - `price`
  - `stock`
- `CartItem` struct:
  - `menuId`
  - `name`
  - `unitPrice`
  - `quantity`
- `BillingSummary` struct:
  - `subtotal`
  - `vat`
  - `grandTotal`
  - `cashTendered`
  - `change`
- `SalesReport` struct:
  - `transactions`
  - `itemsSold`
  - `subtotalRevenue`
  - `vatCollected`
  - `grossRevenue`

### 2. Role Modes
- **Customer Mode**
  - Add item to cart
  - Remove item from cart
  - View cart
  - Clear cart
  - Checkout
- **Admin Mode**
  - View menu stock
  - Restock item
  - View sales report

### 3. Admin PIN Lock
- Admin panel is protected by PIN.
- Current PIN: `1234`
- Max attempts: `3`
- Access denied after failed attempts.

### 4. Interactive Menu + Inventory
- Formatted table view with:
  - ID
  - Name
  - Price
  - Stock
  - Stock status (`OK`, `LOW`, `OUT`)
- Live inventory updates when adding/removing/clearing cart.

### 5. Cart Management
- Adds by item ID + quantity.
- Automatically merges repeated item selections into one cart entry.
- Prevents cart additions beyond stock.
- Supports partial quantity removal.
- Supports full cart clear with stock restoration.

### 6. Billing Engine
- Subtotal calculation from cart.
- VAT auto-applied at `5%`.
- Grand total calculation.
- Cash payment processing with insufficient-cash rejection.
- Change calculation.

### 7. Receipt Generation
- Random 5-digit transaction ID.
- Timestamp (`YYYY-MM-DD HH:MM:SS`).
- Itemized line entries with per-item totals.
- Subtotal, VAT, grand total, cash received, and change.

### 8. Sales Reporting (Admin)
- Tracks per-session totals:
  - Number of transactions
  - Items sold
  - Subtotal revenue
  - VAT collected
  - Gross revenue

### 9. Input Validation & Robustness
- Rejects invalid/non-numeric inputs.
- Enforces numeric ranges for menu choices and quantities.
- Handles long input lines safely by flushing extra characters.
- Graceful handling for out-of-range IDs and invalid operations.

### 10. UI/UX
- ANSI color-coded terminal output:
  - Informational (`cyan/magenta`)
  - Success (`green`)
  - Warnings (`yellow`)
  - Errors (`red`)

## Menu Items Included
- Espresso
- Cappuccino
- Latte
- Blueberry Muffin
- Chicken Sandwich
- French Fries
- Veggie Wrap
- Chocolate Brownie
- Iced Americano
- Mocha
- Club Sandwich
- Cheese Burger
- Chicken Nuggets
- Caesar Salad
- Mineral Water
- Orange Juice

## File Structure
- Source code: `cafe_kiosk.c`
- Documentation: `README.md`

## Build and Run

### GCC / MinGW
```bash
gcc cafe_kiosk.c -o cafe_kiosk
./cafe_kiosk
```

### Clang
```bash
clang cafe_kiosk.c -o cafe_kiosk
./cafe_kiosk
```

### MSVC (Developer Command Prompt)
```bat
cl /W4 /EHsc cafe_kiosk.c
cafe_kiosk.exe
```

## Notes
- Sales and stock are currently in-memory for the running session (no file persistence yet).
- To change admin PIN, edit:
  - `#define ADMIN_PIN "1234"` in `cafe_kiosk.c`.
