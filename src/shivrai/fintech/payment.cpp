#include "shivrai/common/time.hpp"
#include "payment.h"
#include <stdexcept>
#include <sstream>
#include <chrono>
#include <random>

namespace shivrai::fintech {

bool valid(const Payment& p) {
    return !p.id.empty() && !p.from.empty() && !p.to.empty() &&
           p.from != p.to && !p.currency.empty() && p.amount > 0;
}

PaymentProcessor::PaymentProcessor(uint32_t fee_bps) : fee_bps_(fee_bps) {}

uint64_t PaymentProcessor::calc_fee(uint64_t amount) const {
    return (amount * fee_bps_) / 10000;
}

std::string PaymentProcessor::gen_id(const std::string& prefix) const {
    static std::mt19937_64 rng(shivrai::common::monotonic_ns());
    std::ostringstream ss;
    ss << prefix << "_" << rng();
    return ss.str();
}

void PaymentProcessor::notify(const Payment& p) {
    if (callback_) callback_(p);
}

PaymentResult PaymentProcessor::send(const std::string& from, const std::string& to,
                                      uint64_t amount, const std::string& currency,
                                      const std::string& memo) {
    if (from.empty() || to.empty() || from == to || amount == 0 || currency.empty())
        return {false, "", "invalid params", 0};

    uint64_t fee = calc_fee(amount);
    Payment p;
    p.id = gen_id("PAY");
    p.from = from;
    p.to = to;
    p.amount = amount;
    p.fee = fee;
    p.currency = currency;
    p.memo = memo;
    p.type = PaymentType::TRANSFER;
    p.status = PaymentStatus::CONFIRMED;
    p.created_at = shivrai::common::now_seconds();

    payments_[p.id] = p;
    notify(p);
    return {true, p.id, "", fee};
}

Invoice PaymentProcessor::create_invoice(const std::string& merchant_id, uint64_t amount,
                                          const std::string& currency, uint64_t ttl_seconds,
                                          const std::string& memo) {
    Invoice inv;
    inv.id = gen_id("INV");
    inv.merchant_id = merchant_id;
    inv.amount = amount;
    inv.currency = currency;
    inv.memo = memo;
    inv.expires_at = shivrai::common::now_seconds() + ttl_seconds;
    inv.paid = false;
    invoices_[inv.id] = inv;
    return inv;
}

PaymentResult PaymentProcessor::pay_invoice(const std::string& invoice_id,
                                             const std::string& payer) {
    auto it = invoices_.find(invoice_id);
    if (it == invoices_.end()) return {false, "", "invoice not found", 0};
    auto& inv = it->second;
    if (inv.paid) return {false, "", "already paid", 0};

    uint64_t now = shivrai::common::now_seconds();
    if (inv.expires_at > 0 && now > inv.expires_at)
        return {false, "", "invoice expired", 0};

    auto result = send(payer, inv.merchant_id, inv.amount, inv.currency,
                       "Invoice: " + invoice_id);
    if (result.success) {
        inv.paid = true;
        inv.paid_by = payer;
    }
    return result;
}

PaymentResult PaymentProcessor::batch_send(const std::string& from,
                                            const std::vector<std::pair<std::string,uint64_t>>& recipients,
                                            const std::string& currency) {
    if (recipients.empty()) return {false, "", "empty batch", 0};

    uint64_t total_fee = 0;
    std::string batch_id = gen_id("BATCH");

    for (const auto& [to, amount] : recipients) {
        auto r = send(from, to, amount, currency, "Batch: " + batch_id);
        if (!r.success) return {false, batch_id, "batch failed at " + to, total_fee};
        total_fee += r.fee_charged;
    }
    return {true, batch_id, "", total_fee};
}

PaymentStatus PaymentProcessor::status(const std::string& payment_id) const {
    auto it = payments_.find(payment_id);
    if (it == payments_.end()) return PaymentStatus::FAILED;
    return it->second.status;
}

bool PaymentProcessor::refund(const std::string& payment_id) {
    auto it = payments_.find(payment_id);
    if (it == payments_.end()) return false;
    if (it->second.status != PaymentStatus::CONFIRMED) return false;
    it->second.status = PaymentStatus::REFUNDED;
    notify(it->second);
    return true;
}

std::vector<Payment> PaymentProcessor::history(const std::string& addr) const {
    std::vector<Payment> result;
    for (const auto& [id, p] : payments_) {
        if (p.from == addr || p.to == addr) result.push_back(p);
    }
    return result;
}

} // namespace shivrai::fintech
