#ifndef SHIVRAI_HON_PAYMENT_H
#define SHIVRAI_HON_PAYMENT_H
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace shivrai::fintech {

enum class PaymentStatus {
    PENDING,
    CONFIRMED,
    FAILED,
    REFUNDED,
    EXPIRED
};

enum class PaymentType {
    TRANSFER,
    MERCHANT,
    INVOICE,
    BATCH,
    RECURRING
};

struct Payment {
    std::string id;
    std::string from;
    std::string to;
    std::string currency;
    uint64_t amount{0};
    uint64_t fee{0};
    uint64_t created_at{0};
    uint64_t expires_at{0};
    PaymentStatus status{PaymentStatus::PENDING};
    PaymentType type{PaymentType::TRANSFER};
    std::string memo;
    std::string merchant_id;
    std::string invoice_id;
};

struct PaymentResult {
    bool success{false};
    std::string payment_id;
    std::string error;
    uint64_t fee_charged{0};
};

struct Invoice {
    std::string id;
    std::string merchant_id;
    std::string currency;
    uint64_t amount{0};
    uint64_t expires_at{0};
    bool paid{false};
    std::string paid_by;
    std::string memo;
};

struct BatchPayment {
    std::string id;
    std::string from;
    std::vector<std::pair<std::string, uint64_t>> recipients; // {to, amount}
    std::string currency;
    uint64_t total_amount{0};
    uint64_t fee{0};
    PaymentStatus status{PaymentStatus::PENDING};
};

class PaymentProcessor {
public:
    explicit PaymentProcessor(uint32_t fee_bps = 10); // 0.1% default fee

    // Core payments
    PaymentResult send(const std::string& from, const std::string& to,
                       uint64_t amount, const std::string& currency,
                       const std::string& memo = "");

    // Invoice
    Invoice create_invoice(const std::string& merchant_id, uint64_t amount,
                           const std::string& currency, uint64_t ttl_seconds,
                           const std::string& memo = "");
    PaymentResult pay_invoice(const std::string& invoice_id,
                              const std::string& payer);

    // Batch
    PaymentResult batch_send(const std::string& from,
                             const std::vector<std::pair<std::string,uint64_t>>& recipients,
                             const std::string& currency);

    // Status
    PaymentStatus status(const std::string& payment_id) const;
    bool refund(const std::string& payment_id);

    // View
    std::vector<Payment> history(const std::string& addr) const;
    uint64_t fee_bps() const { return fee_bps_; }

    // Callback
    using StatusCallback = std::function<void(const Payment&)>;
    void on_status_change(StatusCallback cb) { callback_ = cb; }

private:
    uint32_t fee_bps_;
    std::unordered_map<std::string, Payment> payments_;
    std::unordered_map<std::string, Invoice> invoices_;
    StatusCallback callback_;

    uint64_t calc_fee(uint64_t amount) const;
    std::string gen_id(const std::string& prefix) const;
    void notify(const Payment& p);
};

bool valid(const Payment& p);

} // namespace shivrai::fintech
#endif
