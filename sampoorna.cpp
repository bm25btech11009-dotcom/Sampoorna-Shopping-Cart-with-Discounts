#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <queue>
#include <utility>

using namespace std;

typedef long long Money;   // Money is stored in paise: Rs 10.50 = 1050 paise.

// One product in Sampoorna.
struct Product {
    int id;
    string name;
    string category;
    string unit;
    Money pricePaise;
    double stock;
    int gstRate;
    int discountPercent;
};

// One product and quantity in the cart.
struct CartItem {
    int productId;
    double quantity;
};

// One row of the bill.
struct BillLine {
    int productId;
    double quantity;
    Money amountPaise;
    Money productDiscountPaise;
    Money cartDiscountPaise;
    Money gstPaise;
};

// Complete bill for one shopping order.
struct Bill {
    vector<BillLine> lines;
    Money subtotalPaise;
    Money productDiscountPaise;
    Money cartDiscountPaise;
    Money cgstPaise;
    Money sgstPaise;
    Money totalPaise;
};

// An order waiting at the checkout counter.
struct Order {
    int orderId;
    Bill bill;
};

// Information needed to undo one cart change.
struct UndoAction {
    int productId;
    double oldQuantity;
};

// Print a money value stored in paise as rupees and paise.
// Example: 4450 becomes Rs 44.50.
void printMoney(Money paise) {
    cout << "Rs " << paise / 100 << ".";
    int remainingPaise = (int)(paise % 100);
    if (remainingPaise < 10) {
        cout << "0";
    }
    cout << remainingPaise;
}

// Print a quantity with two digits after the decimal point.
void printQuantity(double quantity) {
    int wholePart = (int)quantity;
    int decimalPart = (int)((quantity - wholePart) * 100 + 0.5);

    if (decimalPart == 100) {
        wholePart++;
        decimalPart = 0;
    }

    cout << wholePart << ".";
    if (decimalPart < 10) {
        cout << "0";
    }
    cout << decimalPart;
}

// Read an integer. If the input is invalid, ask again.
int readInt(string message) {
    int value;
    while (true) {
        cout << message;
        if (cin >> value) {
            return value;
        }
        cout << "Please enter a valid whole number.\n";
        cin.clear();
        cin.ignore(10000, '\n');
    }
}

// Read a positive quantity. Decimal quantities such as 0.5 are allowed.
double readPositiveDouble(string message) {
    double value;
    while (true) {
        cout << message;
        if (cin >> value && value > 0 && value < 1000000) {
            return value;
        }
        cout << "Please enter a quantity greater than zero.\n";
        cin.clear();
        cin.ignore(10000, '\n');
    }
}

// Read a price that can be zero or greater. Decimal rupees are accepted.
double readNonNegativeDouble(string message) {
    double value;
    while (true) {
        cout << message;
        if (cin >> value && value >= 0 && value < 1000000) {
            return value;
        }
        cout << "Please enter a valid non-negative price.\n";
        cin.clear();
        cin.ignore(10000, '\n');
    }
}

// ------------------------------------------------------------
// 1. HASH TABLE
// Product ID is the key. Its value is the index in products[].
// Collisions are handled with a linked list in each bucket.
// ------------------------------------------------------------
class ProductHashTable {
private:
    struct Node {
        int key;
        int value;
        Node *next;
    };

    vector<Node*> buckets;

    int hashFunction(int productId) {
        int bucketNumber = productId % 17;
        if (bucketNumber < 0) {
            bucketNumber = -bucketNumber;
        }
        return bucketNumber;
    }

public:
    ProductHashTable() {
        buckets.resize(17, NULL);
    }

    void insert(int productId, int productIndex) {
        int bucketNumber = hashFunction(productId);
        Node *current = buckets[bucketNumber];

        // Update the value if this product ID is already present.
        while (current != NULL) {
            if (current->key == productId) {
                current->value = productIndex;
                return;
            }
            current = current->next;
        }

        // Otherwise, insert a new node at the start of the linked list.
        Node *newNode = new Node;
        newNode->key = productId;
        newNode->value = productIndex;
        newNode->next = buckets[bucketNumber];
        buckets[bucketNumber] = newNode;
    }

    // Return the product index, or -1 if the ID is not present.
    int search(int productId) {
        int bucketNumber = hashFunction(productId);
        Node *current = buckets[bucketNumber];

        while (current != NULL) {
            if (current->key == productId) {
                return current->value;
            }
            current = current->next;
        }
        return -1;
    }

    // Free all linked-list nodes created by this hash table.
    void clear() {
        int i;
        for (i = 0; i < (int)buckets.size(); i++) {
            Node *current = buckets[i];
            while (current != NULL) {
                Node *nextNode = current->next;
                delete current;
                current = nextNode;
            }
            buckets[i] = NULL;
        }
    }

    ~ProductHashTable() {
        clear();
    }
};

// ------------------------------------------------------------
// 2. BINARY SEARCH TREE
// Smaller prices go to the left; equal or greater prices go right.
// ------------------------------------------------------------
class PriceBST {
private:
    struct Node {
        Money pricePaise;
        int productId;
        Node *left;
        Node *right;
    };

    Node *root;

    Node* insertNode(Node *current, Money price, int productId) {
        if (current == NULL) {
            Node *newNode = new Node;
            newNode->pricePaise = price;
            newNode->productId = productId;
            newNode->left = NULL;
            newNode->right = NULL;
            return newNode;
        }

        if (price < current->pricePaise) {
            current->left = insertNode(current->left, price, productId);
        } else {
            current->right = insertNode(current->right, price, productId);
        }
        return current;
    }

    // In-order traversal returns products in increasing price order.
    void searchRange(Node *current, Money low, Money high,
                     vector<int> &result) {
        if (current == NULL) {
            return;
        }

        if (current->pricePaise > low) {
            searchRange(current->left, low, high, result);
        }

        if (current->pricePaise >= low && current->pricePaise <= high) {
            result.push_back(current->productId);
        }

        if (current->pricePaise <= high) {
            searchRange(current->right, low, high, result);
        }
    }

    // Delete all nodes before the BST object is destroyed.
    void deleteTree(Node *current) {
        if (current == NULL) {
            return;
        }
        deleteTree(current->left);
        deleteTree(current->right);
        delete current;
    }

public:
    PriceBST() {
        root = NULL;
    }

    void insert(Money price, int productId) {
        root = insertNode(root, price, productId);
    }

    vector<int> findPriceRange(Money low, Money high) {
        vector<int> result;
        searchRange(root, low, high, result);
        return result;
    }

    ~PriceBST() {
        deleteTree(root);
    }
};

// ------------------------------------------------------------
// 3. LINKED LIST
// The customer's current shopping cart is stored in this list.
// ------------------------------------------------------------
class CartList {
private:
    struct Node {
        CartItem item;
        Node *next;
    };

    Node *head;
    Node *tail;

public:
    CartList() {
        head = NULL;
        tail = NULL;
    }

    double getQuantity(int productId) {
        Node *current = head;
        while (current != NULL) {
            if (current->item.productId == productId) {
                return current->item.quantity;
            }
            current = current->next;
        }
        return 0.0;
    }

    // Set the exact quantity. Zero means remove the product.
    void setQuantity(int productId, double newQuantity) {
        Node *current = head;
        Node *previous = NULL;

        // Find the product node.
        while (current != NULL && current->item.productId != productId) {
            previous = current;
            current = current->next;
        }

        // If the product is already in the cart, update or delete it.
        if (current != NULL) {
            if (newQuantity <= 0.000001) {
                if (previous == NULL) {
                    head = current->next;
                } else {
                    previous->next = current->next;
                }

                if (tail == current) {
                    tail = previous;
                }
                delete current;
            } else {
                current->item.quantity = newQuantity;
            }
            return;
        }

        // If it is not present, add a new node at the end.
        if (newQuantity > 0.000001) {
            Node *newNode = new Node;
            newNode->item.productId = productId;
            newNode->item.quantity = newQuantity;
            newNode->next = NULL;

            if (head == NULL) {
                head = newNode;
                tail = newNode;
            } else {
                tail->next = newNode;
                tail = newNode;
            }
        }
    }

    // Copy all cart items into a vector for bill calculation.
    vector<CartItem> getItems() {
        vector<CartItem> items;
        Node *current = head;
        while (current != NULL) {
            items.push_back(current->item);
            current = current->next;
        }
        return items;
    }

    bool isEmpty() {
        return head == NULL;
    }

    // Delete every node and reset the list.
    void clear() {
        Node *current = head;
        while (current != NULL) {
            Node *nextNode = current->next;
            delete current;
            current = nextNode;
        }
        head = NULL;
        tail = NULL;
    }

    ~CartList() {
        clear();
    }
};

// ------------------------------------------------------------
// MAIN SHOPPING SYSTEM
// Connects the product catalogue, cart, billing and checkout.
// ------------------------------------------------------------
class ShoppingSystem {
private:
    vector<Product> products;
    ProductHashTable productTable;
    map<string, vector<int> > categoryTable;
    PriceBST priceTree;

    CartList cart;
    stack<UndoAction> undoStack;
    queue<Order> checkoutQueue;

    int nextOrderId;

    // Add a product to the catalogue and update all search indexes.
    void addProduct(Product product) {
        int index = (int)products.size();
        products.push_back(product);

        productTable.insert(product.id, index);
        categoryTable[product.category].push_back(index);
        priceTree.insert(product.pricePaise, product.id);
    }

    // Load 50 sample products. Prices, GST, discounts and stock are demo data.
    void loadProducts() {
        addProduct({1001, "Bajaj Almond Drop Hair Oil 50ml", "Personal Care", "bottle", 4400, 30, 5, 5});
        addProduct({1002, "Banana", "Fruits & Vegetables", "kg", 5600, 18.50, 0, 0});
        addProduct({1003, "Tasty Mixture Special 200g", "Snacks", "packet", 7000, 25, 5, 10});
        addProduct({1004, "Ching's Veg Manchurian 20g", "Instant Food", "packet", 1000, 40, 5, 5});
        addProduct({1005, "Sunfeast Yippee Magic Masala Noodles 70g", "Instant Food", "packet", 1500, 40, 5, 5});
        addProduct({1006, "Priya Chilli Powder 100g", "Spices", "packet", 5000, 18, 5, 8});
        addProduct({1007, "Ginger", "Fruits & Vegetables", "kg", 18900, 8.00, 0, 0});
        addProduct({1008, "Garlic", "Fruits & Vegetables", "kg", 27900, 7.00, 0, 0});
        addProduct({1009, "Maggi 2-Minute Masala Noodles 4-pack", "Instant Food", "pack", 6000, 30, 5, 7});
        addProduct({1010, "Sona Masoori Rice 5kg", "Staples", "bag", 33500, 24, 0, 5});
        addProduct({1011, "Basmati Rice 1kg", "Staples", "packet", 12500, 28, 0, 5});
        addProduct({1012, "Toor Dal 1kg", "Staples", "packet", 16500, 20, 0, 5});
        addProduct({1013, "Moong Dal 1kg", "Staples", "packet", 13500, 18, 0, 5});
        addProduct({1014, "Chana Dal 1kg", "Staples", "packet", 9500, 22, 0, 0});
        addProduct({1015, "Urad Dal 1kg", "Staples", "packet", 14500, 16, 0, 5});
        addProduct({1016, "Wheat Atta 5kg", "Staples", "bag", 25000, 20, 0, 5});
        addProduct({1017, "Maida 1kg", "Staples", "packet", 4800, 25, 0, 0});
        addProduct({1018, "Sugar 1kg", "Staples", "packet", 4800, 30, 0, 0});
        addProduct({1019, "Iodized Salt 1kg", "Staples", "packet", 2200, 35, 0, 0});
        addProduct({1020, "Poha 500g", "Staples", "packet", 4200, 20, 0, 5});
        addProduct({1021, "Toned Milk 500ml", "Dairy", "packet", 3000, 40, 0, 0});
        addProduct({1022, "Curd 500g", "Dairy", "cup", 3500, 22, 0, 0});
        addProduct({1023, "Butter 100g", "Dairy", "pack", 5800, 15, 5, 5});
        addProduct({1024, "Paneer 200g", "Dairy", "pack", 9000, 12, 5, 5});
        addProduct({1025, "Ghee 500ml", "Dairy", "jar", 31000, 10, 5, 8});
        addProduct({1026, "Potato", "Fruits & Vegetables", "kg", 3200, 35, 0, 0});
        addProduct({1027, "Onion", "Fruits & Vegetables", "kg", 4000, 30, 0, 0});
        addProduct({1028, "Tomato", "Fruits & Vegetables", "kg", 3500, 25, 0, 0});
        addProduct({1029, "Green Chilli 100g", "Fruits & Vegetables", "packet", 1500, 15, 0, 0});
        addProduct({1030, "Lemon (4 pieces)", "Fruits & Vegetables", "pack", 2000, 18, 0, 0});
        addProduct({1031, "Sunflower Oil 1L", "Cooking Oil", "bottle", 14500, 20, 5, 5});
        addProduct({1032, "Groundnut Oil 1L", "Cooking Oil", "bottle", 17500, 14, 5, 5});
        addProduct({1033, "Tea Powder 250g", "Beverages", "packet", 14500, 18, 5, 5});
        addProduct({1034, "Instant Coffee 100g", "Beverages", "jar", 9500, 16, 5, 8});
        addProduct({1035, "Mango Juice 1L", "Beverages", "bottle", 11000, 15, 12, 10});
        addProduct({1036, "Drinking Water 1L", "Beverages", "bottle", 2000, 50, 0, 0});
        addProduct({1037, "Potato Chips 50g", "Snacks", "packet", 2000, 35, 12, 5});
        addProduct({1038, "Glucose Biscuits 120g", "Snacks", "packet", 3000, 28, 5, 0});
        addProduct({1039, "Marie Biscuits 200g", "Snacks", "packet", 3500, 24, 5, 5});
        addProduct({1040, "Roasted Peanuts 200g", "Snacks", "packet", 4500, 18, 5, 8});
        addProduct({1041, "Rusk 200g", "Snacks", "packet", 4500, 16, 5, 5});
        addProduct({1042, "Pasta 500g", "Instant Food", "packet", 7500, 18, 5, 5});
        addProduct({1043, "Tomato Ketchup 500g", "Instant Food", "bottle", 8500, 15, 12, 5});
        addProduct({1044, "Turmeric Powder 100g", "Spices", "packet", 3500, 20, 5, 0});
        addProduct({1045, "Coriander Powder 100g", "Spices", "packet", 4000, 18, 5, 5});
        addProduct({1046, "Cumin Seeds 100g", "Spices", "packet", 5500, 14, 5, 5});
        addProduct({1047, "Shampoo 180ml", "Personal Care", "bottle", 12000, 14, 18, 10});
        addProduct({1048, "Bath Soap 100g", "Personal Care", "bar", 4200, 30, 18, 5});
        addProduct({1049, "Toothpaste 100g", "Personal Care", "pack", 6500, 20, 18, 5});
        addProduct({1050, "Detergent Powder 1kg", "Household", "packet", 11000, 18, 18, 8});
    }

    // Search the hash table and return the product's vector index.
    int findProductIndex(int productId) {
        return productTable.search(productId);
    }

    // Get a pointer to a product using its ID.
    Product* getProduct(int productId) {
        int index = findProductIndex(productId);
        if (index == -1) {
            return NULL;
        }
        return &products[index];
    }

    // Display one product.
    void showProduct(Product product) {
        cout << product.id << " | " << product.name
             << " | " << product.category << " | ";
        printMoney(product.pricePaise);
        cout << " per " << product.unit << " | Stock: ";
        printQuantity(product.stock);
        cout << " | Discount: " << product.discountPercent << "%"
             << " | GST (demo): " << product.gstRate << "%\n";
    }

    // Calculate the bill. The calculations use paise to avoid rupee decimals.
    // Demo rule: extra 5% off when the basket after product offers is Rs 300+.
    Bill calculateBill(vector<CartItem> items) {
        Bill bill;
        bill.subtotalPaise = 0;
        bill.productDiscountPaise = 0;
        bill.cartDiscountPaise = 0;
        bill.cgstPaise = 0;
        bill.sgstPaise = 0;
        bill.totalPaise = 0;

        Money afterProductDiscountTotal = 0;
        int i;

        // First pass: calculate each product's amount and product discount.
        for (i = 0; i < (int)items.size(); i++) {
            Product *product = getProduct(items[i].productId);
            if (product == NULL) {
                continue;
            }

            BillLine line;
            line.productId = product->id;
            line.quantity = items[i].quantity;

            // Multiply unit price by quantity and round to the nearest paisa.
            line.amountPaise = (Money)(product->pricePaise * line.quantity + 0.5);
            line.productDiscountPaise =
                (line.amountPaise * product->discountPercent + 50) / 100;
            line.cartDiscountPaise = 0;
            line.gstPaise = 0;

            bill.subtotalPaise += line.amountPaise;
            bill.productDiscountPaise += line.productDiscountPaise;
            afterProductDiscountTotal += line.amountPaise - line.productDiscountPaise;
            bill.lines.push_back(line);
        }

        // Extra 5% cart discount when the eligible basket reaches Rs 300.
        if (afterProductDiscountTotal >= 30000) {
            bill.cartDiscountPaise = (afterProductDiscountTotal * 5 + 50) / 100;
        }

        // Second pass: share the cart discount across items before GST.
        Money allocatedDiscount = 0;
        Money totalGST = 0;

        for (i = 0; i < (int)bill.lines.size(); i++) {
            BillLine &line = bill.lines[i];
            Product *product = getProduct(line.productId);
            Money lineAfterProductDiscount = line.amountPaise - line.productDiscountPaise;

            if (afterProductDiscountTotal > 0) {
                if (i == (int)bill.lines.size() - 1) {
                    // Give the last line the remaining paise so all shares add up exactly.
                    line.cartDiscountPaise = bill.cartDiscountPaise - allocatedDiscount;
                } else {
                    line.cartDiscountPaise =
                        (bill.cartDiscountPaise * lineAfterProductDiscount) / afterProductDiscountTotal;
                    allocatedDiscount += line.cartDiscountPaise;
                }
            }

            Money taxableAmount = lineAfterProductDiscount - line.cartDiscountPaise;
            line.gstPaise = (taxableAmount * product->gstRate + 50) / 100;
            totalGST += line.gstPaise;
        }

        // Demonstration split: half CGST and half SGST.
        bill.cgstPaise = totalGST / 2;
        bill.sgstPaise = totalGST - bill.cgstPaise;
        bill.totalPaise = afterProductDiscountTotal - bill.cartDiscountPaise + totalGST;

        return bill;
    }

    // Print a complete bill or receipt.
    void printBill(Bill bill, int orderId) {
        cout << "\n================ SAMPoorna BILL ================\n";
        if (orderId != 0) {
            cout << "Order ID: " << orderId << "\n";
        }

        int i;
        for (i = 0; i < (int)bill.lines.size(); i++) {
            BillLine line = bill.lines[i];
            Product *product = getProduct(line.productId);
            if (product == NULL) {
                continue;
            }

            cout << product->name << " | Qty: ";
            printQuantity(line.quantity);
            cout << " " << product->unit << " | Rate: ";
            printMoney(product->pricePaise);
            cout << " | Amount: ";
            printMoney(line.amountPaise);
            cout << "\n";

            if (line.productDiscountPaise > 0) {
                cout << "  Product discount: -";
                printMoney(line.productDiscountPaise);
                cout << "\n";
            }
            if (line.cartDiscountPaise > 0) {
                cout << "  Share of cart discount: -";
                printMoney(line.cartDiscountPaise);
                cout << "\n";
            }
            cout << "  GST on this item (demo): ";
            printMoney(line.gstPaise);
            cout << "\n";
        }

        cout << "-------------------------------------------------\n";
        cout << "Subtotal: ";
        printMoney(bill.subtotalPaise);
        cout << "\nProduct discounts: -";
        printMoney(bill.productDiscountPaise);
        cout << "\nCart discount: -";
        printMoney(bill.cartDiscountPaise);
        cout << "\nCGST (demo): ";
        printMoney(bill.cgstPaise);
        cout << "\nSGST (demo): ";
        printMoney(bill.sgstPaise);
        cout << "\nGRAND TOTAL: ";
        printMoney(bill.totalPaise);
        cout << "\n=================================================\n";
    }

    // Print the current cart and its calculated bill.
    void viewCart() {
        vector<CartItem> items = cart.getItems();
        if (items.size() == 0) {
            cout << "Your cart is empty.\n";
            return;
        }

        cout << "\n---------------- YOUR CART ----------------\n";
        int i;
        for (i = 0; i < (int)items.size(); i++) {
            Product *product = getProduct(items[i].productId);
            if (product != NULL) {
                cout << product->id << " | " << product->name << " | Qty: ";
                printQuantity(items[i].quantity);
                cout << " " << product->unit << " | Amount: ";
                printMoney((Money)(product->pricePaise * items[i].quantity + 0.5));
                cout << "\n";
            }
        }
        printBill(calculateBill(items), 0);
    }

    // Change cart quantity and save the old quantity in the undo stack.
    void changeCartQuantity(int productId, double newQuantity) {
        Product *product = getProduct(productId);
        if (product == NULL) {
            cout << "Product not found.\n";
            return;
        }

        double oldQuantity = cart.getQuantity(productId);

        if (newQuantity > product->stock + 0.000001) {
            cout << "Not enough stock. Available quantity: ";
            printQuantity(product->stock);
            cout << " " << product->unit << "\n";
            return;
        }

        if (newQuantity < 0) {
            cout << "Quantity cannot be negative.\n";
            return;
        }

        if (newQuantity == oldQuantity) {
            cout << "Cart quantity has not changed.\n";
            return;
        }

        cart.setQuantity(productId, newQuantity);

        UndoAction action;
        action.productId = productId;
        action.oldQuantity = oldQuantity;
        undoStack.push(action);

        if (newQuantity <= 0.000001) {
            cout << product->name << " removed from cart.\n";
        } else if (oldQuantity <= 0.000001) {
            cout << product->name << " added to cart.\n";
        } else {
            cout << "Quantity updated for " << product->name << ".\n";
        }
    }

    // Menu option 1.
    void viewAllProducts() {
        cout << "\n---------------- ALL PRODUCTS ----------------\n";
        int i;
        for (i = 0; i < (int)products.size(); i++) {
            showProduct(products[i]);
        }
    }

    // Menu option 2: exact search using the hash table.
    void searchProductById() {
        int id = readInt("Enter product ID: ");
        Product *product = getProduct(id);
        if (product == NULL) {
            cout << "Product not found.\n";
        } else {
            showProduct(*product);
        }
    }

    // Menu option 3: category search using the map.
    void browseByCategory() {
        vector<string> categoryNames;
        map<string, vector<int> >::iterator it;
        int number = 1;

        cout << "\nCategories:\n";
        for (it = categoryTable.begin(); it != categoryTable.end(); it++) {
            cout << number << ". " << it->first << "\n";
            categoryNames.push_back(it->first);
            number++;
        }

        int choice = readInt("Choose category number: ");
        if (choice < 1 || choice > (int)categoryNames.size()) {
            cout << "Invalid category choice.\n";
            return;
        }

        string selectedCategory = categoryNames[choice - 1];
        cout << "\nProducts in " << selectedCategory << ":\n";
        vector<int> indexes = categoryTable[selectedCategory];
        int i;
        for (i = 0; i < (int)indexes.size(); i++) {
            showProduct(products[indexes[i]]);
        }
    }

    // Menu option 4: price range search using the BST.
    void searchByPriceRange() {
        double lowRupees = readNonNegativeDouble("Enter minimum price in rupees: ");
        double highRupees = readNonNegativeDouble("Enter maximum price in rupees: ");

        if (highRupees < lowRupees) {
            cout << "Maximum price cannot be less than minimum price.\n";
            return;
        }

        Money lowPaise = (Money)(lowRupees * 100 + 0.5);
        Money highPaise = (Money)(highRupees * 100 + 0.5);
        vector<int> matches = priceTree.findPriceRange(lowPaise, highPaise);

        if (matches.size() == 0) {
            cout << "No products found in this price range.\n";
            return;
        }

        cout << "\nProducts in this price range (low to high):\n";
        int i;
        for (i = 0; i < (int)matches.size(); i++) {
            Product *product = getProduct(matches[i]);
            if (product != NULL) {
                showProduct(*product);
            }
        }
    }

    // Menu option 5: use a priority queue (max heap) to display best deals.
    void showOffers() {
        // The heap stores (priority score, product's vector position).
        // A higher discount gets a higher score. If discounts are equal,
        // the cheaper product gets a higher score.
        priority_queue<pair<Money, int> > offers;

        Money priceRange = 1;
        int i;
        for (i = 0; i < (int)products.size(); i++) {
            if (products[i].pricePaise > priceRange) {
                priceRange = products[i].pricePaise;
            }
        }
        priceRange++; // Must be greater than the most expensive product price.

        for (i = 0; i < (int)products.size(); i++) {
            if (products[i].discountPercent > 0) {
                Money score = products[i].discountPercent * priceRange
                              - products[i].pricePaise;
                offers.push(make_pair(score, i));
            }
        }

        if (offers.empty()) {
            cout << "No offers are available.\n";
            return;
        }

        cout << "\n---------------- AVAILABLE OFFERS ----------------\n";
        while (!offers.empty()) {
            pair<Money, int> offer = offers.top();
            offers.pop();

            int index = offer.second;
            int discount = products[index].discountPercent;
            cout << discount << "% off | " << products[index].name << " | Save ";
            printMoney((products[index].pricePaise * discount + 50) / 100);
            cout << " per " << products[index].unit << "\n";
        }
    }

    // Menu option 6.
    void addToCart() {
        int id = readInt("Enter product ID: ");
        Product *product = getProduct(id);

        if (product == NULL) {
            cout << "Product not found.\n";
            return;
        }

        cout << product->name << " costs ";
        printMoney(product->pricePaise);
        cout << " per " << product->unit << ". Available stock: ";
        printQuantity(product->stock);
        cout << "\n";

        double quantity = readPositiveDouble("Enter quantity to add (example: 0.5): ");
        double newQuantity = cart.getQuantity(id) + quantity;
        changeCartQuantity(id, newQuantity);
    }

    // Menu option 7: remove the full product entry from the cart.
    void removeFromCart() {
        int id = readInt("Enter product ID to remove: ");
        if (cart.getQuantity(id) <= 0.000001) {
            cout << "This product is not in your cart.\n";
            return;
        }
        changeCartQuantity(id, 0.0);
    }

    // Menu option 9: restore the previous cart quantity.
    void undoLastChange() {
        if (undoStack.empty()) {
            cout << "There is no cart change to undo.\n";
            return;
        }

        UndoAction action = undoStack.top();
        undoStack.pop();
        cart.setQuantity(action.productId, action.oldQuantity);

        cout << "Last cart change undone. Previous quantity restored to ";
        printQuantity(action.oldQuantity);
        cout << ".\n";
    }

    // Menu option 10: reserve stock, save the order, and empty the cart.
    void placeOrder() {
        vector<CartItem> items = cart.getItems();

        if (items.size() == 0) {
            cout << "Your cart is empty. Add products first.\n";
            return;
        }

        // Check every item's stock before changing any stock value.
        int i;
        for (i = 0; i < (int)items.size(); i++) {
            Product *product = getProduct(items[i].productId);
            if (product == NULL || items[i].quantity > product->stock + 0.000001) {
                cout << "Not enough stock for product ID " << items[i].productId << ". Order not placed.\n";
                return;
            }
        }

        Order newOrder;
        newOrder.orderId = nextOrderId;
        nextOrderId++;
        newOrder.bill = calculateBill(items);

        // Reserve stock only after all products have passed validation.
        for (i = 0; i < (int)items.size(); i++) {
            Product *product = getProduct(items[i].productId);
            product->stock = product->stock - items[i].quantity;
            if (product->stock < 0.000001) {
                product->stock = 0;
            }
        }

        checkoutQueue.push(newOrder);
        cart.clear();

        // Old undo operations must not change the next shopping session.
        while (!undoStack.empty()) {
            undoStack.pop();
        }

        cout << "Order #" << newOrder.orderId << " placed in the checkout queue.\n";
        cout << "Amount to pay: ";
        printMoney(newOrder.bill.totalPaise);
        cout << "\nYour cart has been cleared.\n";
    }

    // Menu option 11: process the oldest order first (FIFO).
    void processNextCheckout() {
        if (checkoutQueue.empty()) {
            cout << "No orders are waiting for checkout.\n";
            return;
        }

        Order nextOrder = checkoutQueue.front();
        checkoutQueue.pop();

        cout << "Processing the oldest order first (FIFO).\n";
        printBill(nextOrder.bill, nextOrder.orderId);
    }

    void showMenu() {
        cout << "\n============= SAMPOORNA GROCERY STORE =============\n";
        cout << "1. View all products\n";
        cout << "2. Search product by ID\n";
        cout << "3. Browse by category\n";
        cout << "4. Search by price range\n";
        cout << "5. View available offers\n";
        cout << "6. Add product to cart\n";
        cout << "7. Remove product from cart\n";
        cout << "8. View cart and bill\n";
        cout << "9. Undo last cart change\n";
        cout << "10. Place order\n";
        cout << "11. Process next checkout\n";
        cout << "0. Exit\n";
        cout << "===================================================\n";
    }

public:
    ShoppingSystem() {
        nextOrderId = 5001;
        loadProducts();
    }

    void run() {
        cout << "Welcome to Sampoorna Grocery Store!\n";
        cout << "Prices, offers, stock and GST rates are sample project data.\n";

        int choice;
        do {
            showMenu();
            choice = readInt("Enter your choice: ");

            switch (choice) {
                case 1:
                    viewAllProducts();
                    break;
                case 2:
                    searchProductById();
                    break;
                case 3:
                    browseByCategory();
                    break;
                case 4:
                    searchByPriceRange();
                    break;
                case 5:
                    showOffers();
                    break;
                case 6:
                    addToCart();
                    break;
                case 7:
                    removeFromCart();
                    break;
                case 8:
                    viewCart();
                    break;
                case 9:
                    undoLastChange();
                    break;
                case 10:
                    placeOrder();
                    break;
                case 11:
                    processNextCheckout();
                    break;
                case 0:
                    cout << "Thank you for shopping at Sampoorna!\n";
                    break;
                default:
                    cout << "Please choose a number from 0 to 11.\n";
            }
        } while (choice != 0);
    }
};

int main() {
    ShoppingSystem shop;
    shop.run();
    return 0;
}
