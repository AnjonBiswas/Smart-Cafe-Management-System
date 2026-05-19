# Smart Cafe Management System
**A project taken in my first semester SPL course**
**Command-line cafe management and vending kiosk application written in C (`stdio.h`, `stdlib.h`, `string.h`, `time.h`).**
## Core Features
- Role-based flow: `Customer` and `Admin`
- Admin PIN protection (`1234`, 3 attempts)
- Live inventory with stock status (`OK`, `LOW`, `OUT`)
- Cart operations: add, remove, view, clear
- Automated billing: subtotal, 5% VAT, grand total
- Payment handling with change calculation
- Itemized receipt with timestamp and 5-digit transaction ID
- Session sales report: transactions, items sold, revenue, VAT
- Input validation for menu actions, IDs, quantities, and payment values
- ANSI color output for clearer terminal UX

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

### MSVC
```bat
cl /W4 /EHsc cafe_kiosk.c
cafe_kiosk.exe
```

## Project Files
- `cafe_kiosk.c` - application source code
- `README.md` - project documentation

## Notes
- Inventory and sales data are in-memory only (reset on restart).
- Change admin PIN in `cafe_kiosk.c`:
  - `#define ADMIN_PIN "1234"`
