#include "xilou_public/xilou_logger.h"

#include "third_party/spdlog/include/spdlog/spdlog.h"
#include "third_party/spdlog/include/spdlog/sinks/rotating_file_sink.h"

XILOU_EXPORT void xiloug_setloglevel(int level) {
  (void)level;
  // Create a file rotating logger with 5 MB size max and 3 rotated files
  auto max_size = 1048576 * 5;
  auto max_files = 3;
  auto logger = spdlog::rotating_logger_mt(
      "xilou", "logs/rotating.txt", max_size, max_files);
  spdlog::set_default_logger(logger);
  switch (level) {
  case 0:
      spdlog::set_level(spdlog::level::level_enum::trace);
      break;
  case 1:
    spdlog::set_level(spdlog::level::level_enum::debug);
    break;
  case 2:
    spdlog::set_level(spdlog::level::level_enum::info);
    break;
  case 3:
    spdlog::set_level(spdlog::level::level_enum::warn);
    break;
  case 4:
    spdlog::set_level(spdlog::level::level_enum::err);
    break;
  case 5:
    spdlog::set_level(spdlog::level::level_enum::critical);
    break;
  default:
    spdlog::set_level(spdlog::level::level_enum::off);
    break;
  }
  spdlog::flush_on(spdlog::level::info);
}