
#include <bits/stdc++.h>

using namespace std;
// One simulated preparation minute takes 10 real seconds for the demo.
constexpr int SECONDS_PER_PREP_MINUTE = 10;

int readIntInRange(const string& prompt, int low, int high) {
    while (true) {
        cout << prompt << flush;
        int value;
        if (cin >> value) {
            if (value >= low && value <= high) return value;
            cout << "Please enter a number between " << low << " and " << high << ".\n";
        } else {
            if (cin.eof()) return low;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a whole number.\n";
        }
    }
}

double readPositivePrice(const string& prompt) {
    while (true) {
        cout << prompt << flush;
        double value;
        if (cin >> value) {
            if (isfinite(value) && value > 0.0) return value;
            cout << "Price must be greater than zero.\n";
        } else {
            if (cin.eof()) return 0.0;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a valid price.\n";
        }
    }
}

string readWord(const string& prompt) {
    cout << prompt << flush;
    string value;
    if (!(cin >> value)) {
        if (!cin.eof()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        return "";
    }
    return value;
}

tm localTime(time_t value) {
    tm result{};
#ifdef _WIN32
    localtime_s(&result, &value);
#else
    localtime_r(&value, &result);
#endif
    return result;
}

string formatDateTime(time_t value) {
    if (value <= 0) return "-";
    tm parts = localTime(value);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%d-%m-%Y %I:%M:%S %p", &parts);
    return buffer;
}

string dateKey(time_t value) {
    if (value <= 0) return "";
    tm parts = localTime(value);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &parts);
    return buffer;
}

string todayDateKey() {
    return dateKey(time(nullptr));
}

string dateColumn(time_t value) {
    if (value <= 0) return "-";
    tm parts = localTime(value);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &parts);
    return buffer;
}

string timeColumn(time_t value) {
    if (value <= 0) return "-";
    tm parts = localTime(value);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &parts);
    return buffer;
}

time_t parseDateTimeColumns(const string& date, const string& clock) {
    if (date == "-" || clock == "-" || date.empty() || clock.empty()) return 0;
    tm parts{};
    istringstream input(date + " " + clock);
    input >> get_time(&parts, "%Y-%m-%d %H:%M:%S");
    if (input.fail()) return 0;
    parts.tm_isdst = -1;
    return mktime(&parts);
}

string trimField(string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

vector<string> splitColumns(const string& line) {
    vector<string> fields;
    string field;
    istringstream input(line);
    while (getline(input, field, '|')) fields.push_back(trimField(field));
    return fields;
}

bool parseIntField(const string& text, int& value) {
    try {
        size_t used = 0;
        value = stoi(text, &used);
        return used == text.size();
    } catch (...) {
        return false;
    }
}

string billLabel(int billNumber) {
    return "BILL-" + to_string(billNumber);
}

string rupees(double amount) {
    ostringstream output;
    output << "₹" << fixed << setprecision(2) << amount;
    return output.str();
}

class MenuItem {
private:
    int id_;
    string category_;
    string name_;
    double price_;
    int prepMinutes_;

public:
    MenuItem(int id = 0, string category = "", string name = "",
             double price = 0.0, int prepMinutes = 1)
        : id_(id), category_(move(category)), name_(move(name)),
          price_(price), prepMinutes_(prepMinutes) {}

    int id() const { return id_; }
    const string& category() const { return category_; }
    const string& name() const { return name_; }
    double price() const { return price_; }
    int prepMinutes() const { return prepMinutes_; }
    void setPrice(double price) { price_ = price; }
};

struct OrderLine {
    int itemId = 0;
    string category;
    string name;
    double unitPrice = 0.0;
    // Keep the price charged at order time, even if the menu changes later.
    int quantity = 0;
    int prepMinutes = 1;

    double lineTotal() const { return unitPrice * quantity; }
};

enum class OrderStatus { PENDING = 0, PREPARING = 1, COMPLETED = 2, CANCELLED = 3 };

string statusText(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "PENDING";
        case OrderStatus::PREPARING: return "PREPARING";
        case OrderStatus::COMPLETED: return "COMPLETED";
        case OrderStatus::CANCELLED: return "CANCELLED";
    }
    return "UNKNOWN";
}

class Order {
private:
    int tokenId_ = 0;
    int billNumber_ = 0;
    vector<OrderLine> items_;
    bool priority_ = false;
    OrderStatus status_ = OrderStatus::PENDING;
    int stationNumber_ = 0;
    time_t orderTime_ = 0;
    time_t preparationStart_ = 0;
    time_t expectedReadyTime_ = 0;
    time_t completedTime_ = 0;
    int prepDurationMinutes_ = 0;
    double subtotal_ = 0.0;
    double priorityCharge_ = 0.0;
    double total_ = 0.0;

    friend class CanteenSystem;

public:
    Order() = default;

    Order(const vector<OrderLine>& items, bool priority)
        : items_(items), priority_(priority) {
        calculateBill();
        calculatePreparationDuration();
    }

    void calculateBill() {
        subtotal_ = 0.0;
        for (const auto& item : items_) subtotal_ += item.lineTotal();
        priorityCharge_ = priority_ ? subtotal_ * 0.10 : 0.0;
        total_ = subtotal_ + priorityCharge_;
    }

    void calculatePreparationDuration() {
        prepDurationMinutes_ = 0;
        for (const auto& item : items_) prepDurationMinutes_ += item.prepMinutes * item.quantity;
        if (prepDurationMinutes_ < 1 && !items_.empty()) prepDurationMinutes_ = 1;
    }

    int tokenId() const { return tokenId_; }
    int billNumber() const { return billNumber_; }
    const vector<OrderLine>& items() const { return items_; }
    bool isPriority() const { return priority_; }
    OrderStatus status() const { return status_; }
    int stationNumber() const { return stationNumber_; }
    time_t orderTime() const { return orderTime_; }
    time_t preparationStart() const { return preparationStart_; }
    time_t expectedReadyTime() const { return expectedReadyTime_; }
    time_t completedTime() const { return completedTime_; }
    int prepDurationMinutes() const { return prepDurationMinutes_; }
    double subtotal() const { return subtotal_; }
    double priorityCharge() const { return priorityCharge_; }
    double total() const { return total_; }
};

class Bill {
public:
    static void print(const Order& order, bool preview = false) {
        cout << "\n===============================================================\n";
        cout << (preview ? "                    BILL PREVIEW\n" : "                 PICT COLLEGE CANTEEN\n");
        cout << "===============================================================\n";
        if (preview) {
            cout << "Order ID : assigned after confirmation\n";
            cout << "Bill No. : assigned after confirmation\n";
        } else {
            cout << "Order ID / Token : " << order.tokenId() << "\n";
            cout << "Bill Number      : " << billLabel(order.billNumber()) << "\n";
            cout << "Order Time       : " << formatDateTime(order.orderTime()) << "\n";
            cout << "Order Status     : " << statusText(order.status()) << "\n";
        }
        cout << "Order Type       : " << (order.isPriority() ? "PRIORITY" : "NORMAL") << "\n";
        cout << "----------------------------------------------------------------\n";
        cout << left << setw(5) << "ID" << setw(27) << "Item"
             << right << setw(7) << "Qty" << setw(12) << "Rate" << setw(12) << "Amount" << "\n";
        cout << "----------------------------------------------------------------\n";
        for (const auto& item : order.items()) {
            cout << left << setw(5) << item.itemId << setw(27) << item.name.substr(0, 26)
                 << right << setw(7) << item.quantity
                 << setw(12) << fixed << setprecision(2) << item.unitPrice
                 << setw(12) << item.lineTotal() << "\n";
        }
        cout << "----------------------------------------------------------------\n";
        cout << right << setw(48) << "Food Subtotal: " << rupees(order.subtotal()) << "\n";
        cout << right << setw(48) << (order.isPriority() ? "Priority charge (10%): " : "Priority charge: ")
             << rupees(order.priorityCharge()) << "\n";
        cout << right << setw(48) << "TOTAL PAYABLE: " << rupees(order.total()) << "\n";
        cout << "===============================================================\n";
        if (order.status() == OrderStatus::CANCELLED) {
            cout << "Note: This bill is retained for order history. Refund/payment handling is not modelled.\n";
        } else if (order.isPriority()) {
            cout << "Priority service: charged at 10% of the food subtotal.\n";
        }
        if (!preview) cout << "Keep your Order ID or Bill Number to check status or reprint this bill.\n";
        cout << "===============================================================\n\n";
    }
};

class LinkedOrderQueue {
private:
    struct Node {
        int token;
        Node* next;
        explicit Node(int value) : token(value), next(nullptr) {}
    };
    Node* front_ = nullptr;
    Node* rear_ = nullptr;
    size_t size_ = 0;

public:
    LinkedOrderQueue() = default;
    LinkedOrderQueue(const LinkedOrderQueue&) = delete;
    LinkedOrderQueue& operator=(const LinkedOrderQueue&) = delete;

    ~LinkedOrderQueue() { clear(); }

    bool empty() const { return front_ == nullptr; }
    size_t size() const { return size_; }

    void enqueue(int token) {
        Node* node = new Node(token);
        if (rear_ == nullptr) front_ = rear_ = node;
        else {
            rear_->next = node;
            rear_ = node;
        }
        ++size_;
    }

    int dequeue() {
        if (front_ == nullptr) return 0;
        Node* oldFront = front_;
        int token = oldFront->token;
        front_ = front_->next;
        if (front_ == nullptr) rear_ = nullptr;
        delete oldFront;
        --size_;
        return token;
    }

    bool removeToken(int token) {
        Node* current = front_;
        Node* previous = nullptr;
        while (current != nullptr) {
            if (current->token == token) {
                if (previous == nullptr) front_ = current->next;
                else previous->next = current->next;
                if (current == rear_) rear_ = previous;
                delete current;
                --size_;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

    vector<int> snapshot() const {
        vector<int> result;
        for (Node* current = front_; current != nullptr; current = current->next)
            result.push_back(current->token);
        return result;
    }

    void clear() {
        while (front_ != nullptr) {
            Node* next = front_->next;
            delete front_;
            front_ = next;
        }
        rear_ = nullptr;
        size_ = 0;
    }
};

class OrderHashTable {
private:
    struct Node {
        int key;
        Order* order;
        Node* next;
        Node(int k, Order* value, Node* nextNode) : key(k), order(value), next(nextNode) {}
    };
    vector<Node*> buckets_;

    size_t bucketIndex(int key) const {
        return static_cast<size_t>(key >= 0 ? key : -key) % buckets_.size();
    }

public:
    explicit OrderHashTable(size_t bucketCount = 211) : buckets_(bucketCount, nullptr) {}
    OrderHashTable(const OrderHashTable&) = delete;
    OrderHashTable& operator=(const OrderHashTable&) = delete;
    ~OrderHashTable() { clear(); }

    void insert(int key, Order* order) {
        size_t index = bucketIndex(key);
        for (Node* node = buckets_[index]; node != nullptr; node = node->next) {
            if (node->key == key) {
                node->order = order;
                return;
            }
        }
        buckets_[index] = new Node(key, order, buckets_[index]);
    }

    Order* find(int key) const {
        size_t index = bucketIndex(key);
        for (Node* node = buckets_[index]; node != nullptr; node = node->next)
            if (node->key == key) return node->order;
        return nullptr;
    }

    void clear() {
        for (Node*& bucket : buckets_) {
            while (bucket != nullptr) {
                Node* next = bucket->next;
                delete bucket;
                bucket = next;
            }
        }
    }
};

class ServingStation {
private:
    int number_;
    int activeToken_ = 0;
public:
    explicit ServingStation(int number = 0) : number_(number) {}
    int number() const { return number_; }
    int activeToken() const { return activeToken_; }
    bool isFree() const { return activeToken_ == 0; }
    void assign(int token) { activeToken_ = token; }
    void release() { activeToken_ = 0; }
};

struct StationSnapshot {
    int number = 0;
    int token = 0;
    time_t expectedReady = 0;
};

struct QueueSnapshot {
    vector<int> priorityTokens;
    vector<int> normalTokens;
};

class CanteenSystem {
private:
    vector<MenuItem> menu_;
    vector<unique_ptr<Order>> orders_;
    OrderHashTable orderIndex_;
    LinkedOrderQueue priorityQueue_;
    LinkedOrderQueue normalQueue_;
    array<ServingStation, 4> stations_{{ServingStation(1), ServingStation(2),
                                       ServingStation(3), ServingStation(4)}};
    int lastToken_ = 100;
    int lastBill_ = 5000;
    string businessDate_ = todayDateKey();
    mutable mutex mutex_;
    bool running_ = true;
    thread timerThread_;

    const string ordersFile_ = "orders.txt";
    const string menuPricesFile_ = "menu_prices.txt";

    void initializeMenu() {
        menu_ = {
            {1, "Breakfast", "Poha", 30, 5},
            {2, "Breakfast", "Vada Pav", 20, 4},
            {3, "Breakfast", "Misal Pav", 55, 8},
            {4, "Breakfast", "Veg Sandwich", 45, 6},
            {5, "Snacks", "Samosa", 20, 4},
            {6, "Snacks", "Kachori", 25, 4},
            {7, "Snacks", "Plain Maggi", 40, 6},
            {8, "Snacks", "French Fries", 60, 8},
            {9, "Snacks", "Veg Burger", 65, 8},
            {10, "Snacks", "Pav Bhaji", 70, 9},
            {11, "Meals", "Mini Veg Thali", 75, 10},
            {12, "Meals", "Full Veg Thali", 110, 12},
            {13, "Meals", "Rajma Rice", 80, 9},
            {14, "Meals", "Veg Biryani", 90, 11},
            {15, "Meals", "Masala Dosa", 70, 9},
            {16, "Drinks", "Water Bottle", 20, 1},
            {17, "Drinks", "Tea", 15, 3},
            {18, "Drinks", "Coffee", 25, 4},
            {19, "Drinks", "Cold Coffee", 60, 5},
            {20, "Desserts", "Gulab Jamun (2 pcs)", 35, 3}
        };
    }

    void loadMenuPrices() {
        ifstream input(menuPricesFile_);
        string line;
        while (getline(input, line)) {
            vector<string> fields = splitColumns(line);
            if (fields.size() < 5) continue;
            int id = 0, prep = 0;
            double price = 0.0;
            if (!parseIntField(fields[0], id) || !parseIntField(fields[4], prep)) continue;
            try {
                price = stod(fields[3]);
            } catch (...) {
                continue;
            }
            for (auto& item : menu_) {
                if (item.id() == id && price > 0.0) {
                    item.setPrice(price);
                    break;
                }
            }
        }
    }

    void saveMenuPricesLocked() const {
        ofstream output(menuPricesFile_, ios::trunc);
        if (!output) {
            cerr << "Warning: could not save menu prices to " << menuPricesFile_ << ".\n";
            return;
        }
        output << "CANTEEN MENU PRICE LIST\n";
        output << "LAST SAVED | " << dateColumn(time(nullptr)) << " | " << timeColumn(time(nullptr)) << '\n';
        output << "Current menu prices in rupees. Existing bills keep the price charged at order time.\n\n";
        output << left << setw(4) << "ID" << " | " << setw(12) << "CATEGORY"
               << " | " << setw(26) << "ITEM NAME" << " | " << right << setw(10) << "PRICE (₹)"
               << " | " << setw(10) << "PREP (min)" << '\n';
        output << string(78, '-') << '\n' << fixed << setprecision(2);
        for (const auto& item : menu_) {
            output << left << setw(4) << item.id() << " | " << setw(12) << item.category()
                   << " | " << setw(26) << item.name() << " | " << right << setw(10) << item.price()
                   << " | " << setw(10) << item.prepMinutes() << '\n';
        }
    }

    bool archiveCurrentOrdersFile(const string& fileDate, bool legacy = false) {
        namespace fs = std::filesystem;
        if (!fs::exists(ordersFile_)) return true;

        string baseName = legacy || fileDate.empty()
            ? "orders_legacy_backup.txt"
            : "orders_" + fileDate + ".txt";
        fs::path target(baseName);
        int suffix = 2;
        while (fs::exists(target)) {
            const string stem = legacy || fileDate.empty()
                ? "orders_legacy_backup"
                : "orders_" + fileDate;
            target = stem + "_" + to_string(suffix++) + ".txt";
        }

        error_code error;
        fs::rename(ordersFile_, target, error);
        if (error) {
            error.clear();
            fs::copy_file(ordersFile_, target, fs::copy_options::none, error);
            if (!error) {
                fs::remove(ordersFile_, error);
            }
        }
        if (error) {
            cerr << "Could not archive " << ordersFile_ << ": " << error.message() << '\n';
            return false;
        }
        cout << "Archived previous order register as " << target.string() << ".\n";
        return true;
    }

    void clearDayRecordsLocked() {
        orderIndex_.clear();
        priorityQueue_.clear();
        normalQueue_.clear();
        for (auto& station : stations_) station.release();
        orders_.clear();
        lastToken_ = 100;
    }

    void loadArchivedBillCounter() {
        namespace fs = std::filesystem;
        error_code error;
        for (const auto& entry : fs::directory_iterator(fs::current_path(), error)) {
            if (error || !entry.is_regular_file()) continue;
            const string filename = entry.path().filename().string();
            if (filename.rfind("orders_", 0) != 0 || entry.path().extension() != ".txt") continue;
            ifstream input(entry.path());
            string line;
            while (getline(input, line)) {
                vector<string> fields = splitColumns(line);
                if (fields.size() < 3 || fields[0] != "ORDER") continue;
                int token = 0;
                if (!parseIntField(fields[1], token)) continue;
                string billText = fields[2];
                if (billText.rfind("BILL-", 0) == 0) billText = billText.substr(5);
                int billNumber = 0;
                if (parseIntField(billText, billNumber)) lastBill_ = max(lastBill_, billNumber);
            }
        }
    }

    bool rolloverIfNeededLocked(time_t now) {
        const string newDate = dateKey(now);
        if (newDate == businessDate_) return true;

        saveOrdersLocked();
        if (!archiveCurrentOrdersFile(businessDate_)) return false;
        clearDayRecordsLocked();
        businessDate_ = newDate;
        saveOrdersLocked();
        return true;
    }

    void loadOrders() {
        namespace fs = std::filesystem;
        if (!fs::exists(ordersFile_)) return;

        vector<string> lines;
        {
            ifstream input(ordersFile_);
            string line;
            while (getline(input, line)) lines.push_back(line);
        }

        string fileDate;
        for (const string& line : lines) {
            vector<string> fields = splitColumns(line);
            if (fields.size() >= 2 && fields[0] == "BUSINESS DATE") {
                fileDate = fields[1];
                break;
            }
        }

        if (fileDate.empty()) {
            archiveCurrentOrdersFile("", true);
            return;
        }
        if (fileDate != businessDate_) {
            archiveCurrentOrdersFile(fileDate);
            return;
        }

        optional<Order> currentOrder;
        auto commitOrder = [&]() {
            if (!currentOrder) return;
            Order loaded = *currentOrder;
            currentOrder.reset();

            const time_t now = time(nullptr);
            if (loaded.status_ == OrderStatus::PREPARING && loaded.expectedReadyTime_ > 0
                && now >= loaded.expectedReadyTime_) {
                loaded.status_ = OrderStatus::COMPLETED;
                loaded.completedTime_ = now;
                loaded.stationNumber_ = 0;
            }

            if (loaded.status_ == OrderStatus::PREPARING) {
                const int index = loaded.stationNumber_ - 1;
                if (index < 0 || index >= static_cast<int>(stations_.size()) || !stations_[index].isFree()) {
                    loaded.status_ = OrderStatus::PENDING;
                    loaded.stationNumber_ = 0;
                    loaded.preparationStart_ = 0;
                    loaded.expectedReadyTime_ = 0;
                } else {
                    stations_[index].assign(loaded.tokenId_);
                }
            }

            lastToken_ = max(lastToken_, loaded.tokenId_);
            lastBill_ = max(lastBill_, loaded.billNumber_);
            const int token = loaded.tokenId_;
            auto record = make_unique<Order>(loaded);
            Order* recordPointer = record.get();
            orders_.push_back(move(record));
            orderIndex_.insert(token, recordPointer);
            if (recordPointer->status_ == OrderStatus::PENDING) {
                if (recordPointer->priority_) priorityQueue_.enqueue(token);
                else normalQueue_.enqueue(token);
            }
        };

        for (const string& line : lines) {
            vector<string> fields = splitColumns(line);
            if (fields.empty()) continue;

            if (fields[0] == "ORDER" && fields.size() >= 19) {
                commitOrder();
                int token = 0, station = 0, prepDuration = 0, itemCount = 0;
                int billNumber = 0;
                if (!parseIntField(fields[1], token) || itemCount < 0 || itemCount > 500) continue;
                string billText = fields[2];
                if (billText.rfind("BILL-", 0) == 0) billText = billText.substr(5);
                if (!parseIntField(billText, billNumber)
                    || !parseIntField(fields[7], station)
                    || !parseIntField(fields[17], prepDuration)
                    || !parseIntField(fields[18], itemCount)) continue;

                Order loaded;
                loaded.tokenId_ = token;
                loaded.billNumber_ = billNumber;
                loaded.priority_ = fields[5] == "PRIORITY";
                if (fields[6] == "PENDING") loaded.status_ = OrderStatus::PENDING;
                else if (fields[6] == "PREPARING") loaded.status_ = OrderStatus::PREPARING;
                else if (fields[6] == "COMPLETED") loaded.status_ = OrderStatus::COMPLETED;
                else if (fields[6] == "CANCELLED") loaded.status_ = OrderStatus::CANCELLED;
                else continue;
                loaded.stationNumber_ = station;
                loaded.orderTime_ = parseDateTimeColumns(fields[3], fields[4]);
                loaded.preparationStart_ = parseDateTimeColumns(fields[8], fields[9]);
                loaded.expectedReadyTime_ = parseDateTimeColumns(fields[10], fields[11]);
                loaded.completedTime_ = parseDateTimeColumns(fields[12], fields[13]);
                try {
                    loaded.subtotal_ = stod(fields[14]);
                    loaded.priorityCharge_ = stod(fields[15]);
                    loaded.total_ = stod(fields[16]);
                } catch (...) {
                    continue;
                }
                loaded.prepDurationMinutes_ = prepDuration;
                currentOrder = move(loaded);
                (void)itemCount;
            } else if (fields[0] == "ITEM" && fields.size() >= 8 && currentOrder) {
                int token = 0, id = 0, quantity = 0, prep = 0;
                if (!parseIntField(fields[1], token) || token != currentOrder->tokenId_
                    || !parseIntField(fields[2], id) || !parseIntField(fields[5], quantity)
                    || !parseIntField(fields[7], prep)) continue;
                double price = 0.0;
                try { price = stod(fields[6]); } catch (...) { continue; }
                currentOrder->items_.push_back({id, fields[4], fields[3], price, quantity, prep});
            }
        }
        commitOrder();
        assignAvailableStationsLocked(time(nullptr));
    }

    void saveOrdersLocked() const {
        ofstream output(ordersFile_, ios::trunc);
        if (!output) {
            cerr << "Warning: could not save orders to " << ordersFile_ << ".\n";
            return;
        }

        output << "CANTEEN DAILY ORDER REGISTER\n";
        output << "BUSINESS DATE | " << businessDate_ << '\n';
        output << "Tokens start at 101 each day. Previous registers are archived as orders_YYYY-MM-DD.txt.\n\n";
        output << left << setw(5) << "ORDER" << " | " << right << setw(6) << "TOKEN"
               << " | " << left << setw(10) << "BILL NO" << " | " << setw(10) << "ORDER DATE"
               << " | " << setw(8) << "TIME" << " | " << setw(8) << "TYPE" << " | " << setw(10) << "STATUS"
               << " | " << setw(7) << "STATION" << " | " << setw(10) << "START DATE" << " | " << setw(8) << "START"
               << " | " << setw(10) << "ETA DATE" << " | " << setw(8) << "ETA" << " | " << setw(10) << "READY DATE"
               << " | " << setw(8) << "READY" << " | " << setw(10) << "SUBTOTAL" << " | " << setw(10) << "PRIORITY FEE"
               << " | " << setw(10) << "TOTAL" << " | " << setw(8) << "PREP MIN" << " | " << setw(5) << "ITEMS" << '\n';
        output << string(220, '-') << '\n' << fixed << setprecision(2);

        for (const auto& pointer : orders_) {
            const Order& order = *pointer;
            output << left << setw(5) << "ORDER" << " | " << right << setw(6) << order.tokenId_
                   << " | " << left << setw(10) << billLabel(order.billNumber_)
                   << " | " << setw(10) << dateColumn(order.orderTime_) << " | " << setw(8) << timeColumn(order.orderTime_)
                   << " | " << setw(8) << (order.priority_ ? "PRIORITY" : "NORMAL")
                   << " | " << setw(10) << statusText(order.status_) << " | " << right << setw(7) << order.stationNumber_
                   << " | " << left << setw(10) << dateColumn(order.preparationStart_) << " | " << setw(8) << timeColumn(order.preparationStart_)
                   << " | " << setw(10) << dateColumn(order.expectedReadyTime_) << " | " << setw(8) << timeColumn(order.expectedReadyTime_)
                   << " | " << setw(10) << dateColumn(order.completedTime_) << " | " << setw(8) << timeColumn(order.completedTime_)
                   << " | " << right << setw(10) << order.subtotal_ << " | " << setw(10) << order.priorityCharge_
                   << " | " << setw(10) << order.total_ << " | " << setw(8) << order.prepDurationMinutes_
                   << " | " << setw(5) << order.items_.size() << '\n';
            output << left << setw(5) << "ITEM" << " | " << right << setw(6) << "TOKEN"
                   << " | " << setw(7) << "ITEM ID" << " | " << left << setw(26) << "ITEM NAME"
                   << " | " << setw(12) << "CATEGORY" << " | " << right << setw(5) << "QTY"
                   << " | " << setw(10) << "UNIT PRICE" << " | " << setw(8) << "PREP MIN" << '\n';
            for (const auto& item : order.items_) {
                output << left << setw(5) << "ITEM" << " | " << right << setw(6) << order.tokenId_
                       << " | " << setw(7) << item.itemId << " | " << left << setw(26) << item.name
                       << " | " << setw(12) << item.category << " | " << right << setw(5) << item.quantity
                       << " | " << setw(10) << item.unitPrice << " | " << setw(8) << item.prepMinutes << '\n';
            }
            output << string(220, '-') << '\n';
        }
    }
    // Priority changes the next selection, but never interrupts a busy station.
    int nextWaitingTokenLocked() {
        while (!priorityQueue_.empty()) {
            int token = priorityQueue_.dequeue();
            Order* order = orderIndex_.find(token);
            if (order != nullptr && order->status_ == OrderStatus::PENDING && order->priority_)
                return token;
        }
        while (!normalQueue_.empty()) {
            int token = normalQueue_.dequeue();
            Order* order = orderIndex_.find(token);
            if (order != nullptr && order->status_ == OrderStatus::PENDING && !order->priority_)
                return token;
        }
        return 0;
    }
    bool assignAvailableStationsLocked(time_t now) {
        bool changed = false;
        for (auto& station : stations_) {
            if (!station.isFree()) continue;
            int token = nextWaitingTokenLocked();
            if (token == 0) break;
            Order* order = orderIndex_.find(token);
            if (order == nullptr || order->status_ != OrderStatus::PENDING) continue;

            station.assign(token);
            order->status_ = OrderStatus::PREPARING;
            order->stationNumber_ = station.number();
            order->preparationStart_ = now;
            order->expectedReadyTime_ = now + static_cast<time_t>(order->prepDurationMinutes_) * SECONDS_PER_PREP_MINUTE;
            changed = true;
        }
        return changed;
    }

    void timerLoop() {
        while (true) {
            this_thread::sleep_for(chrono::seconds(1));
            lock_guard<mutex> lock(mutex_);
            if (!running_) break;

            time_t now = time(nullptr);
            if (!rolloverIfNeededLocked(now)) continue;
            bool changed = false;
            for (auto& station : stations_) {
                if (station.isFree()) continue;
                Order* order = orderIndex_.find(station.activeToken());
                if (order == nullptr || order->status_ != OrderStatus::PREPARING) {
                    station.release();
                    changed = true;
                    continue;
                }
                if (order->expectedReadyTime_ > 0 && now >= order->expectedReadyTime_) {
                    order->status_ = OrderStatus::COMPLETED;
                    order->completedTime_ = now;
                    order->stationNumber_ = 0;
                    station.release();
                    changed = true;
                }
            }
            if (assignAvailableStationsLocked(now)) changed = true;
            if (changed) saveOrdersLocked();
        }
    }

public:
    CanteenSystem() {
        initializeMenu();
        loadMenuPrices();
        loadOrders();
        loadArchivedBillCounter();
        saveOrdersLocked();
        saveMenuPricesLocked();
        timerThread_ = thread(&CanteenSystem::timerLoop, this);
    }

    CanteenSystem(const CanteenSystem&) = delete;
    CanteenSystem& operator=(const CanteenSystem&) = delete;

    ~CanteenSystem() {
        {
            lock_guard<mutex> lock(mutex_);
            running_ = false;
        }
        if (timerThread_.joinable()) timerThread_.join();
        lock_guard<mutex> lock(mutex_);
        saveOrdersLocked();
        saveMenuPricesLocked();
    }

    vector<MenuItem> getMenu() const {
        lock_guard<mutex> lock(mutex_);
        return menu_;
    }

    vector<Order> getAllOrders() const {
        lock_guard<mutex> lock(mutex_);
        vector<Order> result;
        result.reserve(orders_.size());
        for (const auto& order : orders_) result.push_back(*order);
        return result;
    }

    optional<Order> findOrderByToken(int token) const {
        lock_guard<mutex> lock(mutex_);
        Order* order = orderIndex_.find(token);
        if (order == nullptr) return nullopt;
        return *order;
    }

    optional<Order> findOrderByBillNumber(int billNumber) const {
        lock_guard<mutex> lock(mutex_);
        for (const auto& order : orders_)
            if (order->billNumber_ == billNumber) return *order;
        return nullopt;
    }

    optional<Order> placeOrder(const vector<OrderLine>& items, bool priority) {
        if (items.empty()) return nullopt;
        lock_guard<mutex> lock(mutex_);
        const time_t now = time(nullptr);
        if (!rolloverIfNeededLocked(now)) return nullopt;

        Order newOrder(items, priority);
        newOrder.tokenId_ = ++lastToken_;
        newOrder.billNumber_ = ++lastBill_;
        newOrder.orderTime_ = now;

        auto record = make_unique<Order>(newOrder);
        Order* recordPointer = record.get();
        orders_.push_back(move(record));
        orderIndex_.insert(recordPointer->tokenId_, recordPointer);

        if (priority) priorityQueue_.enqueue(recordPointer->tokenId_);
        else normalQueue_.enqueue(recordPointer->tokenId_);

        assignAvailableStationsLocked(time(nullptr));
        saveOrdersLocked();
        return *recordPointer;
    }

    string cancelPendingOrder(int token) {
        lock_guard<mutex> lock(mutex_);
        Order* order = orderIndex_.find(token);
        if (order == nullptr) return "No order exists with token " + to_string(token) + ".";
        if (order->status_ != OrderStatus::PENDING)
            return "Cancellation denied. Only PENDING orders can be cancelled; this order is "
                   + statusText(order->status_) + ".";

        if (order->priority_) priorityQueue_.removeToken(token);
        else normalQueue_.removeToken(token);
        order->status_ = OrderStatus::CANCELLED;
        order->stationNumber_ = 0;
        saveOrdersLocked();
        return "Order " + to_string(token) + " has been cancelled successfully.";
    }

    bool updateMenuPrice(int itemId, double price) {
        if (!(price > 0.0) || !isfinite(price)) return false;
        lock_guard<mutex> lock(mutex_);
        for (auto& item : menu_) {
            if (item.id() == itemId) {
                item.setPrice(price);
                saveMenuPricesLocked();
                return true;
            }
        }
        return false;
    }

    vector<StationSnapshot> getStations() const {
        lock_guard<mutex> lock(mutex_);
        vector<StationSnapshot> result;
        for (const auto& station : stations_) {
            StationSnapshot view;
            view.number = station.number();
            view.token = station.activeToken();
            if (view.token != 0) {
                Order* order = orderIndex_.find(view.token);
                if (order != nullptr) view.expectedReady = order->expectedReadyTime_;
            }
            result.push_back(view);
        }
        return result;
    }

    QueueSnapshot getQueues() const {
        lock_guard<mutex> lock(mutex_);
        return {priorityQueue_.snapshot(), normalQueue_.snapshot()};
    }
};

void displayMenu(const vector<MenuItem>& menu) {
    const vector<string> categories = {"Breakfast", "Snacks", "Meals", "Drinks", "Desserts"};
    cout << "\n======================= CANTEEN MENU =======================\n";
    cout << "Prices in rupees (₹). Preparation times are estimates.\n";
    for (const auto& category : categories) {
        cout << "\n--- " << category << " ---\n";
        cout << left << setw(5) << "ID" << setw(30) << "Item"
             << right << setw(12) << "Price" << setw(15) << "Prep time" << "\n";
        cout << string(62, '-') << "\n";
        for (const auto& item : menu) {
            if (item.category() != category) continue;
            cout << left << setw(5) << item.id() << setw(30) << item.name().substr(0, 29)
                 << right << setw(12) << rupees(item.price())
                 << setw(12) << (to_string(item.prepMinutes()) + " min") << "\n";
        }
    }
    cout << "============================================================\n\n";
}

void printOrderStatus(const Order& order) {
    cout << "\n-------------------- ORDER STATUS --------------------\n";
    cout << "Order ID / Token : " << order.tokenId() << "\n";
    cout << "Bill Number      : " << billLabel(order.billNumber()) << "\n";
    cout << "Order Type       : " << (order.isPriority() ? "PRIORITY" : "NORMAL") << "\n";
    cout << "Status           : " << statusText(order.status()) << "\n";
    if (order.status() == OrderStatus::PREPARING) {
        cout << "Serving Station  : " << order.stationNumber() << "\n";
        cout << "Started At       : " << formatDateTime(order.preparationStart()) << "\n";
        cout << "Expected Ready   : " << formatDateTime(order.expectedReadyTime()) << "\n";
        long long remaining = static_cast<long long>(order.expectedReadyTime()) - static_cast<long long>(time(nullptr));
        if (remaining > 0) cout << "Approx. Remaining: " << (remaining + SECONDS_PER_PREP_MINUTE - 1) / SECONDS_PER_PREP_MINUTE << " minute(s)\n";
        else cout << "Approx. Remaining: under 1 minute; status refresh is automatic\n";
    } else if (order.status() == OrderStatus::PENDING) {
        cout << "Waiting in       : " << (order.isPriority() ? "Priority queue" : "Normal queue") << "\n";
    } else if (order.status() == OrderStatus::COMPLETED) {
        cout << "Completed At     : " << formatDateTime(order.completedTime()) << "\n";
    }
    cout << "Bill Total       : " << rupees(order.total()) << "\n";
    cout << "------------------------------------------------------\n\n";
}

optional<int> parseBillNumber(string input) {
    if (input.size() >= 5 && input.substr(0, 5) == "BILL-") input = input.substr(5);
    try {
        size_t used = 0;
        int value = stoi(input, &used);
        if (used == input.size() && value > 0) return value;
    } catch (...) {
        return nullopt;
    }
    return nullopt;
}

void createCustomerOrder(CanteenSystem& system) {
    vector<MenuItem> menu = system.getMenu();
    displayMenu(menu);
    vector<OrderLine> cart;

    cout << "Enter item ID and quantity. Enter 0 as item ID when finished.\n";
    while (true) {
        int id = readIntInRange("Item ID (0 to finish): ", 0, 9999);
        if (id == 0) break;
        auto selected = find_if(menu.begin(), menu.end(), [id](const MenuItem& item) { return item.id() == id; });
        if (selected == menu.end()) {
            cout << "That item ID is not present in the menu. Try again.\n";
            continue;
        }
        int quantity = readIntInRange("Quantity (1-50): ", 1, 50);
        auto existing = find_if(cart.begin(), cart.end(), [id](const OrderLine& line) { return line.itemId == id; });
        if (existing != cart.end()) {
            if (existing->quantity + quantity > 100) {
                cout << "Maximum combined quantity for one item is 100. Item not added.\n";
                continue;
            }
            existing->quantity += quantity;
        } else {
            cart.push_back({selected->id(), selected->category(), selected->name(), selected->price(),
                            quantity, selected->prepMinutes()});
        }
        cout << "Added " << quantity << " x " << selected->name() << ".\n";
    }

    if (cart.empty()) {
        cout << "No items selected. Order was not placed.\n";
        return;
    }

    cout << "\nService type:\n1. Normal order (no priority surcharge)\n2. Priority order (10% surcharge)\n";
    int priorityChoice = readIntInRange("Choose 1 or 2: ", 1, 2);
    bool priority = (priorityChoice == 2);
    Order preview(cart, priority);
    Bill::print(preview, true);

    int confirm = readIntInRange("Confirm order? 1 = Yes, 0 = No: ", 0, 1);
    if (confirm == 0) {
        cout << "Order not placed. No token or bill number was issued.\n";
        return;
    }

    optional<Order> placed = system.placeOrder(cart, priority);
    if (!placed) {
        cout << "Could not place the order. Please try again.\n";
        return;
    }
    cout << "Order placed successfully! First token in a fresh system is 101.\n";
    Bill::print(*placed);
    if (placed->status() == OrderStatus::PREPARING) {
        cout << "Your order has been assigned to Serving Station " << placed->stationNumber() << ".\n";
    } else {
        cout << "All four stations are busy. Your order is waiting in the "
             << (priority ? "priority" : "normal") << " queue.\n";
    }
}

void customerMode(CanteenSystem& system) {
    while (true) {
        cout << "\n================ CUSTOMER MODE ================\n"
             << "1. View Menu\n"
             << "2. Place New Order\n"
             << "3. Check Order Status by Token\n"
             << "4. Cancel My Pending Order\n"
             << "5. Generate / Reprint Bill by Order ID or Bill Number\n"
             << "0. Exit Customer Mode\n";
        int choice = readIntInRange("Choose an option: ", 0, 5);
        if (choice == 0) return;
        if (choice == 1) {
            displayMenu(system.getMenu());
        } else if (choice == 2) {
            createCustomerOrder(system);
        } else if (choice == 3) {
            int token = readIntInRange("Enter order token / ID: ", 1, 1000000000);
            auto order = system.findOrderByToken(token);
            if (order) printOrderStatus(*order);
            else cout << "No order found for token " << token << ".\n";
        } else if (choice == 4) {
            int token = readIntInRange("Enter token for cancellation: ", 1, 1000000000);
            cout << system.cancelPendingOrder(token) << "\n";
            cout << "Policy: cancellation is allowed only while the order is PENDING.\n";
        } else if (choice == 5) {
            cout << "Find bill using:\n1. Order ID / token\n2. Bill number (e.g. BILL-5001 or 5001)\n";
            int method = readIntInRange("Choose 1 or 2: ", 1, 2);
            optional<Order> order;
            if (method == 1) {
                int token = readIntInRange("Enter order ID / token: ", 1, 1000000000);
                order = system.findOrderByToken(token);
            } else {
                string value = readWord("Enter bill number (e.g. BILL-5001 or 5001): ");
                auto number = parseBillNumber(value);
                if (number) order = system.findOrderByBillNumber(*number);
            }
            if (order) Bill::print(*order);
            else cout << "No matching bill was found. Check the ID or bill number.\n";
        }
    }
}

void printTokenList(const vector<int>& tokens) {
    if (tokens.empty()) {
        cout << "Empty";
        return;
    }
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i) cout << " -> ";
        cout << tokens[i];
    }
}

void displayAllOrders(CanteenSystem& system) {
    vector<Order> allOrders = system.getAllOrders();
    vector<StationSnapshot> stations = system.getStations();
    QueueSnapshot queues = system.getQueues();

    cout << "\n================ FOUR SERVING STATIONS ================\n";
    for (const auto& station : stations) {
        cout << "Station " << station.number << ": ";
        if (station.token == 0) cout << "FREE\n";
        else {
            auto order = system.findOrderByToken(station.token);
            cout << "Token " << station.token;
            if (order) cout << " | " << (order->isPriority() ? "PRIORITY" : "NORMAL")
                           << " | expected ready " << formatDateTime(station.expectedReady);
            cout << "\n";
        }
    }

    cout << "\n================ WAITING QUEUES =======================\n";
    cout << "Priority queue (FIFO): "; printTokenList(queues.priorityTokens); cout << "\n";
    cout << "Normal queue (FIFO)  : "; printTokenList(queues.normalTokens); cout << "\n";

    const vector<OrderStatus> sections = {OrderStatus::PENDING, OrderStatus::PREPARING,
                                          OrderStatus::COMPLETED, OrderStatus::CANCELLED};
    for (OrderStatus status : sections) {
        cout << "\n================ " << statusText(status) << " ORDERS ================\n";
        bool found = false;
        for (const auto& order : allOrders) {
            if (order.status() != status) continue;
            found = true;
            cout << "Token " << order.tokenId() << " | " << billLabel(order.billNumber())
                 << " | " << (order.isPriority() ? "PRIORITY" : "NORMAL")
                 << " | " << rupees(order.total())
                 << " | placed " << formatDateTime(order.orderTime());
            if (status == OrderStatus::PREPARING) cout << " | station " << order.stationNumber();
            cout << "\n";
        }
        if (!found) cout << "No orders in this section.\n";
    }
    cout << "\nNote: status completion is estimated from preparation time, not physical food verification.\n";
}

void dailyReport(CanteenSystem& system) {
    vector<Order> allOrders = system.getAllOrders();
    const string today = todayDateKey();
    int totalOrders = 0, pending = 0, preparing = 0, completed = 0, cancelled = 0, priorityCount = 0;
    int unitsCompleted = 0;
    double completedSales = 0.0, priorityCharges = 0.0;
    long long preparationMinutes = 0;
    int completedDurationOrders = 0;
    map<string, int> itemQuantities;

    for (const auto& order : allOrders) {
        if (dateKey(order.orderTime()) != today) continue;
        ++totalOrders;
        if (order.isPriority()) ++priorityCount;
        switch (order.status()) {
            case OrderStatus::PENDING: ++pending; break;
            case OrderStatus::PREPARING: ++preparing; break;
            case OrderStatus::COMPLETED:
                ++completed;
                completedSales += order.total();
                priorityCharges += order.priorityCharge();
                preparationMinutes += order.prepDurationMinutes();
                ++completedDurationOrders;
                for (const auto& item : order.items()) {
                    unitsCompleted += item.quantity;
                    itemQuantities[item.name] += item.quantity;
                }
                break;
            case OrderStatus::CANCELLED: ++cancelled; break;
        }
    }

    cout << "\n================ DAILY CANTEEN REPORT ================\n";
    cout << "Report date (today): " << today << "\n";
    cout << "Orders placed today         : " << totalOrders << "\n";
    cout << "Priority orders             : " << priorityCount << "\n";
    cout << "Pending orders              : " << pending << "\n";
    cout << "Preparing orders            : " << preparing << "\n";
    cout << "Completed orders            : " << completed << "\n";
    cout << "Cancelled orders            : " << cancelled << "\n";
    cout << "Items in completed orders   : " << unitsCompleted << "\n";
    cout << "Completed bill value        : " << rupees(completedSales) << "\n";
    cout << "Priority charges (completed): " << rupees(priorityCharges) << "\n";
    if (completedDurationOrders > 0)
        cout << "Average estimated prep time : " << fixed << setprecision(1)
             << static_cast<double>(preparationMinutes) / completedDurationOrders << " minutes/order\n";
    else cout << "Average estimated prep time : N/A (no completed orders)\n";

    cout << "\nItems sold in completed orders:\n";
    if (itemQuantities.empty()) cout << "No completed orders yet.\n";
    else {
        for (const auto& pair : itemQuantities)
            cout << "  " << left << setw(28) << pair.first << right << pair.second << " unit(s)\n";
    }
    cout << "\nSales here means the bill value of completed orders; actual payment/refund tracking is not implemented.\n";
    cout << "======================================================\n\n";
}

void showCompletedOrdersSortedByBill(CanteenSystem& system) {
    vector<Order> completedOrders;
    for (const auto& order : system.getAllOrders()) {
        if (order.status() == OrderStatus::COMPLETED) completedOrders.push_back(order);
    }
    for (size_t i = 1; i < completedOrders.size(); ++i) {
        Order key = completedOrders[i];
        long long j = static_cast<long long>(i) - 1;
        while (j >= 0 && completedOrders[static_cast<size_t>(j)].total() > key.total()) {
            completedOrders[static_cast<size_t>(j + 1)] = completedOrders[static_cast<size_t>(j)];
            --j;
        }
        completedOrders[static_cast<size_t>(j + 1)] = key;
    }

    cout << "\n======== COMPLETED ORDERS SORTED BY BILL AMOUNT (ASCENDING) ========\n";
    if (completedOrders.empty()) {
        cout << "No completed orders available to sort yet.\n";
        return;
    }
    cout << left << setw(12) << "Order ID" << setw(16) << "Bill Number"
         << setw(14) << "Order Type" << right << setw(14) << "Bill Amount" << "\n";
    cout << string(56, '-') << "\n";
    for (const auto& order : completedOrders) {
        cout << left << setw(12) << order.tokenId() << setw(16) << billLabel(order.billNumber())
             << setw(14) << (order.isPriority() ? "PRIORITY" : "NORMAL")
             << right << setw(14) << rupees(order.total()) << "\n";
    }
    cout << "Sorting algorithm: Insertion Sort | Time complexity: O(n^2)\n";
}

void updateMenuItemPrice(CanteenSystem& system) {
    vector<MenuItem> menu = system.getMenu();
    displayMenu(menu);
    int id = readIntInRange("Enter item ID to update (1-20): ", 1, 20);
    auto found = find_if(menu.begin(), menu.end(), [id](const MenuItem& item) { return item.id() == id; });
    if (found == menu.end()) {
        cout << "No menu item with that ID.\n";
        return;
    }
    cout << found->name() << " current price: " << rupees(found->price()) << "\n";
    double newPrice = readPositivePrice("Enter new price in rupees: ");
    if (newPrice <= 0.0) {
        cout << "Price update cancelled.\n";
        return;
    }
    if (system.updateMenuPrice(id, newPrice)) {
        cout << "Price updated. New orders will use " << rupees(newPrice)
             << "; existing bills keep their original prices.\n";
    } else cout << "Price could not be updated.\n";
}

void managerMode(CanteenSystem& system) {
    while (true) {
        cout << "\n================ MANAGER MODE ================\n"
             << "1. Display All Orders, Stations and Queues\n"
             << "2. Generate Daily Report\n"
             << "3. View Menu and Pricing\n"
             << "4. Update Menu Item Price\n"
             << "5. Sort Completed Orders by Bill Amount (Insertion Sort)\n"
             << "0. Exit Manager Mode\n";
        int choice = readIntInRange("Choose an option: ", 0, 5);
        if (choice == 0) return;
        if (choice == 1) displayAllOrders(system);
        else if (choice == 2) dailyReport(system);
        else if (choice == 3) displayMenu(system.getMenu());
        else if (choice == 4) updateMenuItemPrice(system);
        else if (choice == 5) showCompletedOrdersSortedByBill(system);
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(&cout);
    cout << unitbuf;

    cout << "===============================================================\n";
    cout << " CANTEEN ORDER, TOKEN, BILLING & PREPARATION MANAGEMENT SYSTEM\n";
    cout << "===============================================================\n";
    cout << "Four preparation stations | Priority surcharge: 10%\n";
    cout << "Order tokens start at 101 on a fresh installation.\n";
    cout << "Orders and updated menu prices are saved in local text files.\n";

    CanteenSystem system;
    while (true) {
        cout << "\n================ MAIN MENU =================\n"
             << "1. Customer Mode\n"
             << "2. Manager Mode\n"
             << "0. Exit Program\n";
        int choice = readIntInRange("Choose an option: ", 0, 2);
        if (choice == 0) break;
        if (choice == 1) customerMode(system);
        else if (choice == 2) managerMode(system);
    }

    cout << "\nSystem closed. Orders and menu prices have been saved. Goodbye!\n";
    return 0;
}
