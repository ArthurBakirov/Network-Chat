#pragma once

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>
#include <deque>
#include <optional>
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <future>


#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
