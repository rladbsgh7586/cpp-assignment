#include "pch.hpp"

std::string_view SideStr(Side s) { return s == Side::kBuy ? "BID" : "ASK"; }

std::string FormatBook(const Orderbook& b) {
  std::string out = "\n      price |   qty\n";
  for (int i = kNumLayers - 1; i >= 0; --i)
    if (b.ask_qty[i] > 0)
      out += std::format("ASK {:>7} | {:>6}\n", b.ask_price[i], b.ask_qty[i]);
  out += "    --------+-------\n";
  for (int i = 0; i < kNumLayers; ++i)
    if (b.bid_qty[i] > 0)
      out += std::format("BID {:>7} | {:>6}\n", b.bid_price[i], b.bid_qty[i]);
  return out;
}

bool ParseNonNeg(std::string_view s, int& out) {
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
  return ec == std::errc{} && ptr == s.data() + s.size() && out >= 0;
}
